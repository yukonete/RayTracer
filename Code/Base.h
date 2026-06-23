#pragma once

#ifndef BASE_H
#define BASE_H

#include <any>
#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <memory>
#include <print>
#include <random>
#include <source_location>
#include <span>
#include <stacktrace>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

using isize = ptrdiff_t;
using usize = size_t;

using uintptr = uintptr_t;
using intptr = intptr_t;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

constexpr auto kilobytes(auto value) { return value * 1024; }
constexpr auto megabytes(auto value) { return kilobytes(value) * 1024; }
constexpr auto gigabytes(auto value) { return megabytes(value) * 1024; }
constexpr auto terabytes(auto value) { return gigabytes(value) * 1024; }

template <typename... Args>
void log(std::format_string<Args...> fmt, Args &&...args) {
    std::println(stderr, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[noreturn]] void panic_(std::format_string<Args...> fmt,
                         std::source_location loc, Args &&...args) {
    const auto msg = std::format(fmt, std::forward<Args>(args)...);
    std::println(stderr,
                 "Panic: {}.\n"
                 "At {}:{}.\n"
                 "Stacktrace:\n"
                 "{}",
                 msg, loc.file_name(), loc.line(), std::stacktrace::current());
    std::quick_exit(1);
}

#define panic(fmt, ...)                                                        \
    panic_(fmt, std::source_location::current(), ##__VA_ARGS__);

inline bool is_power_of_two(s64 value) {
    return (value & (value - 1)) == 0;
}

inline isize align_forward(isize pointer, isize alignment) {
    assert(is_power_of_two(alignment));
    isize modulo = pointer % alignment;
    if (modulo == 0) {
        return pointer;
    }
    return pointer + (alignment - modulo);
}

inline void *align_forward(void *pointer, isize alignment) {
    return reinterpret_cast<void *>(
        align_forward(reinterpret_cast<isize>(pointer), alignment));
}

constexpr isize allocation_default_alignment = 2 * sizeof(void *);

struct FixedBuffer {
    FixedBuffer() {
    }
    FixedBuffer(u8 *buffer, isize size)
        : data_(buffer), size_{size} {};

    const u8 *data() const {
        return data_;
    }

    isize size() const {
        return size_;
    }

    isize offset() const {
        return offset_;
    }

    void clear() {
        offset_ = 0;
    }

    void *alloc(isize size, isize alignment = allocation_default_alignment) {
        assert(size >= 0 && alignment >= 0);

        if (size == 0) {
            return nullptr;
        }

        const auto base_adress = reinterpret_cast<isize>(data_);
        const auto current_pointer = base_adress + offset_;
        const auto mem_offset =
            align_forward(current_pointer, alignment) - base_adress;

        if (mem_offset + size > size_) {
            return nullptr;
        }

        offset_ = mem_offset + size;
        return data_ + mem_offset;
    };

private:
    u8 *data_ = nullptr;
    isize size_ = 0;
    isize offset_ = 0;
};

class Arena {
public:
    constexpr static isize default_size = megabytes(2);

    Arena(isize size = default_size) : first(ArenaMemoryBlock::create(size)) {};

    void *alloc(isize size, isize alignment = allocation_default_alignment) {
        return first->alloc(size, alignment);
    };

    template <typename Item, typename... Args>
    Item *push_item(Args &&...args) {
        return push_item_impl<Item>(std::forward<Args>(args)...);
    }

    template <typename Item>
    Item *push_item(Item &&item) {
        return push_item_impl<Item>(std::forward<Item>(item));
    }

    template <TriviallyDestructible Item>
    std::span<Item> push_array(isize count) {
        return std::span<Item>{push_array_pointer<Item>(count),
                               static_cast<std::span<Item>::size_type>(count)};
    }

    template <TriviallyDestructible Item>
    std::span<Item> push_array(std::span<Item> items) {
        auto result = push_array<Item>(static_cast<isize>(items.size()));
        std::ranges::copy(items, result.begin());
        return result;
    }

    template <TriviallyDestructible Item>
    Item *push_array_pointer(isize count) {
        auto pointer =
            static_cast<Item *>(alloc(sizeof(Item) * count, alignof(Item)));
        for (isize i = 0; i < count; ++i) {
            new (&pointer[i]) Item{};
        }
        return pointer;
    }

    void clear() {
        if (first != nullptr) {
            first->clear();
        }
    }

    ~Arena() {
        clear();
    }

private:
    template <typename Item, typename... Args>
    Item *push_item_impl(Args &&...args) {
        auto pointer = alloc(sizeof(Item), alignof(Item));
        if constexpr (!TriviallyDestructible<Item>) {
            last_allocation_ = push_item(AllocationWithDestructor{
                .object = pointer,
                .destructor =
                    [](void *object) { static_cast<Item *>(object)->~Item(); },
                .previous = last_allocation_,
            });
        }
        return new (pointer) Item{std::forward<Args>(args)...};
    }

    struct ArenaMemoryBlock {
        struct Deleter {
            void operator()(ArenaMemoryBlock *pointer) const {
                pointer->~ArenaMemoryBlock();
                delete[] reinterpret_cast<u8*>(pointer);
            };
        };

        static std::unique_ptr<ArenaMemoryBlock, Deleter> create(isize size) {
            static_assert(alignof(ArenaMemoryBlock) <  __STDCPP_DEFAULT_NEW_ALIGNMENT__);

            auto block_info_size_plus_alignment = align_forward(
                sizeof(ArenaMemoryBlock), allocation_default_alignment);
            auto to_allocate = block_info_size_plus_alignment + size;

            auto memory_block = new u8[to_allocate]{};
            auto block = new (memory_block) ArenaMemoryBlock;

            block->buffer = FixedBuffer{
                static_cast<u8 *>(memory_block) + block_info_size_plus_alignment,
                size};

            return std::unique_ptr<ArenaMemoryBlock, Deleter>(block, Deleter{});
        }

        FixedBuffer buffer;
        std::unique_ptr<ArenaMemoryBlock, Deleter> next = nullptr;

        void *alloc(isize size,
                    isize alignment = allocation_default_alignment) {
            if (size > buffer.size()) {
                return nullptr;
            }

            auto allocated = buffer.alloc(size, alignment);
            if (allocated != nullptr) {
                return allocated;
            }

            if (next == nullptr) {
                next = create(buffer.size());
            }

            return next->alloc(size, alignment);
        };

        void clear() {
            if (next != nullptr) {
                next->clear();
            }
            buffer.clear();
        }
    };

    using DestructorFunc = void(void *);

    struct AllocationWithDestructor {
        void *object = nullptr;
        DestructorFunc *destructor = nullptr;
        AllocationWithDestructor *previous = nullptr;
    };

    std::unique_ptr<ArenaMemoryBlock, ArenaMemoryBlock::Deleter> first;
    AllocationWithDestructor *last_allocation_ = nullptr;
};

struct ReadFileToStringResult {
    std::string content;
    bool ok = false;
};

inline ReadFileToStringResult read_file_to_string(const char *path) {
    auto err = std::error_code{};
    const auto file_size = std::filesystem::file_size(path, err);
    if (err) {
        return {};
    }

    auto file = std::ifstream{path, std::ios::binary};
    if (!file.is_open()) {
        return {};
    }

    auto result =
        ReadFileToStringResult{.content = std::string(file_size, '\0')};
    file.read(&result.content[0], file_size);
    if (!file.fail()) {
        result.ok = true;
    }
    return result;
}

inline constexpr f32 infinity = std::numeric_limits<f32>::infinity();
inline constexpr f32 pi = 3.1415926535897932385f;

// Returns a random floating point in range [0; 1).
// Uses thread_local random generator.
template <std::floating_point T>
inline T random_floating_point() {
    thread_local std::random_device rd;
    thread_local std::uniform_real_distribution<T> distribution(T(0.0), T(1.0));
    thread_local std::mt19937 generator(rd());
    return distribution(generator);
}

inline f32 random_f32() {
    return random_floating_point<f32>();
}

inline f32 random_f32(f32 min, f32 max) {
    return min + (max - min) * random_f32();
}

inline int random_int(int min, int max) {
    return static_cast<int>(
        random_f32(static_cast<f32>(min), static_cast<f32>(max + 1)));
}

inline f32 degrees_to_radians(f32 degrees) {
    return degrees * pi / 180.0f;
}

inline f32 clamp(f32 min, f32 max, f32 x) {
    if (x < min) {
        return min;
    }
    if (x > max) {
        return max;
    }
    return x;
}

#endif

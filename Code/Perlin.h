#pragma once

#include "RtWeekend.h"
#include <array>
#include <mdspan>
#include <span>

struct Perlin {
    Perlin() {
        for (int i = 0; i < std::ssize(randvec); ++i) {
            randvec.at(i) = normalize(Vec3::random(-1.0f, 1.0f));
        }

        permute(perm_x);
        permute(perm_y);
        permute(perm_z);
    }

    f32 noise(const Vec3 &p) const {
        auto u = p.x - std::floor(p.x);
        auto v = p.y - std::floor(p.y);
        auto w = p.z - std::floor(p.z);

        auto i = std::floor(p.x);
        auto j = std::floor(p.y);
        auto k = std::floor(p.z);

        std::array<Vec3, 8> c;
        std::mdspan<Vec3, std::extents<isize, 2, 2, 2>> mc(c.data());
        for (int di = 0; di < 2; ++di) {
            for (int dj = 0; dj < 2; ++dj) {
                for (int dk = 0; dk < 2; ++dk) {
                    mc[di, dj, dk] =
                        randvec.at(perm_x.at(static_cast<int>(i + di) & 255) ^
                                   perm_y.at(static_cast<int>(j + dj) & 255) ^
                                   perm_z.at(static_cast<int>(k + dk) & 255));
                }
            }
        }

        return perlin_interpolation(mc, u, v, w);
    }

    f32 turb(const Vec3 &p, int depth) const {
        auto accum = 0.0f;
        auto temp_p = p;
        auto weight = 1.0f;

        for (int i = 0; i < depth; ++i) {
            accum += weight * noise(temp_p);
            weight *= 0.5f;
            temp_p *= 2.0f;
        }

        return std::fabs(accum);
    }

private:
    constexpr static inline int point_count = 256;
    std::array<Vec3, point_count> randvec;
    std::array<int, point_count> perm_x;
    std::array<int, point_count> perm_y;
    std::array<int, point_count> perm_z;

    static void permute(std::span<int> p) {
        for (int i = 0; i < std::ssize(p); ++i) {
            p[i] = i;
        }
        for (int i = std::ssize(p) - 1; i >= 0; --i) {
            int target = random_int(0, i);
            std::swap(p[i], p[target]);
        }
    }

    static f32
    perlin_interpolation(std::mdspan<Vec3, std::extents<isize, 2, 2, 2>> c,
                         f32 u, f32 v, f32 w) {
        auto uu = u * u * (3.0f - 2.0f * u);
        auto vv = v * v * (3.0f - 2.0f * v);
        auto ww = w * w * (3.0f - 2.0f * w);

        auto accum = 0.0f;
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    auto weight_v = Vec3{u - i, v - j, w - k};
                    accum += (i * uu + (1 - i) * (1.0f - uu)) *
                             (j * vv + (1 - j) * (1.0f - vv)) *
                             (k * ww + (1 - k) * (1.0f - ww)) *
                             dot(c[i, j, k], weight_v);
                }
            }
        }

        return accum;
    }
};
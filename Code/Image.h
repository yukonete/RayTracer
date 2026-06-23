#pragma once
#include "Base.h"
#include "Vec3.h"
#include "stb_image.h"

struct Image {
    // TODO: Maybe load and store u8 data, and then convert it to Color on get_pixel_data
    std::unique_ptr<float[]> data;
    int width = 0;
    int height = 0;

    const static inline Color image_missing_color = Color{1.0f, 0.0f, 1.0f};
    constexpr static inline int default_channels = 3;

    static Image load(const char *path) {
        int x, y, n;
        auto *data = stbi_loadf(path, &x, &y, &n, default_channels);
        if (data == nullptr) {
            log("ERROR: Could not open image file \"{}\"", path);
            return {};
        }

        return Image{
            .data = std::unique_ptr<float[]>(data),
            .width = x,
            .height = y,
        };
    }

    Color get_pixel_data(int x, int y) const {
        auto size = height * width;
        auto pixel_index = width * y + x;
        if (data == nullptr || pixel_index >= size) {
            log("ERROR: Attempt to index out of range of image data. Pixel index is "
                "{}, image size is {}.",
                pixel_index, size);
            return image_missing_color;
        }

        auto ptr = data.get() + pixel_index * default_channels;
        return Color{ptr[0], ptr[1], ptr[2]};
    }
};

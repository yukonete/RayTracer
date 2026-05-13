#pragma once

#include "RtWeekend.h"
#include "Image.h"

struct Texture {
	virtual Color value(f32 u, f32 v, const Vec3 &point) const = 0;
};

struct SolidColor : public Texture {
	Color albedo;

	SolidColor(const Color &albedo) : albedo{albedo} {}

	Color value(f32 u, f32 v, const Vec3 &point) const override {
		return albedo;
	}
};

struct CheckerTexture : public Texture {
	const Texture *even = nullptr;
	const Texture *odd = nullptr;
	f32 inv_scale = 0.0f;

	CheckerTexture(f32 scale, const Texture *even, const Texture *odd) : inv_scale{1.0f / scale}, even{even}, odd{odd} {}

	Color value(f32 u, f32 v, const Vec3 &point) const override {
		auto x = static_cast<int>(std::floor(inv_scale * point.x));
		auto y = static_cast<int>(std::floor(inv_scale * point.y));
		auto z = static_cast<int>(std::floor(inv_scale * point.z));
		auto is_even = (x + y + z) % 2;
		if (is_even) {
			return even->value(u, v, point);
		} else {
			return odd->value(u, v, point);
		}
	}
};

struct ImageTexture : public Texture {
	Image *image = nullptr;

	ImageTexture(Image *image) : image{image}{}

	Color value(f32 u, f32 v, const Vec3 &point) const override {
		auto x = static_cast<int>(u * image->width);
		auto y = static_cast<int>((1.0f - v) * image->height);
		return image->get_pixel_data(x, y);
	}
};
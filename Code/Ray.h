#pragma once

#include "Base.h"
#include "Vec3.h"

struct Ray {
	Vec3 origin;
	Vec3 direction;
	f32 time = 0.0f;

	Ray() {}
	Ray(const Vec3 &orig, const Vec3 &dir) : origin{orig}, direction{dir} {}
	Ray(const Vec3 &orig, const Vec3 &dir, f32 time) : origin{orig}, direction{dir}, time{time} {}

	Vec3 at(f32 t) const {
		return origin + t*direction;
	};
};
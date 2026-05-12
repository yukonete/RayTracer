#pragma once

#include "RtWeekend.h"

struct AABB {
	union {
		struct {
			Interval x;
			Interval y;
			Interval z;
		};
		Interval e[3];
	};

	AABB() : x{}, y{}, z{} {}
	AABB(const Interval &x, const Interval &y, const Interval &z) : x{x}, y{y}, z{z} {}
	AABB(const Vec3 &point1, const Vec3 &point2) {
		x = (point1.x <= point2.x) ? Interval{point1.x, point2.x} : Interval{point2.x, point1.x};
        y = (point1.y <= point2.y) ? Interval{point1.y, point2.y} : Interval{point2.y, point1.y};
		z = (point1.z <= point2.z) ? Interval{point1.z, point2.z} : Interval{point2.z, point1.z};
	}
	AABB(const AABB &a, const AABB &b) {
		x = Interval{a.x, b.x};
        y = Interval{a.y, b.y};
		z = Interval{a.z, b.z};
	}

	Interval& operator[](int i) {
		return e[i];
	}

	const Interval& operator[](int i) const {
		return e[i];
	}

	bool hit(const Ray &r, Interval t) const {
		for (int axis = 0; axis < 3; ++axis) {
			auto interval = e[axis];
			auto rd_inverse = 1.0f / r.direction[axis];
			auto t0 = (interval.min - r.origin[axis]) * rd_inverse; 
			auto t1 = (interval.max - r.origin[axis]) * rd_inverse;

			if (t0 < t1) {
				if (t0 > t.min) {
					t.min = t0;
				}
				if (t1 < t.max) {
					t.max = t1;
				}
			} else {
				if (t1 > t.min) {
					t.min = t1;
				}
				if (t0 < t.max) {
					t.max = t0;
				}
			}

			if (t.max <= t.min) {
				return false;
			}
		}

		return true;
	}

	int longest_axis() const {
        if (x.size() > y.size()) {
            return x.size() > z.size() ? 0 : 2;
		}
        else {
            return y.size() > z.size() ? 1 : 2;
		}
	}

	static const AABB empty;
	static const AABB universe;
};
#pragma once

#include "Base.h"

struct Interval {
	f32 min = infinity;
	f32 max = -infinity;

	Interval() {}
	Interval(f32 _min, f32 _max) : min{_min}, max{_max} {}
	Interval(const Interval &a, const Interval &b) {
		min = a.min <= b.min ? a.min : b.min;
        max = a.max >= b.max ? a.max : b.max;
	}

	f32 size() const {
		return max - min;
	}

	bool contains(f32 x) const {
		return x >= min && x <= max;
	}

	bool surrounds(f32 x) const {
		return x > min && x < max;
	}

	Interval expand(f32 delta) const {
		auto padding = delta/2.0f;
		return Interval{min-padding, max+padding};
	}

	static const Interval empty;
	static const Interval universe;
};



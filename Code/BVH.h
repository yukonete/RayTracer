#pragma once

#include <algorithm>
#include <span>

#include "Hittable.h"
#include "AABB.h"
#include "Base.h"
#include "Interval.h"
#include "Ray.h"

struct BVHNode : public Hittable {
	Hittable *left = nullptr;
	Hittable *right = nullptr;
	AABB bbox;

	BVHNode(Arena *arena, std::span<Hittable*> objects) {
		auto objects_count = objects.size();
		assert(objects_count != 0);

		bbox = AABB::empty;
		for (int object_index = 0; object_index < objects_count; ++object_index) {
			bbox = AABB{bbox, objects[object_index]->bounding_box()};
		}

		if (objects_count == 1) {
			left = objects[0];
			right = objects[0];
		} else if (objects_count == 2) {
			left = objects[0];
			right = objects[1];
		} else {
			int axis = bbox.longest_axis();
			std::ranges::sort(objects, [axis](const Hittable *a, const Hittable* b) -> bool {
				auto a_axis_interval = a->bounding_box()[axis];
				auto b_axis_interval = b->bounding_box()[axis];
				return a_axis_interval.min < b_axis_interval.min;		
			});

			auto mid = objects_count / 2;
			left = arena->push_item<BVHNode>(arena, objects.subspan(0, mid));
			right = arena->push_item<BVHNode>(arena, objects.subspan(mid));
		}
	}

	bool hit(const Ray &ray, Interval t, HitRecord& record) const override {
		if (!bbox.hit(ray, t)) {
			return false;
		}

		bool hit_left = left->hit(ray, t, record);
		bool hit_right = right->hit(ray, Interval{t.min, hit_left ? record.t : t.max}, record);

		return hit_left || hit_right;
	}

	const AABB& bounding_box() const override { 
		return bbox; 
	}
};
#pragma once

#include <vector>
#include <memory>
#include "RtWeekend.h"
#include "Hittable.h"

struct HittableList : public Hittable {
	std::vector<Hittable*> objects;
	AABB bbox;

	HittableList() {};
	HittableList(Hittable *hittable) {
		add(hittable);
	};

	void clear() {
		objects.clear();
	}

	void add(Hittable *object) {
		objects.push_back(object);
		bbox = AABB{bbox, object->bounding_box()};
	}

	const AABB& bounding_box() const override {
		return bbox;
	}

	bool hit(const Ray &ray, Interval t, HitRecord& record) const override {
		bool hit_anything = false;
		auto closest_so_far = t.max;

		for (const auto& object : objects) {
			auto temp_rec = HitRecord{};
			if (object->hit(ray, Interval{t.min, closest_so_far}, temp_rec)) {
				hit_anything = true;
				closest_so_far = temp_rec.t;
				record = temp_rec;
			}
		}

		return hit_anything;
	}
};
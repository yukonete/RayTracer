#pragma once

#include "RtWeekend.h"

#include "AABB.h"

struct Material;

struct HitRecord {
	Vec3 point;
	Vec3 normal; // Normalized
	f32 t = 0.0f;
	f32 u = 0.0f;
	f32 v = 0.0f;
	const Material *material = nullptr;
	bool front_face = false;

	void set_face_normal(const Ray &ray, const Vec3 &outward_normal) {
		front_face = dot(ray.direction, outward_normal) < 0.0f;
		if (front_face) {
			normal = outward_normal;
		} else {
			normal = -outward_normal;
		}
	}
};

struct Hittable {
	virtual bool hit(const Ray &ray, Interval t, HitRecord& record) const = 0;
	virtual const AABB& bounding_box() const = 0;
	virtual ~Hittable() = default;
};
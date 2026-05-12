#pragma once

#include <cmath>

#include "RtWeekend.h"
#include "Hittable.h"

struct Material;

struct Sphere : public Hittable {
	Ray center;
	f32 radius = 0.0f;
    Material *material = nullptr;
    AABB bbox;

    Sphere() {};
    Sphere(const Vec3 &c, f32 r, Material* material) : 
        center{c, Vec3{0.0}}, radius{std::fmax(0.0f, r)}, material{material} {
        auto radius_vec = Vec3{r};
        bbox = AABB{c - radius_vec, c + radius_vec};
    }
    Sphere(const Vec3 &c, const Vec3 &c2, f32 r, Material* material) : 
        center{c, c2 - c}, radius{std::fmax(0.0f, r)}, material{material} {
        auto radius_vec = Vec3{r};
        auto box1 = AABB{center.at(0.0f) - radius_vec, center.at(0.0f) + radius_vec};
        auto box2 = AABB{center.at(1.0f) - radius_vec, center.at(1.0f) + radius_vec};
        bbox = AABB{box1, box2};
    }

    const AABB& bounding_box() const override {
        return bbox;
    }

	bool hit(const Ray &ray, Interval t, HitRecord& record) const override {
        auto current_center = center.at(ray.time);
		auto oc = current_center - ray.origin;
        auto a = ray.direction.length_squared();
        auto h = dot(ray.direction, oc);
        auto c = oc.length_squared() - radius*radius;

        auto discriminant = h*h - a*c;
        if (discriminant < 0.0f) {
            return false;
        }

        auto sqrtd = std::sqrt(discriminant);
        auto root = (h - sqrtd) / a;
        if (!t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!t.surrounds(root)) {
                return false;
            }
        }

        record.t = root;
        record.point = ray.at(record.t);
        auto outward_normal = (record.point - current_center) / radius;
        record.set_face_normal(ray, outward_normal);
        record.material = material;
        return true;
	};
};
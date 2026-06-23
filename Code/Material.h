#pragma once

#include "Ray.h"
#include "Hittable.h"
#include "Texture.h"

struct Material {
    virtual bool scatter(const Ray &r_in, const HitRecord &rec, Color &attenuation, Ray &scattered) const = 0;
    virtual ~Material() = default;
};

struct Lambertian : public Material {
    const Texture *texture = nullptr;

    Lambertian(Texture *texture) : texture{texture} {}

    bool scatter(const Ray &r_in, const HitRecord &rec, Color &attenuation, Ray &scattered) const override {
        auto scatter_direction = rec.normal + random_unit_vector();
        if (scatter_direction.near_zero()) {
            scatter_direction = rec.normal;
        }

        scattered = Ray{rec.point, scatter_direction, r_in.time};
        attenuation = texture->value(rec.u, rec.v, rec.point);
        return true;
    }
};

struct Metal : public Material {
    Color albedo;
    f32 fuzz;

    Metal(const Color &albedo, f32 fuzz = 0.0f) : albedo{albedo}, fuzz{fuzz} {}

    bool scatter(const Ray &r_in, const HitRecord &rec, Color &attenuation, Ray &scattered) const override {
        auto reflected = reflect(r_in.direction, rec.normal);
        reflected = normalize(reflected) + (fuzz * random_unit_vector());
        scattered = Ray{rec.point, reflected, r_in.time};
        attenuation = albedo;
        return dot(scattered.direction, rec.normal) > 0.0f;
    }
};

struct Dielectric : public Material {
    // Refractive index in vacuum or air, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media
    f32 refraction_index;

    Dielectric(f32 refraction_index) : refraction_index{refraction_index} {}

    bool scatter(const Ray &r_in, const HitRecord &rec, Color &attenuation, Ray &scattered) const override {
        auto reflectance = [](f32 cosine, f32 refraction_index) -> f32 {
            // Use Schlick's approximation for reflectance.
            auto r0 = (1 - refraction_index) / (1 + refraction_index);
            r0 = r0*r0;
            return r0 + (1.0f-r0)*std::pow((1.0f - cosine),5.0f);
        };

        attenuation = Color{1.0f};
        auto ri = refraction_index;
        if (rec.front_face) {
            ri = 1.0f/refraction_index;
        }

        auto unit_direction = normalize(r_in.direction);
        f32 cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0f);
        f32 sin_theta = std::sqrt(1.0f-cos_theta*cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0f;
        auto direction = Vec3{};
        if (cannot_refract || reflectance(cos_theta, ri) > random_f32()) {
            direction = reflect(unit_direction, rec.normal);
        } else {
            direction = refract(unit_direction, rec.normal, ri);
        }

        scattered = Ray{rec.point, direction, r_in.time};
        return true;
    }
};
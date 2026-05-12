#pragma once

#include "Base.h"
#include <cmath>

struct Vec3 {
	union {
		struct {
			f32 x;
			f32 y;
			f32 z;
		};
        struct {
			f32 r;
			f32 g;
			f32 b;
		};
		f32 e[3];
	};

	Vec3(f32 x_in, f32 y_in, f32 z_in) : x{x_in}, y{y_in}, z{z_in} {}
	explicit Vec3(f32 v) : x{v}, y{v}, z{v} {}
	Vec3() : x{0.0}, y{0.0}, z{0.0} {}

	f32 operator[](int i) const { 
		return e[i]; 
	}
    f32& operator[](int i) {
		return e[i]; 
	}

	Vec3 operator-() const { 
		return Vec3{-e[0], -e[1], -e[2]}; 
	}

	Vec3& operator+=(const Vec3 &v) {
        x += v[0];
        y += v[1];
        z += v[2];
        return *this;
    }

    Vec3& operator-=(const Vec3 &v) {
        *this += -v;
        return *this;
    }

    Vec3& operator*=(f32 t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    Vec3& operator/=(f32 t) {
        *this *= 1/t;
        return *this;
    }

    f32 length() const {
        return std::sqrt(length_squared());
    }

    f32 length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    bool near_zero() const {
        // Return true if the vector is close to zero in all dimensions.
        auto s = 1e-8;
        return (std::fabs(e[0]) < s) && (std::fabs(e[1]) < s) && (std::fabs(e[2]) < s);
    }

    static Vec3 random() {
        return Vec3{random_f32(), random_f32(), random_f32()};
    }

    static Vec3 random(f32 min, f32 max) {
        return Vec3{random_f32(min, max), random_f32(min, max), random_f32(min, max)};
    }
};

using Color = Vec3;

// Vector Utility Functions

inline Vec3 operator+(Vec3 u, const Vec3 &v) {
    u += v;
    return u;
}

inline Vec3 operator-(Vec3 u, const Vec3 &v) {
    u -= v;
    return u;
}

inline Vec3 operator*(const Vec3 &u, const Vec3 &v) {
    return Vec3{u[0] * v[0], u[1] * v[1], u[2] * v[2]};
}

inline Vec3 operator*(f32 t, Vec3 v) {
    v *= t;
    return v;
}

inline Vec3 operator*(const Vec3 &v, f32 t) {
    return t * v;
}

inline Vec3 operator/(Vec3 v, f32 t) {
    v /= t;
    return v;
}

inline f32 dot(const Vec3& u, const Vec3 &v) {
    return u[0] * v[0]
         + u[1] * v[1]
         + u[2] * v[2];
}

inline Vec3 cross(const Vec3 &u, const Vec3 &v) {
    return Vec3{u[1] * v[2] - u[2] * v[1],
                u[2] * v[0] - u[0] * v[2],
                u[0] * v[1] - u[1] * v[0]};
}

inline Vec3 normalize(const Vec3& v) {
    return v / v.length();
}

inline Vec3 random_unit_vector() {
    while(true) {
        auto v = Vec3::random(-1.0, 1.0);
        auto lenght_squared = v.length_squared();
        if (1e-160 < lenght_squared && lenght_squared <= 1.0) {
            return v / std::sqrt(lenght_squared);
        }
    }
}

inline Vec3 random_on_hemisphere(const Vec3 &normal) {
    auto on_unit_sphere = random_unit_vector();
    if (dot(normal, on_unit_sphere) > 0.0) {
        return on_unit_sphere;
    }
    return -on_unit_sphere;
}

inline Vec3 random_in_unit_disk() {
    while (true) {
        auto p = Vec3(random_f32(-1.0, 1.0), random_f32(-1.0, 1.0), 0.0);
        if (p.length_squared() < 1) {
            return p;
        }
    }
}

inline Vec3 reflect(const Vec3 &v, const Vec3 &n) {
    return v - 2.0f*dot(v, n)*n;
}

inline Vec3 refract(const Vec3 &uv, const Vec3 &n, f32 etai_over_etat) {
    auto reflected_perpendicular = etai_over_etat * (uv+std::fmin(dot(-uv, n), 1.0f)*n);
    auto reflected_parallel = -std::sqrt(std::abs(1.0f-reflected_perpendicular.length_squared()))*n;
    return reflected_perpendicular + reflected_parallel;
}

template <>
struct std::formatter<Vec3> {
	template<class ParseContext>
	constexpr ParseContext::iterator parse(ParseContext &ctx) {
		return ctx.begin();
	}

	template<class FmtContext>
	FmtContext::iterator format(Vec3 vec, FmtContext &ctx) const {
		return std::format_to(ctx.out(), "({}, {}, {})", vec[0], vec[1], vec[2]);
	}
};
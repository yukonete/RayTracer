#include <thread>
#include <functional>
#include <cmath>
#include <vector>
#include <span>

#include "Base.h"
#include "Camera.h"
#include "Material.h"
#include "Ray.h"
#include "Vec3.h"
#include "Hittable.h"
#include "Interval.h"

static Color ray_color(const Ray &r, int depth, const Hittable &world) {
    if (depth <= 0) {
        return Color{0.0f};
    }

    auto hit_record = HitRecord{};
    if (world.hit(r, Interval{0.001f, infinity}, hit_record)) {
        auto scattered = Ray{};
        auto attenuation = Color{};
        if (hit_record.material->scatter(r, hit_record, attenuation, scattered)) {
            return attenuation * ray_color(scattered, depth-1, world);
        }
        return Color{0.0f};
    }

    auto unit_direction = normalize(r.direction);
    auto a = 0.5f * (unit_direction.y + 1.0f);
    return (1.0f-a)*Color{1.0f, 1.0f, 1.0f} + a*Color{0.5f, 0.7f, 1.0f};
}

void Camera::init() {
    image_height = static_cast<int>(image_width / aspect_ratio);
	if (image_height < 1) {
		image_height = 1;
	}

    auto theta = degrees_to_radians(vertical_fov);
    auto h = std::tanf(theta/2.0f);
    auto viewport_heigth = 2.0f * h * focus_dist;
    auto viewport_width = viewport_heigth * (static_cast<f32>(image_width)/image_height);

    auto w = normalize(origin - lookat);
    auto u = normalize(cross(up, w));
    auto v = cross(w, u);

    auto viewport_u = u * viewport_width;
    auto viewport_v = -v * viewport_heigth;

    pixel_delta_u = viewport_u / static_cast<f32>(image_width);
    pixel_delta_v = viewport_v / static_cast<f32>(image_height);

    auto viewport_upper_left = origin - (focus_dist * w) - viewport_u/2 - viewport_v/2;
    pixel00_location = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle/2.0f));
    defocus_disk_u = u * defocus_radius;
    defocus_disk_v = v * defocus_radius;

    pixel_samples_scale = 1.0f / samples_per_pixel;
}

Image Camera::render(const Hittable &world, int thread_count) const {
    auto result = Image{.data = std::vector<u8>(image_height * image_width * 3), .image_width = image_width, .image_height = image_height};
    render_to_buffer_multithreaded(result.data, thread_count, world);
    return result;
}

void Camera::render_to_buffer_multithreaded(std::span<u8> buffer, int thread_count, const Hittable &world) const {
    if (thread_count < 2) {
        render_to_buffer(buffer, world);
        return;
    }

    auto image_size = image_height * image_width * 3;      
    auto images = std::vector<std::vector<u8>>(thread_count);
    for (int imageIndex = 0; imageIndex < images.size(); ++imageIndex) {
        images.at(imageIndex).resize(image_size);
    }
    
    auto additional_threads = std::vector<std::jthread>(thread_count - 1);
    for (int additional_thread_index = 0;
        additional_thread_index < additional_threads.size();
        ++additional_thread_index) {
        additional_threads[additional_thread_index] = std::jthread(
            [this](std::span<u8> buffer, const Hittable &world) { 
                render_to_buffer(buffer, world); 
            }, std::span(images.at(additional_thread_index + 1)), std::cref(world));
    }
    render_to_buffer(images.at(0), world);

    for (auto &thread : additional_threads) {
        thread.join();
    }

    for (int byte_index = 0; byte_index < buffer.size(); ++byte_index) {
        int color = 0;
        for (int image_index = 0; image_index < images.size(); ++image_index) {
            color += images.at(image_index).at(byte_index);
        }
        buffer[byte_index] = static_cast<u8>(color/images.size());
    }
}

void Camera::render_to_buffer(std::span<u8> buffer, const Hittable &world) const {
    auto write_color = [](std::span<u8> buffer, int index, const Color &pixel_color) {
        auto linear_to_gamma = [](f32 linear_component) -> f32 {
            if (linear_component > 0.0f) {
                return std::sqrt(linear_component);
            }
            return 0.0f;
        };
            
        auto r = linear_to_gamma(pixel_color.r);
        auto g = linear_to_gamma(pixel_color.g);
        auto b = linear_to_gamma(pixel_color.b);

        auto r_byte = static_cast<u8>(256.0f * clamp(0.000f, 0.999f, r));
        auto g_byte = static_cast<u8>(256.0f * clamp(0.000f, 0.999f, g));
        auto b_byte = static_cast<u8>(256.0f * clamp(0.000f, 0.999f, b));

        buffer[3 * index] = r_byte;
        buffer[3 * index + 1] = g_byte;
        buffer[3 * index + 2] = b_byte;
    };

    if (buffer.size() != image_width * image_height * 3)
    {
        log("Incorrect buffer size. Not rendering.");
        return;
    }

    auto thread_id = std::this_thread::get_id();
    for (int y = 0; y < image_height; ++y) {
        log("Thread {}. Scanlines remaining: {}", thread_id, image_height - y);
        for (int x = 0; x < image_width; ++x) {
            auto pixel_color = Color{0.0f};
            for (int sample = 0; sample < samples_per_pixel; ++sample) {
                auto ray = get_ray(x, y);
                pixel_color += ray_color(ray, max_depth, world);
            }
            write_color(buffer, y * image_width + x, pixel_samples_scale * pixel_color);
        }
    }
    log("{} done.", thread_id);
}

Ray Camera::get_ray(int x, int y) const {
    auto sample_square = []() -> Vec3 {
        return Vec3{random_f32() - 0.5f, random_f32() - 0.5f, 0.0f};
    };
    auto defocus_disk_sample = [this]() -> Vec3 {
        auto p = random_in_unit_disk();
        return p[0] * defocus_disk_u + p[1] * defocus_disk_v;    
    };

    auto offset = sample_square();
    auto pixel_sample = pixel00_location + (x+offset.x)*pixel_delta_u + (y+offset.y)*pixel_delta_v;
    auto ray_origin = origin + defocus_disk_sample();
    auto ray_direction = pixel_sample - ray_origin;
    auto ray_time = random_f32();
    return Ray{ray_origin, ray_direction, ray_time};
}
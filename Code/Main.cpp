#include "BVH.h"
#include "Camera.h"
#include "Hittable.h"
#include "Material.h"
#include "RtWeekend.h"
#include "Sphere.h"
#include "stb_image_write.h"
#include <cmath>
#include <string>
#include <vector>

static int thread_count = 1;

static RenderedFrame bouncing_spheres() {
    Arena arena;

    auto hittables = std::vector<Hittable *>{};

    auto checker_texture = arena.push_item<CheckerTexture>(
        0.32f, arena.push_item<SolidColor>(Color{0.2f, 0.3f, 0.1f}),
        arena.push_item<SolidColor>(Color{0.9f, 0.9f, 0.9f}));
    auto ground_material = arena.push_item<Lambertian>(checker_texture);
    hittables.push_back(arena.push_item<Sphere>(Vec3{0.0f, -1000.0f, 0.0f},
                                                1000.0f, ground_material));

    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            auto choose_material = random_f32();
            auto center =
                Vec3{a + 0.9f * random_f32(), 0.2f, b + 0.9f * random_f32()};

            if ((center - Vec3{4.0f, 0.2f, 0.0f}).length() > 0.9f) {
                Material *sphere_material = nullptr;

                if (choose_material < 0.8f) {
                    auto albedo = Color::random() * Color::random();
                    sphere_material = arena.push_item<Lambertian>(
                        arena.push_item<SolidColor>(albedo));
                } else if (choose_material < 0.95f) {
                    auto albedo = Color::random(0.5f, 1.0f);
                    auto fuzz = random_f32(0.0f, 0.5f);
                    sphere_material = arena.push_item<Metal>(albedo, fuzz);
                } else {
                    sphere_material = arena.push_item<Dielectric>(1.5f);
                }

                auto center2 =
                    center + Vec3{0.0f, random_f32(0.0f, 0.5f), 0.0f};
                hittables.push_back(arena.push_item<Sphere>(
                    center, center2, 0.2f, sphere_material));
            }
        }
    }

    auto material1 = arena.push_item<Dielectric>(1.5f);
    hittables.push_back(
        arena.push_item<Sphere>(Vec3{0.0f, 1.0f, 0.0f}, 1.0f, material1));

    auto material2 = arena.push_item<Lambertian>(
        arena.push_item<SolidColor>(Color{0.4f, 0.2f, 0.1f}));
    hittables.push_back(
        arena.push_item<Sphere>(Vec3{-4.0f, 1.0f, 0.0f}, 1.0f, material2));

    auto material3 = arena.push_item<Metal>(Color{0.7f, 0.6f, 0.5f}, 0.0f);
    hittables.push_back(
        arena.push_item<Sphere>(Vec3{4.0f, 1.0f, 0.0f}, 1.0f, material3));

    auto world = arena.push_item<BVHNode>(&arena, hittables);

    Camera camera{};
    camera.aspect_ratio = 16.0f / 9.0f;
    camera.image_width = 960;
    camera.samples_per_pixel =
        static_cast<int>(std::ceilf(100.0f / static_cast<f32>(thread_count)));
    camera.max_depth = 50;

    camera.vertical_fov = 20;
    camera.origin = Vec3{13.0f, 2.0f, 3.0f};
    camera.lookat = Vec3{0.0f, 0.0f, 0.0f};
    camera.up = Vec3{0.0f, 1.0f, 0.0f};

    camera.defocus_angle = 0.6f;
    camera.focus_dist = 10.0f;

    camera.init();
    return camera.render(*world, thread_count);
}

static RenderedFrame checked_spheres() {
    Arena arena;

    auto hittables = std::vector<Hittable *>{};

    auto checker_texture = arena.push_item<CheckerTexture>(
        0.32f, arena.push_item<SolidColor>(Color{0.2f, 0.3f, 0.1f}),
        arena.push_item<SolidColor>(Color{0.9f, 0.9f, 0.9f}));
    auto material = arena.push_item<Lambertian>(checker_texture);

    hittables.push_back(
        arena.push_item<Sphere>(Vec3{0.0f, -10.0f, 0.0f}, 10.0f, material));
    hittables.push_back(
        arena.push_item<Sphere>(Vec3{0.0f, 10.0f, 0.0f}, 10.0f, material));

    auto world = arena.push_item<BVHNode>(&arena, hittables);

    Camera camera{};
    camera.aspect_ratio = 16.0f / 9.0f;
    camera.image_width = 960;
    camera.samples_per_pixel =
        static_cast<int>(std::ceilf(100.0f / static_cast<f32>(thread_count)));
    camera.max_depth = 50;

    camera.vertical_fov = 20;
    camera.origin = Vec3{13.0f, 2.0f, 3.0f};
    camera.lookat = Vec3{0.0f, 0.0f, 0.0f};
    camera.up = Vec3{0.0f, 1.0f, 0.0f};

    camera.defocus_angle = 0.0f;

    camera.init();
    return camera.render(*world, thread_count);
}

RenderedFrame earth() {
	Arena arena;

	auto earth_image = Image::load("earthmap.jpg");
	auto image_texture = ImageTexture{&earth_image};
	auto earth_material = Lambertian{&image_texture};
	auto globe = Sphere{Vec3{0.0f}, 2.0f, &earth_material};

	Camera camera{};
    camera.aspect_ratio = 16.0f / 9.0f;
    camera.image_width = 960;
    camera.samples_per_pixel =
        static_cast<int>(std::ceilf(100.0f / static_cast<f32>(thread_count)));
    camera.max_depth = 50;

    camera.vertical_fov = 20;
    camera.origin = Vec3{0.0f, 0.0f, 12.0f};
    camera.lookat = Vec3{0.0f, 0.0f, 0.0f};
    camera.up = Vec3{0.0f, 1.0f, 0.0f};

    camera.defocus_angle = 0.0f;

    camera.init();
    return camera.render(globe, thread_count);
}

int main(int argc, char *argv[]) {
    int scene = 3;
    if (argc > 1) {
        try {
            scene = std::stoi(argv[1]);
        } catch (...) {
            log("Cound not parse scene number.");
        }
        if (argc > 2) {
            try {
                int threads_requested = std::stoi(argv[2]);
                if (threads_requested > 1) {
                    thread_count = threads_requested;
                } else {
                    log("Number of threads should be >= 1.");
                }
            } catch (...) {
                log("Cound not parse number of threads.");
            }
        }
    }
    log("Running with {} threads.", thread_count);

    RenderedFrame image;
    switch (scene) {
        case 1: {
            image = bouncing_spheres();
            break;
        }
        case 2: {
            image = checked_spheres();
            break;
        }
		case 3: {
            image = earth();
            break;
        }
        default: {
            log("Scene with number {} does not exists.", scene);
            return 1;
        }
    }

    stbi_write_png("output.png", image.image_width, image.image_height, 3,
                   image.data.data(), image.image_width * 3);
}

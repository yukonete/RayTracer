#include <vector>
#include <cmath>
#include <string>
#include "RtWeekend.h"
#include "stb_image_write.h"
#include "Hittable.h"
#include "Sphere.h"
#include "Camera.h"
#include "Material.h"
#include "BVH.h"

int main(int argc, char *argv[]) {
	int thread_count = 12;
	if (argc > 1) {
		try {
			int threads_requested = std::stoi(argv[1]);
			if (threads_requested > 1) {
				thread_count = threads_requested;
			}
		} catch (...) {
			log("Cound not parse number of threads");
		}
	}
	
	Arena arena;

	auto hittables = std::vector<Hittable*>{};

	auto ground_material = arena.push_item<Lambertian>(Color{0.5f, 0.5f, 0.5f});
	hittables.push_back(arena.push_item<Sphere>(Vec3{0.0f, -1000.0f, 0.0f}, 1000.0f, ground_material));

	for (int a = -11; a < 11; ++a) {
		for (int b = -11; b < 11; ++b) {
			auto choose_material = random_f32();
			auto center = Vec3{a+0.9f*random_f32(), 0.2f, b+0.9f*random_f32()};

			if ((center - Vec3{4.0f, 0.2f, 0.0f}).length() > 0.9f) {
				Material *sphere_material = nullptr;

				if (choose_material < 0.8f) {
					auto albedo = Color::random() * Color::random();
					sphere_material = arena.push_item<Lambertian>(albedo);
				} else if (choose_material < 0.95f) {
					auto albedo = Color::random(0.5f, 1.0f);
					auto fuzz = random_f32(0.0f, 0.5f);
					sphere_material = arena.push_item<Metal>(albedo, fuzz);
				} else {
					sphere_material = arena.push_item<Dielectric>(1.5f);
				}

				//auto center2 = center + Vec3{0.0f, random_f32(0.0f, 0.5f), 0.0f};
				hittables.push_back(arena.push_item<Sphere>(center, 0.2f, sphere_material));
			}
		}
	}
	
	auto material1 = arena.push_item<Dielectric>(1.5f);
	hittables.push_back(arena.push_item<Sphere>(Vec3{0.0f, 1.0f, 0.0f}, 1.0f, material1));

	auto material2 = arena.push_item<Lambertian>(Color{0.4f, 0.2f, 0.1f});
	hittables.push_back(arena.push_item<Sphere>(Vec3{-4.0f, 1.0f, 0.0f}, 1.0f, material2));

	auto material3 = arena.push_item<Metal>(Color{0.7f, 0.6f, 0.5f}, 0.0f);
	hittables.push_back(arena.push_item<Sphere>(Vec3{4.0f, 1.0f, 0.0f}, 1.0f, material3));

	auto world = arena.push_item<BVHNode>(&arena, hittables);

	Camera camera{};
	camera.aspect_ratio = 16.0f / 9.0f;
	camera.image_width = 1920;
	camera.samples_per_pixel = static_cast<int>(std::ceilf(500.0f / static_cast<f32>(thread_count)));
	camera.max_depth = 50;

	camera.vertical_fov = 20;
	camera.origin = Vec3{13.0f, 2.0f, 3.0f};
	camera.lookat = Vec3{0.0f, 0.0f, 0.0f};
	camera.up = Vec3{0.0f, 1.0f, 0.0f};

	camera.defocus_angle = 0.6f;
	camera.focus_dist = 10.0f;

	camera.init();
	auto image = camera.render(*world, thread_count);
	stbi_write_png("output.png", image.image_width, image.image_height, 3, image.data.data(), image.image_width * 3);
}

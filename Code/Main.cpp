#include <cmath>
#include <string_view>
#include <thread>
#include <vector>

#include "BVH.h"
#include "Camera.h"
#include "Hittable.h"
#include "Image.h"
#include "Material.h"
#include "RtWeekend.h"
#include "Sphere.h"
#include "flag.hpp"
#include "stb_image_write.h"

struct Args {
    int scene = 0;
    int thread_count = 0;
    std::string_view out;
    bool help = false;

    static std::optional<Args> parse(int argc, char **argv) {
        auto &scene =
            *FlagHpp::flag_int<int>("scene", 1, "Which scene to render.");
        auto &thread_count = *FlagHpp::flag_int<int>(
            "threads", std::thread::hardware_concurrency(),
            "How many threads to use for rendering.");
        auto &out =
            *FlagHpp::flag_string("out", "output.png", "Output file name.");
        auto &help =
            *FlagHpp::flag_bool("help", false, "Show this help message.");

        if (!FlagHpp::parse(argc, argv) || out.empty()) {
            if (out.empty()) {
                std::println("out can not be an empty string");
            }
            FlagHpp::print_usage(stdout);
            return {};
        }

        return Args{
            .scene = scene,
            .thread_count = thread_count,
            .out = out,
            .help = help,
        };
    }
};

static Args args = {};

//static Camera::Config camera_config = {
//    .aspect_ratio = 16.0f / 9.0f,
//    .image_width = 960,
//    .samples_per_pixel = 400,
//    .max_depth = 50,
//};

static Camera::Config camera_config = {
    .aspect_ratio = 16.0f / 9.0f,
    .image_width = 100,
    .samples_per_pixel = 10,
    .max_depth = 10,
};

static RenderedFrame render(const Hittable &world,
                            const Camera::Config &config) {
    Camera camera{config};
    camera.init();
    return camera.render(world, args.thread_count);
}

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

    auto world = BVHNode{&arena, hittables};

    auto config = camera_config;
    config.vertical_fov = 20;
    config.origin = Vec3{13.0f, 2.0f, 3.0f};
    config.lookat = Vec3{0.0f, 0.0f, 0.0f};
    config.up = Vec3{0.0f, 1.0f, 0.0f};

    config.defocus_angle = 0.6f;
    config.focus_dist = 10.0f;

    return render(world, config);
}

static RenderedFrame checked_spheres() {
    Arena arena;

    auto hittables = std::vector<Hittable *>{};

    auto checker_texture = arena.push_item(CheckerTexture{
        0.32f, arena.push_item(SolidColor{Color{0.2f, 0.3f, 0.1f}}),
        arena.push_item(SolidColor{Color{0.9f, 0.9f, 0.9f}})});
    auto material = arena.push_item(Lambertian{checker_texture});

    hittables.push_back(
        arena.push_item(Sphere{Vec3{0.0f, -10.0f, 0.0f}, 10.0f, material}));
    hittables.push_back(
        arena.push_item(Sphere{Vec3{0.0f, 10.0f, 0.0f}, 10.0f, material}));

    auto world = BVHNode{&arena, hittables};

    auto config = camera_config;
    config.vertical_fov = 20;
    config.origin = Vec3{13.0f, 2.0f, 3.0f};
    config.lookat = Vec3{0.0f, 0.0f, 0.0f};
    config.up = Vec3{0.0f, 1.0f, 0.0f};

    return render(world, config);
}

static RenderedFrame earth() {
    auto earth_image = Image::load("earthmap.jpg");
    auto image_texture = ImageTexture{&earth_image};
    auto earth_material = Lambertian{&image_texture};
    auto globe = Sphere{Vec3{0.0f}, 2.0f, &earth_material};

    auto config = camera_config;
    config.vertical_fov = 20.0f;
    config.origin = Vec3{0.0f, 0.0f, 12.0f};
    config.lookat = Vec3{0.0f, 0.0f, 0.0f};
    config.up = Vec3{0.0f, 1.0f, 0.0f};

    return render(globe, config);
}

static RenderedFrame perlin_spheres() {
    Arena arena;

    std::vector<Hittable *> hittables;

    auto pertext = NoiseTexture{4.0f};
    auto material = Lambertian{&pertext};

    hittables.push_back(arena.push_item(
        Sphere{Vec3(0.0f, -1000.0f, 0.0f), 1000.0f, &material}));
    hittables.push_back(
        arena.push_item(Sphere{Vec3(0.0f, 2.0f, 0.0f), 2.0f, &material}));

    auto world = BVHNode{&arena, hittables};

    auto config = camera_config;
    config.vertical_fov = 20.0f;
    config.origin = Vec3(13.0f, 2.0f, 3.0f);
    config.lookat = Vec3(0.0f, 0.0f, 0.0f);
    config.up = Vec3(0.0f, 1.0f, 0.0f);

    return render(world, config);
}

int main(int argc, char *argv[]) {
    auto args = Args::parse(argc, argv).or_else([]() -> std::optional<Args> {
        std::exit(1);
    }).value();

    if (args.help) {
        FlagHpp::print_usage(stdout);
        return 0;
    }

    camera_config.samples_per_pixel = static_cast<int>(
        std::ceil(static_cast<f32>(camera_config.samples_per_pixel) /
                  static_cast<f32>(args.thread_count)));

    log("Running with {} threads.", args.thread_count);
    log("Selected scene is {}.", args.scene);

    RenderedFrame image;
    switch (args.scene) {
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
        case 4: {
            image = perlin_spheres();
            break;
        }
        default: {
            log("Scene with number {} does not exists.", args.scene);
            return 1;
        }
    }

    stbi_write_png(args.out.data(), image.image_width, image.image_height, 3,
                   image.data.data(), image.image_width * 3);
}

#pragma once


#include <vector>
#include <span>
#include "Base.h"
#include "Hittable.h"


struct RenderedFrame {
    std::vector<u8> data;
    int image_width = 0;
    int image_height = 0;
};

struct Camera {
	f32 aspect_ratio = 1.0f;
    int image_width  = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;

    f32 vertical_fov = 90.0f;

    Vec3 origin = Vec3{0.0f};
    Vec3 lookat = Vec3{0.0f, 0.0f, -1.0f};
    Vec3 up = Vec3{0.0f, 1.0f, 0.0f};

    f32 defocus_angle = 0.0f;  // Variation angle of rays through each pixel
    f32 focus_dist = 10.0f;    // Distance from camera lookfrom point to plane of perfect focus

    void init();
    RenderedFrame render(const Hittable &world, int thread_count = 1) const;
    void render_to_buffer_multithreaded(std::span<u8> buffer, int thread_count, const Hittable &world) const;
	void render_to_buffer(std::span<u8> buffer, const Hittable &world) const;
    Ray get_ray(int x, int y) const;

private: 
    int image_height = 0;
    f32 pixel_samples_scale = 0.0f;

    Vec3 pixel00_location;        
    Vec3 pixel_delta_u;      
    Vec3 pixel_delta_v;    

    Vec3 defocus_disk_u;       // Defocus disk horizontal radius
    Vec3 defocus_disk_v;       // Defocus disk vertical radius
};
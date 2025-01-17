#include "rtweekend.h"

#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"
#include "camera.h"
#include "aarect.h"

#include <thread>
#include <functional>
#include <iostream>

color ray_color(const ray& r, const color& background, const hittable& world, int depth) {
    hit_record rec;

	if (depth <= 0) {
		return color(0, 0, 0);
	}

	// If the ray hits nothing, return the background color.
    if (!world.hit(r, 0.001, infinity, rec))
        return background;

    ray scattered;
    color attenuation;
    color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);

    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted;

    return emitted + attenuation * ray_color(scattered, background, world, depth-1);
}

hittable_list random_scene() {
    hittable_list world;

    auto ground_material = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(ground_material)));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

hittable_list simple_light() {
	hittable_list objects;

	//auto pertext = make_shared<noise_texture>(4);
	auto pertext = make_shared<solid_color>(0.2, 0.1, 0.6);
	objects.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
	objects.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

	auto difflight = make_shared<diffuse_light>(color(4, 4, 4));
	objects.add(make_shared<xy_rect>(3, 5, 1, 3, -2, difflight));

	return objects;
}

int main() {
	std::cerr << "Starting Render\n";

    // Image

    const auto aspect_ratio = 3.0 / 2.0;
    const int image_width = 300;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    int samples_per_pixel = 100;
    const int max_depth = 50;

    // World

	color background(0, 0, 0);
    hittable_list world;

    // Camera

    point3 lookfrom;
    point3 lookat;
    vec3 vup(0,1,0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0;

    auto cam = make_shared<camera>(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

	switch(1) {
		case 1:
			world = random_scene();
			background = color(0.70, 0.80, 1.00);
			lookfrom = point3(13,2,3);
			lookat = point3(0,0,0);
			aperture = 0.1;
		default:
		case 5:
			world = simple_light();
			samples_per_pixel = 300;
			background = color(0, 0, 0);
			lookfrom = point3(26, 3, 6);
			lookat = point3(0, 2, 0);
			aperture = 0.1;
			break;
	}

	// Threading
	const int threads = 10;
	const int samples_per_pixel_per_thread = samples_per_pixel / threads;
	const int pixels = image_height * image_width;
	std::vector<std::thread> pool;			
	std::vector<color*> images;	// Each thread renders and image which is combined and averaged

	std::cerr << "Threads: " << threads << "\nSamples per pixel: " << samples_per_pixel << "\nWidth: " << image_width << "\n" << std::flush;
    // Render

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	auto renderPass = [
		&image_height, 
		&image_width, 
		&samples_per_pixel_per_thread, 
		&world, 
		&cam, 
		&max_depth,
		&background
	] (color outImage[]) {
		int curPixel = 0;
		for (int j = image_height-1; j >= 0; --j) {
			// std::cerr << "\rScanlines remaining: " << j << ' ';
			for (int i = 0; i < image_width; ++i) {
				color pixel_color(0, 0, 0);
				for (int s = 0; s < samples_per_pixel_per_thread; s++) {
					auto u = (i + random_double()) / (image_width - 1);
					auto v = (j + random_double()) / (image_height - 1);
					ray r = cam->get_ray(u, v);
					pixel_color += ray_color(r, background, world, max_depth);
				}
				// write_color(std::cout, pixel_color, samples_per_pixel);
				// outImage->push_back(pixel_color);
				outImage[curPixel++] = pixel_color;
			}
		}	
	};

	for (int i = 0; i < threads; i++) {
		color *outImage = new color[pixels];
		
		pool.push_back(std::thread(renderPass, outImage));

		images.push_back(outImage);
	}

	for (int i = 0; i < threads; i++) {
		std::cerr << "\rWaiting for thread " << i << ' ' << std::flush;
		pool.at(i).join();
	}
	

	std::cerr << "\nimages size: " << images.size() << "\n";
	for (int j = 0; j < pixels; j++) {
		color pixel_color(0.0, 0.0, 0.0);
		for (int i = 0; i < images.size(); i++) {
			pixel_color += images[i][j];
		}

		write_color(std::cout, pixel_color, samples_per_pixel);
	}

	// delete the images
	for (int i = 0; i < images.size(); i++) {
		delete images[i];
	}
}
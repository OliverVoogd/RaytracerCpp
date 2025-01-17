#include <iostream>
#include <string>
#include <fstream>

int main() {
	std::string filename = "image.ppm";
	std::ofstream file;
	file.open(filename, std::ofstream::out);

	const int image_width = 256;
	const int image_height = 256;

	file << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	for (int j = image_height -1; j >= 0; j--) {
		std::cout << "\rScanlines remaining: " << j << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i) {
			auto r = double(i) / (image_width - 1);
			auto g = double(j) / (image_height - 1);
			auto b = 0.25;

			int ir = static_cast<int>(255.999 * r);
			int ig = static_cast<int>(255.999 * g);
			int ib = static_cast<int>(255.999 * b);

			file << ir << " " << ig << ' ' << ib << '\n';
		}
	}

	file.close();
}
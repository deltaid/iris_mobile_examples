#include <vector>
#include <string>
#include <iostream>

#include "utils.h"
#include "iris_engine_v3.h"

#define BUFFER_SIZE (1000 * 1000)

/**
 * @brief Predefined image width for this example.
 *
 */
#define WIDTH (640)
/**
 * @brief Predefined image height for this example.
 *
 */
#define HEIGHT (480)

/**
 * @brief demonstrates how to get the kind3/7 images using ire3 API.
 *
 * @param pixels pointer to image pixels,
 * @param width image width,
 * @param height image height,
 * @param name_prefix prefix to save the output files.
 */
void ire_kind7(uint8_t *pixels, int width, int height,
               const std::string &name_prefix) {
  size_t max_features_size, working_set_size;
  ire3_settings settings = {sizeof(ire3_settings), 200, 6};
  auto rc = ire3_get_max_features_size(&max_features_size);
  rc = ire3_get_extraction_working_set_size(&working_set_size, &settings);
  std::vector<uint8_t> featurev(max_features_size);
  uint8_t *features = featurev.data();
  std::vector<uint8_t> working_setv(working_set_size);
  uint8_t *working_set = working_setv.data();

#define A_WIDTH 320
#define A_HEIGHT 240

  uint8_t kind7[A_WIDTH * A_HEIGHT];
  uint8_t kind3[A_WIDTH * A_HEIGHT];
  ire3_eye_info eye_info = {0};
  ire3_out_images images;
  images.out_image_width = A_WIDTH;
  images.out_image_height = A_HEIGHT;
  images.kind7_pixels = kind7;
  images.kind3_pixels = kind3;

  ire3_extract_features_with_images(pixels, width, height, features, max_features_size,
                        NULL, &eye_info, sizeof(eye_info), working_set,
                        working_set_size, &settings, &images);
  std::cerr << "Sharpness = " << eye_info.sharpness << "\n";
  std::cerr << "Occlusion = " << eye_info.percent_occlusion << "%\n";
  auto name = name_prefix;
  name.replace(name.find(".pgm"), 4, "_ire3_kind3.pgm");
  write_pgm(name.c_str(), images.kind3_pixels, images.out_image_width,
            images.out_image_height);
  name = name_prefix;
  name.replace(name.find(".pgm"), 4, "_ire3_kind7.pgm");
  write_pgm(name.c_str(), images.kind7_pixels, images.out_image_width,
            images.out_image_height);
}

/**
 * @brief Entry point of the example. Saves output files alongside the input
 * file.
 *
 * Requires the `.pgm` binary file with image of size @ref WIDTH x @ref HEIGHT
 * as an input.
 */
int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Exactly one argument is needed: filename\n";
    return 1;
  }
  std::string file(argv[1]);
  std::vector<uint8_t> buffer(BUFFER_SIZE);
  uint8_t *pixels = buffer.data();
  unsigned int width = WIDTH;   // Width of frame or image in pixels
  unsigned int height = HEIGHT; // Height of frame or image in pixels
  unsigned int depth = 0;
  read_pgm(file.c_str(), &pixels, &width, &height, &depth);
  std::cout << "ire_enroll_get_kind3/7 started\n";
  ire_kind7(pixels, width, height, file);
  std::cout << "ire_enroll_get_kind3/7 ended\n";
}

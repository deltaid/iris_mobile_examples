
#include <iostream>
#include <stdint.h>
#include <vector>

#include "utils.h"
#include "iris_engine_v3.h"

#define CHECK(expr, rc)                                                        \
  do {                                                                         \
    if (!(expr)) {                                                             \
      printf("Error: 0x%08x\n", rc);                                           \
    }                                                                          \
  } while (0)

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
 * @brief demonstrates how to enroll and identify from eye crop using ire3 API.
 *
 * @param image pointer to image pixels,
 * @param width image width,
 * @param height image height,
 */
void enroll_identify(uint8_t *image, int width, int height) {
  int score = 0;       // The final score for matching an image with a template
  size_t size = 0;     // The maximum size of the feature structure
  size_t set_size = 0; // The size of the working set (depends on the nominal
                       // resolution of the image)
  uint8_t *working_set = NULL; // Pointer to the working set structure
  uint8_t *features =
      NULL; // Pointer to the serialized (encrypted) feature structure
  size_t feature_size =
      0; // The current size of the serialized (encrypted) feature structure
  size_t des_size =
      0; // The maximum size of the deserialized (decrypted) feature structure
  uint8_t *des_ftr =
      NULL; // Pointer to the deserialized (decrypted) feature structure
  ire3_eye_info eye_info; // The structure to hold the eye information
  ire3_settings settings = {sizeof(settings), 150, 4};

  CHECK(ire3_get_max_features_size(&size) == IRE3_STATUS_OK, IRE3_STATUS_FAIL);
  features = (uint8_t *)malloc(size);

  CHECK(ire3_get_extraction_working_set_size(&set_size, &settings) ==
            IRE3_STATUS_OK,
        IRE3_STATUS_FAIL);
  working_set = (uint8_t *)malloc(set_size);

  CHECK(ire3_get_deserialized_features_size(&des_size) == IRE3_STATUS_OK,
        IRE3_STATUS_FAIL);
  des_ftr = (uint8_t *)malloc(des_size);

  CHECK(ire3_extract_features(image, width, height, features, size,
                              &feature_size, &eye_info, sizeof(ire3_eye_info),
                              (void *)working_set, set_size, &settings,
                              NULL) == IRE3_STATUS_OK,
        IRE3_STATUS_FAIL);

  CHECK(ire3_deserialize_features(features, feature_size, des_ftr, des_size) ==
            IRE3_STATUS_OK,
        IRE3_STATUS_FAIL);

  // Compare the extracted feature of the image to itself as a test
  CHECK(ire3_compare(des_ftr, des_ftr, &settings, &score) == IRE3_STATUS_OK,
        IRE3_STATUS_FAIL);

  printf("The score is: %d\n", score);

  free(features);
  free(working_set);
  free(des_ftr);
  free(image);
}

/**
 * @brief Entry point of the example.
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
  std::cout << "ire_enroll -> ire_identify started\n";
  enroll_identify(pixels, width, height);
  std::cout << "ire_enroll -> ire_identify ended\n";
}
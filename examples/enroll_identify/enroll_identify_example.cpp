/// @file example.cpp
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <fstream>

#include "iris_mobile_v2.h"
#include "utils.h"

void on_score(void *user_context, int i_template, int score) {
  std::cerr << "Scores for template " << i_template << ": " << score << "\n";
}

class Context {
public:
  explicit Context(std::size_t size) : size(size) {
    memory = (uint8_t *)malloc(size);
  }
  ~Context() { free(memory); }

  std::size_t size;
  uint8_t *memory = nullptr;
};

#define BUFFER_SIZE (1000 * 1000)

/**
 * @brief Demonstrates the simple enroll/identify pipeline for 2 eyes.
 *
 * @param pixels is the poiter to the image pixels.
 * @param width image width.
 * @param height image height.

 */
void enroll_identify(uint8_t *pixels, int width, int height) {
  // Allocate context of proper size
  Context enrollment_context(IRM2_GET_CONTEXT_SIZE(1, width, height));
  // Variables for output information
  irm2_eye_info eye_info = {0};
  irm2_enrollment_info enr_info = {0};
  irm2_ui_hints ui_hints = {0};

  // Image parameters: nominal resolution and camera resolution
  // are crucial for the correct work of the program.
  int32_t nom_res = 160;  // px per cm
  int32_t cam_res = 3500; // px per rad;
  uint32_t num_eyes = 2;  // 1 eye on the image
  uint32_t rotation = 0;  // portrait

  // No special flags needed for monochromatic image sensor
  // and non `raw10` aquisition format.
  int b_flags = IRM2_FLAG_NONE;

  irm2_settings s = {
      sizeof(s), (int)width, (int)height, cam_res, nom_res, NULL,
      on_score,  NULL,       NULL,        b_flags, NULL,
      (int)width // stride
  };

  auto rc = irm2_init_enrollment(enrollment_context.memory,
                                 enrollment_context.size, num_eyes, &s);

  // The enrollment loop evaluates N steps, every step is
  // a loop which performs until counting the `num_updates`
  // successfull template updates to ensure the consistent enrollment.
  for (;;) {
    for (int i = 0;; ++i) {
      // Process single frame.
      auto on_frame_rc =
          irm2_on_frame(enrollment_context.memory, pixels, rotation);
      auto hints_rc = irm2_get_ui_hints(enrollment_context.memory, &ui_hints,
                                        sizeof(ui_hints));
      auto enr_info_rc = irm2_get_enrollment_info(enrollment_context.memory,
                                                  &enr_info, sizeof(enr_info));

      std::cout << "Iteration: " << std::dec << i << ": " << std::hex
                << "on frame returns " << on_frame_rc << "\n";
      std::cout << std::hex << "Hints return:  " << hints_rc
                << "; hint: " << ui_hints.eye_distance << "\n";
      std::cout << std::dec << enr_info.step_progress << "% / "
                << enr_info.overall_progress << "%\n";
      // check if capture process is successeed.
      if (enr_info.step_progress == 100)
        break;
    }
    if (enr_info.overall_progress == 100)
      break;
    irm2_continue_enrollment(enrollment_context.memory);
  }

  // Get the template
  uint8_t iris_template[IRM2_TEMPLATE_SIZE];
  irm2_get_template(enrollment_context.memory, iris_template,
                    IRM2_TEMPLATE_SIZE);

  Context identification_context(IRM2_GET_CONTEXT_SIZE(1, width, height));
  const uint8_t *iris_template_pointer[] = {&iris_template[0]};
  rc = irm2_init_identification(identification_context.memory,
                                identification_context.size,
                                iris_template_pointer, 1, 100, &s);
  if (rc) {
    std::cerr << "Identification init fails: " << std::hex << rc << "\n";
    return;
  }

  for (int i = 0;; ++i) {
    // Process single frame.
    auto on_frame_rc =
        irm2_on_frame(identification_context.memory, pixels, rotation);
    int template_id = -1;
    auto result_rc = irm2_get_identification_result(
        identification_context.memory, &template_id);
    std::cout << "on frame returns: " << std::hex << on_frame_rc
              << " result returns: " << result_rc
              << "; result template: " << std::dec << template_id << "\n";

    // check if capture process is successeed.
    if (on_frame_rc == 0)
      break;
  }
}

/**
 * @brief Predefined image width for this example.
 *
 */
#define WIDTH (2336)
/**
 * @brief Predefined image height for this example.
 *
 */
#define HEIGHT (769)

/**
 * @brief Entry point of the example.
 *
 * Requires the binary file with RAW frame of size @ref WIDTH x @ref HEIGHT as
 * an input.
 */
int main(int argc, char *argv[]) {
  std::string file(argv[1]);
  unsigned int width = WIDTH;   // Width of frame or image in pixels
  unsigned int height = HEIGHT; // Height of frame or image in pixels
  unsigned int depth = 0;

  std::vector<char> buf(width * height);
  std::ifstream in(file, std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    std::cerr << file << " not open.\n";
    return;
  }
  in.read(buf.data(), width * height);

  enroll_identify((uint8_t *)buf.data(), width, height);
}
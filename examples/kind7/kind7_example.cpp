#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>

#include "iris_mobile_v2.h"
#include "iris_mobile_v2_capture.h"
#include "iris_image_record.h"
#include "utils.h"

void on_score(void *user_context, int i_template, int score) {
  std::cout << "Scores for template " << i_template << ": " << score << "\n";
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
 * @brief Demonstrates the enrolment end identification from 1 eye crop. The
 * input should be `.pgm` BINARY image
 *
 * @param pixels is the poiter to the image pixels.
 * @param width image width.
 * @param height image height.
 */
void enroll_identify_1_eye(uint8_t *pixels, int width, int height) {
  // Allocate context of proper size
  Context ctx(IRM2_GET_CONTEXT_SIZE(1, width, height));
  // Variables for output information
  irm2_eye_info eye_info = {0};
  irm2_enrollment_info enr_info = {0};
  irm2_ui_hints ui_hints = {0};

  // Image parameters: nominal resolution and camera resolution
  // are crucial for the correct work of the program.
  int32_t nom_res = 200;  // px per cm
  int32_t cam_res = 4000; // px per rad;
  uint32_t num_eyes = 1;  // 1 eye on the image
  uint32_t rotation = 0;  // portrait

  // No special flags needed for monochromatic image sensor
  // and non `raw10` aquisition format.
  int b_flags = IRM2_FLAG_NONE;

  irm2_settings s = {
      sizeof(s), (int)width, (int)height, cam_res, nom_res,
      // No callbacks set on processing steps
      NULL, on_score, NULL, NULL, b_flags,
      // No callback on camera control
      NULL,
      (int)width // stride
  };

  // Initialize enrollment context with all settings.
  auto rc = irm2_init_enrollment(ctx.memory, ctx.size, num_eyes, &s);
  // TODO propper comment here
  for (;;) {
    for (int i = 0;; ++i) {
      // Process single frame.
      auto on_frame_rc = irm2_on_frame(ctx.memory, pixels, rotation);

      auto hints_rc =
          irm2_get_ui_hints(ctx.memory, &ui_hints, sizeof(ui_hints));
      auto info_rc =
          irm2_get_capture_info(ctx.memory, &enr_info, sizeof(enr_info));

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
    irm2_continue_enrollment(ctx.memory);
  }

  // Get the template
  uint8_t iris_template[IRM2_TEMPLATE_SIZE];
  irm2_get_template(ctx.memory, iris_template, IRM2_TEMPLATE_SIZE);

  // Initialize identification context with all settings.
  Context identification_context(IRM2_GET_CONTEXT_SIZE(1, width, height));
  const uint8_t *const templates[1] = {iris_template};
  const uint8_t *iris_template_pointer[] = {&iris_template[0]};
  rc = irm2_init_identification(identification_context.memory,
                                identification_context.size,
                                iris_template_pointer, 1, 100, &s);
  if (rc) {
    std::cout << "Identification init fails: " << std::hex << rc << "\n";
    return;
  }

  // Check one frame
  auto on_frame_rc = irm2_on_frame(identification_context.memory, pixels, 0);
  int template_id = -1;
  auto result_rc = irm2_get_identification_result(identification_context.memory,
                                                  &template_id);
  std::cout << "on frame returns: " << std::hex << on_frame_rc
            << " result returns: " << result_rc
            << "; result template: " << std::dec << template_id << "\n";
}

/**
 * @brief Demonstrate the chain capture->kind7->JPEG2000->unpack.
 *
 * @param pixels is the poiter to the image pixels.
 * @param width image width.
 * @param height image height.
 * @param name_prefix prefix to save the files produced by function.
 */
void capture_kind7_jpeg2000(uint8_t *pixels, int width, int height,
                            const std::string &name_prefix) {
  // Allocate context of proper size
  Context ctx(IRM2_GET_CAPTURE_CONTEXT_SIZE(width, height));
  // Variables for output information
  irm2_eye_info eye_info = {0};
  irm2_enrollment_info enr_info = {0};
  irm2_ui_hints ui_hints = {0};

  // Image parameters: nominal resolution and camera resolution
  // are crucial for the correct work of the program.
  int32_t nom_res = 200;  // px per cm
  int32_t cam_res = 4000; // px per rad;
  uint32_t num_eyes = 1;  // 1 eye on the image
  uint32_t rotation = 0;  // portrait

  // No special flags needed for monochromatic image sensor
  // and non `raw10` aquisition format.
  int b_flags = IRM2_FLAG_NONE;

  irm2_settings s = {
      sizeof(s), (int)width, (int)height, cam_res, nom_res,
      // No callbacks set on processing steps
      NULL, on_score, NULL, NULL, b_flags,
      // No callback on camera control
      NULL,
      (int)width // stride
  };

  int num_updates = 10;
  // Configure capture settings -1 for default.
  // Check iris_mobile_v2_capture.h for more.
  irm2_capture_settings cs = {sizeof(irm2_capture_settings), -1,
                              // Number of template updates to ensure
                              // enrolment consistency.
                              num_updates, -1, -1, -1, -1, -1, -1};
  // Intitializing of the contest for operation.
  auto rc = irm2_init_capture(ctx.memory, ctx.size, num_eyes, &s, &cs);
  if (rc) {
    std::cout << "Capture init fails: " << std::hex << rc << "\n";
    return;
  }

  // The capture loop performs until counting the `num_updates`
  // successfull template updates to ensure the consistent enrollment.
  for (int i = 0;; ++i) {
    // Process single frame.
    auto on_frame_rc = irm2_on_frame(ctx.memory, pixels, rotation);

    auto hints_rc = irm2_get_ui_hints(ctx.memory, &ui_hints, sizeof(ui_hints));
    auto info_rc =
        irm2_get_capture_info(ctx.memory, &enr_info, sizeof(enr_info));

    std::cout << "Iteration: " << std::dec << i << ": " << std::hex
              << "on frame returns " << on_frame_rc << "\n";
    std::cout << std::hex << "Hints return:  " << hints_rc
              << "; hint: " << ui_hints.eye_distance << "\n";
    std::cout << std::dec << enr_info.step_progress << "%\n";
    // check if capture process is successeed.
    if (on_frame_rc == 0)
      break;
  }

  // Allocate space for result image.
  uint8_t cropped[IRM2_CROPPED_WIDTH * IRM2_CROPPED_HEIGHT] = {0};
  // Extract kind 7 type of image
  rc = irm2_get_kind7_image(ctx.memory, IRM2_EYE_UNDEF, IRM2_CROPPED_WIDTH,
                            IRM2_CROPPED_HEIGHT, cropped);
  // Save kind 7 image
  std::string name(name_prefix);
  name.replace(name.find(".pgm"), 4, "_kind7.pgm");
  write_pgm(name.c_str(), cropped, IRM2_CROPPED_WIDTH, IRM2_CROPPED_HEIGHT);

  //-----------------------Packing customized JPEG2000, for more see
  // iris_image_record.h-----------------

  // prepare the header
  IirInfo ii;
  memset(&ii, 0, sizeof(ii));
  memcpy(ii.ich.FormatId, "IIR\0", 4);
  ii.IirType = IIR_2011; // NOT_IIR use this to get `nacked` image without
                         // header for diagnostics later

  ii.NumberOfIrises = 1;
  ii.NumberOfEyes = 1;
  ii.EyeLabel = EYE_UNDEF;

  memset(ii.CaptureDateAndTime, 0xff, 9);
  ii.CaptureDeviceTechnology = 1; // (0x01 ): CMOS/CCD
  ii.CaptureDeviceVendorID = 0x0057;
  ii.RepresentationNumber = 1;

  // This should be consistent with fmtname, see below
  ii.ImageType = Iir2011_IMAGEFORMAT_MONO_JPEG2000;
  ii.ImageFormat = ii.IirType == IIR_2011
                       ? Iir2011_IMAGEFORMAT_MONO_JPEG2000
                       : ii.IirType == IIR_2005
                             ? Iir2005_IMAGEFORMAT_MONO_JPEG2000
                             : Iir2005_IMAGEFORMAT_MONO_RAW;

  ii.Width = IRM2_CROPPED_WIDTH;
  ii.Height = IRM2_CROPPED_HEIGHT;
  ii.BitDepth = 8; // grayscale

  ii.RollAngle = -1;
  ii.RollUncertainty = -1;

  // encode
  char *fmtname = (char *)"jp2";
  unsigned int n_size_out = 0;
  std::vector<uint8_t> buf_out(BUFFER_SIZE);
  uint32_t real_size;
  auto ret = iirPack(cropped, ii.Width, ii.Height, 8, ii.Width * ii.Height, &ii,
                     fmtname, 20000, buf_out.data(), &real_size);
  name = name_prefix;
  name.replace(name.find(".pgm"), 4, std::string("_kind7.") + fmtname);
  write_mem2file(name.c_str(), buf_out.data(), real_size);

  // decode
  memset(&ii, 0, sizeof(ii));
  std::vector<uint8_t> unpacked(1000 * 1000);
  uint32_t unpacked_size;
  ret = iirUnpack(buf_out.data(), real_size, &ii, BUFFER_SIZE, unpacked.data(),
                  &unpacked_size);
  name = name_prefix;
  name.replace(name.find(".pgm"), 4, "_kind7_unpacked.pgm");
  write_pgm(name.c_str(), unpacked.data(), IRM2_CROPPED_WIDTH,
            IRM2_CROPPED_HEIGHT);

  name = name_prefix;
  name.replace(name.find(".pgm"), 4, "_tmp_tmp.pgm");
  write_pgm(name.c_str(), pixels, width, height);
}


/**
 * @brief Entry point of the example.
 *
 * Requires the `.pgm` binary file with image of size @ref WIDTH x @ref HEIGHT as an input.
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

  std::cout << "Enroll and identify started\n";
  enroll_identify_1_eye(pixels, width, height);
  std::cout << "Enroll and identify ended\n\n";

  std::cout << "Capture -> kind7 -> identify started\n";
  capture_kind7_jpeg2000(pixels, width, height, file);
  std::cout << "Capture -> kind7 -> identify ended\n";
  return 0;
}
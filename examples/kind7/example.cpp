#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>

#include "iris_mobile_v2.h"
#include "iris_mobile_v2_capture.h"
#include "iris_image_record.h"

class Context {
    public:
    explicit Context(std::size_t size): size(size) {
        memory = (uint8_t *)malloc(size);
    }
    ~Context() {
        free(memory);
    }
    
    std::size_t size;
    uint8_t *memory = nullptr;
};

#define BUFFER_SIZE (1000*1000)
void capture_enroll(const std::string& file) {
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    uint8_t *pixels = buffer.data(); 
    unsigned int width = 640; // Width of frame or image in pixels
    unsigned int height = 480; // Height of frame or image in pixels
    unsigned int depth = 0;
    read_pgm(file.c_str(), &pixels, &width, &height, &depth); 

    // Allocate context of proper size
    Context ctx(IRM2_GET_CAPTURE_CONTEXT_SIZE(width, height));
    // Variables for output information
    irm2_eye_info eye_info = {0};
    irm2_enrollment_info enr_info = {0};
    irm2_ui_hints ui_hints = {0};
    
    // Image parameters: nominal resolution and camera resolution
    // are crucial for the correct work of the program.
    int32_t nom_res = 200; // px per cm
    int32_t cam_res = 4000; // px per rad;
    uint32_t num_eyes = 1; // 1 eye on the image
    uint32_t rotation = 0; // portrait
    
    // No special flags needed for monochromatic image sensor
    // and non `raw10` aquisition format.
    int b_flags = IRM2_FLAG_NONE;

    irm2_settings s = { sizeof(s), 
                        (int)width, 
                        (int)height, 
                        cam_res, 
                        nom_res,
                        // No callbacks set on processing steps 
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL, 
                        b_flags,
                        // No callback on camera control 
                        NULL, 
                        (int)width //stride
                      }; 

    int num_updates = 10;
    // Configure capture settings -1 for default. 
    // Check iris_mobile_v2_capture.h for more.
    irm2_capture_settings cs = { sizeof(irm2_capture_settings), 
                                 -1, 
                                 // Number of template updates to ensure
                                 // enrolment consistency.
                                 num_updates,  
                                 -1, 
                                 -1, 
                                 -1, 
                                 -1, 
                                 -1, 
                                 -1
                                };
    // Intitializing of the contest for operation.                            
    auto rc = irm2_init_capture(ctx.memory, ctx.size, num_eyes, &s, &cs);
    
    // The enrollment loop performs until counting the `num_updates`
    // successfull template updates to ensure the consistent enrollment.
    for(int i = 0; ; ++i) {
        // Process single frame.
        auto on_frame_rc = irm2_on_frame(ctx.memory, pixels, rotation);
        
        auto hints_rc = irm2_get_ui_hints(ctx.memory, &ui_hints, sizeof(ui_hints));
        auto info_rc = irm2_get_capture_info(ctx.memory, &enr_info, sizeof(enr_info));
        
        std::cout << "Iteration: " << std::dec << i << ": "<< std::hex << "on frame returns " << rc << "\n";  
        std::cout << std::hex << "Hints return:  " << hints_rc << "; hint: " << ui_hints.eye_distance << "\n";  
        std::cout << std::dec << enr_info.step_progress << "%\n";
        // check if capture process is successeed.
        if (on_frame_rc == 0)
            break;
    }

    // Allocate space for result image.
    uint8_t cropped[IRM2_CROPPED_WIDTH * IRM2_CROPPED_HEIGHT] = {0};
    // Extract kind 7 type of image
    rc = irm2_get_kind7_image(ctx.memory, IRM2_EYE_UNDEF, IRM2_CROPPED_WIDTH, IRM2_CROPPED_HEIGHT, cropped);
    // Save kind 7 image
    std::string name(file);
    name.replace(name.find(".pgm"), 4, "_kind7.pgm");
    write_pgm(name.c_str(), cropped, IRM2_CROPPED_WIDTH, IRM2_CROPPED_HEIGHT);

    //-----------------------Packing customized JPEG2000, for more see iris_image_record.h-----------------

    // prepare the header
    IirInfo ii;
    memset(&ii, 0, sizeof(ii));
    memcpy(ii.ich.FormatId, "IIR\0", 4);
    ii.IirType = IIR_2011; // NOT_IIR use this to get `nacked` image without header for diagnostics later

    ii.NumberOfIrises = 1;
    ii.NumberOfEyes = 1;
    ii.EyeLabel = EYE_UNDEF;

    memset(ii.CaptureDateAndTime, 0xff, 9);
    ii.CaptureDeviceTechnology =  1; // (0x01 ): CMOS/CCD
    ii.CaptureDeviceVendorID = 0x0057;
    ii.RepresentationNumber = 1;

    //This should be consistent with fmtname, see below
    ii.ImageType = Iir2011_IMAGEFORMAT_MONO_JPEG2000;
    ii.ImageFormat = ii.IirType == IIR_2011 ? Iir2011_IMAGEFORMAT_MONO_JPEG2000 :
                     ii.IirType == IIR_2005 ? Iir2005_IMAGEFORMAT_MONO_JPEG2000 : 
                                              Iir2005_IMAGEFORMAT_MONO_RAW;

    ii.Width = IRM2_CROPPED_WIDTH;
    ii.Height = IRM2_CROPPED_HEIGHT;
    ii.BitDepth = 8; // grayscale

    ii.RollAngle = -1;
    ii.RollUncertainty = -1;
    
    // encode
    char *fmtname = (char *) "jp2";
    unsigned int n_size_out = 0;
    uint8_t *buf_out = buffer.data();
    uint32_t real_size;
    auto ret = iirPack(cropped, ii.Width, ii.Height, 8, ii.Width*ii.Height, 
                       &ii, fmtname, 20000, buf_out, &real_size);
    name = file;
    name.replace(name.find(".pgm"), 4, std::string("_kind7.") + fmtname);
    write_mem2file(name.c_str(), buf_out, real_size);

    // decode
    memset(&ii, 0, sizeof(ii));
    std::vector<uint8_t> unpacked(1000 * 1000);
    uint32_t unpacked_size;
    ret = iirUnpack(buf_out, real_size, &ii, BUFFER_SIZE, unpacked.data(), &unpacked_size);
    name = file;
    name.replace(name.find(".pgm"), 4, "_kind7_unpacked.pgm");
    write_pgm(name.c_str(), unpacked.data(), IRM2_CROPPED_WIDTH, IRM2_CROPPED_HEIGHT);
}

int main(int argc, char *argv[]) {
    capture_enroll(argv[1]);
}
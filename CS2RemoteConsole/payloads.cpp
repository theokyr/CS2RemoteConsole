#include "payloads.h"
#include "utils.h"

// Define the payloads using the corrected helper function
const std::vector<unsigned char> init_payload = create_payload({
    'V', 'F', 'C', 'S', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x01
});

const std::vector<unsigned char> command_smooth_enable_payload = create_command_payload("cl_smooth 1");
const std::vector<unsigned char> command_smooth_disable_payload = create_command_payload("cl_smooth 0");
const std::vector<unsigned char> command_say_sanity_check_payload = create_command_payload("say sanity!");

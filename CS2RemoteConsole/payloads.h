#ifndef PAYLOADS_H
#define PAYLOADS_H

#include <vector>
#include <string>

std::vector<unsigned char> create_command_payload(const std::string& command);

const std::vector<unsigned char> init_payload = {
    'V', 'F', 'C', 'S', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x01
};

const auto command_smooth_enable_payload = create_command_payload("cl_smooth 1");
const auto command_smooth_disable_payload = create_command_payload("cl_smooth 0");
const auto command_say_sanity_check_payload = create_command_payload("say sanity!");

#endif // PAYLOADS_H

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <vector>

#pragma once

const std::string application_name = "CS2RemoteConsole-client";

const std::vector<unsigned char> init_payload = {
    'V', 'F', 'C', 'S', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x01
};

#endif // CONSTANTS_H

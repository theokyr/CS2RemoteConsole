#ifndef PAYLOADS_H
#define PAYLOADS_H

#include <vector>

#pragma once

const std::vector<unsigned char> init_payload = {
    'V', 'F', 'C', 'S', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x01
};

#endif // PAYLOADS_H

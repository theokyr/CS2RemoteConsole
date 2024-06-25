#include "payloads.h"
#include "utils.h"

// Define the payloads using helper function to convert string literals to byte arrays
const std::vector<unsigned char> init_payload = create_payload({
    'V', 'F', 'C', 'S', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x01
});

const std::vector<unsigned char> cmd_payload = create_payload({
    'C', 'M', 'N', 'D', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
    'c', 'l', '_', 's', 'm', 'o', 'o', 't', 'h', ' ', '1', 0x00
});

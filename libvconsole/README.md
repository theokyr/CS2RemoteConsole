# libvconsole

libvconsole is a static C++ library that implements the VConsole2 Protocol used in Source 2 games by Valve. 

## Features

- Connect to VConsole servers
- Supported receive message types: PRNT, CHAN, AINF, ADON, CVAR, CFGV
- Supported send message types: CMND (TODO investigate: VCFS)
- Callback system for handling incoming messages

## Usage

```cpp
#include "vconsole.h"
#include <iostream>

int main() {
    VConsole console;
    
    if (console.connect("127.0.0.1", 29000)) {
        console.setOnPRNTReceived([](const std::string& channel, const std::string& message) {
            std::cout << "Received message on " << channel << ": " << message << std::endl;
        });
        
        console.sendCmd("echo Hello, VConsole!");
        
        // Process incoming data
        console.processIncomingData();
    }
    
    return 0;
}
```

## Building

Requires a C++17 compatible compiler.

### CMake (Recommended)

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Visual Studio (Windows)

1. Open the libvconsole.vcxproj file in Visual Studio
2. Select your desired configuration (Debug/Release) and platform (x86/x64)
3. Build the project

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

This library is based on the following GitHub repositories:
- [Penguinwizzard/VConsoleLib](https://github.com/Penguinwizzard/VConsoleLib) (C library)
- [uilton-oliveira/VConsoleLib.python](https://github.com/uilton-oliveira/VConsoleLib.python) (Python library)

We express our gratitude to the original authors for their work, which served as a foundation for this C++ implementation.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

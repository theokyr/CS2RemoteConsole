# CS2RemoteConsole

CS2RemoteConsole is a utility that allows you to run commands in Counter-Strike 2's tools mode remotely without using VConsole. This tool is developed by H7per and theokyr.

## Features

- Send initialization and command payloads to Counter-Strike 2's tools mode.
- Listen for responses from the game.
- Simple and modular code structure for easy customization and extension.

## Getting Started

### Prerequisites

- Windows operating system
- Counter-Strike 2 installed
- Visual Studio or any compatible C++ compiler
- Winsock2 library (included in Windows SDK)

### Building the Project

1. Clone the repository:

```bash
git clone https://github.com/yourusername/CS2RemoteConsole.git
cd CS2RemoteConsole
```

2. Open the project in your preferred C++ development environment (e.g., Visual Studio).

3. Build the project.

### Running the Utility

1. Make sure Counter-Strike 2 is running in tools mode.

2. Run the compiled executable.

3. The program will wait for 5 seconds, then attempt to connect to Counter-Strike 2 on localhost at port `29000`.

4. Upon successful connection, the program sends initialization and command payloads to the game.

5. The listener thread will continuously listen for responses from the game and print them to the console.

6. Press Enter to stop listening and terminate the program.

## File Structure

```
CS2RemoteConsole/
├── main.cpp          # Main application logic
├── payloads.h        # Header file for payload definitions
├── payloads.cpp      # Implementation of payloads
├── utils.h           # Header file for utility functions
├── utils.cpp         # Implementation of utility functions
└── README.md         # This readme file
```

## Code Overview

### main.cpp

- Contains the main function that sets up the socket connection, starts the listener thread, and sends payloads.

### payloads.h & payloads.cpp

- `payloads.h` declares the payloads.
- `payloads.cpp` defines the payloads using a helper function for readability.

### utils.h & utils.cpp

- `utils.h` declares utility functions.
- `utils.cpp` implements utility functions for creating payloads and sending them through the socket.

## Example Payloads

Payloads are defined as readable byte arrays:

```cpp
const std::vector<unsigned char> init_payload = create_payload({
    'V', 'F', 'C', 'S', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x01
});

const std::vector<unsigned char> cmd_payload = create_payload({
    'C', 'M', 'N', 'D', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
    'c', 'l', '_', 's', 'm', 'o', 'o', 't', 'h', ' ', '1', 0x00
});
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Authors

- **H7perus**
- **theokyr**

## Acknowledgments

- Valve for CS2 and the awesome VConsole2.
- Counter-Strike 2 community for the inspiration.
- All contributors and supporters of this project.

---

Feel free to customize and extend this utility to suit your needs. Contributions are welcome! If you encounter any issues or have suggestions for improvements, please open an issue or submit a pull request on the GitHub repository.

Happy gaming!


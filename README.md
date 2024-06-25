# CS2RemoteConsole

A simple C++ utility for sending commands to Counter-Strike 2's console remotely.

## Features

- Connect to CS2's console socket
- Send custom commands
- Listen for console output

## Usage

1. Compile and run the program
2. Use the following commands:
    - `0`: Disable smooth
    - `1`: Enable smooth
    - `y`: Toggle console output listening
    - `cmd <your_command>`: Send a custom command (e.g., `cmd noclip`)
    - `x`: Exit the program

## Requirements

- Windows OS
- Counter-Strike 2
- C++ compiler with C++11 support
- Winsock2 library

## Notes

- Ensure CS2 is running in `-tools` mode
- Use responsibly and in accordance with the game's terms of service

## License

[MIT License](LICENSE)
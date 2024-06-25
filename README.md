# CS2RemoteConsole

A C++ utility for sending commands to Counter-Strike 2's console remotely.

## Features

- Connect to CS2's console socket
- Send predefined and custom commands
- Listen for console output
- Configurable settings via INI file

## Usage

1. Ensure the `config.ini` file is in the same directory as the executable or in the current working directory.
2. Run the program.
3. Use the following commands:
   - `0`: Disable smooth
   - `1`: Enable smooth
   - `y`: Toggle console output listening
   - `cmd <your_command>`: Send a custom command (e.g., `cmd noclip`)
   - `x`: Exit the program

## Configuration

Edit `config.ini` to customize:
- `cs2_console_ip`: IP address of the CS2 console (default: 127.0.0.1)
- `cs2_console_port`: Port number of the CS2 console (default: 29000)
- `cs2_console_reconnect_delay`: Delay in milliseconds before reconnection attempts (default: 5000)

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
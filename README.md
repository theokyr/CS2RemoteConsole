# CS2RemoteConsole

A C++ utility for sending commands to Counter-Strike 2's console remotely, with support for a centralized remote server.

## Features

- Connect to CS2's console socket
- Connect to a remote control server
- Send predefined and custom commands
- Listen for console output
- Configurable settings via INI file
- Cross-platform remote server (Windows and Linux)

## Usage

### Client

1. Ensure the `config.ini` file is in the same directory as the executable or in the current working directory.
2. Run the program.
3. Use the following commands:
   - `0`: Disable smooth
   - `1`: Enable smooth
   - `y`: Toggle console output listening
   - `cmd <your_command>`: Send a custom command (e.g., `cmd noclip`)
   - `x`: Exit the program

### Server

1. Run the server program.
2. Enter commands to be broadcasted to all connected clients.
3. Type 'quit' to exit the server.

## Configuration

Edit `config.ini` to customize:
- `cs2_console_ip`: IP address of the CS2 console (default: 127.0.0.1)
- `cs2_console_port`: Port number of the CS2 console (default: 29000)
- `cs2_console_reconnect_delay`: Delay in milliseconds before reconnection attempts (default: 5000)
- `remote_server_ip`: IP address of the remote control server (default: 127.0.0.1)
- `remote_server_port`: Port number of the remote control server (default: 42069)
- `remote_server_reconnect_delay`: Delay in milliseconds before reconnection attempts to the remote server (default: 5000)
- `debug_sanity`: Enable or disable debug sanity checks (default: 0)

## Requirements

### Client
- Windows OS
- Counter-Strike 2
- C++ compiler with C++11 support
- Winsock2 library

### Server
- Windows or Linux OS
- C++ compiler with C++11 support
- Winsock2 library (Windows only)

## Build Instructions

### Client (Windows)
1. Open the project in Visual Studio
2. Build the solution

### Server (Windows)
1. Open the server project in Visual Studio
2. Build the solution

### Server (Linux)
1. Navigate to the server directory
2. Run the following command:
   ```
   g++ -std=c++11 -pthread main.cpp -o server
   ```
3. The compiled server executable will be named `server`

## Notes

- Ensure CS2 is running in `-tools` mode
- Use responsibly and in accordance with the game's terms of service

## License

[MIT License](LICENSE)
# CS2RemoteConsole

CS2RemoteConsole is a C++ utility for remotely interacting with Counter-Strike 2's console, featuring a TUI and support for a centralized remote server.

![Image of the CS2RemoteConsole Server and Client applications running together](https://github.com/theokyr/CS2RemoteConsole/blob/master/docs/image.png?raw=true)

## Features

- Connect, read, and write messages to CS2 through the VConsole socket
- Connect and receive commands from a remote control server
- Display console output with channel-based coloring
- Configurable settings via INI file
- Cross-platform support (Windows and Linux) for both client and server
- CMake build system for cross-platform builds
- Text-based User Interface for easy interaction

## Disclaimer

Use responsibly and in accordance with the [Counter-Strike Fair Play Guidelines](https://blog.counter-strike.net/index.php/fair-play-guidelines/). This tool is
for educational and development purposes only and should only be used in `-tools` mode.

## Requirements

### Client

- Windows or Linux OS
- Counter-Strike 2 running in `-tools` mode
- C++ compiler with C++17 support
- CMake 3.14+
- [PDCursesMod](https://github.com/Bill-Gray/PDCursesMod) (Windows) or ncurses (Linux)
- [spdlog](https://github.com/gabime/spdlog) library

### Server

- Windows or Linux OS
- C++ compiler with C++11 support
- CMake 3.14+

## Building

### CMake (Recommended)

Build both client and server:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

To build only the client or server:

```bash
cmake .. -DBUILD_CLIENT=ON -DBUILD_SERVER=OFF  # Client only
cmake .. -DBUILD_CLIENT=OFF -DBUILD_SERVER=ON  # Server only
```

### Visual Studio (Windows)

1. Open the project in Visual Studio
2. Ensure all required libraries are properly linked
3. Build the solution

### Docker (Server)

1. Ensure Docker and Docker Compose are installed
2. Navigate to the `docker` directory
3. Build and run:
   ```
   docker-compose up --build
   ```

## Configuration

Edit `config.ini` to customize:

- `cs2_console_ip`: IP address of the CS2 console (default: 127.0.0.1)
- `cs2_console_port`: Port number of the CS2 console (default: 29000)
- `cs2_console_reconnect_delay`: Reconnection delay in milliseconds (default: 5000)
- `remote_server_enabled`: Enable or disable remote server connection (default: 1)
- `remote_server_ip`: IP address of the remote control server (default: 127.0.0.1)
- `remote_server_port`: Port number of the remote control server (default: 42069)
- `remote_server_reconnect_delay`: Remote server reconnection delay in milliseconds (default: 5000)
- `debug_sanity_enabled`: Enable or disable debug sanity checks (default: 0)
- `debug_sanity_interval`: Interval for debug sanity checks in milliseconds (default: 5000)

## Usage

### Client

1. Ensure `config.ini` is in the same directory as the executable or in the current working directory
2. Run the program
3. Use the TUI to interact with the CS2 console:
    - Type commands and press Enter to send them to CS2
    - Use Page Up/Down or mouse wheel to scroll through console output

### Server

1. Run the server program
2. Enter commands to be broadcasted to all connected clients
3. Type 'quit' to exit the server

## Architecture

CS2RemoteConsole consists of three main components:

1. **Client**: Connects to both CS2 console and the remote server. It features a TUI for user interaction and displays console output.
2. **Server**: Acts as a central hub for multiple clients, allowing command broadcasting.
3. **libvconsole**: A static library implementing the VConsole2 Protocol used by Source 2 games.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [Penguinwizzard/VConsoleLib](https://github.com/Penguinwizzard/VConsoleLib)
- [uilton-oliveira/VConsoleLib.python](https://github.com/uilton-oliveira/VConsoleLib.python)

We express our gratitude to the original authors of these libraries, which served as a foundation for our VConsole protocol implementation.

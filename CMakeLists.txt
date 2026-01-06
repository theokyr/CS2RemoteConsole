cmake_minimum_required(VERSION 3.14)
project(CS2RemoteConsole VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_CLIENT "Build the CS2RemoteConsole client" ON)
option(BUILD_SERVER "Build the CS2RemoteConsole server" ON)

# Add subdirectories
add_subdirectory(libvconsole)

if(BUILD_CLIENT)
    add_subdirectory(CS2RemoteConsole-client)
endif()

if(BUILD_SERVER)
    add_subdirectory(CS2RemoteConsole-server)
endif()

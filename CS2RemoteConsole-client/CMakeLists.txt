cmake_minimum_required(VERSION 3.14)
project(CS2RemoteConsole-client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/utils.cpp
    src/tui/tui.cpp
    src/connection/connection_cs2console.cpp
    src/connection/connection_remoteserver.cpp
)

set(HEADERS
    src/config.h
    src/constants.h
    src/singletons.h
    src/utils.h
    src/tui/tui.h
    src/connection/connection_cs2console.h
    src/connection/connection_remoteserver.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Common include directories (no curses here - platform-specific below)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../libvconsole/src
)

# Link libvconsole
target_link_libraries(${PROJECT_NAME} PRIVATE vconsole)

# Platform-specific settings
if(WIN32)
    # Windows: Use bundled PDCurses and spdlog
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE pdcurses ws2_32)
else()
    # Linux/macOS: Use system ncurses
    find_package(Curses REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CURSES_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CURSES_LIBRARIES})

    # spdlog: Try system package first, fallback to bundled (excluding bundled curses.h)
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
    else()
        # Use bundled spdlog headers (header-only mode)
        target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        message(STATUS "System spdlog not found, using bundled headers.")
    endif()

    # Threads
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()

# Copy config.ini to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

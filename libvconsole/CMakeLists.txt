cmake_minimum_required(VERSION 3.14)
project(libvconsole VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library sources
set(SOURCES
    src/vconsole.cpp
)

set(HEADERS
    src/vconsole.h
    src/messages.h
)

# Create library
add_library(vconsole STATIC ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(vconsole PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(vconsole PRIVATE ws2_32)
endif()

# Export library
set_target_properties(vconsole PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

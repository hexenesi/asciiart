cmake_minimum_required(VERSION 3.10)

# Set project name and required C++ standard
project(ASCIIConverter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include stbi header definitions BEFORE any target definition that uses image loading
# Note: We need to define STB_IMAGE_IMPLEMENTATION here for the implementation file to use it.
add_definitions(-DSTB_IMAGE_IMPLEMENTATION=1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define the executable target and link all source files
add_executable(ascii_converter
    main.cpp
    ImageConverter.cpp
    ImageConverter.h
)
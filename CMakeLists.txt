cmake_minimum_required(VERSION 3.10)

project(ASCIIConverter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(ascii_converter
    main.cpp
    ImageConverter.cpp
)

# Vendored single-header libraries (stb_image). STB_IMAGE_IMPLEMENTATION is
# defined in ImageConverter.cpp only, so the implementation is compiled once.
target_include_directories(ascii_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(ascii_converter PRIVATE /W4)
else()
    target_compile_options(ascii_converter PRIVATE -Wall -Wextra)
endif()

cmake_minimum_required(VERSION 3.10)

project(ASCIIConverter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(ascii_converter
    main.cpp
    ImageConverter.cpp
    PageLayout.cpp
    PdfWriter.cpp
    PosterRenderer.cpp
)
# PosterRenderer.h and friends live in the project root.
target_include_directories(ascii_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Vendored single-header libraries (stb_image). STB_IMAGE_IMPLEMENTATION is
# defined in ImageConverter.cpp only, so the implementation is compiled once.
target_include_directories(ascii_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(ascii_converter PRIVATE /W4)
else()
    target_compile_options(ascii_converter PRIVATE -Wall -Wextra)
endif()

# --- Tests (run with: ctest --test-dir <build dir>) ---
enable_testing()

add_executable(test_image_converter
    tests/test_image_converter.cpp
    ImageConverter.cpp
)
target_include_directories(test_image_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT MSVC)
    target_compile_options(test_image_converter PRIVATE -Wall -Wextra)
endif()
add_test(NAME unit COMMAND test_image_converter WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_page_layout
    tests/test_page_layout.cpp
    PageLayout.cpp
)
target_include_directories(test_page_layout PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
    target_compile_options(test_page_layout PRIVATE -Wall -Wextra)
endif()
add_test(NAME page_layout COMMAND test_page_layout)

add_executable(test_pdf_writer
    tests/test_pdf_writer.cpp
    PdfWriter.cpp
)
target_include_directories(test_pdf_writer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
    target_compile_options(test_pdf_writer PRIVATE -Wall -Wextra)
endif()
add_test(NAME pdf_writer COMMAND test_pdf_writer)

add_executable(test_poster_renderer
    tests/test_poster_renderer.cpp
    PosterRenderer.cpp
    PageLayout.cpp
    PdfWriter.cpp
)
target_include_directories(test_poster_renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
    target_compile_options(test_poster_renderer PRIVATE -Wall -Wextra)
endif()
add_test(NAME poster_renderer COMMAND test_poster_renderer)

# CLI behavior
add_test(NAME cli_help COMMAND ascii_converter --help)
add_test(NAME cli_no_args COMMAND ascii_converter)
add_test(NAME cli_bad_width COMMAND ascii_converter --width abc x.png)
add_test(NAME cli_unknown_option COMMAND ascii_converter --foo x.png)
add_test(NAME cli_missing_value COMMAND ascii_converter x.png --width)
add_test(NAME cli_bad_brightness COMMAND ascii_converter --brightness 150 x.png)
add_test(NAME cli_missing_file COMMAND ascii_converter does_not_exist.png)
set_tests_properties(cli_no_args cli_bad_width cli_unknown_option cli_missing_value
                     cli_bad_brightness cli_missing_file PROPERTIES WILL_FAIL TRUE)

# PDF output (64x64 fixture; 1 char/px at 6 pt fits one Letter page)
set(TEST_IMAGE ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/circle.png)
add_test(NAME pdf_default COMMAND ascii_converter ${TEST_IMAGE} --pdf default.pdf)
set_tests_properties(pdf_default PROPERTIES
    PASS_REGULAR_EXPRESSION "64x38 chars -> 1x1 = 1 page \\+ overview.*Saved PDF \\(2 pages\\)")
add_test(NAME pdf_pages_wide COMMAND ascii_converter ${TEST_IMAGE} --pdf wide.pdf --paper a4 --pages-wide 3)
set_tests_properties(pdf_pages_wide PROPERTIES PASS_REGULAR_EXPRESSION "405x[0-9]+ chars -> 3x[0-9]+ = ")
add_test(NAME pdf_dry_run COMMAND ascii_converter ${TEST_IMAGE} --pdf dry.pdf --scale 20 --max-pages 500 --dry-run)
set_tests_properties(pdf_dry_run PROPERTIES PASS_REGULAR_EXPRESSION "1280x768 chars -> 10x7 = 70 pages")

# Each case: name|expected error text|arguments
foreach(case
        "scale_and_pages_wide|cannot be used together|--pdf;x.pdf;--scale;1;--pages-wide;2"
        "width_and_scale|cannot be combined|--pdf;x.pdf;--width;10;--scale;1"
        "page_option_without_pdf|--paper requires --pdf|--paper;a4"
        "blocks_in_pdf|not supported in PDF|--pdf;x.pdf;--charset;blocks"
        "bad_paper|--paper must be|--pdf;x.pdf;--paper;legal"
        "too_many_pages|70 pages exceeds --max-pages 50|--pdf;x.pdf;--scale;20")
    string(REPLACE "|" ";" parts "${case}")
    list(GET parts 0 name)
    list(GET parts 1 expected)
    list(REMOVE_AT parts 0 1)
    add_test(NAME pdf_error_${name} COMMAND ascii_converter ${TEST_IMAGE} ${parts})
    set_tests_properties(pdf_error_${name} PROPERTIES PASS_REGULAR_EXPRESSION "Error: .*${expected}")
endforeach()

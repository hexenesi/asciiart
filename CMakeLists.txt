cmake_minimum_required(VERSION 3.10)

project(ASCIIConverter LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Warning flags shared by all targets.
add_library(project_warnings INTERFACE)
if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4)
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra)
endif()

# Vendored miniz (MIT) for PDF stream compression. Only the deflate/inflate core is used.
add_library(miniz STATIC third_party/miniz/miniz.c)
target_include_directories(miniz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz)
target_compile_definitions(miniz PUBLIC MINIZ_NO_ARCHIVE_APIS MINIZ_NO_STDIO MINIZ_NO_TIME)

# Conversion, page layout and PDF code, shared by the CLI and the tests.
# STB_IMAGE_IMPLEMENTATION is defined in src/ImageConverter.cpp only.
add_library(asciiart_core STATIC
    src/AnsiRenderer.cpp
    src/ColorRuns.cpp
    src/HtmlRenderer.cpp
    src/ImageConverter.cpp
    src/PageLayout.cpp
    src/PdfWriter.cpp
    src/PosterRenderer.cpp
    src/SvgRenderer.cpp
    src/TextRenderer.cpp
)
target_include_directories(asciiart_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
)
target_link_libraries(asciiart_core PRIVATE project_warnings miniz)

add_executable(ascii_converter src/main.cpp)
target_link_libraries(ascii_converter PRIVATE asciiart_core project_warnings)

# --- Tests (run with: ctest --test-dir <build dir>) ---
enable_testing()

foreach(test_name test_image_converter test_page_layout test_pdf_writer test_poster_renderer test_color_runs test_renderers)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE asciiart_core project_warnings)
endforeach()

# Unit tests write their PNG fixtures to the working directory.
add_test(NAME unit COMMAND test_image_converter WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME page_layout COMMAND test_page_layout)
# The PDF writer test inflates compressed streams itself.
target_link_libraries(test_pdf_writer PRIVATE miniz)
add_test(NAME pdf_writer COMMAND test_pdf_writer)
add_test(NAME poster_renderer COMMAND test_poster_renderer)
add_test(NAME color_runs COMMAND test_color_runs)
add_test(NAME renderers COMMAND test_renderers)

# CLI behavior
add_test(NAME cli_help COMMAND ascii_converter --help)
add_test(NAME cli_no_args COMMAND ascii_converter)
add_test(NAME cli_bad_width COMMAND ascii_converter --width abc x.png)
add_test(NAME cli_unknown_option COMMAND ascii_converter --foo x.png)
add_test(NAME cli_missing_value COMMAND ascii_converter x.png --width)
add_test(NAME cli_bad_brightness COMMAND ascii_converter --brightness 150 x.png)
set_tests_properties(cli_no_args cli_bad_width cli_unknown_option cli_missing_value
                     cli_bad_brightness PROPERTIES WILL_FAIL TRUE)
add_test(NAME cli_missing_file COMMAND ascii_converter does_not_exist.png)
set_tests_properties(cli_missing_file PROPERTIES
    PASS_REGULAR_EXPRESSION "Error: cannot load image 'does_not_exist.png'")

# PDF output (64x64 fixture; 1 char/px at 6 pt fits one Letter page)
set(TEST_IMAGE ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/circle.png)
add_test(NAME pdf_default COMMAND ascii_converter ${TEST_IMAGE} --pdf default.pdf)
set_tests_properties(pdf_default PROPERTIES
    PASS_REGULAR_EXPRESSION "64x38 chars -> 1x1 = 1 page \\+ overview.*Saved PDF \\(2 pages\\)")
add_test(NAME pdf_pages_wide COMMAND ascii_converter ${TEST_IMAGE} --pdf wide.pdf --paper a4 --pages-wide 3)
set_tests_properties(pdf_pages_wide PROPERTIES PASS_REGULAR_EXPRESSION "405x[0-9]+ chars -> 3x[0-9]+ = ")
add_test(NAME pdf_dry_run COMMAND ascii_converter ${TEST_IMAGE} --pdf dry.pdf --scale 20 --max-pages 500 --dry-run)
set_tests_properties(pdf_dry_run PROPERTIES PASS_REGULAR_EXPRESSION "1280x768 chars -> 10x7 = 70 pages")

# Color output: escape codes on stdout
add_test(NAME color_fg COMMAND ascii_converter ${TEST_IMAGE} --color --width 20)
set_tests_properties(color_fg PROPERTIES PASS_REGULAR_EXPRESSION "\\[38;2;[0-9]+;[0-9]+;[0-9]+m")
add_test(NAME color_bg COMMAND ascii_converter ${TEST_IMAGE} --color --color-style bg --width 20)
set_tests_properties(color_bg PROPERTIES PASS_REGULAR_EXPRESSION ";48;2;[0-9]+;[0-9]+;[0-9]+m")

add_test(NAME pdf_color COMMAND ascii_converter ${TEST_IMAGE} --pdf color.pdf --color --color-style bg)
set_tests_properties(pdf_color PROPERTIES PASS_REGULAR_EXPRESSION "Saved PDF \\(2 pages\\)")

# HTML output
add_test(NAME html_plain COMMAND ascii_converter ${TEST_IMAGE} --html plain.html --width 30)
set_tests_properties(html_plain PROPERTIES PASS_REGULAR_EXPRESSION "Saved HTML to: plain.html")
add_test(NAME html_color COMMAND ascii_converter ${TEST_IMAGE} --html color.html --color --width 30)
set_tests_properties(html_color PROPERTIES PASS_REGULAR_EXPRESSION "Saved HTML to: color.html")

# SVG output
add_test(NAME svg_plain COMMAND ascii_converter ${TEST_IMAGE} --width 20 --svg plain.svg)
set_tests_properties(svg_plain PROPERTIES PASS_REGULAR_EXPRESSION "Saved SVG to: plain.svg")
add_test(NAME svg_color COMMAND ascii_converter ${TEST_IMAGE} --width 20 --color --color-style bg --svg color.svg)
set_tests_properties(svg_color PROPERTIES PASS_REGULAR_EXPRESSION "Saved SVG to: color.svg")

# Each case: name|expected error text|arguments
foreach(case
        "scale_and_pages_wide|cannot be used together|--pdf;x.pdf;--scale;1;--pages-wide;2"
        "width_and_scale|cannot be combined|--pdf;x.pdf;--width;10;--scale;1"
        "page_option_without_pdf|--paper requires --pdf|--paper;a4"
        "blocks_in_pdf|not supported in PDF|--pdf;x.pdf;--charset;blocks"
        "bad_paper|--paper must be|--pdf;x.pdf;--paper;legal"
        "too_many_pages|70 pages exceeds --max-pages 50|--pdf;x.pdf;--scale;20"
        "glue_flap_too_wide|Glue flap is wider|--pdf;x.pdf;--glue-flap;40"
        "glue_flap_without_pdf|--glue-flap requires --pdf|--glue-flap;10"
        "color_style_without_color|--color-style requires --color|--color-style;bg"
        "colors_without_color|--colors requires --color|--colors;16"
        "colors_out_of_range|--colors must be between 2 and 256|--color;--colors;300"
        "bad_color_style|--color-style must be|--color;--color-style;rainbow")
    string(REPLACE "|" ";" parts "${case}")
    list(GET parts 0 name)
    list(GET parts 1 expected)
    list(REMOVE_AT parts 0 1)
    add_test(NAME pdf_error_${name} COMMAND ascii_converter ${TEST_IMAGE} ${parts})
    set_tests_properties(pdf_error_${name} PROPERTIES PASS_REGULAR_EXPRESSION "Error: .*${expected}")
endforeach()

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib> // For EXIT_SUCCESS/FAILURE
#include <stdexcept>

#include "ImageConverter.h"

struct Options {
    std::string image_path;
    int width = 0;  // 0 = automatic
    int height = 0; // 0 = automatic
    double brightness = 0.0;
    double contrast = 1.0;
    std::string charset = ImageConverter::charsetNames().front();
    std::string output_path; // empty = stdout
};

void print_usage(std::ostream& out) {
    out << "Usage: ascii_converter [options] <image>\n"
           "\n"
           "Options:\n"
           "  --width <n>         Output width in characters (default: automatic, up to 100)\n"
           "  --height <n>        Output height in rows (default: automatic)\n"
           "  --brightness <pct>  Brightness offset, -100 to 100 (default: 0)\n"
           "  --contrast <f>      Contrast factor, >= 0 (default: 1.0)\n"
           "  --charset <name>    Character set:";
    for (const auto& name : ImageConverter::charsetNames()) out << ' ' << name;
    out << " (default: " << ImageConverter::charsetNames().front() << ")\n"
           "  --output <file>     Write to file instead of stdout\n"
           "  -h, --help          Show this help\n"
           "\n"
           "Example: ascii_converter foto.jpg --width 120\n";
}

[[noreturn]] void fail(const std::string& message) {
    std::cerr << "Error: " << message << "\n"
              << "Run 'ascii_converter --help' for usage.\n";
    std::exit(EXIT_FAILURE);
}

/** Parses a whole argument as a number; exits with an error on invalid input. */
double parse_number(const std::string& option, const std::string& value) {
    try {
        size_t consumed = 0;
        double result = std::stod(value, &consumed);
        if (consumed == value.size()) return result;
    } catch (...) {
    }
    fail(option + " requires a numeric value, got '" + value + "'.");
}

/** Parses a whole argument as a positive integer; exits with an error on invalid input. */
int parse_positive_int(const std::string& option, const std::string& value) {
    try {
        size_t consumed = 0;
        int result = std::stoi(value, &consumed);
        if (consumed == value.size() && result > 0) return result;
    } catch (...) {
    }
    fail(option + " requires a positive integer, got '" + value + "'.");
}

/** Parses command-line arguments (see README); exits on invalid input or --help. */
Options parse_args(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            std::exit(EXIT_SUCCESS);
        }

        if (arg.size() > 1 && arg[0] == '-') {
            bool known = arg == "--width" || arg == "--height" || arg == "--brightness" ||
                         arg == "--contrast" || arg == "--charset" || arg == "--output";
            if (!known) fail("unknown option '" + arg + "'.");
            if (i + 1 >= argc) fail(arg + " requires a value.");
            std::string value = argv[++i];

            if (arg == "--width") opts.width = parse_positive_int(arg, value);
            else if (arg == "--height") opts.height = parse_positive_int(arg, value);
            else if (arg == "--brightness") opts.brightness = parse_number(arg, value);
            else if (arg == "--contrast") opts.contrast = parse_number(arg, value);
            else if (arg == "--charset") opts.charset = value;
            else opts.output_path = value;
        } else if (opts.image_path.empty()) {
            opts.image_path = arg;
        } else {
            fail("more than one image given ('" + opts.image_path + "' and '" + arg + "').");
        }
    }

    return opts;
}


int main(int argc, char* argv[]) {
    Options opts = parse_args(argc, argv);

    if (opts.image_path.empty()) {
        print_usage(std::cerr);
        return EXIT_FAILURE;
    }

    // --- 2. Configure and run converter ---
    ImageConverter converter(opts.image_path);

    converter.setWidth(opts.width);
    converter.setHeight(opts.height);
    if (!converter.setBrightness(opts.brightness)) fail("--brightness must be between -100 and 100.");
    if (!converter.setContrast(opts.contrast)) fail("--contrast must be 0 or greater.");
    if (!converter.setCharset(opts.charset)) fail("unknown charset '" + opts.charset + "'.");

    std::cerr << "\n==============================================\n";
    std::cerr << "   🚀 Starting ASCII Art Conversion Process 🖼️  \n";
    std::cerr << "==============================================\n";


    try {
        std::string ascii_art = converter.convert();

        if (ascii_art.empty()) {
            throw std::runtime_error("Conversion resulted in empty ASCII art after all processing stages.");
        }

        // --- 3. Output Result ---
        if (!opts.output_path.empty()) {
            std::ofstream outfile(opts.output_path);
            if (outfile.is_open()) {
                outfile << ascii_art;
                outfile.close();
                std::cerr << "\n[SUCCESS] Conversion complete! Saved art to: " << opts.output_path << std::endl;
            } else {
                std::cerr << "[ERROR] Could not open file for writing: " << opts.output_path << ". Printing to console instead.\n";
                std::cout << ascii_art; // Fallback to cout
            }
        } else {
            // Output directly to stdout (console)
            std::cerr << "\n==============================================\n";
            std::cerr << "       ASCII Art Preview (Output to Console)    \n";
            std::cerr << "==============================================\n";
            std::cout << ascii_art;
        }

    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL ERROR] Conversion failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
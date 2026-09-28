/* Implementation of main CLI entry point for the ASCII Converter */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib> // For EXIT_SUCCESS/FAILURE
#include <stdexcept>

// Local header includes (assuming compiler finds them)
#include "ImageConverter.h"

/** Parses a whole argument as a number; exits with an error on invalid input. */
double parse_number(const std::string& option, const char* value) {
    try {
        size_t consumed = 0;
        double result = std::stod(value, &consumed);
        if (consumed == std::string(value).size()) return result;
    } catch (...) {
    }
    std::cerr << "Error: " << option << " requires a numeric value, got '" << value << "'.\n";
    std::exit(EXIT_FAILURE);
}

/**
 * @brief Parses command-line arguments according to the README specification.
 *
 * Expects: ascii_converter [options] <image_path>
 * Options: --width, --height, --brightness, --contrast, --output
 *
 * @param argc Argument count (argc > 1 expected).
 * @param argv Argument vector.
 * @return std::string The path to the image file, or an empty string if none provided.
 */
std::string parse_args(int argc, char* argv[], int& out_w, int& out_h, double& out_b, double& out_c, std::string& out_p, std::string& out_cs) {
    std::string image_path = "";

    // Default values initialization (matching README table defaults where applicable)
    out_w = 0; // Auto-detect
    out_h = 0; // Auto-detect
    out_b = 0.0;
    out_c = 1.0;
    out_p = ""; // Output to console by default
    out_cs = ImageConverter::charsetNames().front();

    // Basic argument parsing loop (Needs robust library implementation in a real app)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--width" && i + 1 < argc) {
            try {
                out_w = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Error: --width requires an integer value.\n";
            }
        } else if (arg == "--height" && i + 1 < argc) {
            try {
                out_h = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Error: --height requires an integer value.\n";
            }
        } else if (arg == "--brightness" && i + 1 < argc) {
            out_b = parse_number(arg, argv[++i]);
        } else if (arg == "--contrast" && i + 1 < argc) {
            out_c = parse_number(arg, argv[++i]);
        } else if (arg == "--charset" && i + 1 < argc) {
            out_cs = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            out_p = argv[++i];
        } else {
            // Assume the first unparsed positional argument is the image path.
            if (image_path.empty()) {
                image_path = arg;
            }
        }
    }

    return image_path;
}


int main(int argc, char* argv[]) {
    // --- 1. Parse Arguments ---
    int width = 0;
    int height = 0;
    double brightness = 0.0;
    double contrast = 1.0;
    std::string output_path = "";
    std::string charset;

    std::string image_file = parse_args(argc, argv, width, height, brightness, contrast, output_path, charset);

    if (image_file.empty()) {
        std::cerr << "Usage: ascii_converter [options] <image> \n";
        std::cerr << "Example: ascii_converter foto.jpg --width 120 --height 60\n";
        std::cerr << "Charsets (--charset):";
        for (const auto& name : ImageConverter::charsetNames()) std::cerr << ' ' << name;
        std::cerr << '\n';
        return EXIT_FAILURE;
    }

    if (width < 0 || height < 0) {
        std::cerr << "Error: --width and --height must be positive integers.\n";
        return EXIT_FAILURE;
    }

    // --- 2. Initialize and Run Converter (FIXED FLOW) ---
    ImageConverter converter(image_file); // Use constructor that takes image path!

    // Apply user-specified parameters (assuming setters now exist)
    converter.setWidth(width);
    converter.setHeight(height);
    if (!converter.setBrightness(brightness)) {
        std::cerr << "Error: --brightness must be between -100 and 100.\n";
        return EXIT_FAILURE;
    }
    if (!converter.setContrast(contrast)) {
        std::cerr << "Error: --contrast must be 0 or greater.\n";
        return EXIT_FAILURE;
    }
    if (!converter.setCharset(charset)) {
        std::cerr << "Error: unknown charset '" << charset << "'. Available:";
        for (const auto& name : ImageConverter::charsetNames()) std::cerr << ' ' << name;
        std::cerr << '\n';
        return EXIT_FAILURE;
    }

    std::cerr << "\n==============================================\n";
    std::cerr << "   🚀 Starting ASCII Art Conversion Process 🖼️  \n";
    std::cerr << "==============================================\n";


    // Core execution flow revised to use the proper sequence of method calls:
    // (loadAndGrayscale is called internally by convert())

    // The conversion call now encapsulates all subsequent steps (Aspect Ratio -> Resize -> Adjust)
    try {
        std::string ascii_art = converter.convert();

        if (ascii_art.empty()) {
            throw std::runtime_error("Conversion resulted in empty ASCII art after all processing stages.");
        }

        // --- 3. Output Result ---
        if (!output_path.empty()) {
            std::ofstream outfile(output_path);
            if (outfile.is_open()) {
                outfile << ascii_art;
                outfile.close();
                std::cerr << "\n[SUCCESS] Conversion complete! Saved art to: " << output_path << std::endl;
            } else {
                std::cerr << "[ERROR] Could not open file for writing: " << output_path << ". Printing to console instead.\n";
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
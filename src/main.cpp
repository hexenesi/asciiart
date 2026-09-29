#include <cstdio>
#include <cstdlib> // For EXIT_SUCCESS/FAILURE
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Errors.h"
#include "ImageConverter.h"
#include "PageLayout.h"
#include "PdfWriter.h"
#include "PosterRenderer.h"
#include "TextRenderer.h"

struct Options {
    std::string image_path;
    int width = 0;      // 0 = automatic
    int height = 0;     // 0 = automatic
    double scale = 0.0; // chars per pixel; 0 = not set
    double brightness = 0.0;
    double contrast = 1.0;
    std::string charset = ImageConverter::charsetNames().front();
    std::string output_path; // empty = stdout
    bool invert = false;

    // Printable PDF output
    std::string pdf_path; // empty = no PDF
    PageSettings page;
    int pages_wide = 0; // 0 = not set
    int max_pages = 50;
    bool dry_run = false;
    std::vector<std::string> page_options_used; // for "requires --pdf" errors
};

void print_usage(std::ostream& out) {
    out << "Usage: ascii_converter [options] <image>\n"
           "\n"
           "Options:\n"
           "  --width <n>         Output width in characters (default: automatic, up to 100)\n"
           "  --height <n>        Output height in rows (default: automatic)\n"
           "  --scale <s>         Characters per source pixel (1 = one char per pixel)\n"
           "  --brightness <pct>  Brightness offset, -100 to 100 (default: 0)\n"
           "  --contrast <f>      Contrast factor, >= 0 (default: 1.0)\n"
           "  --charset <name>    Character set:";
    for (const auto& name : ImageConverter::charsetNames()) out << ' ' << name;
    out << " (default: " << ImageConverter::charsetNames().front() << ")\n"
           "  --invert            Invert intensities (for light text on dark background)\n"
           "  --output <file>     Write text to file instead of stdout\n"
           "  -h, --help          Show this help\n"
           "\n"
           "Printable PDF (tiled pages with overview and neighbour numbers):\n"
           "  --pdf <file>        Write a PDF poster (text is written only with --output)\n"
           "  --paper <name>      letter or a4 (default: letter)\n"
           "  --orientation <o>   portrait, landscape or auto (default: portrait)\n"
           "  --font-size <pt>    Courier size in points (default: 6)\n"
           "  --pages-wide <n>    Scale the art to exactly n pages across\n"
           "  --overlap <n>       Characters repeated on neighbouring pages (default: 0)\n"
           "  --glue-flap <pt>    Glue flap kept past right/bottom joins, marked with dashed\n"
           "                      trim marks; 0 = none (default: 18 pt = 0.25 in, max 36)\n"
           "  --max-pages <n>     Refuse to write more art pages than this (default: 50)\n"
           "  --dry-run           Print the page count and exit without writing\n"
           "\n"
           "With --pdf and no --width/--height/--scale/--pages-wide, the scale is 1 char per pixel.\n"
           "\n"
           "Examples:\n"
           "  ascii_converter foto.jpg --width 120\n"
           "  ascii_converter foto.jpg --pdf poster.pdf --paper a4 --pages-wide 3\n";
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

/** Parses a whole argument as an integer >= min_value; exits with an error on invalid input. */
int parse_int(const std::string& option, const std::string& value, int min_value) {
    try {
        size_t consumed = 0;
        int result = std::stoi(value, &consumed);
        if (consumed == value.size() && result >= min_value) return result;
    } catch (...) {
    }
    fail(option + " requires an integer >= " + std::to_string(min_value) + ", got '" + value + "'.");
}

bool isOneOf(const std::string& s, const std::vector<std::string>& list) {
    for (const auto& item : list)
        if (s == item) return true;
    return false;
}

/** Parses command-line arguments (see README); exits on invalid input or --help. */
Options parse_args(int argc, char* argv[]) {
    Options opts;
    const std::vector<std::string> value_options = {
        "--width", "--height",      "--scale",     "--brightness", "--contrast", "--charset",  "--output",
        "--pdf",   "--orientation", "--font-size", "--pages-wide", "--overlap",  "--max-pages", "--paper",
        "--glue-flap"};
    const std::vector<std::string> page_options = {"--paper",     "--orientation", "--font-size",
                                                   "--pages-wide", "--overlap",   "--max-pages",
                                                   "--dry-run",    "--glue-flap"};

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            std::exit(EXIT_SUCCESS);
        }
        if (isOneOf(arg, page_options)) opts.page_options_used.push_back(arg);

        if (arg == "--invert") {
            opts.invert = true;
        } else if (arg == "--dry-run") {
            opts.dry_run = true;
        } else if (arg.size() > 1 && arg[0] == '-') {
            if (!isOneOf(arg, value_options)) fail("unknown option '" + arg + "'.");
            if (i + 1 >= argc) fail(arg + " requires a value.");
            std::string value = argv[++i];

            if (arg == "--width") {
                opts.width = parse_int(arg, value, 1);
            } else if (arg == "--height") {
                opts.height = parse_int(arg, value, 1);
            } else if (arg == "--scale") {
                opts.scale = parse_number(arg, value);
                if (!(opts.scale > 0)) fail("--scale must be greater than 0.");
            } else if (arg == "--brightness") {
                opts.brightness = parse_number(arg, value);
            } else if (arg == "--contrast") {
                opts.contrast = parse_number(arg, value);
            } else if (arg == "--charset") {
                opts.charset = value;
            } else if (arg == "--output") {
                opts.output_path = value;
            } else if (arg == "--pdf") {
                opts.pdf_path = value;
            } else if (arg == "--paper") {
                if (value == "letter") opts.page.paper = Paper::Letter;
                else if (value == "a4") opts.page.paper = Paper::A4;
                else fail("--paper must be 'letter' or 'a4', got '" + value + "'.");
            } else if (arg == "--orientation") {
                if (value == "portrait") opts.page.orientation = Orientation::Portrait;
                else if (value == "landscape") opts.page.orientation = Orientation::Landscape;
                else if (value == "auto") opts.page.orientation = Orientation::Auto;
                else fail("--orientation must be 'portrait', 'landscape' or 'auto', got '" + value + "'.");
            } else if (arg == "--font-size") {
                opts.page.font_size = parse_number(arg, value);
                if (!(opts.page.font_size > 0)) fail("--font-size must be greater than 0.");
            } else if (arg == "--pages-wide") {
                opts.pages_wide = parse_int(arg, value, 1);
            } else if (arg == "--overlap") {
                opts.page.overlap = parse_int(arg, value, 0);
            } else if (arg == "--glue-flap") {
                opts.page.glue_flap = parse_number(arg, value);
                if (!(opts.page.glue_flap >= 0)) fail("--glue-flap must be 0 or greater.");
            } else {
                opts.max_pages = parse_int(arg, value, 1);
            }
        } else if (opts.image_path.empty()) {
            opts.image_path = arg;
        } else {
            fail("more than one image given ('" + opts.image_path + "' and '" + arg + "').");
        }
    }

    return opts;
}

/** Rejects option combinations that cannot be honoured together. */
void validate(const Options& opts) {
    const bool size_set = opts.width || opts.height;
    if (opts.scale > 0 && opts.pages_wide) fail("--scale and --pages-wide cannot be used together.");
    if ((opts.scale > 0 || opts.pages_wide) && size_set) {
        fail("--width/--height cannot be combined with --scale or --pages-wide.");
    }
    if (opts.pdf_path.empty() && !opts.page_options_used.empty()) {
        fail(opts.page_options_used.front() + " requires --pdf.");
    }
    if (!opts.pdf_path.empty() && opts.charset == "blocks") {
        fail("the 'blocks' charset is not supported in PDF output (ASCII only).");
    }
    if (!opts.pdf_path.empty()) {
        // Check page settings before the (possibly slow) conversion.
        try {
            computeGeometry(opts.page, opts.page.orientation == Orientation::Landscape);
        } catch (const LayoutError& e) {
            fail(e.what());
        }
    }
}

std::string baseName(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

/** Renders the grid into a file; returns false if the file cannot be written. */
bool renderToFile(const std::string& path, const GridRenderer& renderer, const AsciiGrid& grid) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    renderer.render(grid, file);
    return static_cast<bool>(file);
}

/** Converts and writes the PDF poster (and optional text copy). Returns the exit code. */
int runPdf(const Options& opts, ImageConverter& converter) {
    PageSettings page = opts.page;
    converter.setCharAspect(charAspect(page));

    std::string size_desc;
    if (opts.pages_wide) {
        int src_w = 0, src_h = 0;
        if (!ImageConverter::imageSize(opts.image_path, src_w, src_h)) {
            throw ImageLoadError(opts.image_path, "cannot read image size");
        }
        // Auto orientation could change the page width afterwards; fix it so the width is exact.
        if (page.orientation == Orientation::Auto) page.orientation = Orientation::Portrait;
        const int cols = columnsForPagesWide(opts.pages_wide, page);
        converter.setScale(static_cast<double>(cols) / src_w);
        size_desc = std::to_string(opts.pages_wide) + " pages wide";
    } else if (opts.width || opts.height) {
        size_desc = "Fixed size";
    } else {
        const double scale = opts.scale > 0 ? opts.scale : 1.0;
        converter.setScale(scale);
        char buf[48];
        std::snprintf(buf, sizeof buf, "Scale %g char/px", scale);
        size_desc = buf;
    }

    AsciiGrid grid = converter.convertToGrid();
    const int cols = static_cast<int>(grid.at(0).size());
    const int rows = static_cast<int>(grid.size());
    PageLayout layout = computeLayout(cols, rows, page);

    char font_desc[32];
    std::snprintf(font_desc, sizeof font_desc, "%g pt Courier", page.font_size);
    const std::string paper_desc = std::string(page.paper == Paper::A4 ? "A4" : "Letter") + " " +
                                   (layout.geometry.landscape ? "landscape" : "portrait");
    std::cerr << "[SUMMARY] " << cols << "x" << rows << " chars -> " << layout.pages_across << "x"
              << layout.pages_down << " = " << layout.pageCount() << (layout.pageCount() == 1 ? " page" : " pages")
              << " + overview (" << paper_desc << ", "
              << font_desc << ")\n";

    if (layout.pageCount() > opts.max_pages) {
        std::cerr << "Error: " << layout.pageCount() << " pages exceeds --max-pages " << opts.max_pages
                  << ". Raise --max-pages, or reduce --scale / --pages-wide.\n";
        return EXIT_FAILURE;
    }
    if (opts.dry_run) return EXIT_SUCCESS;

    PdfWriter pdf;
    renderPoster(grid, layout, page, {baseName(opts.image_path), size_desc + ", " + paper_desc + ", " + font_desc},
                 pdf);
    if (!pdf.save(opts.pdf_path)) {
        std::cerr << "[ERROR] Could not write PDF: " << opts.pdf_path << "\n";
        return EXIT_FAILURE;
    }
    std::cerr << "[SUCCESS] Saved PDF (" << pdf.pageCount() << " pages) to: " << opts.pdf_path << "\n";

    if (!opts.output_path.empty()) {
        if (!renderToFile(opts.output_path, TextRenderer(), grid)) {
            std::cerr << "[ERROR] Could not open file for writing: " << opts.output_path << "\n";
            return EXIT_FAILURE;
        }
        std::cerr << "[SUCCESS] Saved text to: " << opts.output_path << "\n";
    }
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    Options opts = parse_args(argc, argv);

    if (opts.image_path.empty()) {
        print_usage(std::cerr);
        return EXIT_FAILURE;
    }
    validate(opts);

    ImageConverter converter(opts.image_path);

    converter.setWidth(opts.width);
    converter.setHeight(opts.height);
    converter.setScale(opts.scale);
    converter.setInvert(opts.invert);
    if (!converter.setBrightness(opts.brightness)) fail("--brightness must be between -100 and 100.");
    if (!converter.setContrast(opts.contrast)) fail("--contrast must be 0 or greater.");
    if (!converter.setCharset(opts.charset)) fail("unknown charset '" + opts.charset + "'.");

    try {
        if (!opts.pdf_path.empty()) return runPdf(opts, converter);

        AsciiGrid grid = converter.convertToGrid();

        if (grid.empty()) {
            throw std::runtime_error("Conversion resulted in empty ASCII art after all processing stages.");
        }

        const TextRenderer text;
        if (!opts.output_path.empty()) {
            if (renderToFile(opts.output_path, text, grid)) {
                std::cerr << "[SUCCESS] Conversion complete! Saved art to: " << opts.output_path << std::endl;
            } else {
                std::cerr << "[ERROR] Could not open file for writing: " << opts.output_path
                          << ". Printing to console instead.\n";
                text.render(grid, std::cout); // Fallback to stdout
            }
        } else {
            text.render(grid, std::cout);
        }

    } catch (const ImageLoadError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (const LayoutError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] Conversion failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

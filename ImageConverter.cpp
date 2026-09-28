#include "ImageConverter.h"

#include <iostream>
#include <stdexcept>

// Compile the stb_image implementation in this translation unit only.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
ImageConverter::ImageConverter(const std::string& image_path)
    : m_image_path(image_path) {}

bool ImageConverter::loadAndGrayscale() {
    if (m_image_path.empty()) {
        std::cerr << "[ERROR] Cannot load: Internal image path is not set." << std::endl;
        return false;
    }
    std::cerr << "[INFO] Attempting to load image from: " << m_image_path << std::endl;
    

    int channels_in_file = 0;
    // Force 4 channels (RGBA) regardless of the file format.
    unsigned char* raw_data = stbi_load(m_image_path.c_str(), &m_source_width, &m_source_height, &channels_in_file, 4);

    if (!raw_data) {
        std::cerr << "[ERROR] Failed to load image using stb_image: " << m_image_path << ". Error: " << stbi_failure_reason() << std::endl;
        m_source_width = 0;
        m_source_height = 0;
        m_pixels.clear();
        return false;
    }

    size_t total_pixels = static_cast<size_t>(m_source_width) * static_cast<size_t>(m_source_height);
    m_pixels.resize(total_pixels);

    for (size_t i = 0; i < total_pixels; ++i) {
        const unsigned char* rgba = raw_data + i * 4;
        // Perceived luminance (Rec. 601): Y = 0.299R + 0.587G + 0.114B
        auto y = static_cast<unsigned char>(std::round(rgba[0] * 0.299 + rgba[1] * 0.587 + rgba[2] * 0.114));
        m_pixels[i] = {y, y, y, rgba[3]};
    }

    stbi_image_free(raw_data);
    std::cerr << "[SUCCESS] Image data loaded and converted to grayscale buffer ("
              << m_source_width << "x" << m_source_height << ").\n";
    return true;
}


void ImageConverter::applyAspectRatioCorrection() {
    if (m_source_width == 0 || m_source_height == 0) {
        std::cerr << "[WARNING] Source dimensions are zero. Aspect ratio correction skipped." << std::endl;
        return;
    }

    const double CHARACTER_ASPECT_RATIO = m_char_aspect;
    double source_aspect_ratio = static_cast<double>(m_source_width) / m_source_height;

    if (m_scale > 0.0) {
        // Scale mode: size follows the source, ignoring requested width/height.
        m_width = std::max(1, static_cast<int>(std::round(m_source_width * m_scale)));
        m_height = std::max(1, static_cast<int>(std::round(m_source_height * m_scale / CHARACTER_ASPECT_RATIO)));
        std::cerr << "[INFO] Scale " << m_scale << " chars/pixel: " << m_width << "x" << m_height << std::endl;
        return;
    }

    bool width_set = (m_width != 0);
    bool height_set = (m_height != 0);

    if (!width_set && !height_set) {
        // Case A: Nothing set. Default to source width capped at 100 columns, adjusting height for character aspect ratio.
        std::cerr << "[INFO] No dimensions set. Calculating target dimensions based on optimal scaling." << std::endl;
        const int DEFAULT_MAX_WIDTH = 100;
        m_width = std::min(m_source_width, DEFAULT_MAX_WIDTH);
        m_height = static_cast<int>(std::round(static_cast<double>(m_width) / (source_aspect_ratio * CHARACTER_ASPECT_RATIO)));
        if (m_height == 0) m_height = 1;
        std::cerr << "[INFO] Using source width and adjusted height: " << m_width << "x" << m_height << std::endl;

    } else if (!width_set && height_set) {
        // Case B: Height set (Fixed H). Calculate Width based on source aspect ratio and CHARACTER standard.
        m_width = static_cast<int>(std::round(static_cast<double>(m_height) * source_aspect_ratio * CHARACTER_ASPECT_RATIO));
        if (m_width == 0) m_width = 1;
        std::cerr << "[INFO] Setting Width based on provided Height (" << m_height 
                  << ") to preserve aspect ratio: " << m_width << "x" << m_height << std::endl;

    } else if (width_set && !height_set) {
        // Case C: Width set (Fixed W). Calculate Height based on source aspect ratio and CHARACTER standard.
        m_height = static_cast<int>(std::round(static_cast<double>(m_width) / (source_aspect_ratio * CHARACTER_ASPECT_RATIO)));
        if (m_height == 0) m_height = 1;
        std::cerr << "[INFO] Setting Height based on provided Width (" << m_width 
                  << ") to preserve aspect ratio: " << m_width << "x" << m_height << std::endl;

    } else { 
        // Case D: Both set. Respect user inputs, but warn if deviation is large.
        double target_ratio = static_cast<double>(m_width) / m_height;
        double expected_ratio = source_aspect_ratio * CHARACTER_ASPECT_RATIO;
        if (std::abs(target_ratio - expected_ratio) > 0.25) {
             std::cerr << "[WARNING] User specified dimensions (" << m_width << "x" << m_height 
                       << ") significantly deviate from the source aspect ratio. Image may be stretched." << std::endl;
        } else {
             std::cerr << "[INFO] User specified dimensions preserve aspect ratio well." << std::endl;
        }
    }
}

bool ImageConverter::resizePixels() {
    if(m_pixels.empty()) return false;
    if (m_width <= 0 || m_height <= 0) {
        std::cerr << "[WARNING] Width and Height not set or invalid (setWidth/setHeight missing). Skipping resize.\n";
        return false;
    }
    size_t final_size = static_cast<size_t>(m_width) * m_height;

    // Check if target dimensions match source dimensions, in which case no operation is needed.
    if (m_source_width == m_width && m_source_height == m_height) {
        std::cerr << "[INFO] Source and target dimensions match. Skipping resize.\n";
        return true;
    }

    // Nearest-neighbor: each target pixel samples the source pixel under its center.
    std::vector<Pixel> resized(final_size);

    for (int y_target = 0; y_target < m_height; ++y_target) {
        for (int x_target = 0; x_target < m_width; ++x_target) {
            double src_x_ratio = (x_target + 0.5) * m_source_width / m_width;
            double src_y_ratio = (y_target + 0.5) * m_source_height / m_height;

            int src_x = std::min(static_cast<int>(src_x_ratio), m_source_width - 1);
            int src_y = std::min(static_cast<int>(src_y_ratio), m_source_height - 1);

            size_t source_index = static_cast<size_t>(src_y) * m_source_width + src_x;
            size_t target_index = static_cast<size_t>(y_target) * m_width + x_target;
            resized[target_index] = m_pixels[source_index];
        }
    }
    m_pixels = std::move(resized);
    std::cerr << "[SUCCESS] Pixel data resized via Nearest Neighbor Interpolation to " << m_width << "x" << m_height << "." << std::endl;
    return true;
}

bool ImageConverter::adjustPixelIntensity() {
    // On normalized intensity v in [0, 1]:
    //   v' = C * (v - 0.5) + 0.5 + B / 100
    // Contrast C scales the deviation from mid-gray; brightness B shifts by a percent of the range.
    std::cerr << "[INFO] Applying Brightness (" << m_brightness << "%) and Contrast (" << m_contrast << ").\n";

    const double shift = m_brightness / 100.0;
    for (Pixel& pixel : m_pixels) {
        // Grayscale: R, G and B hold the same intensity.
        double v = pixel.r / 255.0;
        double v_new = std::clamp(m_contrast * (v - 0.5) + 0.5 + shift, 0.0, 1.0);
        unsigned char intensity = static_cast<unsigned char>(std::round(v_new * 255.0));
        pixel.r = pixel.g = pixel.b = intensity;
    }
    std::cerr << "[SUCCESS] Pixel intensities adjusted for Brightness/Contrast.\n";
    return true;
}

namespace {

struct CharsetPreset {
    const char* name;
    std::vector<std::string> glyphs; // darkest to lightest
};

std::vector<std::string> splitGlyphs(const std::string& s) {
    std::vector<std::string> glyphs;
    for (char ch : s) glyphs.emplace_back(1, ch);
    return glyphs;
}

// Well-known ASCII art ramps. "standard"/"detailed" are Paul Bourke's 10- and 70-level ramps.
const std::vector<CharsetPreset>& charsetPresets() {
    static const std::vector<CharsetPreset> presets = {
        {"standard", splitGlyphs("@%#*+=-:. ")},
        {"detailed", splitGlyphs("$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ")},
        {"simple",   splitGlyphs("#+-. ")},
        {"binary",   splitGlyphs("# ")},
        {"blocks",   {"█", "▓", "▒", "░", " "}},
    };
    return presets;
}

} // namespace

bool ImageConverter::imageSize(const std::string& path, int& width, int& height) {
    int channels = 0;
    return stbi_info(path.c_str(), &width, &height, &channels) != 0;
}

std::vector<std::string> ImageConverter::charsetNames() {
    std::vector<std::string> names;
    for (const auto& preset : charsetPresets()) names.emplace_back(preset.name);
    return names;
}

bool ImageConverter::setCharset(const std::string& name) {
    for (const auto& preset : charsetPresets()) {
        if (name == preset.name) {
            m_charset = preset.glyphs;
            return true;
        }
    }
    return false;
}

const std::string& ImageConverter::mapIntensityToChar(unsigned char intensity) const {
    // Dark pixels map to index 0 (densest glyph), light pixels to the last glyph.
    const auto& glyphs = m_charset.empty() ? charsetPresets().front().glyphs : m_charset;
    size_t index = static_cast<size_t>(std::round(intensity / 255.0 * (glyphs.size() - 1)));
    return glyphs[index];
}

AsciiGrid ImageConverter::generateGrid() const {
    AsciiGrid grid(m_height, std::vector<std::string>(m_width));

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const Pixel& pixel = m_pixels[static_cast<size_t>(y) * m_width + x];
            // Invert for light-on-dark terminals, then blend transparent pixels toward
            // the lightest glyph (blank) so they stay empty in both modes.
            double v = m_invert ? 255.0 - pixel.r : pixel.r;
            double alpha = pixel.a / 255.0;
            v = alpha * v + (1.0 - alpha) * 255.0;
            grid[y][x] = mapIntensityToChar(static_cast<unsigned char>(std::round(v)));
        }
    }

    return grid;
}

AsciiGrid ImageConverter::convertToGrid() {
    if (!loadAndGrayscale()) {
        throw std::runtime_error("Failed to load or process image data.");
    }

    // Start from the requested size so repeated calls give the same result.
    m_width = m_requested_width;
    m_height = m_requested_height;
    applyAspectRatioCorrection();
    resizePixels();
    adjustPixelIntensity();

    std::cerr << "[INFO] Conversion steps completed. Generating final ASCII art...\n";
    return generateGrid();
}

std::string ImageConverter::convert() {
    std::string ascii_art;
    for (const auto& row : convertToGrid()) {
        for (const auto& glyph : row) ascii_art += glyph;
        ascii_art += '\n';
    }
    return ascii_art;
}

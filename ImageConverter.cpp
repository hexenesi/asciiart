#include "ImageConverter.h"

#include <iostream>
#include <stdexcept>

// Compile the stb_image implementation in this translation unit only.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// --- Constructor and Setup ---

ImageConverter::ImageConverter(const std::string& image_path) 
    : m_image_path(image_path), m_width(0), m_height(0), m_brightness(0.0), m_contrast(1.0) {
    // Initialize state variables here based on the path provided during construction.
}

ImageConverter::ImageConverter() 
    : m_image_path("") // Empty string means no image loaded yet
{
    std::cerr << "[SETUP] ImageConverter object instantiated without an initial file path." << std::endl;
}

// Destructor required for completeness, though minimal logic shown.
ImageConverter::~ImageConverter() = default;

// --- Core Steps Implementation ---

bool ImageConverter::loadAndGrayscale() {
    if (m_image_path.empty()) { // Ensure an image path was set before calling load
        std::cerr << "[ERROR] Cannot load: Internal image path is not set." << std::endl;
        return false;
    }
    std::cerr << "[INFO] Attempting to load image from: " << m_image_path << std::endl;
    

    int channels_in_file = 0;
    // stbi_load reads the image data. It returns NULL on failure. We force 4 channels for consistency.
    unsigned char* raw_data = stbi_load(m_image_path.c_str(), &m_source_width, &m_source_height, &channels_in_file, 4);

    if (!raw_data) {
        std::cerr << "[ERROR] Failed to load image using stb_image: " << m_image_path << ". Error: " << stbi_failure_reason() << std::endl;
        m_source_width = 0;
        m_source_height = 0;
        m_pixels.clear();
        return false;
    }

    // Allocate memory for the pixel representation (we store it as RGBA, but only use the intensity).
    size_t total_pixels = m_source_width * m_source_height;
    m_pixels.resize(total_pixels);

    // Convert loaded raw RGB/RGBA data into our internal Pixel structure and calculate perceived grayscale intensity.
    for (int i = 0; i < total_pixels; ++i) {
        // Calculate the offset for the current pixel in the raw buffer
        int offset = i * 4;

        // Use standard luminosity calculation for perceived grayscale intensity: Y = 0.299R + 0.587G + 0.114B
        double intensity_double = (raw_data[offset] * 0.299) +
                                   (raw_data[offset+1] * 0.587) +
                                   (raw_data[offset+2] * 0.114);

        // Store the resulting pixel, keeping R/G/B components for potential future use, but basing intensity on 'intensity'.
        m_pixels[i] = {
            static_cast<unsigned char>(round(intensity_double)), // Use calculated grayscale value for all channels initially
            static_cast<unsigned char>(round(intensity_double)),
            static_cast<unsigned char>(round(intensity_double)),
            255                                                   // Assume fully opaque for now
        };
    }

    stbi_image_free(raw_data); // IMPORTANT: Free the memory allocated by stb_image.
    std::cerr << "[SUCCESS] Image data loaded and converted to grayscale buffer ("
              << m_source_width << "x" << m_source_height << ").\n";
    return true;
}


void ImageConverter::applyAspectRatioCorrection() {
    // Check if width and height are still zero (i.e., user didn't set them) or if default needs to be enforced.
    if (m_source_width == 0 || m_source_height == 0) {
        std::cerr << "[WARNING] Source dimensions are zero. Aspect ratio correction skipped." << std::endl;
        return;
    }

    // Placeholder constants based on README: Character aspect ratio is typically W/H = 2.0
    const double CHARACTER_ASPECT_RATIO = 2.0; 
    // We also track the source aspect ratio for better proportionality checks.
    double source_aspect_ratio = static_cast<double>(m_source_width) / m_source_height;

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
    // After this function, m_width and m_height should contain the final target dimensions for resizing.
}

bool ImageConverter::resizePixels() {
    if(m_pixels.empty()) return false;
    if (m_width <= 0 || m_height <= 0) {
        std::cerr << "[WARNING] Width and Height not set or invalid (setWidth/setHeight missing). Skipping resize.\n";
        return false;
    }
    size_t final_size = static_cast<size_t>(m_width) * m_height;

    // Check if target dimensions match source dimensions, in which case no operation is needed.
    if (m_source_width == static_cast<size_t>(m_width) && m_source_height == static_cast<size_t>(m_height)) {
        std::cerr << "[INFO] Source and target dimensions match. Skipping resize.\n";
        return true;
    }

    // Nearest Neighbor Interpolation Logic: Map each pixel (x', y') in the target grid 
    // back to the nearest source coordinate (x, y).
    std::vector<Pixel> resized(final_size);

    for (int y_target = 0; y_target < m_height; ++y_target) {
        for (int x_target = 0; x_target < m_width; ++x_target) {
            // Map target coordinates back to source coordinates (Nearest Neighbor)
            // Source indices must be cast carefully as the math requires doubles for ratios.
            // Sample at target pixel centers; safe for 1-pixel dimensions (no division by n-1).
            double src_x_ratio = (x_target + 0.5) * m_source_width / m_width;
            double src_y_ratio = (y_target + 0.5) * m_source_height / m_height;

            int src_x = std::min(static_cast<int>(src_x_ratio), m_source_width - 1);
            int src_y = std::min(static_cast<int>(src_y_ratio), m_source_height - 1);

            // !!! CRITICAL BOUNDARY CHECK ADDED HERE TO PREVENT ASSERTION FAILURE !!!
            if (src_x < 0 || src_x >= m_source_width || src_y < 0 || src_y >= m_source_height) {
                std::cerr << "[CRITICAL] Source coordinate (" << src_x << ", " << src_y 
                          << ") is out of bounds [0," << m_source_width-1 << "," << m_source_height-1 << "]. Skipping pixel." << std::endl;
                // Set the destination pixel to black/default if source data is invalid
                resized[y_target * m_width + x_target] = {0, 0, 0, 255};
                continue; 
            }
            // Calculate the linear index for the source pixel (in the original m_pixels buffer)
            size_t source_index = src_y * m_source_width + src_x;
            
            // The destination pixel index (row-major order in the new, smaller buffer)
            size_t target_index = y_target * m_width + x_target;

            // Copy the intensity data from the source pixel to the target pixel
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

std::string ImageConverter::generateAsciiArt()  {
    // ===============================================================
    // !!! PLACEHOLDER START: FINAL ASCII STRING GENERATION !!!
    // Iterate over the m_pixels buffer (m_width * m_height). For each pixel,
    // extract the grayscale intensity value and pass it to mapIntensityToChar().
    // Concatenate all characters row by row into a single string.
    // ===============================================================

    std::string ascii_art;
    ascii_art.reserve(static_cast<size_t>(m_height) * (m_width + 1)); // Reserve space for performance

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            // Calculate linear index (assuming row-major order)
            size_t index = y * m_width + x;

            // Retrieve the grayscale intensity from the pixel (since R, G, and B are same, we just use R)
            unsigned char intensity = m_pixels[index].r;
            ascii_art += mapIntensityToChar(intensity);
        }
        ascii_art += '\n'; // Add newline character at the end of each row
    }

    return ascii_art;
}


// --- Public Interface Methods ---

std::string ImageConverter::convert() {
    if (!loadAndGrayscale()) {
        throw std::runtime_error("Failed to load or process image data.");
    }

    // *** NEW STEP ORDER ***
    applyAspectRatioCorrection(); 
    resizePixels();
    adjustPixelIntensity();

    std::cerr << "\n[INFO] Conversion steps completed. Generating final ASCII art...\n";
    return generateAsciiArt();
}
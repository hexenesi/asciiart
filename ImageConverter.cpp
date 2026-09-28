#include "ImageConverter.h"
// Placeholder for external library headers (e.g., #define STB_IMAGE_IMPLEMENTATION and include the header)

// FIX: Keep macro definition here only once for this compilation unit.
#define STB_IMAGE_IMPLEMENTATION 
#include <iostream>
#include <stdexcept>
#include <include/stb_image.h> // Assuming stb_image.h is available
// --- Constructor and Setup ---

ImageConverter::ImageConverter(const std::string& image_path) 
    : m_image_path(image_path), m_width(0), m_height(0), m_brightness(1.0), m_contrast(1.0) {
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
        // Case A: Nothing set. Default to source width, adjusting height for character aspect ratio.
        std::cerr << "[INFO] No dimensions set. Calculating target dimensions based on optimal scaling." << std::endl;
        m_width = m_source_width;
        m_height = static_cast<int>(std::round(static_cast<double>(m_source_height) / CHARACTER_ASPECT_RATIO));
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
    // ===============================================================
    // !!! PLACEHOLDER START: BRIGHTNESS & CONTRAST ADJUSTMENT !!!
    // This must operate on the grayscale intensity value (which we extract from one channel, e.g., R).
    // Formula for adjusting a normalized value 'V' based on Brightness 'B' and Contrast 'C':
    // V_new = C * (V - 0.5) + B*0.5  (assuming initial grayscale is mapped to [0, 1])
    // Then clamp result back into the valid byte range [0, 255].
    // This operation must be performed on every pixel in m_pixels.
    // ===============================================================

    std::cerr << "[INFO] Applying Brightness (" << m_brightness << ") and Contrast (" << m_contrast << ").\n";
    

    for (size_t i = 0; i < m_pixels.size(); ++i) {
        // Since all channels R, G, B were set to the same intensity in loadAndGrayscale,
        // we can take any channel, e.g., Red component (R).
        unsigned char original_intensity = m_pixels[i].r;

        // 1. Normalize V from [0, 255] byte range to [0.0, 1.0] float range.
        double v_norm = static_cast<double>(original_intensity) / 255.0;

        // 2. Apply transformation: V_new_norm = C * (V - 0.5) + B*0.5
        // Note: Since we are operating on normalized [0, 1] values relative to the center point (0.5),
        // Contrast scales deviation from 0.5, and Brightness shifts the result.
        double v_new_norm = m_contrast * (v_norm - 0.5) + (m_brightness * 0.5);

        // 3. Clamp V_new_norm back into [0.0, 1.0] range for safety.
        v_new_norm = std::max(0.0, std::min(1.0, v_new_norm));

        // 4. Scale result back to byte range [0, 255].
        unsigned char new_intensity = static_cast<unsigned char>(std::round(v_new_norm * 255.0));

        // Update all components since they represent the same intensity value in this grayscale model.
        m_pixels[i].r = new_intensity;
        m_pixels[i].g = new_intensity;
        m_pixels[i].b = new_intensity;
    }
    std::cerr << "[SUCCESS] Pixel intensities adjusted for Brightness/Contrast.\n";
    return true;
}

char ImageConverter::mapIntensityToChar(unsigned char intensity)  {
    // An extended ASCII character set gradient from dark to light
    const std::string char_gradient = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
    const int GRADIENT_SIZE = char_gradient.length();

    // Intensity is normalized from [0, 255]. We want low intensity -> index 0 ('@').
    // Normalized position (0.0 to 1.0) based on the pixel value: intensity / 255.0
    double norm_intensity = static_cast<double>(intensity) / 255.0;

    // We need an inverse mapping: low intensity means high ASCII index, and vice versa for character density (which is counter-intuitive).
    // Since the gradient goes from dense ('@', 0% of range) to light ('.', 100% of range), we map:
    // Low Intensity (dark pixel -> near black) should correspond to Index 0.
    // High Intensity (light pixel -> near white) should correspond to Index N-1.

    // We scale the normalized intensity [0, 1] across the index space [0, GRADIENT_SIZE - 1].
    int char_index = static_cast<int>(std::round(norm_intensity * (GRADIENT_SIZE - 1)));

    if (char_index < 0) return ' '; // Fallback for errors
    return char_gradient[char_index];
}

std::string ImageConverter::generateAsciiArt()  {
    // ===============================================================
    // !!! PLACEHOLDER START: FINAL ASCII STRING GENERATION !!!
    // Iterate over the m_pixels buffer (m_width * m_height). For each pixel,
    // extract the grayscale intensity value and pass it to mapIntensityToChar().
    // Concatenate all characters row by row into a single string.
    // ===============================================================

    std::string ascii_art;
    ascii_art.reserve(m_source_height * m_width); // Reserve space for performance

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
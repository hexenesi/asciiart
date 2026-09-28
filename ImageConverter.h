#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>

// Definition of a pixel color structure for easy handling.
struct Pixel {
    unsigned char r, g, b, a; // RGBA components (using unsigned char for byte precision)
};

/**
 * @brief Manages the entire process of converting an image to ASCII art.
 * Implements object-oriented design pattern based on plan approval.
 */
class ImageConverter {
public:
 // Constructor loads the input file path and initializes internal state.
    ImageConverter(const std::string& image_path);
    // --- FIX 1: Default Constructor ---
    ImageConverter(); // Default constructor now required
    ~ImageConverter();

    /**
     * @brief Executes all conversion steps (grayscale, resize, adjust) and generates the ASCII art string.
     * @return The generated ASCII art as a single string.
     */
    std::string convert();

    // --- Configuration Getters/Setters ---

    /** Sets the desired output width for the ASCII grid. */
    void setWidth(int w) { m_width = w; }

    /** Sets the desired output height for the ASCII grid. */
    void setHeight(int h) { m_height = h; }

    /**
     * @brief Sets the brightness offset in percent of the full range (-100 to 100, 0 = unchanged).
     * @return false if out of range; the current value is kept.
     */
    bool setBrightness(double b) {
        if (!(b >= -100.0 && b <= 100.0)) return false;
        m_brightness = b;
        return true;
    }

    /**
     * @brief Sets the contrast factor (>= 0; 1 = unchanged, 0 = flat gray, > 1 = more contrast).
     * @return false if negative; the current value is kept.
     */
    bool setContrast(double c) {
        if (!(c >= 0.0 && std::isfinite(c))) return false;
        m_contrast = c;
        return true;
    }

    /**
     * @brief Sets the output file path. If empty, output will be printed to console.
     * @param output_path The desired file path.
     */
    void setOutputPath(const std::string& output_path) { m_output_path = output_path; }

    /**
     * @brief Selects a character set preset by name (see charsetNames()).
     * @return false if the name is unknown; the current charset is kept.
     */
    bool setCharset(const std::string& name);

    /** Names of the available charset presets, default first. */
    static std::vector<std::string> charsetNames();

    /**
     * @brief Handles loading the image and performing grayscale conversion.
     * NOTE: Placeholder for stb_image or OpenCV call. Must be implemented with external library binding.
     */
    bool loadAndGrayscale();


private:
    // State variables
    std::string m_image_path;
    std::string m_output_path;
    int m_width{0}; // Target width, 0 means auto/default
    int m_height{0}; // Target height, 0 means auto/default
    double m_brightness{0.0}; // percent offset, 0 = unchanged
    double m_contrast{1.0};
    // Glyphs ordered darkest to lightest; strings so multi-byte UTF-8 glyphs work.
    std::vector<std::string> m_charset;

    // Internal data holders (populated during constructor)
    std::vector<Pixel> m_pixels;
    int m_source_width = 0;
    int m_source_height = 0;
    /**
     * @brief Applies aspect ratio correction to the target dimensions based on known character ratios (e.g., 1:2).
     */
    void applyAspectRatioCorrection();


    /**
     * @brief Resizes the pixel data to match target dimensions (m_width x m_height).
     */
    bool resizePixels();

    /**
     * @brief Adjusts the brightness and contrast of all pixels.
     */
    bool adjustPixelIntensity();

    /**
     * @brief Converts a single pixel's intensity (0-255) into an ASCII character using the defined gradient.
     * Intensity is normalized from darkest (low value, dense char) to lightest (high value, space/light char).
     * @param intensity The calculated grayscale intensity (0=black/darkest, 255=white/lightest).
     * @return The corresponding glyph from the active charset.
     */
    const std::string& mapIntensityToChar(unsigned char intensity) const;

    /** Generates the final ASCII art string from the processed pixel grid. */
    std::string generateAsciiArt();
};

#endif // IMAGE_CONVERTER_H
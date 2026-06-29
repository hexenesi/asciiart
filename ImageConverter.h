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

    /** Sets the brightness adjustment factor (0.0 to 2.0). */
    void setBrightness(double b) { m_brightness = std::max(0.0, std::min(2.0, b)); }

    /** Sets the contrast adjustment factor (> 1.0 for contrast increase). */
    void setContrast(double c) { m_contrast = std::max(0.5, c); }

    /**
     * @brief Sets the output file path. If empty, output will be printed to console.
     * @param output_path The desired file path.
     */
    void setOutputPath(const std::string& output_path) { m_output_path = output_path; }

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
    double m_brightness{0.0};
    double m_contrast{1.0};

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
     * @return The corresponding ASCII character.
     */
    char mapIntensityToChar(unsigned char intensity);

    /** Generates the final ASCII art string from the processed pixel grid. */
    std::string generateAsciiArt();
};

#endif // IMAGE_CONVERTER_H
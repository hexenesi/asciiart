#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct Pixel {
    unsigned char r, g, b, a; // After loading: r = g = b = grayscale intensity, a = alpha
};

/**
 * @brief Manages the entire process of converting an image to ASCII art.
 */
class ImageConverter {
public:
    /** Stores the image path; the image is loaded by convert(). */
    explicit ImageConverter(const std::string& image_path);

    /**
     * @brief Executes all conversion steps (grayscale, resize, adjust) and generates the ASCII art string.
     * @return The generated ASCII art as a single string.
     */
    std::string convert();

    // --- Configuration Getters/Setters ---

    /** Sets the desired output width in characters (0 = automatic). */
    void setWidth(int w) { m_requested_width = w; }

    /** Sets the desired output height in rows (0 = automatic). */
    void setHeight(int h) { m_requested_height = h; }

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
     * @brief Selects a character set preset by name (see charsetNames()).
     * @return false if the name is unknown; the current charset is kept.
     */
    bool setCharset(const std::string& name);

    /** Maps dark pixels to light glyphs, for light-on-dark terminals. */
    void setInvert(bool invert) { m_invert = invert; }

    /** Names of the available charset presets, default first. */
    static std::vector<std::string> charsetNames();

private:
    // Configuration
    std::string m_image_path;
    int m_requested_width{0};  // 0 = automatic
    int m_requested_height{0}; // 0 = automatic
    double m_brightness{0.0}; // percent offset, 0 = unchanged
    double m_contrast{1.0};
    bool m_invert{false};
    // Glyphs ordered darkest to lightest; strings so multi-byte UTF-8 glyphs work.
    std::vector<std::string> m_charset;

    // Per-conversion state (reset by convert())
    std::vector<Pixel> m_pixels;
    int m_source_width = 0;
    int m_source_height = 0;
    int m_width = 0;  // Final output width
    int m_height = 0; // Final output height

    /** Loads the image with stb_image and converts it to grayscale. */
    bool loadAndGrayscale();

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
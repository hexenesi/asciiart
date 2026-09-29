#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct Pixel {
    unsigned char r, g, b, a; // After loading: r = g = b = grayscale intensity, a = alpha
};

/** Rows of glyphs; each glyph is a string so multi-byte UTF-8 characters fit in one cell. */
using AsciiGrid = std::vector<std::vector<std::string>>;

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

    /**
     * @brief Runs the same conversion as convert() but returns one glyph per cell.
     * @throws ImageLoadError if the image cannot be loaded.
     */
    AsciiGrid convertToGrid();

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

    /**
     * @brief Sets the glyph cell aspect (line height / character width). Default 2.0.
     * @return false if not a positive finite number; the current value is kept.
     */
    bool setCharAspect(double aspect) {
        if (!(aspect > 0.0 && std::isfinite(aspect))) return false;
        m_char_aspect = aspect;
        return true;
    }

    /**
     * @brief Sets characters per source pixel (1.0 = one char per pixel horizontally).
     * Rows are divided by the char aspect. Takes precedence over width/height and
     * the default 100-column cap. 0 disables scale mode.
     * @return false if negative or not finite; the current value is kept.
     */
    bool setScale(double scale) {
        if (!(scale >= 0.0 && std::isfinite(scale))) return false;
        m_scale = scale;
        return true;
    }

    /** Maps dark pixels to light glyphs, for light-on-dark terminals. */
    void setInvert(bool invert) { m_invert = invert; }

    /** Reads an image's pixel size from its header without decoding it; false if unreadable. */
    static bool imageSize(const std::string& path, int& width, int& height);

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
    double m_scale{0.0};       // Chars per source pixel; 0 = use width/height
    double m_char_aspect{2.0}; // Monospace cells are about twice as tall as wide.
    // Glyphs ordered darkest to lightest; strings so multi-byte UTF-8 glyphs work.
    std::vector<std::string> m_charset;

    // Per-conversion state (reset by convert())
    std::vector<Pixel> m_pixels;
    int m_source_width = 0;
    int m_source_height = 0;
    int m_width = 0;  // Final output width
    int m_height = 0; // Final output height

    /** Loads the image with stb_image and converts it to grayscale. @throws ImageLoadError */
    void loadAndGrayscale();

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

    /** Maps the processed pixel buffer to glyphs. */
    AsciiGrid generateGrid() const;
};

#endif // IMAGE_CONVERTER_H
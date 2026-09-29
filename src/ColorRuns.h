#ifndef COLOR_RUNS_H
#define COLOR_RUNS_H

#include <string>
#include <vector>

#include "ImageConverter.h"

/** fg: glyphs drawn in the cell color. bg: cell background filled with the color. */
enum class ColorStyle { Foreground, Background };

/** Perceived luminance (Rec. 601), 0..255. */
double luminance(const Rgb& color);

/** True if black text reads better than white on this color. */
bool isLight(const Rgb& color);

/**
 * @brief Rounds each channel to `levels` evenly spaced values (2..256; 256 = unchanged).
 * With 32 levels, 255 stays 255 and 0 stays 0, so pure colors survive.
 */
Rgb quantize(Rgb color, int levels);

/** A horizontal stretch of cells that share the same color (or all have none). */
struct ColorRun {
    Rgb color;
    bool has_color = false;
    int start = 0;  // first column of the run
    int length = 0; // number of cells (not bytes: glyphs may be multi-byte UTF-8)
    std::string text; // concatenated glyphs
};

/**
 * @brief Splits cells [begin, end) of a row into runs of equal color.
 * Cells without color group together regardless of their stored color.
 */
std::vector<ColorRun> splitRuns(const std::vector<AsciiCell>& row, int begin, int end);

/** splitRuns over the whole row. */
std::vector<ColorRun> splitRuns(const std::vector<AsciiCell>& row);

#endif // COLOR_RUNS_H

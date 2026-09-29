#include "ColorRuns.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

unsigned char quantizeChannel(unsigned char value, int levels) {
    const double step = 255.0 / (levels - 1);
    return static_cast<unsigned char>(std::round(std::round(value / step) * step));
}

bool sameColor(const AsciiCell& cell, const ColorRun& run) {
    if (cell.has_color != run.has_color) return false;
    return !cell.has_color || cell.color == run.color;
}

} // namespace

double luminance(const Rgb& color) {
    return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

bool isLight(const Rgb& color) {
    return luminance(color) >= 128.0;
}

Rgb quantize(Rgb color, int levels) {
    if (levels < 2 || levels > 256) throw std::invalid_argument("Color levels must be between 2 and 256.");
    if (levels == 256) return color;
    return {quantizeChannel(color.r, levels), quantizeChannel(color.g, levels), quantizeChannel(color.b, levels)};
}

std::vector<ColorRun> splitRuns(const std::vector<AsciiCell>& row, int begin, int end) {
    std::vector<ColorRun> runs;
    begin = std::max(begin, 0);
    end = std::min(end, static_cast<int>(row.size()));
    for (int c = begin; c < end; ++c) {
        const AsciiCell& cell = row[c];
        if (runs.empty() || !sameColor(cell, runs.back())) {
            ColorRun run;
            run.has_color = cell.has_color;
            if (cell.has_color) run.color = cell.color;
            run.start = c;
            runs.push_back(run);
        }
        runs.back().text += cell.glyph;
        ++runs.back().length;
    }
    return runs;
}

std::vector<ColorRun> splitRuns(const std::vector<AsciiCell>& row) {
    return splitRuns(row, 0, static_cast<int>(row.size()));
}

#include "AnsiRenderer.h"

#include <string>

namespace {

const char* const kReset = "\x1b[0m";

std::string rgbParams(const Rgb& c) {
    return std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b);
}

std::string colorCode(const Rgb& c, ColorStyle style) {
    if (style == ColorStyle::Foreground) return "\x1b[38;2;" + rgbParams(c) + "m";
    const Rgb text = isLight(c) ? Rgb{0, 0, 0} : Rgb{255, 255, 255};
    return "\x1b[38;2;" + rgbParams(text) + ";48;2;" + rgbParams(c) + "m";
}

} // namespace

void AnsiRenderer::render(const AsciiGrid& grid, std::ostream& out) const {
    for (const auto& row : grid) {
        bool colored = false; // terminal currently has a color set
        Rgb active;           // the color set, when colored
        for (const ColorRun& run : splitRuns(row)) {
            // In fg mode a space shows no color, so keep whatever is active instead of switching.
            if (m_style == ColorStyle::Foreground && run.text.find_first_not_of(' ') == std::string::npos) {
                out << run.text;
                continue;
            }
            if (run.has_color) {
                if (!colored || active != run.color) out << colorCode(run.color, m_style);
                colored = true;
                active = run.color;
            } else if (colored) {
                out << kReset;
                colored = false;
            }
            out << run.text;
        }
        if (colored) out << kReset;
        out << '\n';
    }
}

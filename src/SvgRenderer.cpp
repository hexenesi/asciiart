#include "SvgRenderer.h"

#include <cstdio>
#include <map>
#include <vector>

namespace {

constexpr double kFontSize = 10.0;
constexpr double kCellWidth = kFontSize * 0.6; // monospace advance
constexpr double kBaselineRatio = 0.8;         // baseline below the cell top, as a fraction of its height

std::string num(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2f", v);
    std::string s = buf;
    s.erase(s.find_last_not_of('0') + 1);
    if (s.back() == '.') s.pop_back();
    return s;
}

std::string hex(const Rgb& c) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

unsigned key(const Rgb& c) {
    return (static_cast<unsigned>(c.r) << 16) | (c.g << 8) | c.b;
}

// Assigns CSS classes to colors in order of first use.
class Palette {
public:
    int classFor(const Rgb& c) {
        auto it = m_index.emplace(key(c), static_cast<int>(m_colors.size()));
        if (it.second) m_colors.push_back(c);
        return it.first->second;
    }
    const std::vector<Rgb>& colors() const { return m_colors; }

private:
    std::map<unsigned, int> m_index;
    std::vector<Rgb> m_colors;
};

struct Segment {
    int start = 0, length = 0;
    std::string text;
    int text_class = -1; // -1 = default black
};

struct Rect {
    int start = 0, length = 0;
    int fill_class = 0;
};

} // namespace

std::string SvgRenderer::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char ch : s) {
        switch (ch) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += ch;
        }
    }
    return out;
}

void SvgRenderer::render(const AsciiGrid& grid, std::ostream& out) const {
    const bool color = m_options.color;
    const bool fg = m_options.style == ColorStyle::Foreground;
    const double cell_h = kCellWidth * m_options.char_aspect;
    const int rows = static_cast<int>(grid.size());
    const int cols = rows ? static_cast<int>(grid[0].size()) : 0;

    Palette palette;
    std::vector<std::vector<Rect>> rects(rows);
    std::vector<std::vector<Segment>> segments(rows);

    for (int r = 0; r < rows; ++r) {
        const auto& row = grid[r];
        if (color && !fg) {
            for (const ColorRun& run : splitRuns(row)) {
                if (run.has_color) rects[r].push_back({run.start, run.length, palette.classFor(run.color)});
            }
        }
        // Text: maximal stretches of non-blank glyphs sharing one text color.
        for (int c = 0; c < static_cast<int>(row.size()); ++c) {
            const AsciiCell& cell = row[c];
            if (cell.glyph == " ") continue;
            int text_class = -1;
            if (color && cell.has_color) {
                if (fg) text_class = palette.classFor(m_options.darken ? darkenForWhite(cell.color) : cell.color);
                else if (!isLight(cell.color)) text_class = palette.classFor({255, 255, 255});
            }
            auto& segs = segments[r];
            if (segs.empty() || segs.back().start + segs.back().length != c || segs.back().text_class != text_class) {
                segs.push_back({c, 0, {}, text_class});
            }
            segs.back().text += cell.glyph;
            ++segs.back().length;
        }
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " << num(cols * kCellWidth) << " "
        << num(rows * cell_h) << "\" width=\"" << num(cols * kCellWidth) << "\" height=\"" << num(rows * cell_h)
        << "\">\n<title>" << escape(m_options.title) << "</title>\n<style>\n"
        << "text { font-family: \"Courier New\", Courier, monospace; font-size: " << num(kFontSize)
        << "px; white-space: pre; }\n";
    for (size_t i = 0; i < palette.colors().size(); ++i) {
        out << ".c" << i << " { fill: " << hex(palette.colors()[i]) << "; }\n";
    }
    out << "</style>\n<rect width=\"100%\" height=\"100%\" fill=\"#fff\"/>\n";

    for (int r = 0; r < rows; ++r) {
        const double top = r * cell_h;
        for (const Rect& rect : rects[r]) {
            out << "<rect class=\"c" << rect.fill_class << "\" x=\"" << num(rect.start * kCellWidth) << "\" y=\""
                << num(top) << "\" width=\"" << num(rect.length * kCellWidth) << "\" height=\"" << num(cell_h)
                << "\"/>\n";
        }
    }
    for (int r = 0; r < rows; ++r) {
        if (segments[r].empty()) continue;
        out << "<text y=\"" << num(r * cell_h + cell_h * kBaselineRatio) << "\">";
        for (const Segment& seg : segments[r]) {
            out << "<tspan x=\"" << num(seg.start * kCellWidth) << "\" textLength=\""
                << num(seg.length * kCellWidth) << "\" lengthAdjust=\"spacingAndGlyphs\"";
            if (seg.text_class >= 0) out << " class=\"c" << seg.text_class << "\"";
            out << ">" << escape(seg.text) << "</tspan>";
        }
        out << "</text>\n";
    }
    out << "</svg>\n";
}

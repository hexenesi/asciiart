#include "HtmlRenderer.h"

#include <cstdio>
#include <map>
#include <vector>

namespace {

std::string hex(const Rgb& c) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

bool isBlank(const std::string& text) {
    return text.find_first_not_of(' ') == std::string::npos;
}

} // namespace

std::string HtmlRenderer::escape(const std::string& s) {
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

void HtmlRenderer::render(const AsciiGrid& grid, std::ostream& out) const {
    const bool fg = m_options.style == ColorStyle::Foreground;

    // Color each run is drawn with, after darkening; runs that need no span are skipped.
    auto displayColor = [&](const Rgb& c) { return (fg && m_options.darken) ? darkenForWhite(c) : c; };
    auto needsSpan = [&](const ColorRun& run) {
        return m_options.color && run.has_color && !(fg && isBlank(run.text));
    };

    // Pass 1: one class per distinct display color, in order of first appearance.
    std::map<unsigned, int> class_of; // packed RGB -> class index
    std::vector<Rgb> classes;
    std::vector<std::vector<ColorRun>> rows;
    rows.reserve(grid.size());
    for (const auto& row : grid) {
        rows.push_back(splitRuns(row));
        for (const ColorRun& run : rows.back()) {
            if (!needsSpan(run)) continue;
            Rgb c = displayColor(run.color);
            unsigned key = (c.r << 16) | (c.g << 8) | c.b;
            if (class_of.emplace(key, static_cast<int>(classes.size())).second) classes.push_back(c);
        }
    }

    char line_height[16];
    std::snprintf(line_height, sizeof line_height, "%.3g", m_options.char_aspect * 0.6);

    out << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n"
        << "<title>" << escape(m_options.title) << "</title>\n<style>\n"
        << "body { margin: 0; background: #fff; }\n"
        << "pre.ascii { margin: 16px; color: #000; font-family: \"Courier New\", Courier, monospace;"
        << " font-size: 10px; line-height: " << line_height << "; }\n";
    for (size_t i = 0; i < classes.size(); ++i) {
        const Rgb& c = classes[i];
        out << ".c" << i << " { ";
        if (fg) out << "color: " << hex(c) << ";";
        else out << "background-color: " << hex(c) << "; color: " << (isLight(c) ? "#000" : "#fff") << ";";
        out << " }\n";
    }
    out << "</style>\n</head>\n<body>\n<pre class=\"ascii\">";

    for (const auto& runs : rows) {
        for (const ColorRun& run : runs) {
            if (needsSpan(run)) {
                Rgb c = displayColor(run.color);
                unsigned key = (c.r << 16) | (c.g << 8) | c.b;
                out << "<span class=\"c" << class_of[key] << "\">" << escape(run.text) << "</span>";
            } else {
                out << escape(run.text);
            }
        }
        out << '\n';
    }
    out << "</pre>\n</body>\n</html>\n";
}

#ifndef HTML_RENDERER_H
#define HTML_RENDERER_H

#include <string>
#include <utility>

#include "ColorRuns.h"
#include "GridRenderer.h"

struct HtmlOptions {
    bool color = false;                      // false: black text only
    ColorStyle style = ColorStyle::Foreground;
    bool darken = true;                      // fg only: darken colors too pale for a white page
    double char_aspect = 2.0;                // cell height / width the grid was built for
    std::string title = "ASCII art";
};

/**
 * @brief Self-contained HTML page: a <pre> block on a white background.
 *
 * With color, each distinct color gets one CSS class (.c0, .c1, ...) and each color run is
 * one <span>. The line height is set from char_aspect so the art keeps its proportions
 * (monospace fonts advance about 0.6 em per character).
 */
class HtmlRenderer : public GridRenderer {
public:
    explicit HtmlRenderer(HtmlOptions options = {}) : m_options(std::move(options)) {}

    void render(const AsciiGrid& grid, std::ostream& out) const override;

    /** Escapes &, < and > (and " for attributes). */
    static std::string escape(const std::string& s);

private:
    HtmlOptions m_options;
};

#endif // HTML_RENDERER_H

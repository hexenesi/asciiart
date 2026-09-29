#ifndef SVG_RENDERER_H
#define SVG_RENDERER_H

#include <string>
#include <utility>

#include "ColorRuns.h"
#include "GridRenderer.h"

struct SvgOptions {
    bool color = false;                      // false: black text only
    ColorStyle style = ColorStyle::Foreground;
    bool darken = true;                      // fg only: darken colors too pale for a white page
    double char_aspect = 2.0;                // cell height / width the grid was built for
    std::string title = "ASCII art";
};

/**
 * @brief Standalone SVG image on a white background, sized in character cells.
 *
 * Cells are 6 x (6 * char_aspect) user units (10-unit monospace font). Each row is one <text>
 * element; each stretch of non-blank glyphs with the same color is a <tspan> with an explicit
 * x and textLength, so columns line up whatever monospace font the viewer picks.
 * In bg mode, each color run is also a <rect> behind the text.
 */
class SvgRenderer : public GridRenderer {
public:
    explicit SvgRenderer(SvgOptions options = {}) : m_options(std::move(options)) {}

    void render(const AsciiGrid& grid, std::ostream& out) const override;

    /** Escapes &, <, > and " for XML text and attributes. */
    static std::string escape(const std::string& s);

private:
    SvgOptions m_options;
};

#endif // SVG_RENDERER_H

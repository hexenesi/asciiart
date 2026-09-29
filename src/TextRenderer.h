#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "GridRenderer.h"

/** Plain text: glyphs only, one line per row, no color. */
class TextRenderer : public GridRenderer {
public:
    void render(const AsciiGrid& grid, std::ostream& out) const override;
};

#endif // TEXT_RENDERER_H

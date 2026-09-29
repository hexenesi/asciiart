#include "TextRenderer.h"

void TextRenderer::render(const AsciiGrid& grid, std::ostream& out) const {
    for (const auto& row : grid) {
        for (const auto& cell : row) out << cell.glyph;
        out << '\n';
    }
}

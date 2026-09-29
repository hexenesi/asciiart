#ifndef GRID_RENDERER_H
#define GRID_RENDERER_H

#include <ostream>

#include "ImageConverter.h"

/**
 * @brief Writes a whole AsciiGrid in one output format (text, ANSI, HTML, SVG).
 *
 * The PDF poster is not a GridRenderer: it also needs a PageLayout (see renderPoster()).
 */
class GridRenderer {
public:
    virtual ~GridRenderer() = default;

    virtual void render(const AsciiGrid& grid, std::ostream& out) const = 0;
};

#endif // GRID_RENDERER_H

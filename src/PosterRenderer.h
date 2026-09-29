#ifndef POSTER_RENDERER_H
#define POSTER_RENDERER_H

#include <string>

#include "ColorRuns.h"
#include "ImageConverter.h"
#include "PageLayout.h"
#include "PdfWriter.h"

/** Descriptive text for the overview page. Must be ASCII (non-ASCII is replaced with '?'). */
struct PosterInfo {
    std::string title;   // e.g. the image file name
    std::string details; // e.g. "Scale 1 char/px, Letter portrait, 6 pt Courier"
};

/** Color settings for the art pages. Labels, marks and the overview always stay black. */
struct PosterColor {
    bool enabled = false;
    ColorStyle style = ColorStyle::Foreground;
    bool darken = true; // fg only: darken colors too pale for white paper
};

/**
 * @brief Renders an overview page followed by one page per layout tile.
 *
 * Art pages: the art slice at the same origin on every sheet, alignment marks at the slice
 * corners, dashed trim marks for glue flaps on edges with a right or lower neighbour, the
 * page number and grid position, and neighbour numbers in the margins.
 * The overview page shows a map of the tile grid with page numbers.
 *
 * @throws std::invalid_argument if the grid does not match the layout or contains
 *         glyphs that are not single printable ASCII characters.
 */
void renderPoster(const AsciiGrid& grid, const PageLayout& layout, const PageSettings& settings,
                  const PosterInfo& info, PdfWriter& pdf, const PosterColor& color = {});

#endif // POSTER_RENDERER_H

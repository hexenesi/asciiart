#include "PosterRenderer.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {

constexpr double kLabelSize = 7.0;   // page and neighbour labels
constexpr double kMarkLength = 8.0;
constexpr double kMarkGap = 2.0;  // gap between a mark and the edge it points at
constexpr double kBaselineRatio = 0.8; // baseline sits this fraction of the line height below the cell top

double textWidth(const std::string& s, double size) {
    return static_cast<double>(s.size()) * size * kCourierAdvance;
}

std::string toAscii(const std::string& s) {
    std::string out;
    for (unsigned char ch : s) out += (ch >= 0x20 && ch <= 0x7E) ? static_cast<char>(ch) : '?';
    return out;
}

void validateGrid(const AsciiGrid& grid, const PageLayout& layout) {
    if (layout.pages.empty()) throw std::invalid_argument("Layout has no pages.");
    const Page& last = layout.pages.back();
    if (grid.size() != static_cast<size_t>(last.row_end)) {
        throw std::invalid_argument("Grid height does not match the layout.");
    }
    for (const auto& row : grid) {
        if (row.size() != static_cast<size_t>(last.col_end)) {
            throw std::invalid_argument("Grid width does not match the layout.");
        }
        for (const auto& glyph : row) {
            if (glyph.size() != 1 || glyph[0] < 0x20 || glyph[0] > 0x7E) {
                throw std::invalid_argument("PDF output supports ASCII charsets only (not 'blocks').");
            }
        }
    }
}

// Alignment marks: two short solid lines at each corner of the art slice, pointing away from it.
// They mark the exact edge of the art, where the neighbouring sheet's edge must land.
void drawAlignmentMarks(PdfWriter& pdf, double left, double bottom, double right, double top) {
    const double g = kMarkGap, l = kMarkLength;
    for (double x : {left, right}) {
        for (double y : {bottom, top}) {
            double dx = (x == left) ? -1 : 1;
            double dy = (y == bottom) ? -1 : 1;
            pdf.line(x + dx * g, y, x + dx * (g + l), y, 0.3, 0.4);
            pdf.line(x, y + dy * g, x, y + dy * (g + l), 0.3, 0.4);
        }
    }
}

// Trim marks: dashed ticks on the cut line of each glue flap, drawn outside the paper that is kept.
// Flaps exist only on edges that join a right or lower neighbour; other edges are cut at the
// alignment marks, so their cut line is already marked.
void drawTrimMarks(PdfWriter& pdf, const Page& page, double flap, double left, double bottom, double right,
                   double top) {
    if (flap <= 0) return;
    const double g = kMarkGap, l = kMarkLength;
    const double kept_right = page.right ? right + flap : right;
    const double kept_bottom = page.down ? bottom - flap : bottom;
    if (page.right) {
        const double x = right + flap;
        pdf.line(x, top + g, x, top + g + l, 0.5, 0.0, true);
        pdf.line(x, kept_bottom - g, x, kept_bottom - g - l, 0.5, 0.0, true);
    }
    if (page.down) {
        const double y = bottom - flap;
        pdf.line(left - g, y, left - g - l, y, 0.5, 0.0, true);
        pdf.line(kept_right + g, y, kept_right + g + l, y, 0.5, 0.0, true);
    }
}

void drawArtPage(PdfWriter& pdf, const AsciiGrid& grid, const PageLayout& layout, const PageSettings& settings,
                 const Page& page) {
    const PageGeometry& g = layout.geometry;
    pdf.beginPage(g.page_width, g.page_height);

    // Art: fixed origin on every sheet so neighbouring pages line up.
    const double art_left = g.art_left;
    const double art_top = g.page_height - g.art_top;
    std::vector<std::string> lines;
    for (int r = page.row_begin; r < page.row_end; ++r) {
        std::string line;
        for (int c = page.col_begin; c < page.col_end; ++c) line += grid[r][c];
        lines.push_back(line);
    }
    pdf.textLines(art_left, art_top - g.line_height * kBaselineRatio, settings.font_size, g.line_height, lines);

    // Marks at the bounds of this page's slice.
    const double slice_right = art_left + (page.col_end - page.col_begin) * g.char_width;
    const double slice_bottom = art_top - (page.row_end - page.row_begin) * g.line_height;
    drawAlignmentMarks(pdf, art_left, slice_bottom, slice_right, art_top);
    drawTrimMarks(pdf, page, settings.glue_flap, art_left, slice_bottom, slice_right, art_top);

    // Labels sit just outside the slice, so partial pages keep them next to the art.
    const double center_x = (art_left + slice_right) / 2;
    const double center_y = (art_top + slice_bottom) / 2;
    const double band = settings.label_band;
    const double label_baseline_offset = kLabelSize * 0.3; // roughly centers caps on a point

    auto centered = [&](const std::string& s, double cx, double cy) {
        pdf.text(cx - textWidth(s, kLabelSize) / 2, cy - label_baseline_offset, kLabelSize, s);
    };

    if (page.up) centered("^ " + std::to_string(page.up), center_x, art_top + band / 2);
    if (page.down) centered("v " + std::to_string(page.down), center_x, slice_bottom - band / 2);
    // Side bands are narrow: arrow above the number.
    if (page.left) {
        centered("<", art_left - band / 2, center_y + kLabelSize / 2);
        centered(std::to_string(page.left), art_left - band / 2, center_y - kLabelSize / 2);
    }
    if (page.right) {
        centered(">", slice_right + band / 2, center_y + kLabelSize / 2);
        centered(std::to_string(page.right), slice_right + band / 2, center_y - kLabelSize / 2);
    }

    const std::string own = "Page " + std::to_string(page.number) + "/" + std::to_string(layout.pageCount()) +
                            "  row " + std::to_string(page.row + 1) + ", col " + std::to_string(page.col + 1);
    // Left end of the bottom band; the "v" label is centered, so they do not collide.
    pdf.text(art_left, slice_bottom - band / 2 - label_baseline_offset, kLabelSize, own);
}

void drawOverviewPage(PdfWriter& pdf, const PageLayout& layout, const PageSettings& settings,
                      const PosterInfo& info) {
    const PageGeometry& g = layout.geometry;
    pdf.beginPage(g.page_width, g.page_height);

    const double left = g.art_left;
    const double right = g.page_width - g.art_left;
    double y = g.page_height - g.art_top - 14;

    pdf.text(left, y, 14, "ASCII poster: " + toAscii(info.title));
    y -= 20;
    const Page& last = layout.pages.back();
    std::vector<std::string> lines = {
        std::to_string(layout.pageCount()) + " pages: " + std::to_string(layout.pages_across) + " across x " +
            std::to_string(layout.pages_down) + " down",
        "Art: " + std::to_string(last.col_end) + " x " + std::to_string(last.row_end) + " characters",
    };
    if (!info.details.empty()) lines.push_back(toAscii(info.details));
    lines.push_back("");
    if (settings.glue_flap > 0) {
        char flap[16];
        std::snprintf(flap, sizeof flap, "%.0f mm", settings.glue_flap * 25.4 / 72);
        lines.push_back("Assembly: cut the left and top edges at the solid alignment marks. Cut the");
        lines.push_back("right and bottom edges at the dashed trim marks, keeping a " + std::string(flap) +
                        " glue flap.");
        lines.push_back("Lay each page over its left and upper neighbours' flaps, with its edge on");
        lines.push_back("their alignment marks, and glue.");
    } else {
        lines.push_back("Assembly: cut each page at its alignment marks (the corner marks) and");
        lines.push_back("butt the pages together.");
    }
    lines.push_back("Join the pages as in the map below. Each page shows its neighbours' numbers in");
    lines.push_back("the margins (^ above, v below, < left, > right).");
    const double info_size = 9, info_leading = 12;
    pdf.textLines(left, y, info_size, info_leading, lines);
    y -= info_leading * lines.size() + 12;

    // Map: tiles scaled uniformly into the remaining box, sized like their art slices.
    const double box_width = right - left;
    const double box_height = y - g.art_top;
    const double tile_w = g.cols_per_page * g.char_width;
    const double tile_h = g.rows_per_page * g.line_height;
    const double grid_w = (last.col_end) * g.char_width;
    const double grid_h = (last.row_end) * g.line_height;
    const double scale = std::min(box_width / grid_w, box_height / grid_h);
    const double map_left = left + (box_width - grid_w * scale) / 2;
    const double map_top = y;

    for (const Page& page : layout.pages) {
        const double x0 = map_left + page.col_begin * g.char_width * scale;
        const double y1 = map_top - page.row_begin * g.line_height * scale;
        const double w = (page.col_end - page.col_begin) * g.char_width * scale;
        const double h = (page.row_end - page.row_begin) * g.line_height * scale;
        pdf.rect(x0, y1 - h, w, h, 0.5, 0.3);

        const std::string number = std::to_string(page.number);
        const double size = std::clamp(std::min(tile_w, tile_h) * scale * 0.3, 4.0, 14.0);
        if (textWidth(number, size) < w * 0.9 && size < h * 0.9) {
            pdf.text(x0 + (w - textWidth(number, size)) / 2, y1 - h / 2 - size * 0.3, size, number);
        }
    }
}

} // namespace

void renderPoster(const AsciiGrid& grid, const PageLayout& layout, const PageSettings& settings,
                  const PosterInfo& info, PdfWriter& pdf) {
    validateGrid(grid, layout);
    drawOverviewPage(pdf, layout, settings, info);
    for (const Page& page : layout.pages) drawArtPage(pdf, grid, layout, settings, page);
}

#include "PageLayout.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

// Floor that tolerates floating-point noise (e.g. 504 / 3.6 = 139.99999...).
int floorCount(double value) {
    return static_cast<int>(std::floor(value + 1e-9));
}

// Pages needed to cover `total` chars with `per_page` per page and `overlap` repeated.
int pagesNeeded(int total, int per_page, int overlap) {
    if (total <= per_page) return 1;
    int step = per_page - overlap;
    return 1 + (total - per_page + step - 1) / step;
}

PageLayout layoutFor(int art_cols, int art_rows, const PageSettings& settings, bool landscape) {
    PageLayout layout;
    layout.geometry = computeGeometry(settings, landscape);
    const PageGeometry& g = layout.geometry;

    layout.pages_across = pagesNeeded(art_cols, g.cols_per_page, settings.overlap);
    layout.pages_down = pagesNeeded(art_rows, g.rows_per_page, settings.overlap);

    const int col_step = g.cols_per_page - settings.overlap;
    const int row_step = g.rows_per_page - settings.overlap;
    auto numberAt = [&](int row, int col) {
        if (row < 0 || col < 0 || row >= layout.pages_down || col >= layout.pages_across) return 0;
        return row * layout.pages_across + col + 1;
    };

    for (int row = 0; row < layout.pages_down; ++row) {
        for (int col = 0; col < layout.pages_across; ++col) {
            Page page;
            page.number = numberAt(row, col);
            page.row = row;
            page.col = col;
            page.up = numberAt(row - 1, col);
            page.down = numberAt(row + 1, col);
            page.left = numberAt(row, col - 1);
            page.right = numberAt(row, col + 1);
            page.col_begin = col * col_step;
            page.col_end = std::min(page.col_begin + g.cols_per_page, art_cols);
            page.row_begin = row * row_step;
            page.row_end = std::min(page.row_begin + g.rows_per_page, art_rows);
            layout.pages.push_back(page);
        }
    }
    return layout;
}

} // namespace

double charAspect(const PageSettings& settings) {
    return settings.leading / kCourierAdvance;
}

PageGeometry computeGeometry(const PageSettings& settings, bool landscape) {
    if (!(settings.font_size > 0) || !(settings.leading > 0) || settings.margin < 0 ||
        settings.label_band < 0 || settings.overlap < 0) {
        throw std::invalid_argument("Invalid page settings.");
    }

    double short_side = settings.paper == Paper::A4 ? 595.0 : 612.0;
    double long_side = settings.paper == Paper::A4 ? 842.0 : 792.0;

    PageGeometry g;
    g.landscape = landscape;
    g.page_width = landscape ? long_side : short_side;
    g.page_height = landscape ? short_side : long_side;
    g.char_width = settings.font_size * kCourierAdvance;
    g.line_height = settings.font_size * settings.leading;

    const double inset = settings.margin + settings.label_band;
    g.art_left = inset;
    g.art_top = inset;
    g.cols_per_page = floorCount((g.page_width - 2 * inset) / g.char_width);
    g.rows_per_page = floorCount((g.page_height - 2 * inset) / g.line_height);

    if (g.cols_per_page <= settings.overlap || g.rows_per_page <= settings.overlap) {
        throw std::invalid_argument("Page too small for the font size, margins and overlap.");
    }
    return g;
}

PageLayout computeLayout(int art_cols, int art_rows, const PageSettings& settings) {
    if (art_cols <= 0 || art_rows <= 0) {
        throw std::invalid_argument("Art size must be positive.");
    }
    if (settings.orientation != Orientation::Auto) {
        return layoutFor(art_cols, art_rows, settings, settings.orientation == Orientation::Landscape);
    }
    PageLayout portrait = layoutFor(art_cols, art_rows, settings, false);
    PageLayout landscape = layoutFor(art_cols, art_rows, settings, true);
    return landscape.pageCount() < portrait.pageCount() ? landscape : portrait;
}

int columnsForPagesWide(int pages_wide, const PageSettings& settings) {
    if (pages_wide <= 0) {
        throw std::invalid_argument("pages_wide must be positive.");
    }
    PageGeometry g = computeGeometry(settings, settings.orientation == Orientation::Landscape);
    return g.cols_per_page + (pages_wide - 1) * (g.cols_per_page - settings.overlap);
}

#ifndef PAGE_LAYOUT_H
#define PAGE_LAYOUT_H

#include <vector>

// Splits a character grid into printable pages. All lengths are in PDF points (1/72 in).

enum class Paper { Letter, A4 };
enum class Orientation { Portrait, Landscape, Auto };

struct PageSettings {
    Paper paper = Paper::Letter;
    Orientation orientation = Orientation::Portrait;
    double font_size = 6.0;   // Courier, points
    double leading = 1.0;     // line height = font_size * leading
    double margin = 36.0;     // unprintable edge, 0.5 in
    double label_band = 18.0; // extra space inside the margin for page/neighbour labels
    int overlap = 0;          // chars repeated on neighbouring pages
    double glue_flap = 18.0;  // paper kept past right/bottom joining edges for gluing (0.25 in); 0 = none
};

/** Courier advance width as a fraction of the font size. */
constexpr double kCourierAdvance = 0.6;

struct PageGeometry {
    double page_width = 0;  // after orientation
    double page_height = 0;
    bool landscape = false;
    double char_width = 0;
    double line_height = 0;
    int cols_per_page = 0;
    int rows_per_page = 0;
    double art_left = 0; // left edge of the art area
    double art_top = 0;  // top edge of the art area, measured from the top of the page
};

/** Page numbers are 1-based; 0 means no neighbour on that side. */
struct Page {
    int number = 0;
    int row = 0; // 0-based grid position
    int col = 0;
    int up = 0, down = 0, left = 0, right = 0;
    int col_begin = 0, col_end = 0; // half-open range of art columns on this page
    int row_begin = 0, row_end = 0; // half-open range of art rows on this page
};

struct PageLayout {
    PageGeometry geometry;
    int pages_across = 0;
    int pages_down = 0;
    std::vector<Page> pages; // row-major, pages[i].number == i + 1

    int pageCount() const { return static_cast<int>(pages.size()); }
};

/** Cell aspect (line height / char width) for the settings' font metrics. */
double charAspect(const PageSettings& settings);

/**
 * @brief Page size and character capacity for one orientation (Auto is treated as Portrait).
 * @throws std::invalid_argument if a page cannot hold more than `overlap` chars in each direction.
 */
PageGeometry computeGeometry(const PageSettings& settings, bool landscape);

/**
 * @brief Tiles an art_cols x art_rows grid onto pages. Auto orientation picks the one with
 * fewer pages (portrait on a tie).
 * @throws std::invalid_argument on non-positive art size or a page too small for the overlap.
 */
PageLayout computeLayout(int art_cols, int art_rows, const PageSettings& settings);

/**
 * @brief Total art columns that fill exactly `pages_wide` pages across, accounting for overlap.
 * Used to derive a scale for --pages-wide. Auto orientation is treated as Portrait.
 */
int columnsForPagesWide(int pages_wide, const PageSettings& settings);

#endif // PAGE_LAYOUT_H

// Unit tests for PageLayout (pure geometry, no files).

#include "PageLayout.h"

#include <iostream>
#include <stdexcept>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n"; \
            ++g_failures;                                                             \
        }                                                                             \
    } while (0)

template <typename F>
bool throwsInvalid(F f) {
    try {
        f();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

// Defaults: 6 pt Courier (3.6 x 6 pt cells), 36 pt margin + 18 pt label band on each side.
void testGeometry() {
    PageSettings s;
    PageGeometry letter = computeGeometry(s, false);
    CHECK(letter.cols_per_page == 140); // (612 - 108) / 3.6
    CHECK(letter.rows_per_page == 114); // (792 - 108) / 6
    CHECK(letter.art_left == 54 && letter.art_top == 54);

    PageGeometry letterLandscape = computeGeometry(s, true);
    CHECK(letterLandscape.page_width == 792 && letterLandscape.page_height == 612);
    CHECK(letterLandscape.cols_per_page == 190); // (792 - 108) / 3.6
    CHECK(letterLandscape.rows_per_page == 84);  // (612 - 108) / 6

    s.paper = Paper::A4;
    PageGeometry a4 = computeGeometry(s, false);
    CHECK(a4.cols_per_page == 135); // (595 - 108) / 3.6 = 135.3
    CHECK(a4.rows_per_page == 122); // (842 - 108) / 6 = 122.3

    CHECK(charAspect(PageSettings{}) > 1.666 && charAspect(PageSettings{}) < 1.667);
}

void testSinglePage() {
    PageLayout layout = computeLayout(140, 114, PageSettings{}); // exact fit
    CHECK(layout.pageCount() == 1);
    const Page& p = layout.pages[0];
    CHECK(p.number == 1 && p.up == 0 && p.down == 0 && p.left == 0 && p.right == 0);
    CHECK(p.col_begin == 0 && p.col_end == 140 && p.row_begin == 0 && p.row_end == 114);

    CHECK(computeLayout(1, 1, PageSettings{}).pageCount() == 1);
}

void testPartialPages() {
    // One column and one row over the exact fit -> 2 x 2
    PageLayout layout = computeLayout(141, 115, PageSettings{});
    CHECK(layout.pages_across == 2 && layout.pages_down == 2);
    const Page& last = layout.pages[3];
    CHECK(last.col_begin == 140 && last.col_end == 141);
    CHECK(last.row_begin == 114 && last.row_end == 115);
}

void testNeighbours() {
    // 3 x 3 grid, numbered row-major:
    // 1 2 3
    // 4 5 6
    // 7 8 9
    PageLayout layout = computeLayout(140 * 3, 114 * 3, PageSettings{});
    CHECK(layout.pages_across == 3 && layout.pages_down == 3 && layout.pageCount() == 9);
    for (int i = 0; i < layout.pageCount(); ++i) CHECK(layout.pages[i].number == i + 1);

    const Page& corner = layout.pages[0];
    CHECK(corner.up == 0 && corner.left == 0 && corner.right == 2 && corner.down == 4);

    const Page& edge = layout.pages[5]; // page 6, right edge
    CHECK(edge.up == 3 && edge.down == 9 && edge.left == 5 && edge.right == 0);

    const Page& middle = layout.pages[4];
    CHECK(middle.row == 1 && middle.col == 1);
    CHECK(middle.up == 2 && middle.down == 8 && middle.left == 4 && middle.right == 6);

    const Page& bottomRight = layout.pages[8];
    CHECK(bottomRight.down == 0 && bottomRight.right == 0 && bottomRight.up == 6 && bottomRight.left == 8);
}

void testOverlap() {
    PageSettings s;
    s.overlap = 10;
    // Step is 130 cols: 140 + 130 = 270 cols fit exactly in 2 pages
    PageLayout layout = computeLayout(270, 10, s);
    CHECK(layout.pages_across == 2);
    CHECK(layout.pages[1].col_begin == 130 && layout.pages[1].col_end == 270);
    CHECK(computeLayout(271, 10, s).pages_across == 3);

    CHECK(columnsForPagesWide(1, s) == 140);
    CHECK(columnsForPagesWide(3, s) == 140 + 2 * 130);
    CHECK(computeLayout(columnsForPagesWide(3, s), 10, s).pages_across == 3);
}

void testAutoOrientation() {
    PageSettings s;
    s.orientation = Orientation::Auto;
    // Wide art: 380 x 84 fits 2 landscape pages vs 3 portrait pages
    PageLayout wide = computeLayout(380, 84, s);
    CHECK(wide.geometry.landscape && wide.pageCount() == 2);

    // Tall art prefers portrait
    PageLayout tall = computeLayout(140, 228, s);
    CHECK(!tall.geometry.landscape && tall.pageCount() == 2);

    // Tie -> portrait
    CHECK(!computeLayout(10, 10, s).geometry.landscape);
}

void testInvalidInput() {
    CHECK(throwsInvalid([] { computeLayout(0, 10, PageSettings{}); }));
    CHECK(throwsInvalid([] { computeLayout(10, -1, PageSettings{}); }));
    CHECK(throwsInvalid([] { columnsForPagesWide(0, PageSettings{}); }));

    PageSettings huge;
    huge.font_size = 700; // 700 pt line > 684 pt usable height: zero rows
    CHECK(throwsInvalid([&] { computeLayout(10, 10, huge); }));

    PageSettings bigOverlap;
    bigOverlap.overlap = 140; // >= cols per page
    CHECK(throwsInvalid([&] { computeLayout(10, 10, bigOverlap); }));

    PageSettings wideFlap;
    wideFlap.glue_flap = 36.5; // > 18 pt band + half the 36 pt margin
    CHECK(throwsInvalid([&] { computeGeometry(wideFlap, false); }));
    wideFlap.glue_flap = 36;
    CHECK(!throwsInvalid([&] { computeGeometry(wideFlap, false); }));
    wideFlap.glue_flap = -1;
    CHECK(throwsInvalid([&] { computeGeometry(wideFlap, false); }));

    PageSettings badFont;
    badFont.font_size = 0;
    CHECK(throwsInvalid([&] { computeGeometry(badFont, false); }));
}

} // namespace

int main() {
    testGeometry();
    testSinglePage();
    testPartialPages();
    testNeighbours();
    testOverlap();
    testAutoOrientation();
    testInvalidInput();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

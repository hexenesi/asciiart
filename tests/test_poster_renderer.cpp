// Unit tests for PosterRenderer: page order, art slices, labels and validation.

#include "PosterRenderer.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n"; \
            ++g_failures;                                                             \
        }                                                                             \
    } while (0)

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

size_t countOf(const std::string& haystack, const std::string& needle) {
    size_t count = 0;
    for (size_t pos = 0; (pos = haystack.find(needle, pos)) != std::string::npos; pos += needle.size()) ++count;
    return count;
}

// Content streams in page order (PdfWriter writes each page's stream right after its page object).
std::vector<std::string> pageContents(const std::string& pdf) {
    std::vector<std::string> out;
    size_t pos = 0;
    while ((pos = pdf.find("stream\n", pos)) != std::string::npos) {
        size_t start = pos + 7;
        size_t end = pdf.find("\nendstream", start);
        out.push_back(pdf.substr(start, end - start));
        pos = end + 10;
    }
    return out;
}

// Grid where each cell's glyph encodes its column band, so slices are recognizable.
AsciiGrid makeGrid(int cols, int rows) {
    AsciiGrid grid(rows, std::vector<AsciiCell>(cols));
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) grid[r][c].glyph = std::string(1, static_cast<char>('A' + (c / 140) + 3 * (r / 114)));
    return grid;
}

std::vector<std::string> render(const AsciiGrid& grid, const PageSettings& settings, PageLayout* out_layout = nullptr,
                                const PosterColor& color = {}) {
    PageLayout layout = computeLayout(static_cast<int>(grid[0].size()), static_cast<int>(grid.size()), settings);
    if (out_layout) *out_layout = layout;
    PdfWriter pdf;
    renderPoster(grid, layout, settings, {"test.png", "Scale 1 char/px"}, pdf, color);
    return pageContents(pdf.finish());
}

void testPageOrderAndOverview() {
    PageLayout layout;
    auto pages = render(makeGrid(140 * 3, 114 * 3), PageSettings{}, &layout);
    CHECK(layout.pageCount() == 9);
    CHECK(pages.size() == 10u); // overview + 9 tiles
    CHECK(contains(pages[0], "(ASCII poster: test.png) Tj"));
    CHECK(contains(pages[0], "(9 pages: 3 across x 3 down) Tj"));
    CHECK(contains(pages[0], "(Scale 1 char/px) Tj"));
    CHECK(contains(pages[0], " re S")); // map tiles
}

void testArtSlices() {
    auto pages = render(makeGrid(140 * 3, 114 * 3), PageSettings{});
    // Page 1 (top-left) holds 'A', page 5 (middle) 'E', page 9 (bottom-right) 'I'.
    CHECK(contains(pages[1], "(" + std::string(140, 'A') + ") Tj"));
    CHECK(contains(pages[5], "(" + std::string(140, 'E') + ") Tj"));
    CHECK(contains(pages[9], "(" + std::string(140, 'I') + ") Tj"));
    CHECK(!contains(pages[5], "(" + std::string(140, 'A') + ")"));
    // Art starts at the same origin on every page: 54 pt inset, first baseline 792 - 54 - 4.8.
    for (size_t i = 1; i < pages.size(); ++i) CHECK(contains(pages[i], "6 TL 54 733.2 Td"));
}

void testNeighbourLabels() {
    auto pages = render(makeGrid(140 * 3, 114 * 3), PageSettings{});
    const std::string& middle = pages[5]; // page 5
    CHECK(contains(middle, "(^ 2) Tj"));
    CHECK(contains(middle, "(v 8) Tj"));
    CHECK(contains(middle, "(<) Tj") && contains(middle, "(4) Tj"));
    CHECK(contains(middle, "(>) Tj") && contains(middle, "(6) Tj"));
    CHECK(contains(middle, "(Page 5/9  row 2, col 2) Tj"));

    const std::string& corner = pages[1]; // page 1: only right and down
    CHECK(!contains(corner, "(^ "));
    CHECK(!contains(corner, "(<) Tj"));
    CHECK(contains(corner, "(v 4) Tj"));
    CHECK(contains(corner, "(>) Tj") && contains(corner, "(2) Tj"));
}

void testSinglePageHasNoNeighbours() {
    auto pages = render(makeGrid(10, 10), PageSettings{});
    CHECK(pages.size() == 2u);
    CHECK(!contains(pages[1], "(^ ") && !contains(pages[1], "(v ") && !contains(pages[1], "(<)") &&
          !contains(pages[1], "(>)"));
    CHECK(contains(pages[1], "(Page 1/1  row 1, col 1) Tj"));
    // 8 alignment-mark lines (2 per corner) and no trim marks without neighbours
    CHECK(countOf(pages[1], " l S") == 8u);
    CHECK(countOf(pages[1], "[2 2] 0 d") == 0u);
}

void testTrimMarks() {
    // 3 x 3 grid; flaps only on edges joining a right or lower neighbour, 2 dashed ticks per flap.
    auto pages = render(makeGrid(140 * 3, 114 * 3), PageSettings{});
    auto dashed = [&](int number) { return countOf(pages[number], "[2 2] 0 d"); };
    CHECK(dashed(1) == 4u); // right + down
    CHECK(dashed(3) == 2u); // down only (right edge of the poster)
    CHECK(dashed(7) == 2u); // right only (bottom edge)
    CHECK(dashed(9) == 0u); // bottom-right corner
    // Right flap cut line: 54 + 140 * 3.6 + 18 = 576
    CHECK(contains(pages[1], "[2 2] 0 d 576 "));
    // Bottom flap cut line: 738 - 114 * 6 - 18 = 36
    CHECK(contains(pages[1], "36 m"));
    CHECK(contains(pages[0], "dashed trim marks, keeping a 6 mm"));

    PageSettings noFlap;
    noFlap.glue_flap = 0;
    auto flat = render(makeGrid(140 * 3, 114 * 3), noFlap);
    for (size_t i = 1; i < flat.size(); ++i) CHECK(countOf(flat[i], "[2 2] 0 d") == 0u);
    CHECK(contains(flat[0], "butt the pages together"));
}

AsciiCell coloredCell(const std::string& glyph, Rgb color) {
    AsciiCell cell(glyph);
    cell.color = color;
    cell.has_color = true;
    return cell;
}

// One row: two dark red '@', a blank pale-yellow cell, a pale-yellow '.', and an uncolored ':'.
AsciiGrid colorGrid() {
    const Rgb red{200, 0, 0}, yellow{255, 255, 0};
    return {{coloredCell("@", red), coloredCell("@", red), coloredCell(" ", yellow), coloredCell(".", yellow),
             AsciiCell(":")}};
}

void testColorForeground() {
    PosterColor color;
    color.enabled = true;
    auto pages = render(colorGrid(), PageSettings{}, nullptr, color);
    const std::string& art = pages[1];
    // Red is dark enough to keep; the yellow run (blank + '.') is darkened (Y 226 -> 110); ':' black.
    CHECK(contains(art, "0.78 0 0 rg (@@) Tj 0.49 0.49 0 rg ( .) Tj 0 0 0 rg (:) Tj"));
    CHECK(!contains(art, " re f")); // no rectangles in fg mode
    // Labels after the art are drawn black (art wrapped in q/Q).
    CHECK(contains(art, "ET Q\n"));
    CHECK(!contains(pages[0], " rg")); // overview has no color

    color.darken = false;
    CHECK(contains(render(colorGrid(), PageSettings{}, nullptr, color)[1], "1 1 0 rg ( .) Tj"));
}

void testColorBackground() {
    PosterColor color;
    color.enabled = true;
    color.style = ColorStyle::Background;
    const std::string art = render(colorGrid(), PageSettings{}, nullptr, color)[1];
    // One rectangle per colored run (red x2, yellow x2), none for the uncolored cell.
    CHECK(countOf(art, " re f Q") == 2u);
    CHECK(contains(art, "q 0.78 0 0 rg 54 731.9 7.3 6.1 re f Q")); // 2 cells: 7.2 + 0.1 overlap
    CHECK(contains(art, "q 1 1 0 rg 61.2 731.9 7.3 6.1 re f Q"));  // backgrounds never darkened
    // White text on dark red, black on yellow and on the uncolored cell.
    CHECK(contains(art, "1 1 1 rg (@@) Tj 0 0 0 rg ( .) Tj (:) Tj"));
}

void testNoColorIsUnchanged() {
    // Color disabled: plain textLines, no color operators at all.
    auto pages = render(colorGrid(), PageSettings{});
    CHECK(!contains(pages[1], " rg"));
    CHECK(contains(pages[1], "(@@ .:) Tj"));
}

void testEscapedGlyphs() {
    AsciiGrid grid(1, std::vector<AsciiCell>{{"("}, {")"}, {"\\"}});
    auto pages = render(grid, PageSettings{});
    CHECK(contains(pages[1], "(\\(\\)\\\\) Tj"));
}

void testRejectsInvalidGrid() {
    auto throws = [](const AsciiGrid& grid, int cols, int rows) {
        PageLayout layout = computeLayout(cols, rows, PageSettings{});
        PdfWriter pdf;
        try {
            renderPoster(grid, layout, PageSettings{}, {}, pdf);
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };
    AsciiGrid blocks(1, std::vector<AsciiCell>{{"\xE2\x96\x88"}});
    CHECK(throws(blocks, 1, 1));
    CHECK(throws(makeGrid(10, 10), 11, 10)); // size mismatch
}

} // namespace

int main() {
    testPageOrderAndOverview();
    testArtSlices();
    testNeighbourLabels();
    testSinglePageHasNoNeighbours();
    testTrimMarks();
    testColorForeground();
    testColorBackground();
    testNoColorIsUnchanged();
    testEscapedGlyphs();
    testRejectsInvalidGrid();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

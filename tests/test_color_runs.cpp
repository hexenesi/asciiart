// Unit tests for color quantization and run splitting.

#include "ColorRuns.h"

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

AsciiCell colored(const std::string& glyph, Rgb color) {
    AsciiCell cell(glyph);
    cell.color = color;
    cell.has_color = true;
    return cell;
}

void testQuantize() {
    // 2 levels: each channel snaps to 0 or 255
    CHECK(quantize({10, 127, 128}, 2) == (Rgb{0, 0, 255}));
    CHECK(quantize({255, 0, 200}, 2) == (Rgb{255, 0, 255}));

    // 32 levels: step 255/31 = 8.226; extremes preserved, mid values snap to the nearest step
    CHECK(quantize({0, 255, 128}, 32) == (Rgb{0, 255, 132}));
    CHECK(quantize({4, 5, 250}, 32) == (Rgb{0, 8, 247}));
    CHECK(quantize({252, 252, 252}, 32) == (Rgb{255, 255, 255}));

    // 256 levels: unchanged
    CHECK(quantize({1, 2, 3}, 256) == (Rgb{1, 2, 3}));

    bool threw = false;
    try {
        quantize({0, 0, 0}, 1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void testUniformRow() {
    std::vector<AsciiCell> row(5, colored("@", {10, 20, 30}));
    auto runs = splitRuns(row);
    CHECK(runs.size() == 1u);
    CHECK(runs[0].start == 0 && runs[0].length == 5 && runs[0].text == "@@@@@");
    CHECK(runs[0].has_color && runs[0].color == (Rgb{10, 20, 30}));
}

void testAlternatingRow() {
    std::vector<AsciiCell> row;
    for (int i = 0; i < 4; ++i) row.push_back(colored(i % 2 ? "b" : "a", i % 2 ? Rgb{0, 0, 255} : Rgb{255, 0, 0}));
    auto runs = splitRuns(row);
    CHECK(runs.size() == 4u);
    for (int i = 0; i < 4; ++i) CHECK(runs[i].start == i && runs[i].length == 1);
}

void testMixedRow() {
    // red red | none none (different stored colors) | blue(multi-byte) blue
    std::vector<AsciiCell> row = {colored("@", {255, 0, 0}), colored("%", {255, 0, 0}), AsciiCell(" "),
                                  AsciiCell(" "), colored("\xE2\x96\x88", {0, 0, 255}),
                                  colored("\xE2\x96\x88", {0, 0, 255})};
    row[2].color = {1, 2, 3}; // ignored: no color
    row[3].color = {9, 9, 9};
    auto runs = splitRuns(row);
    CHECK(runs.size() == 3u);
    CHECK(runs[0].text == "@%" && runs[0].length == 2);
    CHECK(!runs[1].has_color && runs[1].start == 2 && runs[1].length == 2);
    CHECK(runs[2].start == 4 && runs[2].length == 2 && runs[2].text.size() == 6u);
}

void testSubrange() {
    std::vector<AsciiCell> row = {colored("a", {1, 1, 1}), colored("b", {1, 1, 1}), colored("c", {2, 2, 2}),
                                  colored("d", {2, 2, 2})};
    auto runs = splitRuns(row, 1, 3);
    CHECK(runs.size() == 2u);
    CHECK(runs[0].start == 1 && runs[0].text == "b");
    CHECK(runs[1].start == 2 && runs[1].text == "c");
    CHECK(splitRuns(row, 2, 2).empty());
    CHECK(splitRuns({}).empty());
}

} // namespace

int main() {
    testQuantize();
    testUniformRow();
    testAlternatingRow();
    testMixedRow();
    testSubrange();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

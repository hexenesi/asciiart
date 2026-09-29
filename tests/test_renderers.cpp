// Unit tests for GridRenderer implementations (ANSI; HTML and SVG later).

#include "AnsiRenderer.h"
#include "TextRenderer.h"

#include <iostream>
#include <sstream>
#include <string>

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

std::string render(const GridRenderer& renderer, const AsciiGrid& grid) {
    std::ostringstream out;
    renderer.render(grid, out);
    return out.str();
}

const Rgb kRed{255, 0, 0};
const Rgb kYellow{255, 255, 0};

// Row 1: red red | uncolored | yellow. Row 2: uncolored only.
AsciiGrid sampleGrid() {
    return {{colored("@", kRed), colored("%", kRed), AsciiCell(" "), colored(".", kYellow)}, {AsciiCell(":")}};
}

void testAnsiForeground() {
    std::string out = render(AnsiRenderer(ColorStyle::Foreground), sampleGrid());
    CHECK(out == "\x1b[38;2;255;0;0m@% "  // one code for the red run; the space keeps it
                 "\x1b[38;2;255;255;0m."  // yellow run
                 "\x1b[0m\n"              // reset at line end
                 ":\n");                  // uncolored line: no codes at all

    // A visible uncolored glyph after a colored run resets first.
    AsciiGrid grid = {{colored("@", kRed), AsciiCell(":")}};
    CHECK(render(AnsiRenderer(), grid) == "\x1b[38;2;255;0;0m@\x1b[0m:\n");
}

void testAnsiForegroundSkipsColoredSpaces() {
    AsciiGrid grid = {{colored("@", kRed), colored(" ", kYellow), colored(" ", kYellow), colored("@", kRed)}};
    // Spaces do not switch color, so the red code is emitted once.
    CHECK(render(AnsiRenderer(), grid) == "\x1b[38;2;255;0;0m@  @\x1b[0m\n");
    // In bg mode the spaces' background matters.
    CHECK(render(AnsiRenderer(ColorStyle::Background), grid).find(";48;2;255;255;0m  ") != std::string::npos);
    // A line of blank colored cells needs no codes at all.
    CHECK(render(AnsiRenderer(), {{colored(" ", kRed), colored(" ", kRed)}}) == "  \n");
}

void testAnsiBackground() {
    std::string out = render(AnsiRenderer(ColorStyle::Background), sampleGrid());
    // Red is dark -> white text; yellow is light -> black text.
    CHECK(out.find("\x1b[38;2;255;255;255;48;2;255;0;0m@%") == 0);
    CHECK(out.find("\x1b[38;2;0;0;0;48;2;255;255;0m.") != std::string::npos);
    CHECK(out.find("\x1b[0m\n:\n") != std::string::npos);
}

void testAnsiUtf8Glyphs() {
    AsciiGrid grid = {{colored("\xE2\x96\x88", kRed), colored("\xE2\x96\x93", kRed)}};
    CHECK(render(AnsiRenderer(), grid) == "\x1b[38;2;255;0;0m\xE2\x96\x88\xE2\x96\x93\x1b[0m\n");
}

void testTextHasNoEscapes() {
    std::string out = render(TextRenderer(), sampleGrid());
    CHECK(out == "@% .\n:\n");
    CHECK(out.find('\x1b') == std::string::npos);
}

void testLuminanceHelpers() {
    CHECK(isLight(kYellow) && !isLight(kRed));
    CHECK(isLight({255, 255, 255}) && !isLight({0, 0, 0}));
}

} // namespace

int main() {
    testAnsiForeground();
    testAnsiForegroundSkipsColoredSpaces();
    testAnsiBackground();
    testAnsiUtf8Glyphs();
    testTextHasNoEscapes();
    testLuminanceHelpers();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

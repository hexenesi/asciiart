// Unit tests for GridRenderer implementations (ANSI; HTML and SVG later).

#include "AnsiRenderer.h"
#include "HtmlRenderer.h"
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

    // Darkening keeps dark colors, scales pale ones down to the luminance limit.
    CHECK(darkenForWhite(kRed) == kRed); // Y = 76
    Rgb dark_yellow = darkenForWhite(kYellow); // Y = 226 -> scaled by 110/226
    CHECK(dark_yellow == (Rgb{124, 124, 0}));
    CHECK(luminance(darkenForWhite({255, 255, 255})) <= kMaxLuminanceOnWhite + 0.5);
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

size_t countOf(const std::string& haystack, const std::string& needle) {
    size_t count = 0;
    for (size_t pos = 0; (pos = haystack.find(needle, pos)) != std::string::npos; pos += needle.size()) ++count;
    return count;
}

void testHtmlPlain() {
    AsciiGrid grid = {{AsciiCell("<"), AsciiCell("&"), AsciiCell(">")}, {AsciiCell("@")}};
    HtmlOptions options;
    options.title = "a <b>";
    std::string out = render(HtmlRenderer(options), grid);
    CHECK(out.rfind("<!DOCTYPE html>", 0) == 0);
    CHECK(contains(out, "<meta charset=\"utf-8\">"));
    CHECK(contains(out, "<title>a &lt;b&gt;</title>"));
    CHECK(contains(out, "<pre class=\"ascii\">&lt;&amp;&gt;\n@\n</pre>"));
    CHECK(contains(out, "line-height: 1.2;")); // 2.0 aspect * 0.6 em
    CHECK(contains(out, "font-size: 13px;"));
    CHECK(!contains(out, "<span"));
    CHECK(contains(out, "</html>\n"));
}

void testHtmlForeground() {
    HtmlOptions options;
    options.color = true;
    std::string out = render(HtmlRenderer(options), sampleGrid());
    // Red kept, yellow darkened; one class per color; one span per visible run.
    CHECK(contains(out, ".c0 { color: #ff0000; }"));
    CHECK(contains(out, ".c1 { color: #7c7c00; }"));
    CHECK(!contains(out, ".c2"));
    CHECK(contains(out, "<span class=\"c0\">@%</span> <span class=\"c1\">.</span>\n:\n"));

    options.darken = false;
    CHECK(contains(render(HtmlRenderer(options), sampleGrid()), ".c1 { color: #ffff00; }"));
}

void testHtmlBackground() {
    HtmlOptions options;
    options.color = true;
    options.style = ColorStyle::Background;
    AsciiGrid grid = {{colored(" ", kYellow), colored(" ", kYellow), colored("@", kRed)}};
    std::string out = render(HtmlRenderer(options), grid);
    // Backgrounds are never darkened; text is black on light, white on dark.
    CHECK(contains(out, ".c0 { background-color: #ffff00; color: #000; }"));
    CHECK(contains(out, ".c1 { background-color: #ff0000; color: #fff; }"));
    CHECK(contains(out, "<span class=\"c0\">  </span><span class=\"c1\">@</span>"));
}

void testHtmlRepeatedColorsShareClass() {
    HtmlOptions options;
    options.color = true;
    AsciiGrid grid = {{colored("@", kRed), AsciiCell(":"), colored("@", kRed)},
                      {colored("\xE2\x96\x88", kRed)}};
    std::string out = render(HtmlRenderer(options), grid);
    CHECK(countOf(out, "{ color:") == 1u);
    CHECK(countOf(out, "<span class=\"c0\">") == 3u);
    CHECK(contains(out, "<span class=\"c0\">\xE2\x96\x88</span>"));
}

} // namespace

int main() {
    testAnsiForeground();
    testAnsiForegroundSkipsColoredSpaces();
    testAnsiBackground();
    testAnsiUtf8Glyphs();
    testTextHasNoEscapes();
    testLuminanceHelpers();
    testHtmlPlain();
    testHtmlForeground();
    testHtmlBackground();
    testHtmlRepeatedColorsShareClass();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

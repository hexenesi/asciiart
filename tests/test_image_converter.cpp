// Unit tests for ImageConverter. Self-contained: writes its own PNG fixtures
// (uncompressed deflate) so no image encoder or test framework is needed.

#include "ImageConverter.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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

// --- Minimal PNG writer (RGBA8, stored deflate blocks) ---

uint32_t crc32(const std::string& data) {
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

void putU32(std::string& out, uint32_t v) {
    for (int shift = 24; shift >= 0; shift -= 8) out += static_cast<char>((v >> shift) & 0xFF);
}

void writeChunk(std::string& png, const std::string& type, const std::string& data) {
    putU32(png, static_cast<uint32_t>(data.size()));
    std::string body = type + data;
    png += body;
    putU32(png, crc32(body));
}

struct Rgba {
    unsigned char r, g, b, a;
};

std::string writePng(const std::string& name, int w, int h, const std::function<Rgba(int, int)>& pixel) {
    std::string raw;
    for (int y = 0; y < h; ++y) {
        raw += '\0'; // filter: none
        for (int x = 0; x < w; ++x) {
            Rgba p = pixel(x, y);
            raw += static_cast<char>(p.r);
            raw += static_cast<char>(p.g);
            raw += static_cast<char>(p.b);
            raw += static_cast<char>(p.a);
        }
    }

    std::string zlib = "\x78\x01";
    for (size_t pos = 0; pos < raw.size() || pos == 0; pos += 65535) {
        size_t len = std::min<size_t>(65535, raw.size() - pos);
        bool last = pos + len >= raw.size();
        zlib += static_cast<char>(last ? 1 : 0);
        zlib += static_cast<char>(len & 0xFF);
        zlib += static_cast<char>(len >> 8);
        zlib += static_cast<char>(~len & 0xFF);
        zlib += static_cast<char>((~len >> 8) & 0xFF);
        zlib += raw.substr(pos, len);
        if (last) break;
    }
    uint32_t a = 1, b = 0;
    for (unsigned char byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    putU32(zlib, (b << 16) | a);

    std::string ihdr;
    putU32(ihdr, static_cast<uint32_t>(w));
    putU32(ihdr, static_cast<uint32_t>(h));
    ihdr += std::string("\x08\x06\x00\x00\x00", 5); // 8-bit RGBA

    std::string png = "\x89PNG\r\n\x1a\n";
    writeChunk(png, "IHDR", ihdr);
    writeChunk(png, "IDAT", zlib);
    writeChunk(png, "IEND", "");

    std::string path = name + ".png";
    std::ofstream(path, std::ios::binary) << png;
    return path;
}

std::string solid(const std::string& name, int w, int h, unsigned char gray, unsigned char alpha = 255) {
    return writePng(name, w, h, [=](int, int) { return Rgba{gray, gray, gray, alpha}; });
}

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string line; std::getline(in, line);) out.push_back(line);
    return out;
}

std::string convert(const std::string& path, const std::function<void(ImageConverter&)>& configure = {}) {
    ImageConverter converter(path);
    if (configure) configure(converter);
    return converter.convert();
}

// --- Tests ---

void testMissingFileThrows() {
    bool threw = false;
    try {
        convert("does_not_exist.png");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}

void testDefaultSizeCappedAt100() {
    // 400x200 source: width capped to 100, height = 100 / (2.0 source ratio * 2.0 char ratio) = 25
    auto rows = lines(convert(solid("wide", 400, 200, 0)));
    CHECK(rows.size() == 25u);
    CHECK(!rows.empty() && rows[0].size() == 100u);
}

void testWidthOnlyKeepsAspect() {
    // 200x100 source, width 40: height = 40 / (2.0 * 2.0) = 10
    auto rows = lines(convert(solid("aspect", 200, 100, 0), [](ImageConverter& c) { c.setWidth(40); }));
    CHECK(rows.size() == 10u);
    CHECK(!rows.empty() && rows[0].size() == 40u);
}

void testHeightOnlyKeepsAspect() {
    // 200x100 source, height 10: width = 10 * 2.0 * 2.0 = 40
    auto rows = lines(convert(solid("aspect", 200, 100, 0), [](ImageConverter& c) { c.setHeight(10); }));
    CHECK(rows.size() == 10u);
    CHECK(!rows.empty() && rows[0].size() == 40u);
}

void testCustomCharAspect() {
    // 200x100 source, aspect 1.0: width 40 -> height 40 / (2.0 * 1.0) = 20
    auto rows = lines(convert(solid("aspect", 200, 100, 0), [](ImageConverter& c) {
        c.setCharAspect(1.0);
        c.setWidth(40);
    }));
    CHECK(rows.size() == 20u);

    // height 20 -> width 20 * 2.0 * 1.0 = 40
    rows = lines(convert(solid("aspect", 200, 100, 0), [](ImageConverter& c) {
        c.setCharAspect(1.0);
        c.setHeight(20);
    }));
    CHECK(!rows.empty() && rows[0].size() == 40u);

    ImageConverter c("unused.png");
    CHECK(c.setCharAspect(1.67));
    CHECK(!c.setCharAspect(0) && !c.setCharAspect(-1));
}

void testScaleMode() {
    auto path = solid("scale", 300, 200, 0);
    auto size = [&](double scale, double aspect) {
        auto rows = lines(convert(path, [&](ImageConverter& c) {
            c.setCharAspect(aspect);
            c.setScale(scale);
            c.setWidth(10); // ignored in scale mode
        }));
        return std::make_pair(rows.empty() ? 0u : rows[0].size(), rows.size());
    };
    // 1:1 ignores the 100-column cap: 300 cols, 200 / 2.0 = 100 rows
    CHECK(size(1.0, 2.0) == std::make_pair(size_t{300}, size_t{100}));
    CHECK(size(0.5, 2.0) == std::make_pair(size_t{150}, size_t{50}));
    CHECK(size(1.0, 1.0 / 0.6) == std::make_pair(size_t{300}, size_t{120}));
    // Tiny scale never produces an empty image
    CHECK(size(0.0001, 2.0) == std::make_pair(size_t{1}, size_t{1}));

    ImageConverter c(path);
    CHECK(c.setScale(0) && c.setScale(2.5));
    CHECK(!c.setScale(-1));
}

void testOnePixelOutput() {
    auto art = convert(solid("one", 50, 50, 0), [](ImageConverter& c) {
        c.setWidth(1);
        c.setHeight(1);
    });
    CHECK(art == "@\n");
}

void testUpscaleKeepsContent() {
    // Left half black, right half white; upscaling must not smear one into the other.
    auto path = writePng("halves", 2, 1, [](int x, int) {
        unsigned char v = x == 0 ? 0 : 255;
        return Rgba{v, v, v, 255};
    });
    auto art = convert(path, [](ImageConverter& c) {
        c.setWidth(8);
        c.setHeight(2);
    });
    CHECK(art == "@@@@    \n@@@@    \n");
}

void testCharsetMapping() {
    auto black = solid("black", 4, 4, 0);
    auto white = solid("white", 4, 4, 255);
    auto size = [](ImageConverter& c) {
        c.setWidth(1);
        c.setHeight(1);
    };
    CHECK(convert(black, size) == "@\n");
    CHECK(convert(white, size) == " \n");
    CHECK(convert(black, [&](ImageConverter& c) { size(c); c.setCharset("detailed"); }) == "$\n");
    CHECK(convert(black, [&](ImageConverter& c) { size(c); c.setCharset("blocks"); }) == "█\n");

    ImageConverter c(black);
    CHECK(!c.setCharset("nope"));
    CHECK(ImageConverter::charsetNames().front() == "standard");
}

void testInvert() {
    auto size = [](ImageConverter& c) {
        c.setWidth(1);
        c.setHeight(1);
        c.setInvert(true);
    };
    CHECK(convert(solid("black", 4, 4, 0), size) == " \n");
    CHECK(convert(solid("white", 4, 4, 255), size) == "@\n");
}

void testTransparentIsBlank() {
    auto clear = solid("clear", 4, 4, 0, 0);
    auto size = [](ImageConverter& c) {
        c.setWidth(1);
        c.setHeight(1);
    };
    CHECK(convert(clear, size) == " \n");
    CHECK(convert(clear, [&](ImageConverter& c) { size(c); c.setInvert(true); }) == " \n");
}

void testBrightnessAndContrast() {
    auto size = [](ImageConverter& c) {
        c.setWidth(1);
        c.setHeight(1);
    };
    auto black = solid("black", 4, 4, 0);
    CHECK(convert(black, [&](ImageConverter& c) { size(c); c.setBrightness(100); }) == " \n");
    CHECK(convert(solid("white", 4, 4, 255), [&](ImageConverter& c) { size(c); c.setBrightness(-100); }) == "@\n");
    // Contrast 0 flattens everything to mid-gray: middle of "@%#*+=-:. " (index 5 of 0..9 after rounding 4.5).
    CHECK(convert(black, [&](ImageConverter& c) { size(c); c.setContrast(0); }) == "=\n");

    ImageConverter c(black);
    CHECK(c.setBrightness(-100) && c.setBrightness(100) && c.setBrightness(0));
    CHECK(!c.setBrightness(100.5) && !c.setBrightness(-101));
    CHECK(c.setContrast(0) && c.setContrast(3));
    CHECK(!c.setContrast(-0.1));
}

void testGridMatchesString() {
    auto path = writePng("gradient", 64, 32, [](int x, int) {
        auto v = static_cast<unsigned char>(x * 4);
        return Rgba{v, v, v, 255};
    });
    auto configure = [](ImageConverter& c) {
        c.setWidth(20);
        c.setCharset("blocks");
    };
    ImageConverter gridConverter(path);
    configure(gridConverter);
    AsciiGrid grid = gridConverter.convertToGrid();

    auto rows = lines(convert(path, configure));
    CHECK(grid.size() == rows.size());
    std::string joined;
    for (const auto& row : grid) {
        CHECK(row.size() == 20u);
        for (const auto& glyph : row) joined += glyph;
        joined += '\n';
    }
    CHECK(joined == convert(path, configure));
}

void testConvertIsRepeatable() {
    ImageConverter c(solid("repeat", 300, 150, 128));
    auto first = c.convert();
    CHECK(first == c.convert());
}

} // namespace

int main() {
    testMissingFileThrows();
    testDefaultSizeCappedAt100();
    testWidthOnlyKeepsAspect();
    testHeightOnlyKeepsAspect();
    testCustomCharAspect();
    testScaleMode();
    testOnePixelOutput();
    testUpscaleKeepsContent();
    testCharsetMapping();
    testInvert();
    testTransparentIsBlank();
    testBrightnessAndContrast();
    testGridMatchesString();
    testConvertIsRepeatable();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

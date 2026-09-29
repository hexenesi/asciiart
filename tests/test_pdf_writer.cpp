// Unit tests for PdfWriter: document structure, xref offsets and escaping.

#include "PdfWriter.h"

#include "miniz.h"

#include <cstdlib>
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

// Checks every xref entry points at "<id> 0 obj" and startxref points at "xref".
bool xrefIsValid(const std::string& pdf) {
    size_t sx = pdf.rfind("startxref\n");
    if (sx == std::string::npos) return false;
    size_t xref_offset = std::strtoul(pdf.c_str() + sx + 10, nullptr, 10);
    if (pdf.compare(xref_offset, 5, "xref\n") != 0) return false;

    size_t pos = xref_offset + 5;
    size_t first = std::strtoul(pdf.c_str() + pos, nullptr, 10);
    size_t count = std::strtoul(pdf.c_str() + pdf.find(' ', pos) + 1, nullptr, 10);
    if (first != 0) return false;
    pos = pdf.find('\n', pos) + 1;

    for (size_t id = 0; id < count; ++id, pos += 20) {
        if (id == 0) continue; // free entry
        size_t offset = std::strtoul(pdf.c_str() + pos, nullptr, 10);
        std::string expected = std::to_string(id) + " 0 obj";
        if (pdf.compare(offset, expected.size(), expected) != 0) return false;
    }
    return true;
}

// Checks each stream's /Length matches the bytes between "stream\n" and "\nendstream".
bool streamLengthsMatch(const std::string& pdf) {
    size_t pos = 0;
    int streams = 0;
    while ((pos = pdf.find("/Length ", pos)) != std::string::npos) {
        size_t length = std::strtoul(pdf.c_str() + pos + 8, nullptr, 10);
        size_t start = pdf.find("stream\n", pos) + 7;
        size_t end = pdf.find("\nendstream", start);
        if (end - start != length) return false;
        pos = end;
        ++streams;
    }
    return streams > 0;
}

void testEscape() {
    CHECK(PdfWriter::escape("a(b)c\\d") == "a\\(b\\)c\\\\d");
    CHECK(PdfWriter::escape("$@B%8&WM#*") == "$@B%8&WM#*");

    bool threw = false;
    try {
        PdfWriter::escape("\xE2\x96\x88"); // UTF-8 full block
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void testStructure() {
    PdfWriter pdf;
    pdf.beginPage(612, 792);
    pdf.text(54, 700, 8, "Page (1)");
    pdf.textLines(54, 680, 6, 6, {"@%#*", "+=-:", ". \\"});
    pdf.line(10, 10, 20, 20);
    pdf.line(1, 2, 3, 4, 0.5, 0, true);
    pdf.beginPage(842, 595);
    pdf.rect(10, 10, 100, 50, 1, 0.5);

    std::string out = pdf.finish();
    CHECK(out.compare(0, 9, "%PDF-1.4\n") == 0);
    CHECK(out.size() >= 6 && out.compare(out.size() - 6, 6, "%%EOF\n") == 0);
    CHECK(contains(out, "/Count 2"));
    CHECK(contains(out, "/Kids [ 4 0 R 6 0 R ]"));
    CHECK(contains(out, "/MediaBox [0 0 612 792]"));
    CHECK(contains(out, "/MediaBox [0 0 842 595]"));
    CHECK(contains(out, "/BaseFont /Courier"));
    CHECK(contains(out, "(Page \\(1\\)) Tj"));
    CHECK(contains(out, "6 TL 54 680 Td\n(@%#*) Tj\nT* (+=-:) Tj\nT* (. \\\\) Tj\n"));
    CHECK(contains(out, "0.5 G 1 w 10 10 100 50 re S"));
    CHECK(contains(out, "0 G 0.5 w 10 10 m 20 20 l S"));
    CHECK(contains(out, "0 G 0.5 w [2 2] 0 d 1 2 m 3 4 l S"));
    CHECK(xrefIsValid(out));
    CHECK(streamLengthsMatch(out));
    CHECK(pdf.pageCount() == 2);
}

void testDecimalFormatting() {
    PdfWriter pdf;
    pdf.beginPage(595.28, 841.89);
    pdf.text(3.6, 0.25, 6, "x");
    std::string out = pdf.finish();
    CHECK(contains(out, "/MediaBox [0 0 595.28 841.89]"));
    CHECK(contains(out, "3.6 0.25 Td"));
}

void testDrawingWithoutPageThrows() {
    PdfWriter pdf;
    bool threw = false;
    try {
        pdf.text(0, 0, 6, "x");
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
}

void testColorOperations() {
    PdfWriter pdf;
    pdf.beginPage(100, 100);
    const PdfWriter::Color red{255, 0, 0}, blue{0, 0, 255};
    pdf.coloredTextLines(10, 90, 6, 6, {{{"ab", red}, {"c", red}, {"(d)", blue}}, {{"e", blue}, {"f", red}}});
    pdf.fillRect(1, 2, 3, 4, {255, 128, 0});
    pdf.text(10, 10, 8, "after");
    std::string out = pdf.finish();
    // Color emitted only on change, wrapped in q/Q so later text is black again.
    CHECK(contains(out, "q BT /F1 6 Tf 6 TL 10 90 Td\n"
                        "1 0 0 rg (ab) Tj (c) Tj 0 0 1 rg (\\(d\\)) Tj \n"
                        "T* (e) Tj 1 0 0 rg (f) Tj \n"
                        "ET Q\n"));
    CHECK(contains(out, "q 1 0.5 0 rg 1 2 3 4 re f Q\n"));
    CHECK(contains(out, "ET Q\nq 1 0.5 0 rg"));
    CHECK(contains(out, "Q\nBT /F1 8 Tf 10 10 Td (after) Tj ET"));
    CHECK(xrefIsValid(out) && streamLengthsMatch(out));
}

// Draws the same content into a writer; used to compare compressed and plain output.
void drawSample(PdfWriter& pdf) {
    pdf.beginPage(612, 792);
    std::vector<std::string> lines(100, "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ");
    pdf.textLines(54, 700, 6, 6, lines);
    pdf.coloredTextLines(54, 100, 6, 6, {{{"ab", {255, 0, 0}}, {"cd", {0, 0, 255}}}});
    pdf.beginPage(612, 792);
    pdf.text(10, 10, 8, "second");
}

// Returns the raw bytes of each stream, in order.
std::vector<std::string> streams(const std::string& pdf) {
    std::vector<std::string> out;
    size_t pos = 0;
    while ((pos = pdf.find("/Length ", pos)) != std::string::npos) {
        size_t length = std::strtoul(pdf.c_str() + pos + 8, nullptr, 10);
        size_t start = pdf.find("stream\n", pos) + 7;
        out.push_back(pdf.substr(start, length));
        pos = start + length;
    }
    return out;
}

std::string inflate(const std::string& data, size_t expected) {
    std::string out(expected + 16, '\0');
    mz_ulong size = static_cast<mz_ulong>(out.size());
    if (mz_uncompress(reinterpret_cast<unsigned char*>(&out[0]), &size,
                      reinterpret_cast<const unsigned char*>(data.data()), static_cast<mz_ulong>(data.size())) != MZ_OK) {
        return "<inflate failed>";
    }
    out.resize(size);
    return out;
}

void testCompression() {
    PdfWriter plain, packed(/*compress=*/true);
    drawSample(plain);
    drawSample(packed);
    std::string a = plain.finish(), b = packed.finish();

    CHECK(!contains(a, "/FlateDecode"));
    CHECK(countOf(b, "/Filter /FlateDecode") == 2u);
    CHECK(b.size() < a.size() / 3); // repetitive art compresses well
    CHECK(xrefIsValid(b));

    auto raw = streams(a), zipped = streams(b);
    CHECK(raw.size() == 2u && zipped.size() == 2u);
    for (size_t i = 0; i < raw.size() && i < zipped.size(); ++i) {
        CHECK(inflate(zipped[i], raw[i].size()) == raw[i]); // round trip
        CHECK(zipped[i].size() < raw[i].size() || raw[i].size() < 64);
    }
}

void testEmptyDocumentIsValid() {
    std::string out = PdfWriter().finish();
    CHECK(contains(out, "/Count 0"));
    CHECK(xrefIsValid(out));
}

} // namespace

int main() {
    testEscape();
    testStructure();
    testDecimalFormatting();
    testDrawingWithoutPageThrows();
    testColorOperations();
    testCompression();
    testEmptyDocumentIsValid();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

// Unit tests for PdfWriter: document structure, xref offsets and escaping.

#include "PdfWriter.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
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
    testEmptyDocumentIsValid();

    if (g_failures) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}

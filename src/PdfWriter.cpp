#include "PdfWriter.h"

#include <cstdio>

#include "miniz.h"
#include <fstream>
#include <stdexcept>

namespace {

// Compact number formatting: at most 2 decimals, no trailing zeros ("12", "3.6", "0.25").
std::string num(double value) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2f", value);
    std::string s = buf;
    s.erase(s.find_last_not_of('0') + 1);
    if (s.back() == '.') s.pop_back();
    if (s == "-0") s = "0";
    return s;
}

// zlib-format (RFC 1950) deflate, as required by /FlateDecode.
std::string deflate(const std::string& data) {
    mz_ulong size = mz_compressBound(static_cast<mz_ulong>(data.size()));
    std::string out(size, '\0');
    int status = mz_compress2(reinterpret_cast<unsigned char*>(&out[0]), &size,
                              reinterpret_cast<const unsigned char*>(data.data()),
                              static_cast<mz_ulong>(data.size()), MZ_DEFAULT_LEVEL);
    if (status != MZ_OK) throw std::runtime_error("PDF stream compression failed.");
    out.resize(size);
    return out;
}

} // namespace

std::string PdfWriter::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        if (ch < 0x20 || ch > 0x7E) {
            throw std::invalid_argument("PDF text must be printable ASCII.");
        }
        if (ch == '(' || ch == ')' || ch == '\\') out += '\\';
        out += static_cast<char>(ch);
    }
    return out;
}

void PdfWriter::beginPage(double width, double height) {
    m_pages.push_back({width, height, {}});
}

std::string& PdfWriter::content() {
    if (m_pages.empty()) throw std::logic_error("PdfWriter: beginPage() must be called before drawing.");
    return m_pages.back().content;
}

void PdfWriter::text(double x, double y, double font_size, const std::string& s) {
    content() += "BT /F1 " + num(font_size) + " Tf " + num(x) + " " + num(y) + " Td (" + escape(s) + ") Tj ET\n";
}

void PdfWriter::textLines(double x, double y, double font_size, double leading, const std::vector<std::string>& lines) {
    if (lines.empty()) return;
    std::string& c = content();
    c += "BT /F1 " + num(font_size) + " Tf " + num(leading) + " TL " + num(x) + " " + num(y) + " Td\n";
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) c += "T* ";
        c += "(" + escape(lines[i]) + ") Tj\n";
    }
    c += "ET\n";
}

namespace {
std::string fillColor(const PdfWriter::Color& c) {
    return num(c.r / 255.0) + " " + num(c.g / 255.0) + " " + num(c.b / 255.0) + " rg";
}
} // namespace

void PdfWriter::coloredTextLines(double x, double y, double font_size, double leading,
                                 const std::vector<std::vector<TextRun>>& lines) {
    if (lines.empty()) return;
    std::string& c = content();
    c += "q BT /F1 " + num(font_size) + " Tf " + num(leading) + " TL " + num(x) + " " + num(y) + " Td\n";
    bool have_color = false;
    Color current;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) c += "T* ";
        for (const TextRun& run : lines[i]) {
            if (!have_color || run.color != current) {
                c += fillColor(run.color) + " ";
                current = run.color;
                have_color = true;
            }
            c += "(" + escape(run.text) + ") Tj ";
        }
        c += "\n";
    }
    c += "ET Q\n";
}

void PdfWriter::fillRect(double x, double y, double w, double h, Color color) {
    content() += "q " + fillColor(color) + " " + num(x) + " " + num(y) + " " + num(w) + " " + num(h) + " re f Q\n";
}

void PdfWriter::line(double x1, double y1, double x2, double y2, double width, double gray, bool dashed) {
    content() += "q " + num(gray) + " G " + num(width) + " w " + (dashed ? "[2 2] 0 d " : "") + num(x1) + " " + num(y1) + " m " + num(x2) + " " +
                 num(y2) + " l S Q\n";
}

void PdfWriter::rect(double x, double y, double w, double h, double width, double gray) {
    content() += "q " + num(gray) + " G " + num(width) + " w " + num(x) + " " + num(y) + " " + num(w) + " " + num(h) +
                 " re S Q\n";
}

std::string PdfWriter::finish() const {
    // Object numbers: 1 catalog, 2 pages tree, 3 font, then (page, content) pairs.
    const int page_count = pageCount();
    const int object_count = 3 + 2 * page_count;
    std::vector<size_t> offsets(object_count + 1, 0);

    std::string out = "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n"; // binary marker comment
    auto beginObject = [&](int id) {
        offsets[id] = out.size();
        out += std::to_string(id) + " 0 obj\n";
    };

    beginObject(1);
    out += "<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";

    beginObject(2);
    out += "<< /Type /Pages /Kids [";
    for (int i = 0; i < page_count; ++i) out += " " + std::to_string(4 + 2 * i) + " 0 R";
    out += " ] /Count " + std::to_string(page_count) + " >>\nendobj\n";

    beginObject(3);
    out += "<< /Type /Font /Subtype /Type1 /BaseFont /Courier /Encoding /WinAnsiEncoding >>\nendobj\n";

    for (int i = 0; i < page_count; ++i) {
        const PageData& page = m_pages[i];
        const int page_id = 4 + 2 * i;
        const int content_id = page_id + 1;

        beginObject(page_id);
        out += "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + num(page.width) + " " + num(page.height) +
               "] /Resources << /Font << /F1 3 0 R >> >> /Contents " + std::to_string(content_id) +
               " 0 R >>\nendobj\n";

        beginObject(content_id);
        if (m_compress) {
            std::string packed = deflate(page.content);
            out += "<< /Length " + std::to_string(packed.size()) + " /Filter /FlateDecode >>\nstream\n";
            out += packed;
        } else {
            out += "<< /Length " + std::to_string(page.content.size()) + " >>\nstream\n";
            out += page.content;
        }
        out += "\nendstream\nendobj\n";
    }

    const size_t xref_offset = out.size();
    out += "xref\n0 " + std::to_string(object_count + 1) + "\n";
    out += "0000000000 65535 f \n";
    for (int id = 1; id <= object_count; ++id) {
        char entry[21];
        std::snprintf(entry, sizeof entry, "%010zu 00000 n \n", offsets[id]);
        out += entry;
    }
    out += "trailer\n<< /Size " + std::to_string(object_count + 1) + " /Root 1 0 R >>\n";
    out += "startxref\n" + std::to_string(xref_offset) + "\n%%EOF\n";
    return out;
}

bool PdfWriter::save(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file << finish();
    return static_cast<bool>(file);
}

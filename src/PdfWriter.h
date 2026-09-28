#ifndef PDF_WRITER_H
#define PDF_WRITER_H

#include <string>
#include <vector>

/**
 * @brief Minimal PDF 1.4 writer: pages with Courier text and lines, no dependencies.
 *
 * Coordinates are PDF points with the origin at the bottom-left of the page.
 * Text must be ASCII (the standard Courier font uses a single-byte encoding).
 * Streams are uncompressed.
 */
class PdfWriter {
public:
    /** Starts a new page; later drawing calls go to it. */
    void beginPage(double width, double height);

    /** Draws one line of text with its baseline at (x, y). */
    void text(double x, double y, double font_size, const std::string& s);

    /** Draws lines top to bottom; the first baseline is at (x, y), each next one `leading` lower. */
    void textLines(double x, double y, double font_size, double leading, const std::vector<std::string>& lines);

    /** Strokes a straight line. gray: 0 = black, 1 = white. dashed: 2 pt on, 2 pt off. */
    void line(double x1, double y1, double x2, double y2, double width = 0.5, double gray = 0.0,
              bool dashed = false);

    /** Strokes a rectangle outline with its bottom-left corner at (x, y). */
    void rect(double x, double y, double w, double h, double width = 0.5, double gray = 0.0);

    int pageCount() const { return static_cast<int>(m_pages.size()); }

    /** Serializes the document. */
    std::string finish() const;

    /** Writes finish() to a file; returns false if the file cannot be written. */
    bool save(const std::string& path) const;

    /**
     * @brief Escapes a string for a PDF literal: backslash-escapes ( ) and \\.
     * @throws std::invalid_argument on non-ASCII or control characters.
     */
    static std::string escape(const std::string& s);

private:
    struct PageData {
        double width;
        double height;
        std::string content;
    };
    std::vector<PageData> m_pages;

    std::string& content();
};

#endif // PDF_WRITER_H

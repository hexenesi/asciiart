#ifndef ANSI_RENDERER_H
#define ANSI_RENDERER_H

#include "ColorRuns.h"
#include "GridRenderer.h"

/**
 * @brief Terminal output with 24-bit ANSI color, one escape code per color run.
 *
 * Foreground: glyphs in the cell color (ESC[38;2;R;G;Bm).
 * Background: cells filled with the color (ESC[48;2;R;G;Bm) and glyphs in black or white,
 * whichever contrasts more. In fg mode, runs of spaces keep the active color (a space shows
 * no color). Every line ends with a reset (ESC[0m) if it used color, so
 * colors never bleed into the next line or the prompt. Cells without color use the
 * terminal's default colors.
 */
class AnsiRenderer : public GridRenderer {
public:
    explicit AnsiRenderer(ColorStyle style = ColorStyle::Foreground) : m_style(style) {}

    void render(const AsciiGrid& grid, std::ostream& out) const override;

private:
    ColorStyle m_style;
};

#endif // ANSI_RENDERER_H

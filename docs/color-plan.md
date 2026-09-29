# Plan: Color output (terminal, HTML, PDF, SVG)

## Goal

Keep the source image's color through the pipeline and render it in every output format: ANSI terminal, HTML, PDF posters and SVG. Grayscale output stays the default and byte-identical.

## Current state

- `ImageConverter::loadAndGrayscale()` overwrites R, G and B with the luminance, so color is lost at load time.
- `AsciiGrid` holds glyphs only.
- `PdfWriter` draws black text and gray lines only; streams are uncompressed (about 1 byte per character).
- Only the PDF renderer exists; text output is built in `main.cpp`.

## Decisions

| Topic | Decision |
|---|---|
| Color style | Both: `fg` (colored glyphs, default) and `bg` (colored cell backgrounds, mosaic look) |
| Light colors on white paper | In `fg` mode for PDF/HTML/SVG, darken colors whose luminance is above a threshold so they stay visible; the terminal keeps true colors |
| Quantization | On by default: 32 levels per channel, adjustable with `--colors` |
| Compression | Vendor `miniz` (single file, MIT) in `third_party/`; keep the build free of system dependencies |
| Option names | `--color`, `--color-style fg\|bg`, `--colors <levels>`, `--html <file>`, `--svg <file>` |

## Design

### 1. Color data model

- `Pixel` keeps the original R, G, B and alpha, plus a separate luminance used for glyph selection.
- Brightness/contrast apply to the luminance only; the color is left unchanged (simplest, predictable).
- Resize:
  - Nearest neighbour keeps colors as they are.
  - Optional later improvement: box-filter averaging, for smoother color in downscaled output.
- `AsciiGrid` becomes a grid of cells: `struct AsciiCell { std::string glyph; Rgb color; }`.
  - `convert()` still joins glyphs only, so plain text output is unchanged.
  - Transparent cells keep the blank glyph and are marked as having no color.

### 2. Quantization and runs

- `quantize(Rgb, levels)` rounds each channel to `levels` steps.
  - It is applied once when the grid is built, so every renderer sees the same colors.
- A helper splits a row into runs of equal color. It is shared by all renderers, so each draws one run per color change instead of one per cell.

### 3. Renderer interface

With five outputs (text, ANSI, HTML, PDF, SVG), introduce a small interface:

```cpp
class GridRenderer {
public:
    virtual ~GridRenderer() = default;
    virtual void render(const AsciiGrid& grid, std::ostream& out) = 0;
};
```

- The text, ANSI, HTML and SVG renderers implement it.
- The PDF poster keeps its layout-aware entry point, `renderPoster`, which also takes the `PageLayout`.
- The text output moves out of `main.cpp` into a `TextRenderer`.

### 4. Terminal (ANSI)

- `--color` without a file output prints 24-bit color codes: `\x1b[38;2;R;G;Bm` for `fg`, `\x1b[48;2;R;G;Bm` for `bg`.
- Codes are emitted per run, and each line ends with a reset (`\x1b[0m`).
- `--output file.txt` never contains escape codes, unless a later `--ansi` option is added.

### 5. HTML

- A single self-contained file:
  - A `<pre>` element with a monospace font and `line-height: 1`.
  - One `<span class="cN">` per run.
  - A `<style>` block with one class per quantized color (`.c12{color:#aabbcc}`, or `background-color` in `bg` mode).
- Supports every charset, including `blocks` (UTF-8, HTML-escaped).
- Character aspect depends on the browser font (about 0.6 em advance); document it rather than try to fix it.
- Single view only, with no page tiling.

### 6. PDF

- `PdfWriter` gains:
  - A fill color (`r g b rg`).
  - Filled rectangles (`re f`).
  - Optional Flate compression of content streams (`/Filter /FlateDecode`, via miniz).
- The poster renderer:
  - `fg`: in each line, set the color before each run and let text advance naturally (`R G B rg (run) Tj`).
  - `bg`: filled rectangles for each run of equal color per row, then the glyphs in black (or white on dark cells).
- Labels, marks and the overview page stay black.
- Light-color darkening in `fg` mode (see Decisions).
- Size estimate without compression: about 10–15 bytes per character when colors change often. Quantization plus Flate should bring that close to the grayscale size.

### 7. SVG

- One `<text>` per row, with a `<tspan fill="#...">` per run.
  - Every run gets an explicit `x` (column × advance), so columns stay aligned regardless of font.
  - `font-family` is Courier / monospace.
- `bg` mode: one `<rect>` per run, before the text.
- The `viewBox` is sized in character cells, so the image scales cleanly.
- Single image, no tiling.

### 8. CLI

- New options:
  - `--color`.
  - `--color-style fg|bg` (default `fg`).
  - `--colors <levels>` (default 32; 256 = no quantization).
  - `--html <file>`, `--svg <file>`.
- `--pdf`, `--html` and `--svg` can be combined in one run. `--output` stays plain text.
- Errors:
  - `--color-style` or `--colors` without `--color`.
  - `--colors` outside 2–256.
  - The `blocks` charset with `--pdf` (unchanged).

### 9. Tests

- **Data model:**
  - Colors survive load and resize.
  - Grayscale output is byte-identical with `--color` off.
  - Transparent cells have no color.
- **Quantization and runs:**
  - Level rounding.
  - Run splitting (all same, all different, mixed).
- **ANSI:** escape sequences per run, reset at line end, no codes in `--output` files.
- **HTML:**
  - Well-formed structure.
  - One class per color.
  - Escaping of `<`, `>`, `&`.
  - `blocks` glyphs.
- **PDF:**
  - `rg` operators per run.
  - Rectangles in `bg` mode.
  - Compressed streams decode to the original content (round trip with miniz).
  - Check with `pdfinfo`/`mutool`.
- **SVG:** one `<text>` per row, `x` positions match columns, colors per run.
- **CLI:** option errors, combined outputs.

## Commit sequence

1. Color data model (`Pixel`, `AsciiCell`), grayscale output unchanged.
2. Quantization and run splitting.
3. `GridRenderer` interface; move text output into `TextRenderer`.
4. ANSI terminal output (`--color`, `--color-style`, `--colors`).
5. HTML output (`--html`).
6. PDF color: fill color, runs, `bg` rectangles, light-color darkening.
7. PDF compression with miniz.
8. SVG output (`--svg`).
9. README.

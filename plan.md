# Plan: Printable tiled output (PDF)

## Goal

Convert large images at 1 px : 1 char (aspect-corrected) or at an adjustable scale, and split the result into numbered pages that print on Letter or A4. Each page shows its neighbours' page numbers so the sheets can be assembled into a poster.

## Decisions

| Topic | Decision |
|---|---|
| Output format | PDF, hand-written, using the built-in Courier font (no new dependencies) |
| Neighbour labels | Neighbour numbers in the margins, plus an overview page with the grid map |
| Sizing | `--scale` (chars per pixel) and `--pages-wide` (fit to N pages across), not combinable |
| Default font size | 6 pt (about 150 × 115 chars on a Letter page) |
| Default overlap | 0 chars |
| PDF selection | Separate `--pdf file.pdf` flag; can be combined with `--output` for a text copy |

## Size warning

At 1 px : 1 char, a 4000×3000 photo at 6 pt on Letter is about 28 × 22 ≈ 600 pages. Safety features: page-count summary, `--dry-run`, and `--max-pages` (default 50).

## Design

### 1. Character aspect from font metrics

- Courier advance width = 0.6 × font size; line height = font size × leading (default 1.0).
- Character aspect = line height ÷ char width = 1.0 / 0.6 ≈ 1.67 for PDF.
- Replace the fixed `CHARACTER_ASPECT_RATIO` (2.0) in `ImageConverter` with `setCharAspect(double)`.
  - Console output keeps 2.0.
  - PDF output uses the font-derived value.
- This also covers the README improvement "Factor de corrección de aspecto configurable".

### 2. Scale mode

- `--scale s`: characters per source pixel.
  - Columns = source width × s.
  - Rows = source height × s ÷ char aspect.
- `--scale 1` = 1 px : 1 char horizontally, height corrected for aspect.
- `--pages-wide n`: compute the scale so the art is exactly n pages wide.
- `--scale` and `--pages-wide` are mutually exclusive, and neither can be combined with `--width`/`--height`.
- The 100-column default cap does not apply in scale mode.

### 3. Grid output from the converter

- Add `convertToGrid()`, returning rows of glyphs.
- `convert()` joins the grid into a string, so console output stays byte-identical (covered by existing tests).
- The paginator needs rows and columns to slice pages.

### 4. `PageLayout` (pure math, unit-testable)

Inputs:

- Paper `--paper letter|a4` (612×792 pt / 595×842 pt).
- `--orientation portrait|landscape|auto` (auto = fewer pages).
- Margin (default 0.5 in) plus a label band for the neighbour numbers.
- `--font-size`, leading.
- `--overlap k`: characters repeated on neighbouring pages to help alignment.

Outputs:

- Characters per page (columns × rows).
- Grid size: `ceil(cols / cols_per_page)` × `ceil(rows / rows_per_page)`.
- For each page: number (row-major, from 1), grid position, neighbours (up/down/left/right or none), and the slice of the art it contains.

### 5. `PdfWriter` (no dependencies)

- Minimal PDF 1.4: catalog, pages tree, Type1 Courier font (standard font, not embedded), one content stream per page, exact xref byte offsets.
- Escape `(`, `)` and `\` in text strings (the `detailed` charset contains all three).
- ASCII only: `--charset blocks` with PDF output is rejected with a clear error.
- Uncompressed streams (large but valid). Compression can come later.

### 6. Page content

- **Overview page (first):** map of the page grid with every number, image name, scale, paper, total page count, and assembly instructions.
- **Art pages:**
  - Page number and grid position in a corner (e.g. "7 · row 2, col 3").
  - Neighbour numbers centred in the matching margin (↑ 3, ↓ 11, ← 6, → 8).
  - Light cut marks at the corners of the art area.
- The art area has the same position on every sheet, so neighbouring pages line up.

### 7. CLI

- PDF output is selected by `--pdf file.pdf`. `--output` still writes plain text, and both can be used together.
- New options: `--paper`, `--orientation`, `--font-size`, `--scale`, `--pages-wide`, `--overlap`, `--max-pages`, `--dry-run`.
- A summary is always printed to stderr, e.g. `2400×1440 chars → 17×13 = 221 pages (A4 portrait, 6 pt)`.
- Errors:
  - Conflicting sizing options.
  - `blocks` charset with PDF.
  - Page count over `--max-pages`.
  - Page options (`--paper`, `--orientation`, `--font-size`, `--pages-wide`, `--overlap`, `--max-pages`) without `--pdf`.

### 8. Tests

- `PageLayout`:
  - Exact-fit and partial pages.
  - Neighbours at corners, edges and middle.
  - Overlap math.
  - Auto orientation.
- Scale: 1:1 sizing, and `--pages-wide` fits exactly n pages.
- PDF:
  - Starts with `%PDF`.
  - Expected page count.
  - Valid xref offsets.
  - Escaping.
- CLI: conflicts, `blocks` + PDF, `--max-pages`, `--dry-run`.
- Manual: print a small 2×2 grid and check alignment.

## Commit sequence

1. Configurable character aspect.
2. Grid output (`convertToGrid`).
3. Scale mode and `--pages-wide`.
4. `PageLayout` + tests.
5. `PdfWriter` + tests.
6. Page labels and overview page.
7. CLI options, `--dry-run`, `--max-pages`.
8. README.

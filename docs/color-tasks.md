# Tasks: Color output

See [color-plan.md](color-plan.md) for the design. One commit per numbered group.

## 0. Decisions

- [x] 0.1 Color style: `fg` and `bg` both, `fg` default
- [x] 0.2 Darken light colors in `fg` mode on white outputs (PDF, HTML, SVG) above a luminance threshold; terminal keeps true colors
- [x] 0.3 Quantization default: 32 levels per channel
- [x] 0.4 Compression: vendor miniz
- [x] 0.5 Option names: `--color`, `--color-style`, `--colors`, `--html`, `--svg`

## 1. Color data model

- [x] 1.1 `Pixel` keeps original RGB + alpha, plus a separate luminance
- [x] 1.2 Brightness/contrast adjust luminance only
- [x] 1.3 `AsciiCell { glyph, color, has_color }`; `AsciiGrid` = rows of cells
- [x] 1.4 `convert()` joins glyphs only; plain text output byte-identical
- [x] 1.5 Update `PosterRenderer` and tests to the new grid type
- [x] 1.6 Tests: color survives load + resize; transparent cells have no color; grayscale output unchanged

## 2. Quantization and runs

- [x] 2.1 `quantize(Rgb, levels)` applied when the grid is built (only with color on)
- [x] 2.2 Run splitter: row → list of (color, glyph string) runs
- [x] 2.3 Tests: rounding at 2, 32 and 256 levels; runs for uniform, alternating and mixed rows

## 3. Renderer interface

- [ ] 3.1 `GridRenderer` interface (`render(grid, ostream)`)
- [ ] 3.2 `TextRenderer`: move plain text output out of `main.cpp`
- [ ] 3.3 Tests: `TextRenderer` output equals `convert()`

## 4. ANSI terminal

- [ ] 4.1 `AnsiRenderer` for `fg` (`38;2`) and `bg` (`48;2`), one code per run, reset at line end
- [ ] 4.2 CLI: `--color`, `--color-style`, `--colors`; stdout uses ANSI only with `--color`
- [ ] 4.3 `--output` files never contain escape codes
- [ ] 4.4 Errors: `--color-style`/`--colors` without `--color`; `--colors` outside 2–256
- [ ] 4.5 Tests: escape sequences and resets; CLI errors

## 5. HTML

- [ ] 5.1 `HtmlRenderer`: `<pre>`, one class per quantized color, spans per run
- [ ] 5.2 HTML escaping (`<`, `>`, `&`), UTF-8 for `blocks`
- [ ] 5.3 `bg` mode via `background-color`
- [ ] 5.4 CLI: `--html <file>`
- [ ] 5.5 Tests: structure, class count, escaping, `blocks`; CLI writes file

## 6. PDF color

- [ ] 6.1 `PdfWriter`: fill color (`rg`) and filled rectangles (`re f`)
- [ ] 6.2 `fg`: color set per run within each text line
- [ ] 6.3 `bg`: merged run rectangles behind glyphs; black or white glyphs by background luminance
- [ ] 6.4 Darken light colors in `fg` mode
- [ ] 6.5 Labels, marks and overview stay black
- [ ] 6.6 Tests: `rg` per run, rectangles in `bg`, darkening; `pdfinfo`/`mutool` check; visual check of a rendered page

## 7. PDF compression

- [ ] 7.1 Vendor miniz in `third_party/miniz/` (license file included)
- [ ] 7.2 `PdfWriter`: `/Filter /FlateDecode` on content streams; keep uncompressed option for tests/debugging
- [ ] 7.3 Tests: stream round trip through miniz; `/Length` matches compressed bytes; readers open the file
- [ ] 7.4 Measure size before/after on a large color poster

## 8. SVG

- [ ] 8.1 `SvgRenderer`: `<text>` per row, `<tspan>` per run with explicit `x`
- [ ] 8.2 `bg` mode: `<rect>` per run
- [ ] 8.3 `viewBox` in cell units; XML escaping
- [ ] 8.4 CLI: `--svg <file>`
- [ ] 8.5 Tests: structure, x positions, colors; CLI writes file

## 9. Docs and verification

- [ ] 9.1 README: color options, styles, formats, file size notes
- [ ] 9.2 Visual checks: terminal, browser (HTML, SVG), PDF viewer, one printed color page

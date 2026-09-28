# Tasks: Printable tiled output (PDF)

See [plan.md](plan.md) for the design. One commit per numbered group.

## 0. Decisions

- [x] Default font size: 6 pt
- [x] Default overlap: 0
- [x] PDF selection: separate `--pdf file.pdf` flag (combinable with `--output`)

## 1. Configurable character aspect

- [ ] Add `setCharAspect(double)` to `ImageConverter` (reject values <= 0)
- [ ] Replace `CHARACTER_ASPECT_RATIO` constant with the member (default 2.0)
- [ ] Tests: width-only and height-only sizing with a non-default aspect
- [ ] Verify console output unchanged (existing tests pass)

## 2. Grid output

- [ ] Add `convertToGrid()` returning `std::vector<std::vector<std::string>>` (rows of glyphs)
- [ ] Reimplement `convert()` on top of `convertToGrid()`
- [ ] Tests: grid dimensions match string output; console output byte-identical

## 3. Scale mode

- [ ] Add `setScale(double)`: cols = src_w × s, rows = src_h × s ÷ aspect
- [ ] Skip the 100-column cap in scale mode
- [ ] `--pages-wide n`: compute scale from page columns (needs `PageLayout`; wire in group 7)
- [ ] Tests: `--scale 1` gives src_w columns; `--scale 0.5` halves; rounding never gives 0

## 4. PageLayout

- [ ] Paper sizes: Letter 612×792 pt, A4 595×842 pt
- [ ] Orientation: portrait, landscape, auto (fewer pages)
- [ ] Printable area = paper − margins − label band
- [ ] Chars per page from font size (0.6 em advance) and leading
- [ ] Grid size with ceil division; overlap support
- [ ] Page records: number (row-major from 1), row/col, neighbours, art slice (col/row ranges)
- [ ] Tests: exact fit, partial last row/col, 1×1 grid, neighbours at corners/edges/middle, overlap, auto orientation

## 5. PdfWriter

- [ ] Header, catalog, pages tree, Courier Type1 font object
- [ ] One content stream per page (BT/Tf/TL/Td/Tj/T*)
- [ ] String escaping for `(`, `)`, `\`
- [ ] Line drawing for cut marks
- [ ] xref table with exact byte offsets, trailer
- [ ] Tests: `%PDF` header, `/Count` matches pages, every xref offset points at `N 0 obj`, escaping

## 6. Page content

- [ ] Fixed art origin on every page so sheets align
- [ ] Page number + grid position in a corner
- [ ] Neighbour numbers centred in top/bottom/left/right margins (omitted at edges)
- [ ] Cut marks at art-area corners
- [ ] Overview page: grid map with numbers, image name, scale, paper, page count, assembly note
- [ ] Tests: labels present for middle page; absent neighbours at edges; overview is page 1 of the PDF

## 7. CLI

- [ ] `--pdf <file>` option; `--output` keeps writing text and can be combined with it
- [ ] Options: `--paper`, `--orientation`, `--font-size`, `--scale`, `--pages-wide`, `--overlap`, `--max-pages` (default 50), `--dry-run`
- [ ] Use font-derived aspect for PDF output
- [ ] Summary line on stderr (chars, grid, pages, paper, font size)
- [ ] Errors: `--scale` + `--pages-wide`; either + `--width`/`--height`; `blocks` + PDF; over `--max-pages`; page options without `--pdf`
- [ ] Update `--help`
- [ ] CLI tests in CTest for each error and for `--dry-run`

## 8. Docs and verification

- [ ] README: new options, printing workflow, size warning
- [ ] Manual print test: 2×2 grid on Letter and A4, check alignment and labels
- [ ] Large-image run (e.g. 4000 px) with `--dry-run` and a real PDF; check file size and time

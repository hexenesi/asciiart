# Tasks: Printable tiled output (PDF)

See [plan.md](plan.md) for the design. One commit per numbered group.

## 0. Decisions

- [x] Default font size: 6 pt
- [x] Default overlap: 0
- [x] PDF selection: separate `--pdf file.pdf` flag (combinable with `--output`)

## 1. Configurable character aspect

- [x] Add `setCharAspect(double)` to `ImageConverter` (reject values <= 0)
- [x] Replace `CHARACTER_ASPECT_RATIO` constant with the member (default 2.0)
- [x] Tests: width-only and height-only sizing with a non-default aspect
- [x] Verify console output unchanged (existing tests pass)

## 2. Grid output

- [x] Add `convertToGrid()` returning `std::vector<std::vector<std::string>>` (rows of glyphs)
- [x] Reimplement `convert()` on top of `convertToGrid()`
- [x] Tests: grid dimensions match string output; console output byte-identical

## 3. Scale mode

- [x] Add `setScale(double)`: cols = src_w × s, rows = src_h × s ÷ aspect
- [x] Skip the 100-column cap in scale mode
- [x] `--pages-wide n`: compute scale from page columns (needs `PageLayout`; wire in group 7)
- [x] Tests: `--scale 1` gives src_w columns; `--scale 0.5` halves; rounding never gives 0

## 4. PageLayout

- [x] Paper sizes: Letter 612×792 pt, A4 595×842 pt
- [x] Orientation: portrait, landscape, auto (fewer pages)
- [x] Printable area = paper − margins − label band
- [x] Chars per page from font size (0.6 em advance) and leading
- [x] Grid size with ceil division; overlap support
- [x] Page records: number (row-major from 1), row/col, neighbours, art slice (col/row ranges)
- [x] Tests: exact fit, partial last row/col, 1×1 grid, neighbours at corners/edges/middle, overlap, auto orientation

## 5. PdfWriter

- [x] Header, catalog, pages tree, Courier Type1 font object
- [x] One content stream per page (BT/Tf/TL/Td/Tj/T*)
- [x] String escaping for `(`, `)`, `\`
- [x] Line drawing for cut marks
- [x] xref table with exact byte offsets, trailer
- [x] Tests: `%PDF` header, `/Count` matches pages, every xref offset points at `N 0 obj`, escaping

## 6. Page content

- [x] Fixed art origin on every page so sheets align
- [x] Page number + grid position in a corner
- [x] Neighbour numbers centred in top/bottom/left/right margins (omitted at edges)
- [x] Corner marks at art-area corners (now alignment marks, see group 9)
- [x] Overview page: grid map with numbers, image name, scale, paper, page count, assembly note
- [x] Tests: labels present for middle page; absent neighbours at edges; overview is page 1 of the PDF

## 7. CLI

- [x] `--pdf <file>` option; `--output` keeps writing text and can be combined with it
- [x] Options: `--paper`, `--orientation`, `--font-size`, `--scale`, `--pages-wide`, `--overlap`, `--max-pages` (default 50), `--dry-run`
- [x] Use font-derived aspect for PDF output
- [x] Summary line on stderr (chars, grid, pages, paper, font size)
- [x] Errors: `--scale` + `--pages-wide`; either + `--width`/`--height`; `blocks` + PDF; over `--max-pages`; page options without `--pdf`
- [x] Update `--help`
- [x] CLI tests in CTest for each error and for `--dry-run`

## 8. Docs and verification

- [x] README: new options, printing workflow, size warning
- [ ] Manual print test: 2×2 grid on Letter and A4, check alignment and labels (needs a printer; print at 100% / actual size)
- [x] Large-image run with `--dry-run` and a real PDF; check file size and time (962x1280 photo: 1:1 = 49 pages + overview, 0.26 s, 830 KB; A4 4 pages wide = 16 + overview, 0.11 s, 275 KB)

## 9. Alignment vs trim marks

- [x] Rename the existing corner marks to alignment marks (exact art edge)
- [x] Dashed trim marks on right/bottom joining edges, `glue_flap` outside the art (default 18 pt = 0.25 in)
- [x] `--glue-flap <pt>` option (0 = no flaps; max = label band + half the margin = 36 pt)
- [x] Validate page settings before converting the image
- [x] Overview assembly instructions for both modes
- [x] Tests: dashed lines, trim positions and counts per page, flap validation, CLI errors
- [x] README

### 0.10.x (Summer 2022)
- Refactor/Review PdfField hierarchy. Finish API review
- PdfInputDevice: should merge with PdfOutputDevice
- Add a PdfRectRaw that avoid normalization by default
- Fix PdfFontMetrics handling of symbol encoding
- Fix/complete handling of text extraction in rotated pages

### After 0.10
- Check PdfSignature to have correct /ByteRange and /Contents
values in the dictionary after signing with SignDocument
- Add PdfString(string&&) and PdfName(string&&) constructors that
either assume UTF-8 and/or checks for used codepoints
- Check PdfStreamedDocument working
- Check/Review doxygen doc

### After 1.0
- Added version of PdfFont::TryGetSubstituteFont for rendering
  (metrics of loaded font override metrics found on /FontFile)
  - Added method to retrieve shared_ptr from PdfObject, PdfFont (and
  maybe others) to possibly outlive document destruction
- PdfDifferenceEncoding: Rework Adobe Glyph List handling and moving it to private folder
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize mm::chars to not initialize memory, keeping std::string compatibility
- Add backtrace: https://github.com/boostorg/stacktrace
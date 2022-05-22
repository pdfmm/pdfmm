### 0.10.x (Summer 2022)
- Refactor/Review PdfField hierarchy. Finish API review
- Added version of PdfFont::TryGetSubstituteFont for rendering
  (metrics of loaded font override metrics found on /FontFile)
- Added text extraction API
- Added PdfMath functionalities (matrix transformations and so on)
- PdfInputDevice: should merge with PdfOutputDevice
- Use unique_ptr/shared_ptr where possibile (PdfField), use shared_ptr in PdfIndirectObjectList?
- FontManager could use unique_ptr or shared_ptr?
- Add a PdfRectRaw that avoid normalization by default

### After 0.10
- Check PdfStreamedDocument working
- Check/Review doxygen doc
- Add backtrace: https://github.com/boostorg/stacktrace
- PdfDifferenceEncoding: Rework Adobe Glyph List handling and moving it to private folder
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize mm::chars to not initialize memory, keeping std::string compatibility

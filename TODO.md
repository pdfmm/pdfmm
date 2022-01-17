### 0.9.20
- Re-enable unit tests with Catch (https://github.com/catchorg/Catch2)

### 1.0
- Refactor/Review PdfField hierarchy
- Added version of PdfFont::TryGetSubstituteFont for rendering
  (metrics of loaded font override metrics found on /FontFile)
- Added text extraction API
- Added PdfMath functionalities (matrix transformations and so on)
- PdfInputDevice: should merge with PdfOutputDevice
- Use unique_ptr/shared_ptr where possibile (PdfField), use shared_ptr in PdfIndirectObjectList?
- FontManager could use unique_ptr or shared_ptr?
- Remove PdfLocaleImbue
- PdfRect should avoid normalization
- PdfDifferenceEncoding: Move char code conversion data to private folder

### After 1.0
- Check/Review doxygen doc
- Add backtrace: https://github.com/boostorg/stacktrace
- Fix PdfIndirectObjectList::RenumberObjects
- Check working of linearization
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize mm::chars to not initialize memory, keeping std::string compatibility

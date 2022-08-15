### 0.10.x (Summer 2022)
- Refactor/Review PdfField hierarchy. Finish API review
- PdfImage: make it read only, remove SetImageData()
- Make PdfObjectStream not filter by default in PdfMemDocument

Ideas:
- PdfObject::TryFindKeyAsSafe()
- Evaluate rename PdfDocument::GetFontManager() -> GetFonts()

### After 0.10
- PdfElement: Optmize, keep dictionary/array pointer. Add GetObjectPtr()
- PdfPageCollection::CreatePage() with PdfPageSize or default inferred from doc
- Fix PdfFontMetrics handling of symbol encoding
- Fix/complete handling of text extraction in rotated pages
- Check PdfWriter should really update doc trailer when saving.
  Now the new trailer is written but the doc still has the old one
- Check PdfSignature to have correct /ByteRange and /Contents
values in the dictionary after signing with SignDocument
- Add PdfString(string&&) and PdfName(string&&) constructors that
either assume UTF-8 and/or checks for used codepoints
- Added PdfResources::GetResource with enum type
- Add a PdfRect-like class PdfCorners that avoid coordinates normalization
  by default
- Add PdfPage::GetRectRaw(), make GetRect() return normalized rotation rect
- Make PdfMemDocument do collect garbage by default
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

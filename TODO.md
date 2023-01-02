### After 0.10
- PdfFontManager: Add font load from file/better debugging
  in lookup (eg. to spot found font with different name)
- Review all page import functions to check correct working/improve
  the code
- PdfCanvas: Add CopyTo facilities, see PdfContents
- Review PdfNameTree/PdfFileSpec
- Check accessibility of PdfEncrypt.h classe, check AESV3 namings
- PdfFilterFactory: Move CreateFilterList somewhere else (PdfFilter), make it private
- Rename NameToColorSpaceRaw/ColorSpaceToNameRaw to something more consistent?
- Add PdfAnnotation::GetRectRaw, make GetRect() return normalized rotation rect
- Make PdfObjectStream not flate filter by default in PdfMemDocument?
- PdfElement: Optimize, keep dictionary/array pointer. Add GetObjectPtr()
- PdfPageCollection: Fix caching (it's very slow at the moment)
- PdfPageCollection: Add iteration on PdfPage*. See PdfAnnotationCollection
- PdfPageCollection::CreatePage() with PdfPageSize or default inferred from doc
- Fix PdfFontMetrics handling of symbol encoding
- Fix/complete handling of text extraction in rotated pages
- Check PdfWriter should really update doc trailer when saving.
  Now the new trailer is written but the doc still has the old one
- PdfImage: cache PdfColorSpace
- Check PdfSignature to have correct /ByteRange and /Contents
values in the dictionary after signing with SignDocument
- Evaluate move more utf8::next to utf8::unchecked::next
- Add PdfString(string&&) and PdfName(string&&) constructors that
either assume UTF-8 and/or checks for used codepoints
- Added PdfResources::GetResource with enum type
- Add a PdfRect-like class PdfCorners that avoid coordinates normalization
  by default
- Add PdfPage::GetRectRaw(), make GetRect() return normalized rotation rect
- Check PdfStreamedDocument working
- Check/Review doxygen doc
- Move IO System headers to common/
- Extract Matrix PdfMath.h -> Matrix.h, move it to common/
- PdfToggleButton: Add proper IsChecked/ExportValue handling
- Review PdfPage::SetICCProfile()
- Review PdfPageCollection::AppendDocumentPages(),
  PdfPageCollection::InsertDocumentPageAt(), PdfPage::MoveAt()

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

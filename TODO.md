### Short term
- PdfElement-> PdfDictionaryElement, PdfArrayElement that have GetDictionary(), GetArray()
- Evaluate renaming PdfObject::GetOwner() -> GetParent()
- Evaluate renaming PdfOwnedObject::GetOwner() -> GetParent()

### 1.0
- Refactor/Review PdfField hierarchy
- Refactor PdfInputDevice: should merge with PdfOutputDevice
- PdfContents: create on demand /Contents. First create a single stream, after array
- Use unique_ptr/shared_ptr where possibile (PdfField), use shared_ptr in PdfIndirectObjectList?
- Revision PdfWriteFlags
- Check/Review doxygen doc
- FontManager could use unique_ptr or shared_ptr?
- Remove PdfLocaleImbue
- PdfRect should avoid normalization
- Optimize PdfInputDevice that takes buffer to not copy it (use istringviewstream)
- PdfDifferenceEncoding: Move char code conversion data to private folder

### After 1.0
- Add backtrace: https://github.com/boostorg/stacktrace
- Fix PdfIndirectObjectList::RenumberObjects
- Check functioning of garbage collection
- Check working of linearization
- Option to unfold Unicode ligatures to separate codepoints during encoded -> utf8 conversion
- Option to convert Unicode ligatures <-> separate codepoints when drawing strings/converting to encoded
- Optimize mm::chars to not initialize memory, keeping std::string compatibility

/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FILE_SPEC_H
#define PDF_FILE_SPEC_H

#include "podofo/base/PdfDefines.h"

#include "podofo/base/PdfString.h"

#include "PdfElement.h"

namespace PoDoFo {

class PdfDocument;

/**
 *  A file specification is used in the PDF file to referr to another file.
 *  The other file can be a file outside of the PDF or can be embedded into
 *  the PDF file itself.
 */
class PODOFO_DOC_API PdfFileSpec final : public PdfElement
{
public:
    PdfFileSpec(PdfDocument& doc, const std::string_view& filename, bool bEmbedd, bool bStripPath = false);

    PdfFileSpec(PdfDocument& doc, const std::string_view& filename, const char* data, size_t size, bool bStripPath = false);

    PdfFileSpec(PdfObject& obj);

    /** Gets file name for the FileSpec
     *  \param canUnicode Whether can return file name in unicode (/UF)
     *  \returns the filename of this file specification.
     *           if no general name is available
     *           it will try the Unix, Mac and DOS keys too.
     */
    const PdfString& GetFilename(bool canUnicode) const;

private:

    /** Initialize a filespecification from a filename
     *  \param pszFilename filename
     *  \param bEmbedd embedd the file data into the PDF file
     *  \param bStripPath whether to strip path from the file name string
     */
    void Init(const std::string_view& filename, bool bEmbedd, bool bStripPath);

    /** Initialize a filespecification from an in-memory buffer
     *  \param pszFilename filename
     *  \param data Data of the file
     *  \param size size of the data buffer
     *  \param bStripPath whether to strip path from the file name string
     */
    void Init(const std::string_view& filename, const char* data, size_t size, bool bStripPath);

    /** Create a file specification string from a filename
     *  \param pszFilename filename
     *  \returns a file specification string
     */
    PdfString CreateFileSpecification(const std::string_view& filename) const;

    /** Embedd a file into a stream object
     *  \param pStream write the file to this objects stream
     *  \param pszFilename the file to embedd
     */
    void EmbeddFile(PdfObject* pStream, const std::string_view& filename) const;

    /** Strips path from a file, according to \a bStripPath
     *  \param pszFilename a file name string
     *  \param bStripPath whether to strip path from the file name string
     *  \returns Either unchanged \a pszFilename, if \a bStripPath is false;
     *     or \a pszFilename without a path part, if \a bStripPath is true
     */
    std::string MaybeStripPath(const std::string_view& filename, bool bStripPath) const;

    /* Petr P. Petrov 17 September 2009*/
    /** Embeds the file from memory
      */
    void EmbeddFileFromMem(PdfObject* pStream, const char* data, size_t size) const;
};

};

#endif // PDF_FILE_SPEC_H


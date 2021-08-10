/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_PARSER_OBJECT_H
#define PDF_PARSER_OBJECT_H

#include "PdfDefines.h"
#include "PdfObject.h"
#include "PdfTokenizer.h"

namespace mm {

class PdfEncrypt;

/**
 * A PdfParserObject constructs a PdfObject from a PDF file.
 * Parsing starts always at the current file position.
 */
class PDFMM_API PdfParserObject : public PdfObject
{
public:
    /** Parse the object data from the given file handle starting at
     *  the current position.
     *  \param document document where to resolve object references
     *  \param device an open reference counted input device which is positioned in
     *                 front of the object which is going to be parsed.
     *  \param buffer buffer to use for parsing to avoid reallocations
     *  \param offset the position in the device from which the object shall be read
     *                 if lOffset = -1, the object will be read from the current
     *                 position in the file.
     */
    PdfParserObject(PdfDocument& document, const PdfRefCountedInputDevice& device, const PdfSharedBuffer& buffer, ssize_t offset = -1);

    /** Parse the object data for an internal object.
     *  You have to call ParseDictionaryKeys as next function call.
     *
     *  The following two parameters are used to avoid allocation of a new
     *  buffer in PdfSimpleParser.
     *
     *  \warning This constructor is for internal usage only!
     *
     *  \param buffer buffer to use for parsing to avoid reallocations
     */
    explicit PdfParserObject(const PdfSharedBuffer& buffer);

    /** Tries to free all memory allocated by this
     *  PdfObject (variables and streams) and reads
     *  it from disk again if it is requested another time.
     *
     *  This will only work if load on demand is used.
     *  If the object is dirty if will not be free'd.
     *
     *  \param force if true the object will be free'd
     *                even if IsDirty() returns true.
     *                So you will loose any changes made
     *                to this object.
     *
     *  \see IsLoadOnDemand
     *  \see IsDirty
     */
    void FreeObjectMemory(bool force = false);

    /** Parse the object data from the given file handle
     *  If delayed loading is enabled, only the object and generation number
     *  is read now and everything else is read later.
     *
     *  \param encrypt an encryption dictionary which is used to decrypt
     *                  strings and streams during parsing or nullptr if the PDF
     *                  file was not encrypted
     *  \param isTrailer whether this is a trailer dictionary or not.
     *                    trailer dictionaries do not have a object number etc.
     */
    void ParseFile(PdfEncrypt* encrypt, bool isTrailer = false);

    void ForceStreamParse();

    /** Returns if this object has a stream object appended.
     *  which has to be parsed.
     *  \returns true if there is a stream
     */
    inline bool HasStreamToParse() const { return m_HasStream; }

    /** \returns true if this PdfParser loads all objects at
     *                the time they are accessed for the first time.
     *                The default is to load all object immediately.
     *                In this case false is returned.
     */
    inline bool IsLoadOnDemand() const { return m_LoadOnDemand; }

    /** Sets whether this object shall be loaded on demand
     *  when it's data is accessed for the first time.
     *  \param bDelayed if true the object is loaded delayed.
     */
    inline void SetLoadOnDemand(bool bDelayed) { m_LoadOnDemand = bDelayed; }

    /** Gets an offset in which the object beginning is stored in the file.
     *  Note the offset points just after the object identificator ("0 0 obj").
     *
     * \returns an offset in which the object is stored in the source device,
     *     or -1, if the object was created on demand.
     */
    inline ssize_t GetOffset() const { return m_Offset; }

protected:
    void DelayedLoadImpl() override;
    void DelayedLoadStreamImpl() override;

private:
    /** Starts reading at the file position m_StreamOffset and interprets all bytes
     *  as contents of the objects stream.
     *  It is assumed that the dictionary has a valid /Length key already.
     *
     *  Called from DelayedLoadStream(). Do not call directly.
     */
    void ParseStream();

    /** Initialize private members in this object with their default values
     */
    void InitPdfParserObject();

    /** Parse the object data from the given file handle
     *  \param bIsTrailer whether this is a trailer dictionary or not.
     *                    trailer dictionaries do not have a object number etc.
     */
    void ParseFileComplete(bool isTrailer);

    void ReadObjectNumber();

private:
    PdfRefCountedInputDevice m_device;
    PdfSharedBuffer m_buffer;
    PdfTokenizer m_tokenizer;
    PdfEncrypt* m_Encrypt;
    bool m_IsTrailer;

    // Should the object try to defer loading of its contents until needed?
    // If false, object contents will be loaded during ParseFile(...). Note that
    //          this still uses the delayed loading infrastructure.
    // If true, loading will be triggered the first time the information is needed by
    //          an external caller.
    // Outside callers should not be able to tell the difference between the two modes
    // of operation.
    bool m_LoadOnDemand;
    ssize_t m_Offset;
    bool m_HasStream;
    size_t m_StreamOffset;
};

};

#endif // PDF_PARSER_OBJECT_H

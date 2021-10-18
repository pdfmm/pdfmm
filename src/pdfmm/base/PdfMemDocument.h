/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_MEM_DOCUMENT_H
#define PDF_MEM_DOCUMENT_H

#include "PdfDefines.h"

#include "PdfDocument.h"
#include "PdfExtension.h"

namespace mm {

class PdfAcroForm;
class PdfDestination;
class PdfDictionary;
class PdfEncrypt;
class PdfFileSpec;
class PdfFont;
class PdfInfo;
class PdfNameTree;
class PdfOutlines;
class PdfPage;
class PdfPageTree;
class PdfParser;
class PdfRect;
class PdfWriter;

/** PdfMemDocument is the core class for reading and manipulating
 *  PDF files and writing them back to disk.
 *
 *  PdfMemDocument was designed to allow easy access to the object
 *  structur of a PDF file.
 *
 *  PdfMemDocument should be used whenever you want to change
 *  the object structure of a PDF file.
 *
 *  When you are only creating PDF files, please use PdfStreamedDocument
 *  which is usually faster for creating PDFs.
 *
 *  \see PdfDocument
 *  \see PdfStreamedDocument
 *  \see PdfParser
 *  \see PdfWriter
 */
class PDFMM_API PdfMemDocument : public PdfDocument
{
    friend class PdfWriter;

public:
    /** Construct a new PdfMemDocument
     */
    PdfMemDocument(bool empty = false);

    /** Construct a copy of the given document
     */
    PdfMemDocument(const PdfMemDocument& rhs);

    ~PdfMemDocument();

    /** Load a PdfMemDocument from a file
     *
     *  \param filename filename of the file which is going to be parsed/opened
     *
     *  When the bForUpdate is set to true, the filename is copied
     *  for later use by WriteUpdate.
     *
     *  \see WriteUpdate, LoadFromBuffer, LoadFromDevice
     */
    void Load(const std::string_view& filename, const std::string_view& password = { });

    /** Load a PdfMemDocument from a buffer in memory
     *
     *  \param buffer a memory area containing the PDF data
     *
     *  \see WriteUpdate, Load, LoadFromDevice
     */
    void LoadFromBuffer(const std::string_view& buffer, const std::string_view& password = { });

    /** Load a PdfMemDocument from a PdfRefCountedInputDevice
     *
     *  \param device the input device containing the PDF
     *
     *  \see WriteUpdate, Load, LoadFromBuffer
     */
    void LoadFromDevice(const std::shared_ptr<PdfInputDevice>& device, const std::string_view& password = { });

    /** Writes the complete document to a file
     *
     *  \param filename filename of the document
     *
     *  \see Write, WriteUpdate
     *
     *  This is an overloaded member function for your convenience.
     */
    void Write(const std::string_view& filename, PdfSaveOptions options = PdfSaveOptions::None);

    /** Writes the complete document to an output device
     *
     *  \param device write to this output device
     *
     *  \see WriteUpdate
     */
    void Write(PdfOutputDevice& device, PdfSaveOptions options = PdfSaveOptions::None);

    /** Writes the document changes to a file
     *
     *  \param filename filename of the document
     *
     *  Writes the document changes to a file as an incremental update.
     *  The document should be loaded with bForUpdate = true, otherwise
     *  an exception is thrown.
     *
     *  Beware when overwriting existing files. Plain file overwrite is allowed
     *  only if the document was loaded with the same filename (and the same overloaded
     *  function), otherwise the destination file cannot be the same as the source file,
     *  because the destination file is truncated first and only then the source file
     *  content is copied into it.
     *
     *  \see Write, WriteUpdate
     *
     *  This is an overloaded member function for your convenience.
     */
    void WriteUpdate(const std::string_view& filename, PdfSaveOptions options = PdfSaveOptions::None);

    /** Writes the document changes to an output device
     *
     *  \param device write to this output device
     *  \param bTruncate whether to truncate the device first and fill it
     *                   with the content of the source document; the default is true.
     *
     *  Writes the document changes to the output device as an incremental update.
     *  The document should be loaded with bForUpdate = true, otherwise
     *  an exception is thrown.
     *
     *  The bTruncate is used to determine whether saving to the same file or not.
     *  In case the bTruncate is true, a new source stream is opened and its whole
     *  content is copied to the device first. Otherwise the device is the same
     *  file which had been loaded and the caller is responsible to position
     *  the device at the place, where the update should be written (basically
     *  at the end of the stream).
     *
     *  \see Write, WriteUpdate
     */
    void WriteUpdate(PdfOutputDevice& device, PdfSaveOptions options = PdfSaveOptions::None);

    /** Set the write mode to use when writing the PDF.
     *  \param writeMode write mode
     */
    void SetWriteMode(PdfWriteMode writeMode) { m_WriteMode = writeMode; }

    inline PdfWriteMode GetWriteMode() const  override { return m_WriteMode; }

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param version  version of the pdf document
     */
    inline void SetPdfVersion(PdfVersion version) { m_Version = version; }

    inline PdfVersion GetPdfVersion() const override { return m_Version; }

    /** Add a vendor-specific extension to the current PDF version.
     *  \param ns namespace of the extension
     *  \param level level of the extension
     */
    void AddPdfExtension(const PdfName& ns, int64_t level);

    /** Checks whether the documents is tagged to imlpement a vendor-specific
     *  extension to the current PDF version.
     *  \param ns  namespace of the extension
     *  \param level  level of the extension
     */
    bool HasPdfExtension(const PdfName& ns, int64_t level) const;

    /** Remove a vendor-specific extension to the current PDF version.
     *  \param ns  namespace of the extension
     *  \param level  level of the extension
     */
    void RemovePdfExtension(const PdfName& ns, int64_t level);

    /** Return the list of all vendor-specific extensions to the current PDF version.
     *  \param ns  namespace of the extension
     *  \param level  level of the extension
     */
    std::vector<PdfExtension> GetPdfExtensions() const;

    /** Encrypt the document during writing.
     *
     *  \param userPassword the user password (if empty the user does not have
     *                      to enter a password to open the document)
     *  \param ownerPassword the owner password
     *  \param protection several PdfPermissions values or'ed together to set
     *                    the users permissions for this document
     *  \param algorithm the revision of the encryption algorithm to be used
     *  \param keyLength the length of the encryption key ranging from 40 to 256 bits
     *                    (only used if algorithm >= PdfEncryptAlgorithm::RC4V2)
     *
     *  \see PdfEncrypt
     */
    void SetEncrypted(const std::string& userPassword, const std::string& ownerPassword,
        PdfPermissions protection = PdfPermissions::Default,
        PdfEncryptAlgorithm algorithm = PdfEncryptAlgorithm::AESV2,
        PdfKeyLength keyLength = PdfKeyLength::L40);

    /** Encrypt the document during writing using a PdfEncrypt object
     *
     *  \param encrypt an encryption object that will be owned by PdfMemDocument
     */
    void SetEncrypted(const PdfEncrypt& encrypt);

    /**
     * \returns true if this PdfMemDocument creates an encrypted PDF file
     */
    inline bool IsEncrypted() const { return m_Encrypt != nullptr; }

    inline bool IsLinearized() const override { return m_Linearized; }

    /** Copies one or more pages from another PdfMemDocument to this document
     *  \param doc the document to append
     *  \param atIndex the first page number to copy (0-based)
     *  \param pageCount the number of pages to copy
     *  \returns this document
     */
    const PdfMemDocument& InsertPages(const PdfMemDocument& doc, unsigned atIndex, unsigned pageCount);

    bool IsPrintAllowed() const override;

    bool IsEditAllowed() const override;

    bool IsCopyAllowed() const override;

    bool IsEditNotesAllowed() const override;

    bool IsFillAndSignAllowed() const override;

    bool IsAccessibilityAllowed() const override;

    bool IsDocAssemblyAllowed() const override;

    bool IsHighPrintAllowed() const override;

    /** Tries to free all memory allocated by the given
     *  PdfObject (variables and streams) and reads
     *  it from disk again if it is requested another time.
     *
     *  This will only work if load on demand is used. Other-
     *  wise any call to this method will be ignored. Load on
     *  demand is currently always enabled when using PdfMemDocument.
     *  If the object is dirty if will not be free'd.
     *
     *  \param ref free all memory allocated by the object
     *              with this reference.
     *  \param force if true the object will be free'd
     *                even if IsDirty() returns true.
     *                So you will loose any changes made
     *                to this object.
     *
     *  This is an overloaded member for your convenience.
     */
    void FreeObjectMemory(const PdfReference& ref, bool force = false);

    /** Tries to free all memory allocated by the given
     *  PdfObject (variables and streams) and reads
     *  it from disk again if it is requested another time.
     *
     *  This will only work if load on demand is used. Other-
     *  wise any call to this method will be ignored. Load on
     *  demand is currently always enabled when using PdfMemDocument.
     *  If the object is dirty if will not be free'd.
     *
     *  \param obj free object from memory
     *  \param force if true the object will be free'd
     *                even if IsDirty() returns true.
     *                So you will loose any changes made
     *                to this object.
     *
     *  \see IsDirty
     */
    void FreeObjectMemory(PdfObject* obj, bool force = false);

    /**
     * \returns the parsers encryption object or nullptr if the read PDF file was not encrypted
     */
    inline const PdfEncrypt* GetEncrypt() const { return m_Encrypt.get(); }

private:
    /** Deletes one or more pages from this document
     *  It does NOT remove any PdfObjects from memory - just the reference from the pages tree.
     *  If you want to delete resources of this page, you have to delete them yourself,
     *  but the resources might be used by other pages, too.
     *
     *  \param atIndex the first page number to delete (0-based)
     *  \param pageCount the number of pages to delete
     *  \returns this document
     */
    void DeletePages(unsigned atIndex, unsigned pageCount);

    /** Internal method to load all objects from a PdfParser object.
     *  The objects will be removed from the parser and are now
     *  owned by the PdfMemDocument.
     */
    void InitFromParser(PdfParser& parser);

    /** Clear all internal variables
     */
    void Clear();
    void clear();

private:
    PdfMemDocument& operator=(const PdfMemDocument&) = delete;

private:
    PdfVersion m_Version;
    PdfVersion m_InitialVersion;
    bool m_HasXRefStream;
    int64_t m_PrevXRefOffset;
    PdfWriteMode m_WriteMode;
    bool m_Linearized;
    std::unique_ptr<PdfEncrypt> m_Encrypt;
};

};


#endif	// PDF_MEM_DOCUMENT_H

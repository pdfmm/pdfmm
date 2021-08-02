/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCRYPT_H
#define PDF_ENCRYPT_H

#include "PdfDefines.h"
#include "PdfString.h"
#include "PdfReference.h"

namespace PoDoFo
{

class PdfDictionary;
class PdfInputStream;
class PdfObject;
class PdfOutputStream;
class AESCryptoEngine;
class RC4CryptoEngine;
    
/* Class representing PDF encryption methods. (For internal use only)
 * Based on code from Ulrich Telle: http://wxcode.sourceforge.net/components/wxpdfdoc/
 * Original Copyright header:
 * Name:        pdfencrypt.h
 * Purpose:     
 * Author:      Ulrich Telle
 * Modified by:
 * Created:     2005-08-16
 * Copyright:   (c) Ulrich Telle
 * Licence:     wxWindows licence
 */

/** A enum specifying a valid keylength for a PDF encryption key.
 *  Keys must be in the range 40 to 128 bit and have to be a
 *  multiple of 8.
 *
 *  Adobe Reader supports only keys with 40, 128 or 256 bits!
 */
enum class EPdfKeyLength
{
    L40 = 40,
    L56 = 56,
    L80 = 80,
    L96 = 96,
    L128 = 128,
#ifdef PODOFO_HAVE_LIBIDN
    L256 = 256
#endif //PODOFO_HAVE_LIBIDN
};

/** Set user permissions/restrictions on a document
 */
enum class EPdfPermissions
{
    None = 0,
    Print = 0x00000004,  ///< Allow printing the document
    Edit = 0x00000008,  ///< Allow modifying the document besides annotations, form fields or chaning pages
    Copy = 0x00000010,  ///< Allow text and graphic extraction
    EditNotes = 0x00000020,  ///< Add or modify text annoations or form fields (if EPdfPermissions::Edit is set also allow to create interactive form fields including signature)
    FillAndSign = 0x00000100,  ///< Fill in existing form or signature fields 
    Accessible = 0x00000200,  ///< Extract text and graphics to support user with disabillities
    DocAssembly = 0x00000400,  ///< Assemble the document: insert, create, rotate delete pages or add bookmarks
    HighPrint = 0x00000800,   ///< Print a high resolution version of the document
    Default = Print
        | Edit
        | Copy
        | EditNotes
        | FillAndSign
        | Accessible
        | DocAssembly
        | HighPrint
};

/**
 * The encryption algorithm.
 */
enum class EPdfEncryptAlgorithm
{
    None = 0,
    RC4V1 = 1, ///< RC4 Version 1 encryption using a 40bit key
    RC4V2 = 2, ///< RC4 Version 2 encryption using a key with 40-128bit
    AESV2 = 4, ///< AES encryption with a 128 bit key (PDF1.6)
#ifdef PODOFO_HAVE_LIBIDN
    AESV3 = 8 ///< AES encryption with a 256 bit key (PDF1.7 extension 3) - Support added by P. Zent
#endif //PODOFO_HAVE_LIBIDN
};

/** A class that is used to encrypt a PDF file and 
 *  set document permisions on the PDF file.
 *
 *  As a user of this class, you have only to instanciate a
 *  object of this class and pass it to PdfWriter, PdfMemDocument,
 *  PdfStreamedDocument or PdfImmediateWriter.
 *  You do not have to call any other method of this class. The above
 *  classes know how to handle encryption using Pdfencrypt.
 *
 */
class PODOFO_API PdfEncrypt
{
public:
    virtual ~PdfEncrypt();

    /** Create a PdfEncrypt object which can be used to encrypt a PDF file.
     *
     *  \param userPassword the user password (if empty the user does not have
     *                      to enter a password to open the document)
     *  \param ownerPassword the owner password
     *  \param protection several EPdfPermissions values or'ed together to set
     *                    the users permissions for this document
     *  \param eAlgorithm the revision of the encryption algorithm to be used
     *  \param eKeyLength the length of the encryption key ranging from 40 to 128 bits
     *                    (only used if eAlgorithm == EPdfEncryptAlgorithm::RC4V2)
     *
     *  \see GenerateEncryptionKey with the documentID to generate the real
     *       encryption key using this information
     */
    static std::unique_ptr<PdfEncrypt> CreatePdfEncrypt(const std::string_view& userPassword,
        const std::string_view& ownerPassword,
        EPdfPermissions protection = EPdfPermissions::Default,
        EPdfEncryptAlgorithm eAlgorithm = EPdfEncryptAlgorithm::AESV2,
        EPdfKeyLength eKeyLength = EPdfKeyLength::L40);

    /** Initialize a PdfEncrypt object from an encryption dictionary in a PDF file.
     *
     *  This is required for encrypting a PDF file, but handled internally in PdfParser
     *  for you.
     *
     *  Will use only encrypting algorithms that are enabled.
     *
     *  \param pObject a PDF encryption dictionary
     *
     *  \see GetEnabledEncryptionAlgorithms
     */
    static std::unique_ptr<PdfEncrypt> CreatePdfEncrypt(const PdfObject* pObject);

    /** Copy constructor
     *
     *  \param rhs another PdfEncrypt object which is copied
     */
    static std::unique_ptr<PdfEncrypt> CreatePdfEncrypt(const PdfEncrypt& rhs);

    /**
     * Retrieve the list of encryption algorithms that are used
     * when loading a PDF document.
     *
     * By default all alogrithms are enabled.
     *
     * \see IsEncryptionEnabled
     * \see SetEnabledEncryptionAlgorithms
     *
     * \return an or'ed together list of all enabled encryption algorithms
     */
    static EPdfEncryptAlgorithm GetEnabledEncryptionAlgorithms();

    /**
     * Specify the list of encryption algorithms that should be used by PoDoFo
     * when loading a PDF document.
     *
     * This can be used to disable for example AES encryption/decryption
     * which is unstable in certain cases.
     *
     * \see GetEnabledEncryptionAlgorithms
     * \see IsEncryptionEnabled
     */
    static void SetEnabledEncryptionAlgorithms(EPdfEncryptAlgorithm nEncryptionAlgorithms);

    /**
     * Test if a certain encryption algorithm is enabled for loading PDF documents.
     *
     * \returns ture if the encryption algorithm is enabled
     * \see GetEnabledEncryptionAlgorithms
     * \see SetEnabledEncryptionAlgorithms
     */
    static bool IsEncryptionEnabled(EPdfEncryptAlgorithm eAlgorithm);

    /** Generate encryption key from user and owner passwords and protection key
     *
     *  \param documentId the documentId of the current document
     */
    void GenerateEncryptionKey(const PdfString& documentId);

    /** Fill all keys into a encryption dictionary.
     *  This dictionary is usually added to the PDF files trailer
     *  under the /Encryption key.
     *
     *  \param rDictionary an empty dictionary which is filled with information about
     *                     the used encryption algorithm
     */
    virtual void CreateEncryptionDictionary(PdfDictionary& rDictionary) const = 0;

    /** Create a PdfInputStream that decrypts all data read from
     *  it using the current settings of the PdfEncrypt object.
     *
     *  Warning: Currently only RC4 based encryption is supported using output streams!
     *
     *  \param pInputStream the created PdfInputStream reads all decrypted
     *         data to this input stream.
     *
     *  \returns a PdfInputStream that decrypts all data.
     */
    virtual std::unique_ptr<PdfInputStream> CreateEncryptionInputStream(PdfInputStream& pInputStream, size_t inputLen) = 0;

    /** Create a PdfOutputStream that encrypts all data written to
     *  it using the current settings of the PdfEncrypt object.
     *
     *  Warning: Currently only RC4 based encryption is supported using output streams!
     *
     *  \param pOutputStream the created PdfOutputStream writes all encrypted
     *         data to this output stream.
     *
     *  \returns a PdfOutputStream that encryts all data.
     */
    virtual std::unique_ptr<PdfOutputStream> CreateEncryptionOutputStream(PdfOutputStream& pOutputStream) = 0;

    /**
     * Tries to authenticate a user using either the user or owner password
     *
     * \param password owner or user password
     * \param documentId the documentId of the PDF file
     *
     * \returns true if either the owner or user password matches password
     */
    bool Authenticate(const std::string_view& password, const PdfString& documentId);

    /** Get the encryption algorithm of this object.
     * \returns the EPdfEncryptAlgorithm of this object
     */
    inline EPdfEncryptAlgorithm GetEncryptAlgorithm() const { return m_eAlgorithm; }

    /** Checks if printing this document is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsPrintAllowed() const;

    /** Checks if modifiying this document (besides annotations, form fields or changing pages) is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to modfiy this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsEditAllowed() const;

    /** Checks if text and graphics extraction is allowed.
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics from this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsCopyAllowed() const;

    /** Checks if it is allowed to add or modify annotations or form fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to add or modify annotations or form fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsEditNotesAllowed() const;

    /** Checks if it is allowed to fill in existing form or signature fields
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to fill in existing form or signature fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsFillAndSignAllowed() const;

    /** Checks if it is allowed to extract text and graphics to support users with disabillities
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to extract text and graphics to support users with disabillities
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsAccessibilityAllowed() const;

    /** Checks if it is allowed to insert, create, rotate, delete pages or add bookmarks
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed  to insert, create, rotate, delete pages or add bookmarks
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsDocAssemblyAllowed() const;

    /** Checks if it is allowed to print a high quality version of this document
     *  Every PDF consuming applications has to adhere this value!
     *
     *  \returns true if you are allowed to print a high quality version of this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsHighPrintAllowed() const;

    /** Get the U object value (user)
     */
    const unsigned char* GetUValue() const { return m_uValue; }

    /** Get the O object value (owner)
     */
    const unsigned char* GetOValue() const { return m_oValue; }

    /** Get the encryption key value (owner)
     */
    const unsigned char* GetEncryptionKey() const { return m_encryptionKey; }

    /** Get the P object value (protection)
     */
    EPdfPermissions GetPValue() const { return m_pValue; }

    /** Get the revision number of the encryption method
     */
    int GetRevision() const { return m_rValue; }

    /** Get the key length of the encryption key in bits
     */
    int GetKeyLength() const;

    /** Is metadata encrypted
     */
    bool IsMetadataEncrypted() const { return m_bEncryptMetadata; }

    /** Encrypt a character string
     */
    void Encrypt(const std::string_view& view, std::string& out) const;

    /** Decrypt a character string
     */
    void Decrypt(const std::string_view &view, std::string& out) const;

    /** Calculate stream size
     */
    virtual size_t CalculateStreamLength(size_t length) const = 0;

    /** Calculate stream offset
     */
    virtual size_t CalculateStreamOffset() const = 0;


    /** Set the reference of the object that is currently encrypted.
     *
     *  This value will be used in following calls of Encrypt
     *  to encrypt the object.
     *
     *  \see Encrypt
     */
    void SetCurrentReference(const PdfReference& rRef);

protected:
    PdfEncrypt();

    PdfEncrypt(const PdfEncrypt& rhs);

protected:
    virtual void Decrypt(const char* inStr, size_t inLen,
        char* outStr, size_t& outLen) const = 0;

    virtual void Encrypt(const char* inStr, size_t inLen,
        char* outStr, size_t outLen) const = 0;

    virtual bool Authenticate(const std::string_view& password, const  std::string_view& documentId) = 0;

    virtual void GenerateEncryptionKey(const std::string_view& documentId) = 0;

    // Check two keys for equality
    bool CheckKey(unsigned char key1[32], unsigned char key2[32]);

    EPdfEncryptAlgorithm m_eAlgorithm;   // The used encryption algorithm
    unsigned m_keyLength;                // Length of encryption key
    int m_rValue;                        // Revision
    EPdfPermissions m_pValue;            // P entry in pdf document
    EPdfKeyLength m_eKeyLength;          // The key length
    std::string m_userPass;              // User password
    std::string m_ownerPass;             // Owner password
    unsigned char m_uValue[48];          // U entry in pdf document
    unsigned char m_oValue[48];          // O entry in pdf document
    unsigned char m_encryptionKey[32];   // Encryption key
    PdfReference m_curReference;         // Reference of the current PdfObject
    std::string m_documentId;            // DocumentID of the current document
    bool m_bEncryptMetadata;             // Is metadata encrypted

private:
    static EPdfEncryptAlgorithm s_nEnabledEncryptionAlgorithms; // Or'ed int containing the enabled encryption algorithms
};

#ifdef PODOFO_HAVE_LIBIDN

/** A pure virtual class that is used to encrypt a PDF file (AES-256)
 *  This class is the base for classes that implement algorithms based on SHA hashes
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncrypt*, it is created through CreatePdfEncrypt factory method
 *
 */
class PdfEncryptSHABase : public PdfEncrypt
{
public:
    PdfEncryptSHABase();

    // copy constructor
    PdfEncryptSHABase(const PdfEncrypt& rhs);

    void CreateEncryptionDictionary(PdfDictionary& rDictionary) const override;

    // Get the UE object value (user)
    const unsigned char* GetUEValue() const { return m_ueValue; }

    // Get the OE object value (owner)
    const unsigned char* GetOEValue() const { return m_oeValue; }

    // Get the Perms object value (encrypted protection)
    const unsigned char* GetPermsValue() const { return m_permsValue; }

    bool Authenticate(const std::string& documentID, const std::string& password,
        const std::string& uValue, const std::string& ueValue,
        const std::string& oValue, const std::string& oeValue,
        EPdfPermissions pValue, const std::string& permsValue,
        int lengthValue, int rValue);

protected:
    // NOTE: We must declare again without body otherwise the other Authenticate overload hides it
    bool Authenticate(const std::string_view& password, const std::string_view& documentId) override = 0;

    // Generate initial vector
    void GenerateInitialVector(unsigned char iv[]) const;

    // Compute encryption key to be used with AES-256
    void ComputeEncryptionKey();

    // Generate the U and UE entries
    void ComputeUserKey(const unsigned char* userpswd, size_t len);

    // Generate the O and OE entries
    void ComputeOwnerKey(const unsigned char* userpswd, size_t len);

    // Preprocess password for use in EAS-256 Algorithm
    // outBuf needs to be at least 127 bytes long
    void PreprocessPassword(const std::string_view& password, unsigned char* outBuf, size_t& len);

    unsigned char m_ueValue[32];        // UE entry in pdf document
    unsigned char m_oeValue[32];        // OE entry in pdf document
    unsigned char m_permsValue[16];     // Perms entry in pdf document
};
    
/** A pure virtual class that is used to encrypt a PDF file (RC4, AES-128)
 *  This class is the base for classes that implement algorithms based on MD5 hashes
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncrypt*, it is created through CreatePdfEncrypt factory method
 *
 */
#endif // PODOFO_HAVE_LIBIDN

/** A pure virtual class that is used to encrypt a PDF file (AES-128/256)
 *  This class is the base for classes that implement algorithms based on AES
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncrypt*, it is created through CreatePdfEncrypt factory method
 *
 */
class PdfEncryptAESBase
{
public:
    ~PdfEncryptAESBase();

protected:
    PdfEncryptAESBase();

    void BaseDecrypt(const unsigned char* key, unsigned keylen, const unsigned char* iv,
        const unsigned char* textin, size_t textlen,
        unsigned char* textout, size_t& textoutlen) const;
    void BaseEncrypt(const unsigned char* key, unsigned keylen, const unsigned char* iv,
        const unsigned char* textin, size_t textlen,
        unsigned char* textout, size_t textoutlen) const;

    AESCryptoEngine* m_aes;                // AES encryptor
};

/** A pure virtual class that is used to encrypt a PDF file (RC4-40..128)
 *  This class is the base for classes that implement algorithms based on RC4
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncrypt*, it is created through CreatePdfEncrypt factory method
 *
 */
class PdfEncryptRC4Base
{
public:
    ~PdfEncryptRC4Base();

protected:
    PdfEncryptRC4Base();

    // AES encryption
    void RC4(const unsigned char* key, unsigned keylen,
        const unsigned char* textin, size_t textlen,
        unsigned char* textout, size_t textoutlen) const;

    RC4CryptoEngine* m_rc4;                // AES encryptor
};
    
class PdfEncryptMD5Base : public PdfEncrypt, public PdfEncryptRC4Base
{
public:
    PdfEncryptMD5Base();

    // copy constructor
    PdfEncryptMD5Base(const PdfEncrypt& rhs);

    void CreateEncryptionDictionary(PdfDictionary& rDictionary) const override;

    /** Create a PdfString of MD5 data generated from a buffer in memory.
     *  \param pBuffer the buffer of which to calculate the MD5 sum
     *  \param nLength the length of the buffer
     *
     *  \returns an MD5 sum as PdfString
     */
    static PdfString GetMD5String(const unsigned char* pBuffer, int nLength);

    // Calculate the binary MD5 message digest of the given data
    static void GetMD5Binary(const unsigned char* data, int length, unsigned char* digest);

    // NOTE: We must declare again without body otherwise the other Authenticate overload hides it
    bool Authenticate(const std::string_view& password, const std::string_view& documentId) override = 0;

    bool Authenticate(const std::string& documentID, const std::string& password,
        const std::string& uValue, const std::string& oValue,
        EPdfPermissions pValue, int lengthValue, int rValue);

protected:

    // Generate initial vector
    void GenerateInitialVector(unsigned char iv[]) const;

    // Compute owner key
    void ComputeOwnerKey(const unsigned char userPad[32], const unsigned char ownerPad[32],
        int keylength, int revision, bool authenticate, unsigned char ownerKey[32]);

    // Pad a password to 32 characters
    void PadPassword(const std::string_view& password, unsigned char pswd[32]);

    // Compute encryption key and user key
    void ComputeEncryptionKey(const std::string& documentID,
        const unsigned char userPad[32], const unsigned char ownerKey[32],
        EPdfPermissions pValue, EPdfKeyLength keyLength, int revision,
        unsigned char userKey[32], bool bEncryptMetadata);

    /** Create the encryption key for the current object.
     *
     *  \param objkey pointer to an array of at least MD5_HASHBYTES (=16) bytes length
     *  \param pnKeyLen pointer to an integer where the actual keylength is stored.
     */
    void CreateObjKey(unsigned char objkey[16], unsigned& pnKeyLen) const;

    unsigned char m_rc4key[16];         // last RC4 key
    unsigned char m_rc4last[256];       // last RC4 state table

};
    
/** A class that is used to encrypt a PDF file (AES-128)
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncryptAES*, it is created through CreatePdfEncrypt factory method	
 *
 */
class PdfEncryptAESV2 : public PdfEncryptMD5Base, public PdfEncryptAESBase
{
public:
    PdfEncryptAESV2(PdfString oValue, PdfString uValue, EPdfPermissions pValue, bool bEncryptMetadata);
    PdfEncryptAESV2(const PdfEncrypt& rhs) : PdfEncryptMD5Base(rhs) { }
    PdfEncryptAESV2(const std::string_view& userPassword, const std::string_view& ownerPassword,
        EPdfPermissions protection = EPdfPermissions::Default);

    std::unique_ptr<PdfInputStream> CreateEncryptionInputStream(PdfInputStream& pInputStream, size_t inputLen) override;
    std::unique_ptr<PdfOutputStream> CreateEncryptionOutputStream(PdfOutputStream& pOutputStream) override;

    void Encrypt(const char* inStr, size_t inLen,
        char* outStr, size_t outLen) const override;
    void Decrypt(const char* inStr, size_t inLen,
        char* outStr, size_t& outLen) const override;

    size_t CalculateStreamOffset() const override;

    size_t CalculateStreamLength(size_t length) const override;

protected:
    void GenerateEncryptionKey(const std::string_view& documentId) override;

    bool Authenticate(const std::string_view& password, const std::string_view& documentId) override;
};

#ifdef PODOFO_HAVE_LIBIDN

/** A class that is used to encrypt a PDF file (AES-256)
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncryptAES*, it is created through CreatePdfEncrypt factory method	
 *
 */
class PdfEncryptAESV3 : public PdfEncryptSHABase, public PdfEncryptAESBase
{
public:
    PdfEncryptAESV3(PdfString oValue, PdfString oeValue, PdfString uValue, PdfString ueValue, EPdfPermissions pValue, PdfString permsValue);
    PdfEncryptAESV3(const PdfEncrypt& rhs) : PdfEncryptSHABase(rhs) {}
    PdfEncryptAESV3(const std::string_view& userPassword, const std::string_view& ownerPassword,
        EPdfPermissions protection = EPdfPermissions::Default);

    std::unique_ptr<PdfInputStream> CreateEncryptionInputStream(PdfInputStream& pInputStream, size_t inputLen) override;
    std::unique_ptr<PdfOutputStream> CreateEncryptionOutputStream(PdfOutputStream& pOutputStream) override;

    // Encrypt a character string
    void Encrypt(const unsigned char* inStr, size_t inLen,
        unsigned char* outStr, size_t outLen) const override;
    void Decrypt(const unsigned char* inStr, size_t inLen,
        unsigned char* outStr, size_t& outLen) const override;

    size_t CalculateStreamOffset() const override;

    size_t CalculateStreamLength(size_t length) const override;

protected:
    bool Authenticate(const std::string_view& password, const std::string_view& documentId) override;

    void GenerateEncryptionKey(const string_view& documentId) override;
};

#endif // PODOFO_HAVE_LIBIDN

/** A class that is used to encrypt a PDF file (RC4 40-bit and 128-bit)
 *
 *  Client code is working only with PdfEncrypt class and knows nothing
 *	about PdfEncryptRC4, it is created through CreatePdfEncrypt factory method	
 *
 */
class PdfEncryptRC4 : public PdfEncryptMD5Base
{
public:
    PdfEncryptRC4(PdfString oValue, PdfString uValue,
        EPdfPermissions pValue, int rValue, EPdfEncryptAlgorithm eAlgorithm, int length, bool bEncryptMetadata);
    PdfEncryptRC4(const PdfEncrypt& rhs) : PdfEncryptMD5Base(rhs) {}
    PdfEncryptRC4(const std::string_view& userPassword, const std::string_view& ownerPassword,
        EPdfPermissions protection = EPdfPermissions::Default,
        EPdfEncryptAlgorithm eAlgorithm = EPdfEncryptAlgorithm::RC4V1,
        EPdfKeyLength eKeyLength = EPdfKeyLength::L40);

    void Encrypt(const char* inStr, size_t inLen,
        char* outStr, size_t outLen) const override;

    void Decrypt(const char* inStr, size_t inLen,
        char* outStr, size_t& outLen) const override;

    std::unique_ptr<PdfInputStream> CreateEncryptionInputStream(PdfInputStream& pInputStream, size_t inputLen) override;

    std::unique_ptr<PdfOutputStream> CreateEncryptionOutputStream(PdfOutputStream& pOutputStream) override;

    size_t CalculateStreamOffset() const override;

    size_t CalculateStreamLength(size_t length) const override;

protected:
    void GenerateEncryptionKey(const std::string_view& documentId) override;

    bool Authenticate(const std::string_view& password, const std::string_view& documentId) override;
};

}
ENABLE_BITMASK_OPERATORS(PoDoFo::EPdfPermissions);
ENABLE_BITMASK_OPERATORS(PoDoFo::EPdfEncryptAlgorithm);

#endif // PDF_ENCRYPT_H

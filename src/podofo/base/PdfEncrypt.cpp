/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// ---------------------------
// PdfEncrypt implementation
// Based on code from Ulrich Telle: http://wxcode.sourceforge.net/components/wxpdfdoc/
// ---------------------------

#include "PdfDefinesPrivate.h"
#include "PdfEncrypt.h"

#include "PdfDictionary.h"
#include "PdfFilter.h"

#include <sstream>

#ifdef PODOFO_HAVE_LIBIDN
// AES-256 dependencies :
// SASL
#include <stringprep.h>
#include <openssl/sha.h>
#endif // PODOFO_HAVE_LIBIDN

#include <openssl/opensslconf.h>
#include <openssl/md5.h>
#include <openssl/evp.h>

using namespace std;
using namespace PoDoFo;

#ifdef PODOFO_HAVE_LIBIDN
EPdfEncryptAlgorithm PdfEncrypt::s_nEnabledEncryptionAlgorithms =
EPdfEncryptAlgorithm::RC4V1 |
EPdfEncryptAlgorithm::RC4V2 |
EPdfEncryptAlgorithm::AESV2 |
EPdfEncryptAlgorithm::AESV3;
#else // PODOFO_HAVE_LIBIDN
EPdfEncryptAlgorithm PdfEncrypt::s_nEnabledEncryptionAlgorithms =
EPdfEncryptAlgorithm::RC4V1 |
EPdfEncryptAlgorithm::RC4V2 |
EPdfEncryptAlgorithm::AESV2;
#endif // PODOFO_HAVE_LIBIDN

// Default value for P (permissions) = no permission
#define PERMS_DEFAULT (EPdfPermissions)0xFFFFF0C0

#define AES_IV_LENGTH 16
#define AES_BLOCK_SIZE 16

namespace PoDoFo
{

// A class that holds the AES Crypto object
class AESCryptoEngine
{
public:
    AESCryptoEngine()
    {
        aes = EVP_CIPHER_CTX_new();
    }
    
    EVP_CIPHER_CTX* getEngine()
    {
        return aes;
    }
    
    ~AESCryptoEngine()
    {
        EVP_CIPHER_CTX_free(aes);
    }

private:
    EVP_CIPHER_CTX *aes;
};

// A class that holds the RC4 Crypto object
// Either CCCrpytor or EVP_CIPHER_CTX
class RC4CryptoEngine
{
public:

    RC4CryptoEngine()
    {
        rc4 = EVP_CIPHER_CTX_new();
    }

    EVP_CIPHER_CTX* getEngine()
    {
        return rc4;
    }

    ~RC4CryptoEngine()
    {
        EVP_CIPHER_CTX_free(rc4);
    }

private:
    EVP_CIPHER_CTX* rc4;
};
    
/** A class that can encrypt/decrpyt streamed data block wise
 *  This is used in the input and output stream encryption implementation.
 *  Only the RC4 encryption algorithm is supported
 */
class PdfRC4Stream
{
public:
    PdfRC4Stream(unsigned char rc4key[256], unsigned char rc4last[256],
        unsigned char* key, unsigned keylen) :
        m_a(0), m_b(0)
    {
        size_t i;
        size_t j;
        size_t t;

        if (std::memcmp(key, rc4key, keylen) != 0)
        {
            for (i = 0; i < 256; i++)
                m_rc4[i] = static_cast<unsigned char>(i);

            j = 0;
            for (i = 0; i < 256; i++)
            {
                t = static_cast<size_t>(m_rc4[i]);
                j = (j + t + static_cast<size_t>(key[i % keylen])) % 256;
                m_rc4[i] = m_rc4[j];
                m_rc4[j] = static_cast<unsigned char>(t);
            }

            std::memcpy(rc4key, key, keylen);
            std::memcpy(rc4last, m_rc4, 256);
        }
        else
        {
            std::memcpy(m_rc4, rc4last, 256);
        }
    }

    ~PdfRC4Stream()
    {
    }

    /** Encrypt or decrypt a block
     *
     *  \param pBuffer the input/output buffer. Data is read from this buffer and also stored here
     *  \param lLen    the size of the buffer
     */
    size_t Encrypt(char* pBuffer, size_t lLen)
    {
        unsigned char k;
        int t;

        // Do not encode data with no length
        if (!lLen)
            return lLen;

        for (size_t i = 0; i < lLen; i++)
        {
            m_a = (m_a + 1) % 256;
            t = m_rc4[m_a];
            m_b = (m_b + t) % 256;

            m_rc4[m_a] = m_rc4[m_b];
            m_rc4[m_b] = static_cast<unsigned char>(t);

            k = m_rc4[(m_rc4[m_a] + m_rc4[m_b]) % 256];
            pBuffer[i] = pBuffer[i] ^ k;
        }

        return lLen;
    }

private:
    unsigned char m_rc4[256];

    int m_a;
    int m_b;

};

/** A PdfOutputStream that encrypt all data written
 *  using the RC4 encryption algorithm
 */
class PdfRC4OutputStream : public PdfOutputStream
{
public:
    PdfRC4OutputStream(PdfOutputStream& pOutputStream, unsigned char rc4key[256],
        unsigned char rc4last[256], unsigned char* key, unsigned keylen) :
        m_pOutputStream(&pOutputStream), m_stream(rc4key, rc4last, key, keylen)
    {
    }

    /** Write data to the output stream
     *
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer
     */
    void WriteImpl(const char* pBuffer, size_t lLen) override
    {
        buffer_t outputBuffer(lLen);
        std::memcpy(outputBuffer.data(), pBuffer, lLen);

        m_stream.Encrypt(outputBuffer.data(), lLen);
        m_pOutputStream->Write(outputBuffer.data(), lLen);
    }

    /** Close the PdfOutputStream.
     *  This method may throw exceptions and has to be called
     *  before the destructor to end writing.
     *
     *  No more data may be written to the output device
     *  after calling close.
     */
    void Close() override
    {
    }

private:
    PdfOutputStream* m_pOutputStream;
    PdfRC4Stream m_stream;
};

/** A PdfInputStream that decrypts all data read
 *  using the RC4 encryption algorithm
 */
class PdfRC4InputStream : public PdfInputStream
{
public:
    PdfRC4InputStream(PdfInputStream& pInputStream, size_t inputLen, unsigned char rc4key[256], unsigned char rc4last[256],
        unsigned char* key, unsigned keylen) :
        m_pInputStream(&pInputStream),
        m_inputLen(inputLen),
        m_stream(rc4key, rc4last, key, keylen) { }

protected:
    size_t ReadImpl(char* pBuffer, size_t lLen, bool& eof) override
    {
        // CHECK-ME: The code has never been tested after refactor
        // If it's correct, remove this warning
        bool streameof;
        size_t read = m_pInputStream->Read(pBuffer, std::min(lLen, m_inputLen), streameof);
        m_inputLen -= read;
        eof = streameof || m_inputLen == 0;
        return m_stream.Encrypt(pBuffer, read);
    }

private:
    PdfInputStream* m_pInputStream;
    size_t m_inputLen;
    PdfRC4Stream m_stream;
};

/** A PdfAESInputStream that decrypts all data read
 *  using the AES encryption algorithm
 */
class PdfAESInputStream : public PdfInputStream
{
public:
    PdfAESInputStream(PdfInputStream& pInputStream, size_t inputLen, unsigned char* key, unsigned keylen) :
        m_pInputStream(&pInputStream),
        m_inputLen(inputLen),
        m_inputEof(false),
        m_init(true),
        m_keyLen(keylen),
        m_drainLeft(-1)
    {
        m_ctx = EVP_CIPHER_CTX_new();
        if (m_ctx == nullptr)
            PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);

        std::memcpy(this->m_key, key, keylen);
    }

    ~PdfAESInputStream()
    {
        EVP_CIPHER_CTX_free(m_ctx);
    }

protected:
    size_t ReadImpl(char* buffer, size_t len, bool& eof) override
    {
        int outlen = 0;
        size_t drainLen;
        size_t read;
        if (m_inputEof)
            goto DrainBuffer;

        int rc;
        if (m_init)
        {
            // Read the initialization vector separately first
            char iv[AES_IV_LENGTH];
            bool streameof;
            read = m_pInputStream->Read(iv, AES_IV_LENGTH, streameof);
            if (read != AES_IV_LENGTH)
                PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Can't read enough bytes for AES IV");

            const EVP_CIPHER* cipher;
            switch (m_keyLen)
            {
                case (size_t)EPdfKeyLength::L128 / 8:
                {
                    cipher = EVP_aes_128_cbc();
                    break;
                }
#ifdef PODOFO_HAVE_LIBIDN
                case (size_t)EPdfKeyLength::L256 / 8:
                {
                    cipher = EVP_aes_256_cbc();
                    break;
                }
#endif
                default:
                    PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Invalid AES key length");
            }

            rc = EVP_DecryptInit_ex(m_ctx, cipher, nullptr, m_key, (unsigned char*)iv);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing AES encryption engine");

            m_inputLen -= AES_IV_LENGTH;
            m_init = false;
        }

        bool streameof;
        read = m_pInputStream->Read(buffer, std::min(len, m_inputLen), streameof);
        m_inputLen -= read;

        // Quote openssl.org: "the decrypted data buffer out passed to EVP_DecryptUpdate() should have sufficient room
        //  for (inl + cipher_block_size) bytes unless the cipher block size is 1 in which case inl bytes is sufficient."
        // So we need to create a buffer that is bigger than lLen.
        m_tempBuffer.resize(len + AES_BLOCK_SIZE);
        rc = EVP_DecryptUpdate(m_ctx, m_tempBuffer.data(), &outlen, (unsigned char*)buffer, (int)read);

        if (rc != 1)
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-decryption data");

        PODOFO_ASSERT((size_t)outlen <= len);
        std::memcpy(buffer, m_tempBuffer.data(), (size_t)outlen);

        if (m_inputLen == 0 || streameof)
        {
            m_inputEof = true;

            int drainLeft;
            rc = EVP_DecryptFinal_ex(m_ctx, m_tempBuffer.data(), &drainLeft);
            if (rc != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-decryption data padding");

            m_drainLeft = (size_t)drainLeft;
            goto DrainBuffer;
        }

        return outlen;

    DrainBuffer:
        drainLen = std::min(len - outlen, m_drainLeft);
        std::memcpy(buffer + outlen, m_tempBuffer.data(), drainLen);
        m_drainLeft -= (int)drainLen;
        if (m_drainLeft == 0)
            eof = true;

        return outlen + drainLen;
    }

private:
    EVP_CIPHER_CTX* m_ctx;
    PdfInputStream* m_pInputStream;
    size_t m_inputLen;
    bool m_inputEof;
    bool m_init;
    unsigned char m_key[32];
    unsigned m_keyLen;
    vector<unsigned char> m_tempBuffer;
    size_t m_drainLeft;
};

}

PdfEncrypt::~PdfEncrypt() { }

void PdfEncrypt::GenerateEncryptionKey(const PdfString& documentId)
{
    GenerateEncryptionKey(documentId.GetRawData());
}

bool PdfEncrypt::Authenticate(const string_view& password, const PdfString& documentId)
{
    return Authenticate(password, documentId.GetRawData());
}

EPdfEncryptAlgorithm PdfEncrypt::GetEnabledEncryptionAlgorithms()
{
    return PdfEncrypt::s_nEnabledEncryptionAlgorithms;
}

void PdfEncrypt::SetEnabledEncryptionAlgorithms(EPdfEncryptAlgorithm nEncryptionAlgorithms)
{
    PdfEncrypt::s_nEnabledEncryptionAlgorithms = nEncryptionAlgorithms;
}

bool PdfEncrypt::IsEncryptionEnabled(EPdfEncryptAlgorithm eAlgorithm)
{
    return (PdfEncrypt::s_nEnabledEncryptionAlgorithms & eAlgorithm) != EPdfEncryptAlgorithm::None;
}

static unsigned char padding[] =
"\x28\xBF\x4E\x5E\x4E\x75\x8A\x41\x64\x00\x4E\x56\xFF\xFA\x01\x08\x2E\x2E\x00\xB6\xD0\x68\x3E\x80\x2F\x0C\xA9\xFE\x64\x53\x69\x7A";

unique_ptr<PdfEncrypt> PdfEncrypt::CreatePdfEncrypt(const string_view& userPassword,
    const string_view& ownerPassword,
    EPdfPermissions protection,
    EPdfEncryptAlgorithm eAlgorithm,
    EPdfKeyLength eKeyLength)
{
    switch (eAlgorithm)
    {
#ifdef PODOFO_HAVE_LIBIDN
        case EPdfEncryptAlgorithm::AESV3:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(userPassword, ownerPassword, protection));
#endif // PODOFO_HAVE_LIBIDN
        case EPdfEncryptAlgorithm::RC4V2:
        case EPdfEncryptAlgorithm::RC4V1:
            return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(userPassword, ownerPassword, protection, eAlgorithm, eKeyLength));
        case EPdfEncryptAlgorithm::AESV2:
        default:
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(userPassword, ownerPassword, protection));
    }
}

unique_ptr<PdfEncrypt> PdfEncrypt::CreatePdfEncrypt(const PdfObject* pObject)
{
    if (!pObject->GetDictionary().HasKey("Filter") ||
        pObject->GetDictionary().GetKey("Filter")->GetName() != "Standard")
    {
        std::ostringstream oss;
        if (pObject->GetDictionary().HasKey("Filter"))
            oss << "Unsupported encryption filter: " << pObject->GetDictionary().GetKey("Filter")->GetName().GetString();
        else
            oss << "Encryption dictionary does not have a key /Filter.";

        PODOFO_RAISE_ERROR_INFO(EPdfError::UnsupportedFilter, oss.str().c_str());
    }

    int lV;
    int64_t lLength;
    int rValue;
    EPdfPermissions pValue;
    PdfString oValue;
    PdfString uValue;
    PdfName cfmName;
    bool encryptMetadata = true;

    try
    {
        lV = static_cast<int>(pObject->GetDictionary().MustGetKey("V").GetNumber());
        rValue = static_cast<int>(pObject->GetDictionary().MustGetKey("R").GetNumber());

        pValue = static_cast<EPdfPermissions>(pObject->GetDictionary().MustGetKey("P").GetNumber());

        oValue = pObject->GetDictionary().MustGetKey("O").GetString();
        uValue = pObject->GetDictionary().MustGetKey("U").GetString();

        if (pObject->GetDictionary().HasKey("Length"))
            lLength = pObject->GetDictionary().GetKey("Length")->GetNumber();
        else
            lLength = 0;

        const PdfObject* encryptMetadataObj = pObject->GetDictionary().GetKey("EncryptMetadata");
        if (encryptMetadataObj && encryptMetadataObj->IsBool())
            encryptMetadata = encryptMetadataObj->GetBool();

        const PdfObject* stmfObj = pObject->GetDictionary().GetKey("StmF");
        if (stmfObj && stmfObj->IsName())
        {
            const PdfObject* obj = pObject->GetDictionary().GetKey("CF");
            if (obj && obj->IsDictionary())
            {
                obj = obj->GetDictionary().GetKey(stmfObj->GetName());
                if (obj && obj->IsDictionary())
                {
                    obj = obj->GetDictionary().GetKey("CFM");
                    if (obj && obj->IsName())
                        cfmName = obj->GetName();
                }
            }
        }
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__, "Invalid or missing key in encryption dictionary");
        throw e;
    }

    if ((lV == 1) && (rValue == 2 || rValue == 3)
        && PdfEncrypt::IsEncryptionEnabled(EPdfEncryptAlgorithm::RC4V1))
    {
        return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(oValue, uValue, pValue, rValue, EPdfEncryptAlgorithm::RC4V1, (int)EPdfKeyLength::L40, encryptMetadata));
    }
    else if ((((lV == 2) && (rValue == 3)) || cfmName == "V2")
        && PdfEncrypt::IsEncryptionEnabled(EPdfEncryptAlgorithm::RC4V2))
    {
        // [Alexey] - lLength is int64_t. Please make changes in encryption algorithms
        return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(oValue, uValue, pValue, rValue, EPdfEncryptAlgorithm::RC4V2, static_cast<int>(lLength), encryptMetadata));
    }
    else
    {
        if ((lV == 4) && (rValue == 4)
            && PdfEncrypt::IsEncryptionEnabled(EPdfEncryptAlgorithm::AESV2))
        {
            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(oValue, uValue, pValue, encryptMetadata));
        }
#ifdef PODOFO_HAVE_LIBIDN
        else if ((lV == 5) && (rValue == 5)
            && PdfEncrypt::IsEncryptionEnabled(EPdfEncryptAlgorithm::AESV3))
        {
            PdfString permsValue = pObject->GetDictionary().GetKey("Perms")->GetString();
            PdfString oeValue = pObject->GetDictionary().GetKey("OE")->GetString();
            PdfString ueValue = pObject->GetDictionary().GetKey("UE")->GetString();

            return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(oValue, oeValue, uValue, ueValue, pValue, permsValue));
        }
#endif // PODOFO_HAVE_LIBIDN
        else
        {
            std::ostringstream oss;
            oss << "Unsupported encryption method Version=" << lV << " Revision=" << rValue;
            PODOFO_RAISE_ERROR_INFO(EPdfError::UnsupportedFilter, oss.str().c_str());
        }
    }
}

unique_ptr<PdfEncrypt> PdfEncrypt::CreatePdfEncrypt(const PdfEncrypt& rhs)
{
    if (rhs.m_eAlgorithm == EPdfEncryptAlgorithm::AESV2)
        return unique_ptr<PdfEncrypt>(new PdfEncryptAESV2(rhs));
#ifdef PODOFO_HAVE_LIBIDN
    else if (rhs.m_eAlgorithm == EPdfEncryptAlgorithm::AESV3)
        return unique_ptr<PdfEncrypt>(new PdfEncryptAESV3(rhs));
#endif // PODOFO_HAVE_LIBIDN
    else
        return unique_ptr<PdfEncrypt>(new PdfEncryptRC4(rhs));
}

PdfEncrypt::PdfEncrypt()
    : m_eAlgorithm(EPdfEncryptAlgorithm::AESV2), m_keyLength(0), m_rValue(0), m_pValue(EPdfPermissions::None),
    m_eKeyLength(EPdfKeyLength::L128), m_bEncryptMetadata(true)
{
    memset(m_uValue, 0, 48);
    memset(m_oValue, 0, 48);
    memset(m_encryptionKey, 0, 32);
};

PdfEncrypt::PdfEncrypt(const PdfEncrypt& rhs)
{
    m_eAlgorithm = rhs.m_eAlgorithm;
    m_eKeyLength = rhs.m_eKeyLength;

    m_pValue = rhs.m_pValue;
    m_rValue = rhs.m_rValue;

    m_keyLength = rhs.m_keyLength;

    m_curReference = rhs.m_curReference;
    m_documentId = rhs.m_documentId;
    m_userPass = rhs.m_userPass;
    m_ownerPass = rhs.m_ownerPass;
    m_bEncryptMetadata = rhs.m_bEncryptMetadata;
}

bool PdfEncrypt::CheckKey(unsigned char key1[32], unsigned char key2[32])
{
    // Check whether the right password had been given
    bool success = true;
    for (unsigned k = 0; success && k < m_keyLength; k++)
        success = success && (key1[k] == key2[k]);

    return success;
}

PdfEncryptMD5Base::PdfEncryptMD5Base()
    : m_rc4key{ }, m_rc4last{ }
{
}

PdfEncryptMD5Base::PdfEncryptMD5Base(const PdfEncrypt& rhs) : PdfEncrypt(rhs)
{
    const PdfEncrypt* ptr = &rhs;

    std::memcpy(m_uValue, rhs.GetUValue(), sizeof(unsigned char) * 32);
    std::memcpy(m_oValue, rhs.GetOValue(), sizeof(unsigned char) * 32);

    std::memcpy(m_encryptionKey, rhs.GetEncryptionKey(), sizeof(unsigned char) * 16);

    std::memcpy(m_rc4key, static_cast<const PdfEncryptMD5Base*>(ptr)->m_rc4key, sizeof(unsigned char) * 16);
    std::memcpy(m_rc4last, static_cast<const PdfEncryptMD5Base*>(ptr)->m_rc4last, sizeof(unsigned char) * 256);
    m_bEncryptMetadata = static_cast<const PdfEncryptMD5Base*>(ptr)->m_bEncryptMetadata;
}

void PdfEncryptMD5Base::PadPassword(const std::string_view& password, unsigned char pswd[32])
{
    size_t m = password.length();

    if (m > 32) m = 32;

    size_t j;
    size_t p = 0;
    for (j = 0; j < m; j++)
        pswd[p++] = static_cast<unsigned char>(password[j]);

    for (j = 0; p < 32 && j < 32; j++)
        pswd[p++] = padding[j];
}

bool PdfEncryptMD5Base::Authenticate(const std::string& documentID, const std::string& password,
    const std::string& uValue, const std::string& oValue,
    EPdfPermissions pValue, int lengthValue, int rValue)
{
    m_pValue = pValue;
    m_keyLength = lengthValue / 8;
    m_rValue = rValue;

    std::memcpy(m_uValue, uValue.c_str(), 32);
    std::memcpy(m_oValue, oValue.c_str(), 32);

    return Authenticate(password, documentID);
}

void PdfEncryptMD5Base::ComputeOwnerKey(const unsigned char userPad[32], const unsigned char ownerPad[32],
    int keyLength, int revision, bool authenticate, unsigned char ownerKey[32])
{
    unsigned char mkey[MD5_DIGEST_LENGTH];
    unsigned char digest[MD5_DIGEST_LENGTH];

    MD5_CTX ctx;
    int status = MD5_Init(&ctx);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing MD5 hashing engine");
    status = MD5_Update(&ctx, ownerPad, 32);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");
    status = MD5_Final(digest, &ctx);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

    if ((revision == 3) || (revision == 4))
    {
        // only use for the input as many bit as the key consists of
        for (int k = 0; k < 50; ++k)
        {
            status = MD5_Init(&ctx);
            if (status != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing MD5 hashing engine");
            status = MD5_Update(&ctx, digest, keyLength);
            if (status != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");
            status = MD5_Final(digest, &ctx);
            if (status != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");
        }
        std::memcpy(ownerKey, userPad, 32);
        for (unsigned i = 0; i < 20; ++i)
        {
            for (int j = 0; j < keyLength; ++j)
            {
                if (authenticate)
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned>(digest[j] ^ (19 - i)));
                else
                    mkey[j] = static_cast<unsigned char>(static_cast<unsigned>(digest[j] ^ i));
            }
            RC4(mkey, keyLength, ownerKey, 32, ownerKey, 32);
        }
    }
    else
    {
        RC4(digest, 5, userPad, 32, ownerKey, 32);
    }
}

void PdfEncryptMD5Base::ComputeEncryptionKey(const std::string& documentId,
    const unsigned char userPad[32], const unsigned char ownerKey[32],
    EPdfPermissions pValue, EPdfKeyLength keyLength, int revision,
    unsigned char userKey[32], bool encryptMetadata)
{
    unsigned j;
    unsigned k;
    m_keyLength = (int)keyLength / 8;

    MD5_CTX ctx;
    int status = MD5_Init(&ctx);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing MD5 hashing engine");

    status = MD5_Update(&ctx, userPad, 32);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

    status = MD5_Update(&ctx, ownerKey, 32);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

    unsigned char ext[4];
    ext[0] = static_cast<unsigned char> (((unsigned)pValue >> 0) & 0xFF);
    ext[1] = static_cast<unsigned char> (((unsigned)pValue >> 8) & 0xFF);
    ext[2] = static_cast<unsigned char> (((unsigned)pValue >> 16) & 0xFF);
    ext[3] = static_cast<unsigned char> (((unsigned)pValue >> 24) & 0xFF);
    status = MD5_Update(&ctx, ext, 4);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

    unsigned docIdLength = static_cast<unsigned>(documentId.length());
    unsigned char* docId = nullptr;
    if (docIdLength > 0)
    {
        docId = new unsigned char[docIdLength];
        size_t j;
        for (j = 0; j < docIdLength; j++)
        {
            docId[j] = static_cast<unsigned char>(documentId[j]);
        }
        status = MD5_Update(&ctx, docId, docIdLength);
        if (status != 1)
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");
    }

    // If document metadata is not being encrypted, 
    // pass 4 bytes with the value 0xFFFFFFFF to the MD5 hash function.
    if (!encryptMetadata)
    {
        unsigned char noMetaAddition[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        status = MD5_Update(&ctx, noMetaAddition, 4);
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    status = MD5_Final(digest, &ctx);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

    // only use the really needed bits as input for the hash
    if (revision == 3 || revision == 4)
    {
        for (k = 0; k < 50; ++k)
        {
            status = MD5_Init(&ctx);
            if (status != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing MD5 hashing engine");

            status = MD5_Update(&ctx, digest, m_keyLength);
            if (status != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

            status = MD5_Final(digest, &ctx);
            if (status != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");
        }
    }

    std::memcpy(m_encryptionKey, digest, m_keyLength);

    // Setup user key
    if (revision == 3 || revision == 4)
    {
        status = MD5_Init(&ctx);
        if (status != 1)
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing MD5 hashing engine");

        status = MD5_Update(&ctx, padding, 32);
        if (status != 1)
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

        if (docId != nullptr)
        {
            status = MD5_Update(&ctx, docId, docIdLength);
            if (status != 1)
                PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");
        }

        status = MD5_Final(digest, &ctx);
        if (status != 1)
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

        std::memcpy(userKey, digest, 16);
        for (k = 16; k < 32; ++k)
            userKey[k] = 0;

        for (k = 0; k < 20; k++)
        {
            for (j = 0; j < m_keyLength; ++j)
            {
                digest[j] = static_cast<unsigned char>(m_encryptionKey[j] ^ k);
            }

            RC4(digest, m_keyLength, userKey, 16, userKey, 16);
        }
    }
    else
    {
        RC4(m_encryptionKey, m_keyLength, padding, 32, userKey, 32);
    }

    if (docId != nullptr)
        delete[] docId;
}

void PdfEncryptMD5Base::CreateObjKey(unsigned char objkey[16], unsigned& pnKeyLen) const
{
    const unsigned n = static_cast<unsigned>(m_curReference.ObjectNumber());
    const unsigned g = static_cast<unsigned>(m_curReference.GenerationNumber());

    unsigned nkeylen = m_keyLength + 5;
    unsigned char nkey[MD5_DIGEST_LENGTH + 5 + 4];
    for (unsigned j = 0; j < m_keyLength; j++)
        nkey[j] = m_encryptionKey[j];

    nkey[m_keyLength + 0] = static_cast<unsigned char>(0xFF & n);
    nkey[m_keyLength + 1] = static_cast<unsigned char>(0xFF & (n >> 8));
    nkey[m_keyLength + 2] = static_cast<unsigned char>(0xFF & (n >> 16));
    nkey[m_keyLength + 3] = static_cast<unsigned char>(0xFF & g);
    nkey[m_keyLength + 4] = static_cast<unsigned char>(0xFF & (g >> 8));

    if (m_eAlgorithm == EPdfEncryptAlgorithm::AESV2)
    {
        // AES encryption needs some 'salt'
        nkeylen += 4;
        nkey[m_keyLength + 5] = 0x73;
        nkey[m_keyLength + 6] = 0x41;
        nkey[m_keyLength + 7] = 0x6C;
        nkey[m_keyLength + 8] = 0x54;
    }

    GetMD5Binary(nkey, nkeylen, objkey);
    pnKeyLen = (m_keyLength <= 11) ? m_keyLength + 5 : 16;
}

PdfEncryptRC4Base::PdfEncryptRC4Base()
{
    m_rc4 = new RC4CryptoEngine();
}

PdfEncryptRC4Base::~PdfEncryptRC4Base()
{
    delete m_rc4;
}
    
/**
 * RC4 is the standard encryption algorithm used in PDF format
 */

void PdfEncryptRC4Base::RC4(const unsigned char* key, unsigned keylen,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen) const
{
    EVP_CIPHER_CTX* rc4 = m_rc4->getEngine();

    if (textlen != textoutlen)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing RC4 encryption engine");

    // Don't set the key because we will modify the parameters
    int status = EVP_EncryptInit_ex(rc4, EVP_rc4(), nullptr, nullptr, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing RC4 encryption engine");

    status = EVP_CIPHER_CTX_set_key_length(rc4, keylen);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing RC4 encryption engine");

    // We finished modifying parameters so now we can set the key
    status = EVP_EncryptInit_ex(rc4, nullptr, nullptr, key, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing RC4 encryption engine");

    int dataOutMoved;
    status = EVP_EncryptUpdate(rc4, textout, &dataOutMoved, textin, (int)textlen);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error RC4-encrypting data");

    status = EVP_EncryptFinal_ex(rc4, &textout[dataOutMoved], &dataOutMoved);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error RC4-encrypting data");
}
        
void PdfEncryptMD5Base::GetMD5Binary(const unsigned char* data, int length, unsigned char* digest)
{
    int status;
    MD5_CTX ctx;
    status = MD5_Init(&ctx);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing MD5 hashing engine");

    status = MD5_Update(&ctx, data, length);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");

    status = MD5_Final(digest, &ctx);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error MD5-hashing data");
}

void PdfEncryptMD5Base::GenerateInitialVector(unsigned char iv[]) const
{
    GetMD5Binary(reinterpret_cast<const unsigned char*>(m_documentId.c_str()),
        static_cast<unsigned>(m_documentId.length()), iv);
}
    
PdfString PdfEncryptMD5Base::GetMD5String(const unsigned char* pBuffer, int nLength)
{
    char data[MD5_DIGEST_LENGTH];

    GetMD5Binary(pBuffer, nLength, reinterpret_cast<unsigned char*>(data));

    return PdfString::FromRaw({ data, MD5_DIGEST_LENGTH });
}
    
void PdfEncryptMD5Base::CreateEncryptionDictionary(PdfDictionary& rDictionary) const
{
    rDictionary.AddKey("Filter", PdfName("Standard"));

    if (m_eAlgorithm == EPdfEncryptAlgorithm::AESV2 || !m_bEncryptMetadata)
    {
        PdfDictionary cf;
        PdfDictionary stdCf;

        if (m_eAlgorithm == EPdfEncryptAlgorithm::RC4V2)
            stdCf.AddKey("CFM", PdfName("V2"));
        else
            stdCf.AddKey("CFM", PdfName("AESV2"));
        stdCf.AddKey("Length", static_cast<int64_t>(16));

        rDictionary.AddKey("O", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValue()), 32 }));
        rDictionary.AddKey("U", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValue()), 32 }));

        stdCf.AddKey("AuthEvent", PdfName("DocOpen"));
        cf.AddKey("StdCF", stdCf);

        rDictionary.AddKey("CF", cf);
        rDictionary.AddKey("StrF", PdfName("StdCF"));
        rDictionary.AddKey("StmF", PdfName("StdCF"));

        rDictionary.AddKey("V", static_cast<int64_t>(4));
        rDictionary.AddKey("R", static_cast<int64_t>(4));
        rDictionary.AddKey("Length", static_cast<int64_t>(128));
        if (!m_bEncryptMetadata)
            rDictionary.AddKey("EncryptMetadata", PdfVariant(false));
    }
    else if (m_eAlgorithm == EPdfEncryptAlgorithm::RC4V1)
    {
        rDictionary.AddKey("V", static_cast<int64_t>(1));
        // Can be 2 or 3
        rDictionary.AddKey("R", static_cast<int64_t>(m_rValue));
    }
    else if (m_eAlgorithm == EPdfEncryptAlgorithm::RC4V2)
    {
        rDictionary.AddKey("V", static_cast<int64_t>(2));
        rDictionary.AddKey("R", static_cast<int64_t>(3));
        rDictionary.AddKey("Length", PdfVariant(static_cast<int64_t>(m_eKeyLength)));
    }

    rDictionary.AddKey("O", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValue()), 32 }));
    rDictionary.AddKey("U", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValue()), 32 }));
    rDictionary.AddKey("P", PdfVariant(static_cast<int64_t>(this->GetPValue())));
}
    
void PdfEncryptRC4::GenerateEncryptionKey(const string_view& documentId)
{
    unsigned char userpswd[32];
    unsigned char ownerpswd[32];

    // Pad passwords
    PadPassword(m_userPass, userpswd);
    PadPassword(m_ownerPass, ownerpswd);

    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, m_keyLength, m_rValue, false, m_oValue);

    // Compute encryption key and U value
    m_documentId = documentId;
    ComputeEncryptionKey(m_documentId, userpswd,
        m_oValue, m_pValue, m_eKeyLength, m_rValue, m_uValue, m_bEncryptMetadata);
}
    
bool PdfEncryptRC4::Authenticate(const string_view& password, const std::string_view& documentId)
{
    bool success = false;

    m_documentId = documentId;

    // Pad password
    unsigned char userKey[32];
    unsigned char pswd[32];
    PadPassword(password, pswd);

    // Check password: 1) as user password, 2) as owner password
    ComputeEncryptionKey(m_documentId, pswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata);

    success = CheckKey(userKey, m_uValue);
    if (!success)
    {
        unsigned char userpswd[32];
        ComputeOwnerKey(m_oValue, pswd, m_keyLength, m_rValue, true, userpswd);
        ComputeEncryptionKey(m_documentId, userpswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata);
        success = CheckKey(userKey, m_uValue);

        if (success)
            m_ownerPass = password;
    }
    else
        m_userPass = password;

    return success;
}

size_t PdfEncryptRC4::CalculateStreamOffset() const
{
    return 0;
}

size_t PdfEncryptRC4::CalculateStreamLength(size_t length) const
{
    return length;
}

void PdfEncryptRC4::Encrypt(const char* inStr, size_t inLen,
    char* outStr, size_t outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen);
    this->RC4(objkey, keylen, (const unsigned char *)inStr, inLen,
        (unsigned char*)outStr, outLen);
}

void PdfEncryptRC4::Decrypt(const char* inStr, size_t inLen,
    char* outStr, size_t& outLen) const
{
    Encrypt(inStr, inLen, outStr, outLen);
}

unique_ptr<PdfInputStream> PdfEncryptRC4::CreateEncryptionInputStream(PdfInputStream& pInputStream, size_t inputLen)
{
    (void)inputLen;
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen);
    return unique_ptr<PdfInputStream>(new PdfRC4InputStream(pInputStream, inputLen, m_rc4key, m_rc4last, objkey, keylen));
}

PdfEncryptRC4::PdfEncryptRC4(PdfString oValue, PdfString uValue, EPdfPermissions pValue, int rValue,
    EPdfEncryptAlgorithm eAlgorithm, int length, bool encryptMetadata)
{
    m_pValue = pValue;
    m_rValue = rValue;
    m_eAlgorithm = eAlgorithm;
    m_eKeyLength = static_cast<EPdfKeyLength>(length);
    m_keyLength = length / 8;
    m_bEncryptMetadata = encryptMetadata;
    std::memcpy(m_oValue, oValue.GetRawData().data(), 32);
    std::memcpy(m_uValue, uValue.GetRawData().data(), 32);

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_encryptionKey, 0, 32);
}

PdfEncryptRC4::PdfEncryptRC4(const string_view& userPassword, const string_view& ownerPassword, EPdfPermissions protection,
    EPdfEncryptAlgorithm eAlgorithm, EPdfKeyLength eKeyLength)
{
    // setup object
    int keyLength = static_cast<int>(eKeyLength);

    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_eAlgorithm = eAlgorithm;
    m_eKeyLength = eKeyLength;

    switch (eAlgorithm)
    {
        case EPdfEncryptAlgorithm::RC4V2:
        {
            keyLength = keyLength - keyLength % 8;
            keyLength = (keyLength >= 40) ? ((keyLength <= 128) ? keyLength : 128) : 40;
            m_rValue = 3;
            m_keyLength = keyLength / 8;
            break;
        }
        case EPdfEncryptAlgorithm::RC4V1:
        default:
        {
            m_rValue = 2;
            m_keyLength = 40 / 8;
            break;
        }
        case EPdfEncryptAlgorithm::AESV2:
#ifdef PODOFO_HAVE_LIBIDN
        case EPdfEncryptAlgorithm::AESV3:
#endif // PODOFO_HAVE_LIBIDN
        {
            break;
        }
    }

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_oValue, 0, 48);
    std::memset(m_uValue, 0, 48);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_encryptionKey, 0, 32);

    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}

unique_ptr<PdfOutputStream> PdfEncryptRC4::CreateEncryptionOutputStream(PdfOutputStream& pOutputStream)
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen);
    return unique_ptr<PdfOutputStream>(new PdfRC4OutputStream(pOutputStream, m_rc4key, m_rc4last, objkey, keylen));
}
    
PdfEncryptAESBase::PdfEncryptAESBase()
{
    m_aes = new AESCryptoEngine();
}

PdfEncryptAESBase::~PdfEncryptAESBase()
{
    delete m_aes;
}

void PdfEncryptAESBase::BaseDecrypt(const unsigned char* key, unsigned keyLen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t& outLen) const
{
    if ((textlen % 16) != 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-decryption data length not a multiple of 16");

    EVP_CIPHER_CTX* aes = m_aes->getEngine();

    int rc;
    if (keyLen == (int)EPdfKeyLength::L128 / 8)
        rc = EVP_DecryptInit_ex(aes, EVP_aes_128_cbc(), nullptr, key, iv);
#ifdef PODOFO_HAVE_LIBIDN
    else if (keyLen == (int)EPdfKeyLength::L256 / 8)
        status = EVP_DecryptInit_ex(aes, EVP_aes_256_cbc(), nullptr, key, iv);
#endif
    else
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Invalid AES key length");

    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing AES decryption engine");

    int dataOutMoved;
    rc = EVP_DecryptUpdate(aes, textout, &dataOutMoved, textin, (int)textlen);
    outLen = dataOutMoved;
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-decryption data");

    rc = EVP_DecryptFinal_ex(aes, textout + outLen, &dataOutMoved);
    outLen += dataOutMoved;
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-decryption data final");
}

void PdfEncryptAESBase::BaseEncrypt(const unsigned char* key, unsigned keyLen, const unsigned char* iv,
    const unsigned char* textin, size_t textlen,
    unsigned char* textout, size_t textoutlen) const
{
    (void)textoutlen;
    EVP_CIPHER_CTX* aes = m_aes->getEngine();

    int rc;
    if (keyLen == (int)EPdfKeyLength::L128 / 8)
        rc = EVP_EncryptInit_ex(aes, EVP_aes_128_cbc(), nullptr, key, iv);
#ifdef PODOFO_HAVE_LIBIDN
    else if (keyLen == (int)EPdfKeyLength::L256 / 8)
        status = EVP_EncryptInit_ex(aes, EVP_aes_256_cbc(), nullptr, key, iv);
#endif
    else
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Invalid AES key length");

    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing AES encryption engine");

    int dataOutMoved;
    rc = EVP_EncryptUpdate(aes, textout, &dataOutMoved, textin, (int)textlen);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");

    rc = EVP_EncryptFinal_ex(aes, &textout[dataOutMoved], &dataOutMoved);
    if (rc != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");
}
    
void PdfEncryptAESV2::GenerateEncryptionKey(const string_view& documentId)
{
    unsigned char userpswd[32];
    unsigned char ownerpswd[32];

    // Pad passwords
    PadPassword(m_userPass, userpswd);
    PadPassword(m_ownerPass, ownerpswd);

    // Compute O value
    ComputeOwnerKey(userpswd, ownerpswd, m_keyLength, m_rValue, false, m_oValue);

    // Compute encryption key and U value
    m_documentId = documentId;
    ComputeEncryptionKey(m_documentId, userpswd,
        m_oValue, m_pValue, m_eKeyLength, m_rValue, m_uValue, m_bEncryptMetadata);
}

bool PdfEncryptAESV2::Authenticate(const string_view& password, const string_view& documentId)
{
    m_documentId = documentId;

    // Pad password
    unsigned char pswd[32];
    PadPassword(password, pswd);

    // Check password: 1) as user password, 2) as owner password
    unsigned char userKey[32];
    ComputeEncryptionKey(m_documentId, pswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata);

    bool success = CheckKey(userKey, m_uValue);
    if (success)
    {
        m_userPass = password;
    }
    else
    {
        unsigned char userpswd[32];
        ComputeOwnerKey(m_oValue, pswd, m_keyLength, m_rValue, true, userpswd);
        ComputeEncryptionKey(m_documentId, userpswd, m_oValue, m_pValue, m_eKeyLength, m_rValue, userKey, m_bEncryptMetadata);
        success = CheckKey(userKey, m_uValue);

        if (success)
            m_ownerPass = password;
    }

    return success;
}
    
size_t PdfEncryptAESV2::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}
    
void PdfEncryptAESV2::Encrypt(const char* inStr, size_t inLen,
    char* outStr, size_t outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen);
    size_t offset = CalculateStreamOffset();
    this->GenerateInitialVector((unsigned char *)outStr);
    this->BaseEncrypt(objkey, keylen, (unsigned char*)outStr, (const unsigned char*)inStr,
        inLen, (unsigned char*)outStr + offset, outLen - offset);
}

void PdfEncryptAESV2::Decrypt(const char* inStr, size_t inLen,
    char* outStr, size_t& outLen) const
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    CreateObjKey(objkey, keylen);

    size_t offset = CalculateStreamOffset();
    if (inLen <= offset)
    {
        // Is empty
        outLen = 0;
        return;
    }

    this->BaseDecrypt(objkey, keylen, (const unsigned char*)inStr,
        (const unsigned char*)inStr + offset,
        inLen - offset, (unsigned char*)outStr, outLen);
}
    
PdfEncryptAESV2::PdfEncryptAESV2(const string_view& userPassword, const string_view& ownerPassword, EPdfPermissions protection) : PdfEncryptAESBase()
{
    // setup object
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_eAlgorithm = EPdfEncryptAlgorithm::AESV2;

    m_rValue = 4;
    m_eKeyLength = EPdfKeyLength::L128;
    m_keyLength = (int)EPdfKeyLength::L128 / 8;

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_oValue, 0, 48);
    std::memset(m_uValue, 0, 48);
    std::memset(m_encryptionKey, 0, 32);

    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}
    
PdfEncryptAESV2::PdfEncryptAESV2(PdfString oValue, PdfString uValue, EPdfPermissions pValue, bool encryptMetadata) : PdfEncryptAESBase()
{
    m_pValue = pValue;
    m_eAlgorithm = EPdfEncryptAlgorithm::AESV2;

    m_eKeyLength = EPdfKeyLength::L128;
    m_keyLength = (int)EPdfKeyLength::L128 / 8;
    m_rValue = 4;
    m_bEncryptMetadata = encryptMetadata;
    std::memcpy(m_oValue, oValue.GetRawData().data(), 32);
    std::memcpy(m_uValue, uValue.GetRawData().data(), 32);

    // Init buffers
    std::memset(m_rc4key, 0, 16);
    std::memset(m_rc4last, 0, 256);
    std::memset(m_encryptionKey, 0, 32);
}

size_t PdfEncryptAESV2::CalculateStreamLength(size_t length) const
{
    size_t realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
        realLength += 16;

    return realLength;
}
    
unique_ptr<PdfInputStream> PdfEncryptAESV2::CreateEncryptionInputStream(PdfInputStream& pInputStream, size_t inputLen)
{
    unsigned char objkey[MD5_DIGEST_LENGTH];
    unsigned keylen;
    this->CreateObjKey(objkey, keylen);
    return unique_ptr<PdfInputStream>(new PdfAESInputStream(pInputStream, inputLen, objkey, keylen));
}
    
unique_ptr<PdfOutputStream> PdfEncryptAESV2::CreateEncryptionOutputStream(PdfOutputStream&)
{
    /*unsigned char objkey[MD5_DIGEST_LENGTH];
     int keylen;

     this->CreateObjKey( objkey, &keylen );*/

    PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "CreateEncryptionOutputStream does not yet support AESV2");
}
    
#ifdef PODOFO_HAVE_LIBIDN
    
PdfEncryptSHABase::PdfEncryptSHABase()
    : m_ueValue{ }, m_oeValue{ }, m_permsValue{ }
{
}

PdfEncryptSHABase::PdfEncryptSHABase(const PdfEncrypt& rhs) : PdfEncrypt(rhs)
{
    const PdfEncrypt* ptr = &rhs;

    std::memcpy(m_uValue, rhs.GetUValue(), sizeof(unsigned char) * 48);
    std::memcpy(m_oValue, rhs.GetOValue(), sizeof(unsigned char) * 48);

    std::memcpy(m_encryptionKey, rhs.GetEncryptionKey(), sizeof(unsigned char) * 32);

    std::memcpy(m_permsValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_permsValue, sizeof(unsigned char) * 16);

    std::memcpy(m_ueValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_ueValue, sizeof(unsigned char) * 32);
    std::memcpy(m_oeValue, static_cast<const PdfEncryptSHABase*>(ptr)->m_oeValue, sizeof(unsigned char) * 32);
}
    
void PdfEncryptSHABase::ComputeUserKey(const unsigned char* userpswd, size_t len)
{
    // Generate User Salts
    unsigned char vSalt[8];
    unsigned char kSalt[8];

    for (int i = 0; i < 8; i++)
    {
        vSalt[i] = rand() % 255;
        kSalt[i] = rand() % 255;
    }

    // Generate hash for U
    unsigned char hashValue[32];

    SHA256_CTX context;
    SHA256_Init(&context);

    SHA256_Update(&context, userpswd, len);
    SHA256_Update(&context, vSalt, 8);
    SHA256_Final(hashValue, &context);

    // U = hash + validation salt + key salt
    std::memcpy(m_uValue, hashValue, 32);
    std::memcpy(m_uValue + 32, vSalt, 8);
    std::memcpy(m_uValue + 32 + 8, kSalt, 8);

    // Generate hash for UE
    SHA256_Init(&context);
    SHA256_Update(&context, userpswd, len);
    SHA256_Update(&context, kSalt, 8);
    SHA256_Final(hashValue, &context);

    // UE = AES-256 encoded file encryption key with key=hash
    // CBC mode, no padding, init vector=0

    EVP_CIPHER_CTX* aes;
    aes = EVP_CIPHER_CTX_new();

    int status = EVP_EncryptInit_ex(aes, EVP_aes_256_cbc(), nullptr, hashValue, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing AES encryption engine");
    EVP_CIPHER_CTX_set_padding(aes, 0); // disable padding

    int dataOutMoved;
    status = EVP_EncryptUpdate(aes, m_ueValue, &dataOutMoved, m_encryptionKey, m_keyLength);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");

    status = EVP_EncryptFinal_ex(aes, &m_ueValue[dataOutMoved], &dataOutMoved);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");

    EVP_CIPHER_CTX_free(aes);
}

void PdfEncryptSHABase::ComputeOwnerKey(const unsigned char* ownerpswd, size_t len)
{
    // Generate User Salts
    unsigned char vSalt[8];
    unsigned char kSalt[8];

    for (int i = 0; i < 8; i++)
    {
        vSalt[i] = rand() % 255;
        kSalt[i] = rand() % 255;
    }

    // Generate hash for O
    unsigned char hashValue[32];
    SHA256_CTX context;
    SHA256_Init(&context);
    SHA256_Update(&context, ownerpswd, len);
    SHA256_Update(&context, vSalt, 8);
    SHA256_Update(&context, m_uValue, 48);
    SHA256_Final(hashValue, &context);

    // O = hash + validation salt + key salt
    std::memcpy(m_oValue, hashValue, 32);
    std::memcpy(m_oValue + 32, vSalt, 8);
    std::memcpy(m_oValue + 32 + 8, kSalt, 8);

    // Generate hash for OE
    SHA256_Init(&context);
    SHA256_Update(&context, ownerpswd, len);
    SHA256_Update(&context, kSalt, 8);
    SHA256_Update(&context, m_uValue, 48);
    SHA256_Final(hashValue, &context);

    // OE = AES-256 encoded file encryption key with key=hash
    // CBC mode, no padding, init vector=0

    EVP_CIPHER_CTX* aes;
    aes = EVP_CIPHER_CTX_new();

    int status = EVP_EncryptInit_ex(aes, EVP_aes_256_cbc(), nullptr, hashValue, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing AES encryption engine");
    EVP_CIPHER_CTX_set_padding(aes, 0); // disable padding

    int dataOutMoved;
    status = EVP_EncryptUpdate(aes, m_oeValue, &dataOutMoved, m_encryptionKey, m_keyLength);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");

    status = EVP_EncryptFinal_ex(aes, &m_oeValue[dataOutMoved], &dataOutMoved);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");

    EVP_CIPHER_CTX_free(aes);
}

void PdfEncryptSHABase::PreprocessPassword(const std::string_view& password, unsigned char* outBuf, size_t& len)
{
    char* password_sasl;

    if (stringprep_profile(password.data(), &password_sasl, "SASLprep", STRINGPREP_NO_UNASSIGNED) != STRINGPREP_OK)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidPassword, "Error processing password through SASLprep");
    }

    size_t l = strlen(password_sasl);
    len = l > 127 ? 127 : l;

    std::memcpy(outBuf, password_sasl, len);
    free(password_sasl);
}

void PdfEncryptSHABase::ComputeEncryptionKey()
{
    // Seed once for all
    srand((unsigned)time(nullptr));

    for (int i = 0; i < m_keyLength; i++)
        m_encryptionKey[i] = rand() % 255;
}
    
bool PdfEncryptSHABase::Authenticate(const std::string& documentID, const std::string& password,
    const std::string& uValue, const std::string& ueValue,
    const std::string& oValue, const std::string& oeValue,
    EPdfPermissions pValue, const std::string& permsValue,
    int lengthValue, int rValue)
{
    m_pValue = pValue;
    m_keyLength = lengthValue / 8;
    m_rValue = rValue;

    std::memcpy(m_uValue, uValue.c_str(), 48);
    std::memcpy(m_ueValue, ueValue.c_str(), 32);
    std::memcpy(m_oValue, oValue.c_str(), 48);
    std::memcpy(m_oeValue, oeValue.c_str(), 32);
    std::memcpy(m_permsValue, permsValue.c_str(), 16);

    return Authenticate(password, documentID);
}
    
void PdfEncryptSHABase::GenerateInitialVector(unsigned char iv[]) const
{
    for (int i = 0; i < AES_IV_LENGTH; i++)
        iv[i] = rand() % 255;
}
    
void PdfEncryptSHABase::CreateEncryptionDictionary(PdfDictionary& rDictionary) const
{
    rDictionary.AddKey("Filter", PdfName("Standard"));

    PdfDictionary cf;
    PdfDictionary stdCf;

    rDictionary.AddKey("V", static_cast<int64_t>(5));
    rDictionary.AddKey("R", static_cast<int64_t>(5));
    rDictionary.AddKey("Length", static_cast<int64_t>(256));

    stdCf.AddKey("CFM", PdfName("AESV3"));
    stdCf.AddKey("Length", static_cast<int64_t>(32));

    rDictionary.AddKey("O", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOValue()), 48 }));
    rDictionary.AddKey("OE", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetOEValue()), 32 }));
    rDictionary.AddKey("U", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUValue()), 48 }));
    rDictionary.AddKey("UE", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetUEValue()), 32 }));
    rDictionary.AddKey("Perms", PdfString::FromRaw({ reinterpret_cast<const char*>(this->GetPermsValue()), 16 }));

    stdCf.AddKey("AuthEvent", PdfName("DocOpen"));
    cf.AddKey("StdCF", stdCf);

    rDictionary.AddKey("CF", cf);
    rDictionary.AddKey("StrF", PdfName("StdCF"));
    rDictionary.AddKey("StmF", PdfName("StdCF"));

    rDictionary.AddKey("P", PdfVariant(static_cast<int64_t>(this->GetPValue())));
}
    
void PdfEncryptAESV3::GenerateEncryptionKey(const string_view& documentId)
{
    (void)documentId;

    // Prepare passwords
    unsigned char userpswd[127];
    unsigned char ownerpswd[127];
    size_t userpswdLen;
    size_t ownerpswdLen;
    PreprocessPassword(m_userPass, userpswd, userpswdLen);
    PreprocessPassword(m_ownerPass, ownerpswd, ownerpswdLen);

    // Compute encryption key
    ComputeEncryptionKey();

    // Compute U and UE values
    ComputeUserKey(userpswd, userpswdLen);

    // Compute O and OE value
    ComputeOwnerKey(ownerpswd, ownerpswdLen);

    // Compute Perms value
    unsigned char perms[16];
    // First 4 bytes = 32bits permissions
    perms[3] = ((unsigned)m_pValue >> 24) & 0xFF;
    perms[2] = ((unsigned)m_pValue >> 16) & 0xFF;
    perms[1] = ((unsigned)m_pValue >> 8) & 0xFF;
    perms[0] = ((unsigned)m_pValue >> 0) & 0xFF;
    // Placeholder for future versions that may need 64-bit permissions
    perms[4] = 0xFF;
    perms[5] = 0xFF;
    perms[6] = 0xFF;
    perms[7] = 0xFF;
    // if EncryptMetadata is false, this value should be set to 'F'
    perms[8] = m_bEncryptMetadata ? 'T' : 'F';
    // Next 3 bytes are mandatory
    perms[9] = 'a';
    perms[10] = 'd';
    perms[11] = 'b';
    // Next 4 bytes are ignored
    perms[12] = 0;
    perms[13] = 0;
    perms[14] = 0;
    perms[15] = 0;

    // Encrypt Perms value

    EVP_CIPHER_CTX* aes;
    aes = EVP_CIPHER_CTX_new();

    int status = EVP_EncryptInit_ex(aes, EVP_aes_256_ecb(), nullptr, m_encryptionKey, nullptr);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error initializing AES encryption engine");
    EVP_CIPHER_CTX_set_padding(aes, 0); // disable padding

    int dataOutMoved;
    status = EVP_EncryptUpdate(aes, m_permsValue, &dataOutMoved, perms, 16);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");

    status = EVP_EncryptFinal_ex(aes, &m_permsValue[dataOutMoved], &dataOutMoved);
    if (status != 1)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Error AES-encrypting data");

    EVP_CIPHER_CTX_free(aes);
}

bool PdfEncryptAESV3::Authenticate(const std::string_view& password, const std::string_view& documentId)
{
    (void)documentId;
    bool success = false;

    // Prepare password
    unsigned char pswd_sasl[127];
    size_t pswdLen;
    PreprocessPassword(password, pswd_sasl, pswdLen);

    // Test 1: is it the user key ?
    unsigned char hashValue[32];
    SHA256_CTX context;
    SHA256_Init(&context);
    SHA256_Update(&context, pswd_sasl, pswdLen); // password
    SHA256_Update(&context, m_uValue + 32, 8); // user Validation Salt
    SHA256_Final(hashValue, &context);

    success = CheckKey(hashValue, m_uValue);
    if (!success)
    {
        // Test 2: is it the owner key ?
        SHA256_Init(&context);
        SHA256_Update(&context, pswd_sasl, pswdLen); // password
        SHA256_Update(&context, m_oValue + 32, 8); // owner Validation Salt
        SHA256_Update(&context, m_uValue, 48); // U string
        SHA256_Final(hashValue, &context);

        success = CheckKey(hashValue, m_oValue);

        if (success)
        {
            m_ownerPass = password;
            // ISO 32000: "Compute an intermediate owner key by computing the SHA-256 hash of
            // the UTF-8 password concatenated with the 8 bytes of owner Key Salt, concatenated with the 48-byte U string."
            SHA256_Init(&context);
            SHA256_Update(&context, pswd_sasl, pswdLen); // password
            SHA256_Update(&context, m_oValue + 40, 8); // owner Key Salt
            SHA256_Update(&context, m_uValue, 48); // U string
            SHA256_Final(hashValue, &context);

            // ISO 32000: "The 32-byte result is the key used to decrypt the 32-byte OE string using
            // AES-256 in CBC mode with no padding and an initialization vector of zero.
            // The 32-byte result is the file encryption key"
            EVP_CIPHER_CTX* aes = m_aes->getEngine();
            EVP_DecryptInit_ex(aes, EVP_aes_256_cbc(), nullptr, hashValue, 0); // iv zero
            EVP_CIPHER_CTX_set_padding(aes, 0); // no padding
            int lOutLen;
            EVP_DecryptUpdate(aes, m_encryptionKey, &lOutLen, m_oeValue, 32);
        }
    }
    else
    {
        m_userPass = password;
        // ISO 32000: "Compute an intermediate user key by computing the SHA-256 hash of
        // the UTF-8 password concatenated with the 8 bytes of user Key Salt"
        SHA256_Init(&context);
        SHA256_Update(&context, pswd_sasl, pswdLen); // password
        SHA256_Update(&context, m_uValue + 40, 8); // user Key Salt
        SHA256_Final(hashValue, &context);

        // ISO 32000: "The 32-byte result is the key used to decrypt the 32-byte UE string using
        // AES-256 in CBC mode with no padding and an initialization vector of zero.
        // The 32-byte result is the file encryption key"
        EVP_CIPHER_CTX* aes = m_aes->getEngine();
        EVP_DecryptInit_ex(aes, EVP_aes_256_cbc(), nullptr, hashValue, 0); // iv zero
        EVP_CIPHER_CTX_set_padding(aes, 0); // no padding
        int lOutLen;
        EVP_DecryptUpdate(aes, m_encryptionKey, &lOutLen, m_ueValue, 32);
    }

    // TODO Validate permissions (or not...)

    return success;
}

size_t PdfEncryptAESV3::CalculateStreamOffset() const
{
    return AES_IV_LENGTH;
}

void PdfEncryptAESV3::Encrypt(const unsigned char* inStr, size_t inLen,
    unsigned char* outStr, size_t outLen) const
{
    size_t offset = CalculateStreamOffset();
    this->GenerateInitialVector(outStr);
    this->BaseEncrypt(const_cast<unsigned char*>(m_encryptionKey), m_keyLength, outStr, inStr, inLen, &outStr[offset], outLen - offset);
}

void PdfEncryptAESV3::Decrypt(const unsigned char* inStr, size_t inLen,
    unsigned char* outStr, size_t& outLen) const
{
    size_t offset = CalculateStreamOffset();

    this->BaseDecrypt(const_cast<unsigned char*>(m_encryptionKey), m_keyLength, inStr, &inStr[offset], inLen - offset, outStr, outLen);
}

PdfEncryptAESV3::PdfEncryptAESV3(const string_view& userPassword, const string_view& ownerPassword, EPdfPermissions protection) : PdfEncryptAESBase()
{
    // setup object
    m_userPass = userPassword;
    m_ownerPass = ownerPassword;
    m_eAlgorithm = EPdfEncryptAlgorithm::AESV3;

    m_rValue = 5;
    m_eKeyLength = EPdfKeyLength::L256;
    m_keyLength = (int)EPdfKeyLength::L256 / 8;

    // Init buffers
    std::memset(m_oValue, 0, 48);
    std::memset(m_uValue, 0, 48);
    std::memset(m_encryptionKey, 0, 32);
    std::memset(m_ueValue, 0, 32);
    std::memset(m_oeValue, 0, 32);

    // Compute P value
    m_pValue = PERMS_DEFAULT | protection;
}

PdfEncryptAESV3::PdfEncryptAESV3(PdfString oValue, PdfString oeValue, PdfString uValue, PdfString ueValue, EPdfPermissions pValue, PdfString permsValue) : PdfEncryptAESBase()
{
    m_pValue = pValue;
    m_eAlgorithm = EPdfEncryptAlgorithm::AESV3;

    m_eKeyLength = EPdfKeyLength::L256;
    m_keyLength = (int)EPdfKeyLength::L256 / 8;
    m_rValue = 5;
    std::memcpy(m_oValue, oValue.GetRawData().data(), 48);
    std::memcpy(m_oeValue, oeValue.GetRawData().data(), 32);
    std::memcpy(m_uValue, uValue.GetRawData().data(), 48);
    std::memcpy(m_ueValue, ueValue.GetRawData().data(), 32);
    std::memcpy(m_permsValue, permsValue.GetRawData().data(), 16);
    std::memset(m_encryptionKey, 0, 32);
}

size_t PdfEncryptAESV3::CalculateStreamLength(size_t length) const
{
    size_t realLength = ((length + 15) & ~15) + AES_IV_LENGTH;
    if (length % 16 == 0)
    {
        realLength += 16;
    }

    return realLength;
}

unique_ptr<PdfInputStream> PdfEncryptAESV3::CreateEncryptionInputStream(PdfInputStream& pInputStream, size_t inputLen)
{
    return unique_ptr<PdfInputStream>(new PdfAESInputStream(pInputStream, inputLen, m_encryptionKey, 32));
}

unique_ptr<PdfOutputStream> PdfEncryptAESV3::CreateEncryptionOutputStream(PdfOutputStream&)
{
    PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "CreateEncryptionOutputStream does not yet support AESV3");
}
    
#endif // PODOFO_HAVE_LIBIDN

int PdfEncrypt::GetKeyLength() const
{
    return m_keyLength * 8;
}

void PdfEncrypt::Encrypt(const string_view& view, string& out) const
{
    size_t outputLen = this->CalculateStreamLength(view.size());
    out.resize(outputLen);
    this->Encrypt(view.data(), view.size(), out.data(), outputLen);
}

void PdfEncrypt::Decrypt(const string_view& view, string& out) const
{
    // FIX-ME: The following clearly seems hardcoded for AES
    // It was found like this in PdfString and PdfTokenizer
    // Fix it so it will allocate the exact amount of memory
    // needed, including RC4
    size_t outBufferLen = view.size() - this->CalculateStreamOffset();
    out.resize(outBufferLen + 16 - (outBufferLen % 16));
    this->Decrypt(view.data(), view.size(), out.data(), outBufferLen);
    out.resize(outBufferLen);
    out.shrink_to_fit();
}

void PdfEncrypt::SetCurrentReference(const PdfReference& rRef)
{
    m_curReference = rRef;
}

bool PdfEncrypt::IsPrintAllowed() const
{
    return (m_pValue & EPdfPermissions::Print) == EPdfPermissions::Print;
}

bool PdfEncrypt::IsEditAllowed() const
{
    return (m_pValue & EPdfPermissions::Edit) == EPdfPermissions::Edit;
}

bool PdfEncrypt::IsCopyAllowed() const
{
    return (m_pValue & EPdfPermissions::Copy) == EPdfPermissions::Copy;
}

bool PdfEncrypt::IsEditNotesAllowed() const
{
    return (m_pValue & EPdfPermissions::EditNotes) == EPdfPermissions::EditNotes;
}

bool PdfEncrypt::IsFillAndSignAllowed() const
{
    return (m_pValue & EPdfPermissions::FillAndSign) == EPdfPermissions::FillAndSign;
}

bool PdfEncrypt::IsAccessibilityAllowed() const
{
    return (m_pValue & EPdfPermissions::Accessible) == EPdfPermissions::Accessible;
}

bool PdfEncrypt::IsDocAssemblyAllowed() const
{
    return (m_pValue & EPdfPermissions::DocAssembly) == EPdfPermissions::DocAssembly;
}

bool PdfEncrypt::IsHighPrintAllowed() const
{
    return (m_pValue & EPdfPermissions::HighPrint) == EPdfPermissions::HighPrint;
}

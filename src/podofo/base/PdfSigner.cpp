#include "PdfSigner.h"
#include "../base/PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

constexpr const char* ByteRangeBeacon = "[ 0 1234567890 1234567890 1234567890]";
constexpr size_t BufferSize = 65536;

static size_t ReadForSignature(PdfOutputDevice& device,
    size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* pBuffer, size_t lLen);
static void AdjustByteRange(PdfOutputDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize);
static void SetSignature(PdfOutputDevice& device, const string_view& sigData,
    size_t conentsBeaconOffset);
static void PrepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon);

PdfSigner::~PdfSigner() { }

unsigned PdfSigner::GetSignatureSize()
{
    return (unsigned)ComputeSignature(true).size();
}

string PdfSigner::GetSignatureFilter() const
{
    // Default value
    return "Adobe.PPKLite";
}

void PoDoFo::SignDocument(PdfMemDocument& doc, PdfOutputDevice& device, PdfSigner& signer,
    PdfSignatureField& signature, PdfSignFlags flags)
{
    (void)flags;

    unsigned signatureSize = signer.GetSignatureSize();
    PdfSignatureBeacons beacons;
    PrepareBeaconsData(signatureSize, beacons.ContentsBeacon, beacons.ByteRangeBeacon);
    signature.PrepareForSigning(signer.GetSignatureFilter(), signer.GetSignatureSubFilter(), beacons);
    auto form = doc.GetAcroForm();
    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    form->GetObject()->GetDictionary().AddKey("SigFlags", PdfObject((int64_t)3));

    doc.WriteUpdate(device);
    device.Flush();

    AdjustByteRange(device, *beacons.ByteRangeOffset, *beacons.ContentsOffset, beacons.ContentsBeacon.size());
    device.Flush();

    // Read data from the device to prepare the signature
    signer.Reset();
    device.Seek(0);
    vector<char> buffer(BufferSize);
    size_t readBytes;
    while ((readBytes = ReadForSignature(device, *beacons.ContentsOffset, beacons.ContentsBeacon.size(),
        buffer.data(), BufferSize)) != 0)
    {
        signer.AppendData({ buffer.data(), readBytes });
    }

    string signatureBuf = signer.ComputeSignature(false);
    if (signatureBuf.size() > signatureSize)
        throw runtime_error("Actual signature size smaller than given size");

    SetSignature(device, signatureBuf, *beacons.ContentsOffset);
    device.Flush();
}

size_t ReadForSignature(PdfOutputDevice& device, size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* pBuffer, size_t lLen)
{
    size_t pos = device.Tell();
    size_t numRead = 0;
    // Check if we are before beacon
    if (pos < conentsBeaconOffset)
    {
        size_t readSize = std::min(lLen, conentsBeaconOffset - pos);
        if (readSize > 0)
        {
            numRead = device.Read(pBuffer, readSize);
            pBuffer += numRead;
            lLen -= numRead;
            if (lLen == 0)
                return numRead;
        }
    }

    // shift at the end of beacon
    if ((pos + numRead) >= conentsBeaconOffset
        && pos < (conentsBeaconOffset + conentsBeaconSize))
    {
        device.Seek(conentsBeaconOffset + conentsBeaconSize);
    }

    // read after beacon
    lLen = std::min(lLen, device.GetLength() - device.Tell());
    if (lLen == 0)
        return numRead;

    return numRead + device.Read(pBuffer, lLen);
}

void AdjustByteRange(PdfOutputDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize)
{
    // Get final position
    size_t sFileEnd = device.GetLength();
    PdfArray arr;
    arr.push_back(PdfVariant(static_cast<int64_t>(0)));
    arr.push_back(PdfVariant(static_cast<int64_t>(conentsBeaconOffset)));
    arr.push_back(PdfVariant(static_cast<int64_t>(conentsBeaconOffset + conentsBeaconSize)));
    arr.push_back(PdfVariant(static_cast<int64_t>(sFileEnd - (conentsBeaconOffset + conentsBeaconSize))));

    device.Seek(byteRangeOffset);
    arr.Write(device, EPdfWriteMode::Compact, nullptr);
}

void SetSignature(PdfOutputDevice& device, const string_view& contentsData,
    size_t conentsBeaconOffset)
{
    PdfString sig(contentsData.data(), contentsData.length(), true);

    // Position at contents beacon after '<'
    device.Seek(conentsBeaconOffset);
    // Write the beacon data
    sig.Write(device, PoDoFo::EPdfWriteMode::Compact, nullptr);
}

void PrepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon)
{
    // Just prepare strings with spaces, for easy writing later
    contentsBeacon.resize((signatureSize * 2) + 2, ' '); // Signature bytes will be encoded
                                                         // as an hex string
    byteRangeBeacon.resize(char_traits<char>::length(ByteRangeBeacon), ' ');
}

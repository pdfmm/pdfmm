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

string PdfSigner::GetSignatureFilter() const
{
    // Default value
    return "Adobe.PPKLite";
}

void PoDoFo::SignDocument(PdfMemDocument& doc, PdfOutputDevice& device, PdfSigner& signer,
    PdfSignatureField& signature, PdfSignFlags flags)
{
    (void)flags;

    string signatureBuf;
    signer.ComputeSignature(signatureBuf, true);
    size_t beaconSize = signatureBuf.size();
    PdfSignatureBeacons beacons;
    PrepareBeaconsData(beaconSize, beacons.ContentsBeacon, beacons.ByteRangeBeacon);
    signature.PrepareForSigning(signer.GetSignatureFilter(), signer.GetSignatureSubFilter(),
        signer.GetSignatureType(), beacons);
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

    signer.ComputeSignature(signatureBuf, false);
    if (signatureBuf.size() > beaconSize)
        throw runtime_error("Actual signature size bigger than beacon size");

    // Ensure the signature will be as big as the
    // beacon size previously cached to fill all
    // available reserved space for the /Contents
    signatureBuf.resize(beaconSize);
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

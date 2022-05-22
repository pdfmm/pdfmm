/**
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfSigner.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

constexpr const char* ByteRangeBeacon = "[ 0 1234567890 1234567890 1234567890]";
constexpr size_t BufferSize = 65536;

static size_t ReadForSignature(PdfOutputDevice& device,
    size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* buffer, size_t len);
static void AdjustByteRange(PdfOutputDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize, charbuff& buffer);
static void SetSignature(PdfOutputDevice& device, const string_view& sigData,
    size_t conentsBeaconOffset, charbuff& buffer);
static void PrepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon);

PdfSigner::~PdfSigner() { }

string PdfSigner::GetSignatureFilter() const
{
    // Default value
    return "Adobe.PPKLite";
}

void mm::SignDocument(PdfMemDocument& doc, PdfOutputDevice& device, PdfSigner& signer,
    PdfSignature& signature, PdfSaveOptions opts)
{
    string signatureBuf;
    signer.ComputeSignature(signatureBuf, true);
    size_t beaconSize = signatureBuf.size();
    PdfSignatureBeacons beacons;
    PrepareBeaconsData(beaconSize, beacons.ContentsBeacon, beacons.ByteRangeBeacon);
    signature.PrepareForSigning(signer.GetSignatureFilter(), signer.GetSignatureSubFilter(),
        signer.GetSignatureType(), beacons);
    auto& form = doc.GetOrCreateAcroForm();

    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    // NOTE: This enables the signature panel visualization
    form.GetObject().GetDictionary().AddKey("SigFlags", (int64_t)3);

    auto acroForm = doc.GetAcroForm();
    if (acroForm != nullptr)
    {
        // NOTE: Adobe is crazy and if the /NeedAppearances is set to true,
        // it will not show up the signature upon signing. Just
        // remore the key just in case it's present (defaults to false)
        acroForm->GetDictionary().RemoveKey("NeedAppearances");
    }

    doc.SaveUpdate(device, opts);
    device.Flush();

    charbuff buffer;
    AdjustByteRange(device, *beacons.ByteRangeOffset, *beacons.ContentsOffset,
        beacons.ContentsBeacon.size(), buffer);
    device.Flush();

    // Read data from the device to prepare the signature
    signer.Reset();
    device.Seek(0);
    size_t readBytes;
    buffer.resize(BufferSize);
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
    SetSignature(device, signatureBuf, *beacons.ContentsOffset, buffer);
    device.Flush();
}

size_t ReadForSignature(PdfOutputDevice& device, size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* buffer, size_t len)
{
    size_t pos = device.Tell();
    size_t numRead = 0;
    // Check if we are before beacon
    if (pos < conentsBeaconOffset)
    {
        size_t readSize = std::min(len, conentsBeaconOffset - pos);
        if (readSize > 0)
        {
            numRead = device.Read(buffer, readSize);
            buffer += numRead;
            len -= numRead;
            if (len == 0)
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
    len = std::min(len, device.GetLength() - device.Tell());
    if (len == 0)
        return numRead;

    return numRead + device.Read(buffer, len);
}

void AdjustByteRange(PdfOutputDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize, charbuff& buffer)
{
    // Get final position
    size_t fileEnd = device.GetLength();
    PdfArray arr;
    arr.Add(PdfObject(static_cast<int64_t>(0)));
    arr.Add(PdfObject(static_cast<int64_t>(conentsBeaconOffset)));
    arr.Add(PdfObject(static_cast<int64_t>(conentsBeaconOffset + conentsBeaconSize)));
    arr.Add(PdfObject(static_cast<int64_t>(fileEnd - (conentsBeaconOffset + conentsBeaconSize))));

    device.Seek(byteRangeOffset);
    arr.Write(device, PdfWriteFlags::None, nullptr, buffer);
}

void SetSignature(PdfOutputDevice& device, const string_view& contentsData,
    size_t conentsBeaconOffset, charbuff& buffer)
{
    auto sig = PdfString::FromRaw(contentsData);

    // Position at contents beacon after '<'
    device.Seek(conentsBeaconOffset);
    // Write the beacon data
    sig.Write(device, PdfWriteFlags::None, nullptr, buffer);
}

void PrepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon)
{
    // Just prepare strings with spaces, for easy writing later
    contentsBeacon.resize((signatureSize * 2) + 2, ' '); // Signature bytes will be encoded
                                                         // as an hex string
    byteRangeBeacon.resize(char_traits<char>::length(ByteRangeBeacon), ' ');
}

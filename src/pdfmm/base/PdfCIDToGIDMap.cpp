#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfCIDToGIDMap.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"

using namespace std;
using namespace mm;

PdfCIDToGIDMap::PdfCIDToGIDMap(CIDToGIDMap&& map)
    : m_cidToGidMap(std::move(map)) { }

PdfCIDToGIDMap PdfCIDToGIDMap::Create(const PdfObject& cidToGidMapObj)
{
    CIDToGIDMap map;
    // Table 115 — Entries in a CIDFont dictionary
    // "The glyph index for a particular CID value c shall be
    // a 2 - byte value stored in bytes 2 × c and 2 × c + 1,
    // where the first byte shall be the high - order byte"
    auto buffer = cidToGidMapObj.MustGetStream().GetFilteredCopy();
    for (unsigned i = 0, count = (unsigned)buffer.size() / 2; i < count; i++)
    {
        unsigned gid = (unsigned)buffer[i * 2 + 1] << 8 | (unsigned)buffer[i * 2 + 0];
        map[i] = gid;
    }

    return PdfCIDToGIDMap(std::move(map));
}

bool PdfCIDToGIDMap::TryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    auto found = m_cidToGidMap.find(cid);
    if (found == m_cidToGidMap.end())
    {
        gid = 0;
        return false;
    }

    gid = found->second;
    return true;
}

void PdfCIDToGIDMap::ExportTo(PdfObject& descendantFont)
{
    auto cidToGidMap = descendantFont.MustGetDocument().GetObjects().CreateDictionaryObject();
    descendantFont.GetDictionary().AddKeyIndirect("CIDToGIDMap", cidToGidMap);
    auto& stream = cidToGidMap->GetOrCreateStream();
    stream.BeginAppend();
    unsigned previousCid = 0;
    array<char, 2> entry;
    for (auto& pair : m_cidToGidMap)
    {
        entry = { };
        unsigned cid = previousCid;
        for (; cid < pair.first; cid++)
        {
            // Write zeroes for missing mappings
            stream.Append(entry.data(), 2);
        }

        utls::WriteUInt16BE(entry.data(), (uint16_t)pair.second);
        stream.Append(entry.data(), 2);
        previousCid = cid;
    }
    stream.EndAppend();
}

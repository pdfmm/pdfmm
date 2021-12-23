#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfResources.h"
#include "PdfDictionary.h"
#include "PdfCanvas.h"

using namespace std;
using namespace mm;

PdfResources::PdfResources(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

PdfResources::PdfResources(PdfDictionary& dict)
    : PdfDictionaryElement(dict.AddKey("Resources", PdfDictionary()))
{
    GetDictionary().AddKey("ProcSet", PdfCanvas::GetProcSet());
}

void PdfResources::AddResource(const PdfName& type, const PdfName& key, const PdfObject* obj)
{
    // TODO: Create the dictionary on demand
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (obj == nullptr)
        dict->RemoveKey(key);
    else
        dict->AddKeyIndirect(key, obj);
}

PdfDictionaryIndirectIterable PdfResources::GetResourceIterator(const string_view& type)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return PdfDictionaryIndirectIterable();

    return dict->GetIndirectIterator();
}

PdfDictionaryConstIndirectIterable PdfResources::GetResourceIterator(const string_view& type) const
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return PdfDictionaryConstIndirectIterable();

    return((const PdfDictionary&)*dict).GetIndirectIterator();
}

void PdfResources::RemoveResource(const string_view& type, const string_view& key)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return;

    dict->RemoveKey(key);
}

PdfObject* PdfResources::GetResource(const string_view& type, const string_view& key)
{
    return getResource(type, key);
}

const PdfObject* PdfResources::GetResource(const string_view& type, const string_view& key) const
{
    return getResource(type, key);
}

PdfObject* PdfResources::getResource(const string_view& type, const string_view& key) const
{
    PdfDictionary* dict;
    auto typeObj = const_cast<PdfResources&>(*this).GetDictionary().FindKey(type);
    if (typeObj == nullptr || !typeObj->TryGetDictionary(dict))
        return nullptr;

    return dict->FindKey(key);
}

bool PdfResources::tryGetDictionary(const string_view& type, PdfDictionary*& dict) const
{
    auto typeObj = const_cast<PdfResources&>(*this).GetDictionary().FindKey(type);
    if (typeObj == nullptr)
    {
        dict = nullptr;
        return false;
    }

    return typeObj->TryGetDictionary(dict);
}

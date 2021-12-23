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
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return;

    if (obj == nullptr)
        dict->RemoveKey(key);
    else
        dict->AddKeyIndirect(key, obj);
}

PdfDictionaryIndirectIterator PdfResources::GetResourceIterator(const PdfName& type)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return PdfDictionaryIndirectIterator();

    return dict->GetIndirectIterator();
}

const PdfDictionaryIndirectIterator PdfResources::GetResourceIterator(const PdfName& type) const
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return PdfDictionaryIndirectIterator();

    return dict->GetIndirectIterator();
}

void PdfResources::RemoveResource(const PdfName& type, const PdfName& key)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return;

    dict->RemoveKey(key);
}

PdfObject* PdfResources::GetResource(const PdfName& type, const PdfName& key)
{
    return getResource(type, key);
}

const PdfObject* PdfResources::GetResource(const PdfName& type, const PdfName& key) const
{
    return getResource(type, key);
}

PdfObject* PdfResources::getResource(const PdfName& type, const PdfName& key) const
{
    PdfDictionary* dict;
    auto typeObj = const_cast<PdfResources&>(*this).GetDictionary().FindKey(type);
    if (typeObj == nullptr || !typeObj->TryGetDictionary(dict))
        return nullptr;

    return dict->FindKey(key);
}

bool PdfResources::tryGetDictionary(const PdfName& type, PdfDictionary*& dict) const
{
    auto typeObj = const_cast<PdfResources&>(*this).GetDictionary().FindKey(type);
    if (typeObj == nullptr)
    {
        dict = nullptr;
        return false;
    }

    return typeObj->TryGetDictionary(dict);
}

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

PdfObject* PdfResources::GetFromResources(const PdfName& type, const PdfName& key)
{
    return getFromResources(type, key);
}

const PdfObject* PdfResources::GetFromResources(const PdfName& type, const PdfName& key) const
{
    return getFromResources(type, key);
}

PdfObject* PdfResources::getFromResources(const PdfName& type, const PdfName& key) const
{
    auto typeObj = const_cast<PdfResources&>(*this).GetDictionary().FindKey(type);
    if (typeObj == nullptr || !typeObj->IsDictionary())
        return nullptr;

    return typeObj->GetDictionary().FindKey(key);
}

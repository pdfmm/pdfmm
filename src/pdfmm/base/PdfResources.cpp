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
    auto typeObj = GetDictionary().FindKey(type);
    if (typeObj == nullptr || !typeObj->IsDictionary())
        return nullptr;

    return typeObj->GetDictionary().FindKey(key);
}

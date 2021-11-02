#include "PdfXObjectPostScript.h"

using namespace std;
using namespace mm;

PdfXObjectPostScript::PdfXObjectPostScript(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::PostScript)
{

}

PdfRect PdfXObjectPostScript::GetRect() const
{
    return PdfRect();
}

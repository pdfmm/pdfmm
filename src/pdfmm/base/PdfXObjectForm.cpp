#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfXObjectForm.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfPage.h"

using namespace std;
using namespace mm;

PdfXObjectForm::PdfXObjectForm(PdfDocument& doc, const PdfRect& rect, const string_view& prefix)
    : PdfXObject(doc, PdfXObjectType::Form, prefix), m_Rect(rect)
{
    InitXObject(rect);
}

PdfXObjectForm::PdfXObjectForm(PdfDocument& doc, const PdfDocument& sourceDoc, unsigned pageIndex, const string_view& prefix, bool useTrimBox)
    : PdfXObject(doc, PdfXObjectType::Form, prefix)
{
    InitXObject(m_Rect);

    // Implementation note: source document must be different from distination
    if (&doc == reinterpret_cast<const PdfDocument*>(&sourceDoc))
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);

    // After filling set correct BBox, independent of rotation
    m_Rect = doc.FillXObjectFromDocumentPage(*this, sourceDoc, pageIndex, useTrimBox);

    InitAfterPageInsertion(sourceDoc, pageIndex);
}

PdfXObjectForm::PdfXObjectForm(PdfDocument& doc, unsigned pageIndex, const string_view& prefix, bool useTrimBox)
    : PdfXObject(doc, PdfXObjectType::Form, prefix)
{
    InitXObject(m_Rect);

    // After filling set correct BBox, independent of rotation
    m_Rect = doc.FillXObjectFromExistingPage(*this, pageIndex, useTrimBox);

    InitAfterPageInsertion(doc, pageIndex);
}

PdfXObjectForm::PdfXObjectForm(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::Form)
{
    if (obj.GetDictionary().HasKey("BBox"))
        m_Rect = PdfRect(obj.GetDictionary().MustFindKey("BBox").GetArray());

    auto resources = obj.GetDictionary().FindKey("Resources");
    if (resources != nullptr)
        m_Resources.reset(new PdfResources(*resources));
}

void PdfXObjectForm::EnsureResourcesCreated()
{
    if (m_Resources == nullptr)
        m_Resources.reset(new PdfResources(GetDictionary()));

    // A Form XObject must have a stream
    GetObject().ForceCreateStream();
}

bool PdfXObjectForm::HasRotation(double& teta) const
{
    teta = 0;
    return false;
}

void PdfXObjectForm::SetRect(const PdfRect& rect)
{
    PdfArray bbox;
    rect.ToArray(bbox);
    GetObject().GetDictionary().AddKey("BBox", bbox);
    m_Rect = rect;
}

PdfResources* PdfXObjectForm::getResources() const
{
    return m_Resources.get();
}

inline PdfObjectStream& PdfXObjectForm::GetStreamForAppending(PdfStreamAppendFlags flags)
{
    (void)flags; // Flags have no use here
    return GetObject().GetOrCreateStream();
}

PdfRect PdfXObjectForm::GetRect() const
{
    return m_Rect;
}

PdfObject* PdfXObjectForm::getContentsObject() const
{
    return &const_cast<PdfXObjectForm&>(*this).GetObject();
}

PdfResources& PdfXObjectForm::GetOrCreateResources()
{
    EnsureResourcesCreated();
    return *m_Resources;
}

void PdfXObjectForm::InitXObject(const PdfRect& rect)
{
    // Initialize static data
    if (m_Matrix.IsEmpty())
    {
        // This matrix is the same for all PdfXObjects so cache it
        m_Matrix.Add(PdfObject(static_cast<int64_t>(1)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(1)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
    }

    PdfArray bbox;
    rect.ToArray(bbox);
    this->GetObject().GetDictionary().AddKey("BBox", bbox);
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(PdfXObjectType::Form)));
    this->GetObject().GetDictionary().AddKey("FormType", PdfVariant(static_cast<int64_t>(1))); // only 1 is only defined in the specification.
    this->GetObject().GetDictionary().AddKey("Matrix", m_Matrix);
}

void PdfXObjectForm::InitAfterPageInsertion(const PdfDocument& doc, unsigned pageIndex)
{
    PdfArray bbox;
    m_Rect.ToArray(bbox);
    this->GetObject().GetDictionary().AddKey("BBox", bbox);

    int rotation = doc.GetPageTree().GetPage(pageIndex).GetRotationRaw();
    // correct negative rotation
    if (rotation < 0)
        rotation = 360 + rotation;

    // Swap offsets/width/height for vertical rotation
    switch (rotation)
    {
        case 90:
        case 270:
        {
            double temp;

            temp = m_Rect.GetWidth();
            m_Rect.SetWidth(m_Rect.GetHeight());
            m_Rect.SetHeight(temp);

            temp = m_Rect.GetLeft();
            m_Rect.SetLeft(m_Rect.GetBottom());
            m_Rect.SetBottom(temp);
        }
        break;

        default:
            break;
    }

    // Build matrix for rotation and cropping
    double alpha = -rotation / 360.0 * 2.0 * M_PI;

    double a, b, c, d, e, f;

    a = cos(alpha);
    b = sin(alpha);
    c = -sin(alpha);
    d = cos(alpha);

    switch (rotation)
    {
        case 90:
            e = -m_Rect.GetLeft();
            f = m_Rect.GetBottom() + m_Rect.GetHeight();
            break;

        case 180:
            e = m_Rect.GetLeft() + m_Rect.GetWidth();
            f = m_Rect.GetBottom() + m_Rect.GetHeight();
            break;

        case 270:
            e = m_Rect.GetLeft() + m_Rect.GetWidth();
            f = -m_Rect.GetBottom();
            break;

        case 0:
        default:
            e = -m_Rect.GetLeft();
            f = -m_Rect.GetBottom();
            break;
    }

    PdfArray matrix;
    matrix.Add(PdfObject(a));
    matrix.Add(PdfObject(b));
    matrix.Add(PdfObject(c));
    matrix.Add(PdfObject(d));
    matrix.Add(PdfObject(e));
    matrix.Add(PdfObject(f));

    this->GetObject().GetDictionary().AddKey("Matrix", matrix);
}

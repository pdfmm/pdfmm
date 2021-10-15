/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FIELD_H
#define PDF_FIELD_H

#include "PdfDefines.h"

#include "PdfName.h"
#include "PdfString.h"
#include "PdfDictionary.h"
#include "PdfAnnotation.h"

namespace mm {

class PdfAcroForm;
class PdfAction;
class PdfDocument;
class PdfPage;
class PdfRect;
class PdfReference;
class PdfStreamedDocument;

/** The type of PDF field
 */
enum class PdfFieldType
{
    Unknown = 0,
    PushButton,
    CheckBox,
    RadioButton,
    TextField,
    ComboBox,
    ListBox,
    Signature,
};

/** The possible highlighting modes
 *  for a PdfField. I.e the visual effect
 *  that is to be used when the mouse
 *  button is pressed.
 *
 *  The default value is
 *  PdfHighlightingMode::Invert
 */
enum class PdfHighlightingMode
{
    Unknown = 0,
    None,           ///< Do no highlighting
    Invert,         ///< Invert the PdfField
    InvertOutline,  ///< Invert the fields border
    Push,           ///< Display the fields down appearance (requires an additional appearance stream to be set)
};

enum class PdfFieldFlags
{
    ReadOnly = 0x0001,
    Required = 0x0002,
    NoExport = 0x0004
};

// TODO: Inherit PdfElement
class PDFMM_API PdfField
{
protected:
    PdfField(PdfFieldType fieldType, PdfPage& page, const PdfRect& rect);

    PdfField(PdfFieldType fieldType, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

    PdfField(PdfFieldType fieldType, PdfObject& obj, PdfAnnotation* widget);

    /**
     *  Set a bit in the field flags value of the fields dictionary.
     *
     *  \param value the value specifying the bits to set
     *  \param set if true the value will be set otherwise
     *              they will be cleared.
     *
     *  \see GetFieldFlag
     */
    void SetFieldFlag(int64_t value, bool set);

    /**
     *  \param value it is checked if these bits are set
     *  \param defvalue the returned value if no field flags are specified
     *
     *  \returns true if given bits are set in the field flags
     *
     *  \see SetFieldFlag
     */
    bool GetFieldFlag(int64_t value, bool defvalue) const;

    /**
    *  \param obj the object to test for field flags
    *  \param value is set with the flag if found
    *  \returns true if flag is found
    */
    static bool GetFieldFlags(const PdfObject& obj, int64_t& value);

    /**
     * \param create create the dictionary if it does not exist
     *
     * \returns a pointer to the appearance characteristics dictionary
     *          of this object or nullptr if it does not exists.
     */
    PdfObject* GetAppearanceCharacteristics(bool create) const;

    void AssertTerminalField() const;

public:
    /** Create a PdfAcroForm dictionary object from an existing PdfObject
     *	\param obj the object to create from
     *  \param widget the widget annotation of this field
     */
    PdfField(PdfObject& obj, PdfAnnotation* widget);

    virtual ~PdfField() { }

    /** Create a PdfAcroForm dictionary object from an existing annottion
    *  \param widget the widget annotation of this field
    *  \returns the pointer to the created field
    */
    static PdfField* CreateField(PdfAnnotation& widget);

    /** Create a PdfAcroForm dictionary object from an existing object
    *  \returns the pointer to the created field
    */
    static PdfField* CreateField(PdfObject& obj);

    PdfField* CreateChildField();

    PdfField* CreateChildField(PdfPage& page, const PdfRect& rect);

    /** Infer the field type from the given object
    *  \param obj the object to infer the field type from
    *  \returns the inferred type
    */
    static PdfFieldType GetFieldType(const PdfObject& obj);

    /** Get the page of this PdfField
     *
     *  \returns the page of this PdfField
     */
    PdfPage* GetPage() const;

    /** Set the highlighting mode which should be used when the user
     *  presses the mouse button over this widget.
     *
     *  \param mode the highliting mode
     *
     *  The default value is PdfHighlightingMode::Invert
     */
    void SetHighlightingMode(PdfHighlightingMode mode);

    /**
     * \returns the highlighting mode to be used when the user
     *          presses the mouse button over this widget
     */
    PdfHighlightingMode GetHighlightingMode() const;

    /**
     * Sets the border color of the field to be transparent
     */
    void SetBorderColorTransparent();

    /**
     * Sets the border color of the field
     *
     * \param gray gray value of the color
     */
    void SetBorderColor(double gray);

    /**
     * Sets the border color of the field
     *
     * \param red red
     * \param green green
     * \param blue blue
     */
    void SetBorderColor(double red, double green, double blue);

    /**
     * Sets the border color of the field
     *
     * \param cyan cyan
     * \param magenta magenta
     * \param yellow yellow
     * \param black black
     */
    void SetBorderColor(double cyan, double magenta, double yellow, double black);

    /**
     * Sets the background color of the field to be transparent
     */
    void SetBackgroundColorTransparent();

    /**
     * Sets the background color of the field
     *
     * \param gray gray value of the color
     */
    void SetBackgroundColor(double gray);

    /**
     * Sets the background color of the field
     *
     * \param red red
     * \param green green
     * \param blue blue
     */
    void SetBackgroundColor(double red, double green, double blue);

    /**
     * Sets the background color of the field
     *
     * \param cyan cyan
     * \param magenta magenta
     * \param yellow yellow
     * \param black black
     */
    void SetBackgroundColor(double cyan, double magenta, double yellow, double black);

    /** Sets the field name of this PdfField
     *
     *  PdfFields require a field name to work correctly in acrobat reader!
     *  This name can be used to access the field in JavaScript actions.
     *
     *  \param name the field name of this pdf field
     */
    void SetName(const PdfString& name);

    /** \returns the field name of this PdfField
     */
    std::optional<PdfString> GetName() const;

    /** \returns the field name of this PdfField at this level of the hierarchy
     */
    std::optional<PdfString> GetNameRaw() const;

    /** \returns the parents qualified name of this PdfField
     *
     *  \param escapePartialNames escape non compliant partial names
     */
    std::string GetFullName(bool escapePartialNames = false) const;

    /**
     * Set the alternate name of this field which
     * is used to display the fields name to the user
     * (e.g. in error messages).
     *
     * \param name a name that can be displayed to the user
     */
    void SetAlternateName(const PdfString& name);

    /** \returns the fields alternate name
     */
    std::optional<PdfString> GetAlternateName() const;

    /**
     * Sets the fields mapping name which is used when exporting
     * the fields data
     *
     * \param name the mapping name of this PdfField
     */
    void SetMappingName(const PdfString& name);

    /** \returns the mapping name of this field
     */
    std::optional<PdfString> GetMappingName() const;

    /** Set this field to be readonly.
     *  I.e. it will not interact with the user
     *  and respond to mouse button events.
     *
     *  This is useful for fields that are pure calculated.
     *
     *  \param readOnly specifies if this field is read-only.
     */
    void SetReadOnly(bool readOnly);

    /**
     * \returns true if this field is read-only
     *
     * \see SetReadOnly
     */
    bool IsReadOnly() const;

    /** Required fields must have a value
     *  at the time the value is exported by a submit action
     *
     *  \param required if true this field requires a value for submit actions
     */
    void SetRequired(bool required);

    /**
     * \returns true if this field is required for submit actions
     *
     * \see SetRequired
     */
    bool IsRequired() const;

    /** Sets if this field can be exported by a submit action
     *
     *  Fields can be exported by default.
     *
     *  \param exprt if false this field cannot be exported by submit actions
     */
    void SetNoExport(bool exprt);

    /**
     * \returns true if this field can be exported by submit actions
     *
     * \see SetExport
     */
    bool IsNoExport() const;

    void SetMouseEnterAction(const PdfAction& action);
    void SetMouseLeaveAction(const PdfAction& action);
    void SetMouseDownAction(const PdfAction& action);
    void SetMouseUpAction(const PdfAction& action);

    void SetFocusEnterAction(const PdfAction& action);
    void SetFocusLeaveAction(const PdfAction& action);

    void SetPageOpenAction(const PdfAction& action);
    void SetPageCloseAction(const PdfAction& action);

    void SetPageVisibleAction(const PdfAction& action);
    void SetPageInvisibleAction(const PdfAction& action);

    void SetKeystrokeAction(const PdfAction& action);
    void SetValidateAction(const PdfAction& action);

    PdfFieldType GetType() const;

private:
    PdfField(const PdfField& rhs) = delete;

    void Init(PdfAcroForm* parent);
    void AddAlternativeAction(const PdfName& name, const PdfAction& action);
    static PdfField* createField(PdfFieldType type, PdfObject& obj, PdfAnnotation* widget);
    PdfField* createChildField(PdfPage* page, const PdfRect& rect);

public:
    PdfAnnotation* GetWidgetAnnotation() const;
    PdfObject& GetObject() const;
    PdfDictionary& GetDictionary();
    const PdfDictionary& GetDictionary() const;

private:
    PdfFieldType m_Field;
    PdfObject* m_Object;
    PdfAnnotation* m_Widget;
};

};

#endif // PDF_FIELD_H

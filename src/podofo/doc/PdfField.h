/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FIELD_H
#define PDF_FIELD_H

#include "podofo/base/PdfDefines.h"  
#include "podofo/base/PdfName.h"
#include "podofo/base/PdfString.h"
#include "podofo/base/PdfDictionary.h"

#include "PdfAnnotation.h"

namespace PoDoFo {

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
 *  EPdfHighlightingMode::Invert
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
class PODOFO_DOC_API PdfField
{
protected:
    PdfField(PdfFieldType eField, PdfPage& page, const PdfRect& rect);

    PdfField(PdfFieldType eField, PdfDocument& doc, PdfAnnotation* pWidget, bool insertInAcroform);

    PdfField(PdfFieldType eField, PdfPage& page, const PdfRect& rRect, bool bDefaultApperance);

    PdfField(PdfFieldType eField, PdfObject& obj, PdfAnnotation* widget);

    /** 
     *  Set a bit in the field flags value of the fields dictionary.
     *
     *  \param lValue the value specifying the bits to set
     *  \param bSet if true the value will be set otherwise
     *              they will be cleared.
     *
     *  \see GetFieldFlag
     */
    void SetFieldFlag( int64_t lValue, bool bSet );

    /**
     *  \param lValue it is checked if these bits are set
     *  \param bDefault the returned value if no field flags are specified
     *
     *  \returns true if given bits are set in the field flags
     *
     *  \see SetFieldFlag
     */
    bool GetFieldFlag(int64_t lValue, bool bDefault ) const;

    /**
    *  \param rObject the object to test for field flags
    *  \param lValue is set with the flag if found
    *  \returns true if flag is found
    */
    static bool GetFieldFlags( const PdfObject &rObject, int64_t & lValue );

    /**
     * \param bCreate create the dictionary if it does not exist
     *
     * \returns a pointer to the appearance characteristics dictionary
     *          of this object or nullptr if it does not exists.
     */
    PdfObject* GetAppearanceCharacteristics( bool bCreate ) const;

    void AssertTerminalField() const;

public:
    /** Create a PdfAcroForm dictionary object from an existing PdfObject
     *	\param pObject the object to create from
     *  \param pWidget the widget annotation of this field
     */
    PdfField(PdfObject& obj, PdfAnnotation* pWidget);

    virtual ~PdfField() { }

    /** Create a PdfAcroForm dictionary object from an existing annottion
    *  \param pWidget the widget annotation of this field
    *  \returns the pointer to the created field
    */
    static PdfField* CreateField(PdfAnnotation& pWidget);

    /** Create a PdfAcroForm dictionary object from an existing object
    *  \returns the pointer to the created field
    */
    static PdfField* CreateField(PdfObject& pObject);

    PdfField * CreateChildField();

    PdfField * CreateChildField(PdfPage &page, const PdfRect &rect);

    /** Infer the field type from the given object
    *  \param pObject the object to infer the field type from
    *  \returns the inferred type
    */
    static PdfFieldType GetFieldType( const PdfObject &pObject );

    /** Get the page of this PdfField
     *
     *  \returns the page of this PdfField
     */
    PdfPage* GetPage() const;

    /** Set the highlighting mode which should be used when the user
     *  presses the mouse button over this widget.
     *
     *  \param eMode the highliting mode
     *
     *  The default value is EPdfHighlightingMode::Invert
     */
    void SetHighlightingMode( PdfHighlightingMode eMode );

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
     * \param dGray gray value of the color
     */
    void SetBorderColor( double dGray );

    /**
     * Sets the border color of the field
     *
     * \param dRed red
     * \param dGreen green
     * \param dBlue blue
     */
    void SetBorderColor( double dRed, double dGreen, double dBlue );

    /**
     * Sets the border color of the field
     *
     * \param dCyan cyan
     * \param dMagenta magenta
     * \param dYellow yellow
     * \param dBlack black
     */
    void SetBorderColor( double dCyan, double dMagenta, double dYellow, double dBlack );

    /**
     * Sets the background color of the field to be transparent
     */
    void SetBackgroundColorTransparent();

    /**
     * Sets the background color of the field
     *
     * \param dGray gray value of the color
     */
    void SetBackgroundColor( double dGray );

    /**
     * Sets the background color of the field
     *
     * \param dRed red
     * \param dGreen green
     * \param dBlue blue
     */
    void SetBackgroundColor( double dRed, double dGreen, double dBlue );

    /**
     * Sets the background color of the field
     *
     * \param dCyan cyan
     * \param dMagenta magenta
     * \param dYellow yellow
     * \param dBlack black
     */
    void SetBackgroundColor( double dCyan, double dMagenta, double dYellow, double dBlack );

    /** Sets the field name of this PdfField
     *
     *  PdfFields require a field name to work correctly in acrobat reader!
     *  This name can be used to access the field in JavaScript actions.
     *  
     *  \param rsName the field name of this pdf field
     */
    void SetName( const PdfString & rsName );

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
     * \param rsName a name that can be displayed to the user
     */
    void SetAlternateName( const PdfString & rsName );

    /** \returns the fields alternate name
     */
    std::optional<PdfString> GetAlternateName() const;

    /**
     * Sets the fields mapping name which is used when exporting
     * the fields data
     *
     * \param rsName the mapping name of this PdfField
     */
    void SetMappingName( const PdfString & rsName ); 

    /** \returns the mapping name of this field
     */
    std::optional<PdfString> GetMappingName() const;

    /** Set this field to be readonly.
     *  I.e. it will not interact with the user
     *  and respond to mouse button events.
     *
     *  This is useful for fields that are pure calculated.
     *
     *  \param bReadOnly specifies if this field is read-only.
     */
    void SetReadOnly( bool bReadOnly );

    /** 
     * \returns true if this field is read-only
     *
     * \see SetReadOnly
     */
    bool IsReadOnly() const;

    /** Required fields must have a value
     *  at the time the value is exported by a submit action
     * 
     *  \param bRequired if true this field requires a value for submit actions
     */
    void SetRequired( bool bRequired );

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
     *  \param bExport if false this field cannot be exported by submit actions
     */
    void SetNoExport( bool bExport );

    /** 
     * \returns true if this field can be exported by submit actions
     *
     * \see SetExport
     */
    bool IsNoExport() const;

    void SetMouseEnterAction( const PdfAction & rAction );
    void SetMouseLeaveAction( const PdfAction & rAction );
    void SetMouseDownAction( const PdfAction & rAction );
    void SetMouseUpAction( const PdfAction & rAction );

    void SetFocusEnterAction( const PdfAction & rAction );
    void SetFocusLeaveAction( const PdfAction & rAction );

    void SetPageOpenAction( const PdfAction & rAction );
    void SetPageCloseAction( const PdfAction & rAction );

    void SetPageVisibleAction( const PdfAction & rAction );
    void SetPageInvisibleAction( const PdfAction & rAction );

    void SetKeystrokeAction( const PdfAction & rAction );
    void SetValidateAction( const PdfAction & rAction );
    
    PdfFieldType GetType() const;

private:
    PdfField(const PdfField& rhs) = delete;

    void Init(PdfAcroForm *pParent);
    void AddAlternativeAction( const PdfName & rsName, const PdfAction & rAction );
    static PdfField* createField(PdfFieldType type, PdfObject& obj, PdfAnnotation* widget);
    PdfField * createChildField(PdfPage *page, const PdfRect &rect);

 public:
     PdfAnnotation* GetWidgetAnnotation() const;
     PdfObject& GetObject() const;
     PdfDictionary& GetDictionary();
     const PdfDictionary& GetDictionary() const;

 private:
     PdfFieldType  m_eField;
     PdfObject* m_pObject;
     PdfAnnotation* m_pWidget;
};

};

#endif // PDF_FIELD_H

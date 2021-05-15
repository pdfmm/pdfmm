/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_FIELD_H_
#define _PDF_FIELD_H_

#include "podofo/base/PdfDefines.h"  
#include "podofo/base/PdfName.h"
#include "podofo/base/PdfString.h"

#include "PdfAnnotation.h"

namespace PoDoFo {

class PdfAcroForm;
class PdfAction;
class PdfAnnotation;
class PdfDocument;
class PdfObject;
class PdfPage;
class PdfRect;
class PdfReference;
class PdfStreamedDocument;

/** The type of PDF field
 */
enum class EPdfField
{
    PushButton, 
    CheckBox,
    RadioButton,
    TextField,
    ComboBox,
    ListBox,
    Signature,

    Unknown = 0xff
};

/** The possible highlighting modes
 *  for a PdfField. I.e the visual effect
 *  that is to be used when the mouse 
 *  button is pressed.
 *
 *  The default value is 
 *  EPdfHighlightingMode::Invert
 */
enum class EPdfHighlightingMode
{
    None,           ///< Do no highlighting
    Invert,         ///< Invert the PdfField
    InvertOutline,  ///< Invert the fields border
    Push,           ///< Display the fields down appearance (requires an additional appearance stream to be set)
    Unknown = 0xff
};

enum class EPdfFieldFlags
{
    ReadOnly = 0x0001,
    Required = 0x0002,
    NoExport = 0x0004
};

class PODOFO_DOC_API PdfField
{
protected:
    PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect);

    PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfDocument &pDoc, bool insertInAcroform);

    PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc, bool bDefaultApperance );

    PdfField( EPdfField eField, PdfObject* pObject, PdfAnnotation* pWidget );

    /** 
     *  Set a bit in the field flags value of the fields dictionary.
     *
     *  \param lValue the value specifying the bits to set
     *  \param bSet if true the value will be set otherwise
     *              they will be cleared.
     *
     *  \see GetFieldFlag
     */
    void SetFieldFlag( long lValue, bool bSet );

    /**
     *  \param lValue it is checked if these bits are set
     *  \param bDefault the returned value if no field flags are specified
     *
     *  \returns true if given bits are set in the field flags
     *
     *  \see SetFieldFlag
     */
    bool GetFieldFlag( long lValue, bool bDefault ) const;

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
    PdfField( PdfObject* pObject, PdfAnnotation* pWidget );

    virtual ~PdfField() { }

    /** Create a PdfAcroForm dictionary object from an existing annottion
    *  \param pWidget the widget annotation of this field
    *  \returns the pointer to the created field
    */
    static PdfField * CreateField( PdfAnnotation* pWidget );

    /** Create a PdfAcroForm dictionary object from an existing object
    *  \returns the pointer to the created field
    */
    static PdfField * CreateField( PdfObject *pObject );

    PdfField * CreateChildField();

    PdfField * CreateChildField(PdfPage &page, const PdfRect &rect);

    /** Infer the field type from the given object
    *  \param pObject the object to infer the field type from
    *  \returns the inferred type
    */
    static EPdfField GetFieldType( const PdfObject &pObject );

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
    void SetHighlightingMode( EPdfHighlightingMode eMode );

    /** 
     * \returns the highlighting mode to be used when the user
     *          presses the mouse button over this widget
     */
    EPdfHighlightingMode GetHighlightingMode() const;
   
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
    PdfString GetName() const;

    /** \returns the field name of this PdfField at this level of the hierarchy
     */
    PdfString GetNameRaw() const;

    /** \returns the parents qualified name of this PdfField
     *
     *  \param escapePartialNames escape non compliant partial names
     */
    PdfString GetFullName(bool escapePartialNames = false) const;

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
    PdfString GetAlternateName() const;

    /**
     * Sets the fields mapping name which is used when exporting
     * the fields data
     *
     * \param rsName the mapping name of this PdfField
     */
    void SetMappingName( const PdfString & rsName ); 

    /** \returns the mapping name of this field
     */
    PdfString GetMappingName() const;

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
    
    /** 
     * \returns the type of this field
     */
    EPdfField GetType() const;

private:
    PdfField(const PdfField& rhs) = delete;

    void Init(PdfAcroForm *pParent);
    void AddAlternativeAction( const PdfName & rsName, const PdfAction & rAction );
    static PdfField * createField(EPdfField type, PdfObject* pObject, PdfAnnotation* pWidget );
    PdfField * createChildField(PdfPage *page, const PdfRect &rect);

 public:
     PdfAnnotation* GetWidgetAnnotation() const;
     PdfObject* GetFieldObject() const;
     PdfDictionary& GetDictionary();
     const PdfDictionary& GetDictionary() const;

 private:
     EPdfField  m_eField;
     PdfObject* m_pObject;
     PdfAnnotation* m_pWidget;
};

class PODOFO_DOC_API PdfButton : public PdfField
{
    friend class PdfField;
 protected:
    enum
    {
        ePdfButton_NoToggleOff      = 0x0004000,
        ePdfButton_Radio            = 0x0008000,
        ePdfButton_PushButton       = 0x0010000,
        ePdfButton_RadioInUnison    = 0x2000000
    };

    /** Create a new PdfButton
    */
    PdfButton(EPdfField eField, PdfAnnotation* pWidget, PdfDocument &doc, bool insertInAcrofrom);

    /** Create a new PdfButton
    */
    PdfButton( EPdfField eField, PdfObject* pObject, PdfAnnotation* pWidget );

    /** Create a new PdfButton
     */
    PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect);

 public:
    /**
     * \returns true if this is a pushbutton
     */
    bool IsPushButton() const;

    /**
     * \returns true if this is a checkbox
     */
    bool IsCheckBox() const;

    /**
     * \returns true if this is a radiobutton
     */
    bool IsRadioButton() const;

    /** Set the normal caption of this button
     *
     *  \param rsText the caption
     */
    void SetCaption( const PdfString & rsText );

    /** 
     *  \returns the caption of this button
     */
    const PdfString GetCaption() const;

};

/** A push button is a button which has no state and value
 *  but can toggle actions.
 */
class PODOFO_DOC_API PdfPushButton : public PdfButton
{
    friend class PdfField;
private:
    /** Create a new PdfPushButton
     */
    PdfPushButton( PdfObject* pObject, PdfAnnotation* pWidget );

public:
    /** Create a new PdfPushButton
     */
    PdfPushButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

    /** Create a new PdfPushButton
     */
    PdfPushButton( PdfPage* pPage, const PdfRect & rRect);

    /** Set the rollover caption of this button
     *  which is displayed when the cursor enters the field
     *  without the mouse button being pressed
     *
     *  \param rsText the caption
     */
    void SetRolloverCaption( const PdfString & rsText );

    /** 
     *  \returns the rollover caption of this button
     */
    const PdfString GetRolloverCaption() const;

    /** Set the alternate caption of this button
     *  which is displayed when the button is pressed.
     *
     *  \param rsText the caption
     */
    void SetAlternateCaption( const PdfString & rsText );

    /** 
     *  \returns the rollover caption of this button
     */
    const PdfString GetAlternateCaption() const;

 private:
    void Init();
};

/** A checkbox can be checked or unchecked by the user
 */
class PODOFO_DOC_API PdfCheckBox : public PdfButton
{
    friend class PdfField;
private:
    /** Create a new PdfCheckBox
     */
    PdfCheckBox( PdfObject* pObject, PdfAnnotation* pWidget );
public:
    /** Create a new PdfRadioButton
     */
    PdfCheckBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

    /** Create a new PdfCheckBox
     */
    PdfCheckBox( PdfPage* pPage, const PdfRect & rRect);

    /** Set the appearance stream which is displayed when the checkbox
     *  is checked.
     *
     *  \param rXObject an xobject which contains the drawing commands for a checked checkbox
     */
    void SetAppearanceChecked( const PdfXObject & rXObject );

    /** Set the appearance stream which is displayed when the checkbox
     *  is unchecked.
     *
     *  \param rXObject an xobject which contains the drawing commands for an unchecked checkbox
     */
    void SetAppearanceUnchecked( const PdfXObject & rXObject );

    /** Sets the state of this checkbox
     *
     *  \param bChecked if true the checkbox will be checked
     */
    void SetChecked( bool bChecked );

    /**
     * \returns true if the checkbox is checked
     */
    bool IsChecked() const;

 private:

    /** Add a appearance stream to this checkbox
     *
     *  \param rName name of the appearance stream
     *  \param rReference reference to the XObject containing the appearance stream
     */
    void AddAppearanceStream( const PdfName & rName, const PdfReference & rReference );
};

/** A radio button
 * TODO: This is just a stub
*/
class PODOFO_DOC_API PdfRadioButton : public PdfButton
{
    friend class PdfField;
private:
    /** Create a new PdfRadioButton
     */
    PdfRadioButton( PdfObject *pObject, PdfAnnotation* pWidget );
public:
    /** Create a new PdfRadioButton
     */
    PdfRadioButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

    /** Create a new PdfRadioButton
     */
    PdfRadioButton( PdfPage* pPage, const PdfRect & rRect);
};

/** A textfield in a PDF file.
 *  
 *  Users can enter text into a text field.
 *  Single and multi line text is possible,
 *  as well as richtext. The text can be interpreted
 *  as path to a file which is going to be submitted.
 */
class PODOFO_DOC_API PdfTextField : public PdfField
{
    friend class PdfField;
private:
    enum
    {
        ePdfTextField_MultiLine     = 0x0001000,
        ePdfTextField_Password      = 0x0002000,
        ePdfTextField_FileSelect    = 0x0100000,
        ePdfTextField_NoSpellcheck  = 0x0400000,
        ePdfTextField_NoScroll      = 0x0800000,
        ePdfTextField_Comb          = 0x1000000,
        ePdfTextField_RichText      = 0x2000000
    };

private:
    /** Create a new PdfTextField
     */
    PdfTextField( PdfObject *pObject, PdfAnnotation* pWidget );

public:
    /** Create a new PdfTextField
     */
    PdfTextField(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

    /** Create a new PdfTextField
     */
    PdfTextField( PdfPage* pPage, const PdfRect & rRect);

    /** Sets the text contents of this text field.
     *
     *  \param rsText the text of this field
     */
    void SetText( const PdfString & rsText );

    /**
     *  \returns the text contents of this text field
     */
    PdfString GetText() const;

    /** Sets the max length in characters of this textfield
     *  \param nMaxLen the max length of this textfields in characters
     */
    void SetMaxLen(int64_t nMaxLen);

    /** 
     * \returns the max length of this textfield in characters or -1
     *          if no max length was specified
     */
    int64_t GetMaxLen() const;

    /**
     *  Create a multi-line text field that can contains multiple lines of text.
     *  \param bMultiLine if true a multi line field is generated, otherwise
     *                    the text field can contain only a single line of text.
     *
     *  The default is to create a single line text field.
     */
    void SetMultiLine( bool bMultiLine );

    /** 
     * \returns true if this text field can contain multiple lines of text
     */
    bool IsMultiLine() const;

    /** 
     *  Create a password text field that should not echo entered
     *  characters visibly to the screen.
     *
     *  \param bPassword if true a password field is created
     *
     *  The default is to create no password field
     */
    void SetPasswordField( bool bPassword );

    /**
     * \returns true if this field is a password field that does
     *               not echo entered characters on the screen
     */
    bool IsPasswordField() const;

    /** 
     *  Create a file selection field.
     *  The entered contents are treated as filename to a file
     *  whose contents are submitted as the value of the field.
     *
     *  \param bFile if true the contents are treated as a pathname
     *               to a file to submit
     */
    void SetFileField( bool bFile );

    /**
     * \returns true if the contents are treated as filename
     */
    bool IsFileField() const;

    /** 
     *  Enable/disable spellchecking for this text field
     *
     *  \param bSpellcheck if true spellchecking will be enabled
     *
     *  Text fields are spellchecked by default
     */
    void SetSpellcheckingEnabled( bool bSpellcheck );

    /** 
     *  \returns true if spellchecking is enabled for this text field
     */
    bool IsSpellcheckingEnabled() const;

    /** 
     *  Enable/disable scrollbars for this text field
     *
     *  \param bScroll if true scrollbars will be enabled
     *
     *  Text fields have scrollbars by default
     */
    void SetScrollBarsEnabled( bool bScroll );

    /** 
     *  \returns true if scrollbars are enabled for this text field
     */
    bool IsScrollBarsEnabled() const;

    /** 
     *  Divide the text field into max-len equal
     *  combs.
     *
     *  \param bCombs if true enable division into combs
     *
     *  By default combs are disabled. Requires the max-len
     *  property to be set.
     *
     *  \see SetMaxLen
     */
    void SetCombs( bool bCombs );

    /**
     * \returns true if the text field has a division into equal combs set on it
     */
    bool IsCombs() const;

    /**
     * Creates a richtext field.
     *
     * \param bRichText if true creates a richtext field
     *
     * By default richtext is disabled.
     */
    void SetRichText( bool bRichText );

    /** 
     * \returns true if this is a richtext text field
     */
    bool IsRichText() const;

 private:
    void Init();
};

// TODO: Multiselect
/** A list of items in a PDF file.
 *  You cannot create this object directly, use
 *  PdfComboBox or PdfListBox instead.
 *  
 *  \see PdfComboBox 
 *  \see PdfListBox
 */
class PODOFO_DOC_API PdfListField : public PdfField
{
    friend class PdfField;
protected:
    enum
    {
        ePdfListField_Combo         = 0x0020000,
        ePdfListField_Edit          = 0x0040000,
        ePdfListField_Sort          = 0x0080000,
        ePdfListField_MultiSelect   = 0x0200000,
        ePdfListField_NoSpellcheck  = 0x0400000,
        ePdfListField_CommitOnSelChange = 0x4000000
    };

    /** Create a new PdfListField
     */
    PdfListField(EPdfField eField, PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

    /** Create a new PdfListField
     */
    PdfListField( EPdfField eField, PdfObject *pObject, PdfAnnotation* pWidget );

    /** Create a new PdfTextField
     */
    PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect);

 public:
    /**
     * Inserts a new item into the list
     *
     * @param rsValue the value of the item
     * @param rsDisplayName an optional display string that is displayed in the viewer
     *                      instead of the value
     */
    void InsertItem( const PdfString & rsValue, const PdfString & rsDisplayName = PdfString::StringNull );

    /** 
     * Removes an item for the list
     *
     * @param nIndex index of the item to remove
     */
    void RemoveItem(unsigned nIndex);

    /** 
     * @param nIndex index of the item
     * @returns the value of the item at the specified index
     */
    const PdfString GetItem(unsigned nIndex) const;

    /** 
     * @param nIndex index of the item
     * @returns the display text of the item or if it has no display text
     *          its value is returned. This call is equivalent to GetItem() 
     *          in this case
     *
     * \see GetItem
     */
    const PdfString GetItemDisplayText( int nIndex ) const;

    /**
     * \returns the number of items in this list
     */
    size_t GetItemCount() const;

    /** Sets the currently selected item
     *  \param nIndex index of the currently selected item
     */
    void SetSelectedIndex( int nIndex );

    /** Sets the currently selected item
     *
     *  \returns the selected item or -1 if no item was selected
     */
    int GetSelectedIndex() const;

    /** 
     * \returns true if this PdfListField is a PdfComboBox and false
     *               if it is a PdfListBox
     */
    bool IsComboBox() const;

    /** 
     *  Enable/disable spellchecking for this combobox
     *
     *  \param bSpellcheck if true spellchecking will be enabled
     *
     *  combobox are spellchecked by default
     */
    void SetSpellcheckingEnabled( bool bSpellcheck );
    
    /** 
     *  \returns true if spellchecking is enabled for this combobox
     */
    bool IsSpellcheckingEnabled() const;

    /**
     * Enable or disable sorting of items.
     * The sorting does not happen in acrobat reader
     * but whenever adding items using PoDoFo or another
     * PDF editing application.
     *
     * \param bSorted enable/disable sorting
     */
    void SetSorted( bool bSorted );

    /**
     * \returns true if sorting is enabled
     */
    bool IsSorted() const;

    /**
     * Sets wether multiple items can be selected by the
     * user in the list.
     *
     * \param bMulti if true multiselect will be enabled
     *
     * By default multiselection is turned off.
     */
    void SetMultiSelect( bool bMulti );

    /** 
     * \returns true if multi selection is enabled
     *               for this list
     */
    bool IsMultiSelect() const;

    void SetCommitOnSelectionChange( bool bCommit );
    bool IsCommitOnSelectionChange() const;
};

/** A combo box with a drop down list of items.
 */
class PODOFO_DOC_API PdfComboBox : public PdfListField
{
    friend class PdfField;
private:
    /** Create a new PdfComboBox
     */
    PdfComboBox( PdfObject *pObject, PdfAnnotation* pWidget );

public:
    /** Create a new PdfComboBox
     */
    PdfComboBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

    /** Create a new PdfTextField
     */
    PdfComboBox( PdfPage* pPage, const PdfRect & rRect);

    /**
     * Sets the combobox to be editable
     *
     * \param bEdit if true the combobox can be edited by the user
     *
     * By default a combobox is not editable
     */
    void SetEditable( bool bEdit );

    /** 
     *  \returns true if this is an editable combobox
     */
    bool IsEditable() const;

};

/** A list box
 */
class PODOFO_DOC_API PdfListBox : public PdfListField
{
    friend class PdfField;
 private:
    /** Create a new PdfListBox
     */
     PdfListBox( PdfObject *pObject, PdfAnnotation* pWidget );

public:
    /** Create a new PdfListBox
     */
    PdfListBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

    /** Create a new PdfListBox
     */
    PdfListBox( PdfPage* pPage, const PdfRect & rRect);

    /** Create a PdfListBox from a PdfField
     * 
     *  \param rhs a PdfField that is a PdfComboBox
     *
     *  Raises an error if PdfField::GetType() != EPdfField::ListBox
     */
    PdfListBox( const PdfField & rhs );

};

};

#endif // _PDF_ACRO_FORM_H__PDF_NAMES_TREE_H_

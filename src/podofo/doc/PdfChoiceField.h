/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CHOICE_FIELD_H
#define PDF_CHOICE_FIELD_H

#include "PdfField.h"

namespace PoDoFo
{
    // TODO: Multiselect
    /** A list of items in a PDF file.
     *  You cannot create this object directly, use
     *  PdfComboBox or PdfListBox instead.
     *
     *  \see PdfComboBox
     *  \see PdfListBox
     */
    class PODOFO_DOC_API PdChoiceField : public PdfField
    {
        friend class PdfField;
    protected:
        enum
        {
            ePdfListField_Combo = 0x0020000,
            ePdfListField_Edit = 0x0040000,
            ePdfListField_Sort = 0x0080000,
            ePdfListField_MultiSelect = 0x0200000,
            ePdfListField_NoSpellcheck = 0x0400000,
            ePdfListField_CommitOnSelChange = 0x4000000
        };

        PdChoiceField(PdfFieldType eField, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

        PdChoiceField(PdfFieldType eField, PdfObject& obj, PdfAnnotation* widget);

        PdChoiceField(PdfFieldType eField, PdfPage& page, const PdfRect& rect);

    public:
        /**
         * Inserts a new item into the list
         *
         * @param rsValue the value of the item
         * @param rsDisplayName an optional display string that is displayed in the viewer
         *                      instead of the value
         */
        void InsertItem(const PdfString& rsValue, const std::optional<PdfString>& rsDisplayName = {});

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
        PdfString GetItem(unsigned nIndex) const;

        /**
         * @param nIndex index of the item
         * @returns the display text of the item or if it has no display text
         *          its value is returned. This call is equivalent to GetItem()
         *          in this case
         *
         * \see GetItem
         */
        std::optional<PdfString> GetItemDisplayText(int nIndex) const;

        /**
         * \returns the number of items in this list
         */
        size_t GetItemCount() const;

        /** Sets the currently selected item
         *  \param nIndex index of the currently selected item
         */
        void SetSelectedIndex(int nIndex);

        /** Sets the currently selected item
         *
         *  \returns the selected item or -1 if no item was selected
         */
        int GetSelectedIndex() const;

        /**
         * \returns true if this PdChoiceField is a PdfComboBox and false
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
        void SetSpellcheckingEnabled(bool bSpellcheck);

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
        void SetSorted(bool bSorted);

        /**
         * \returns true if sorting is enabled
         */
        bool IsSorted() const;

        /**
         * Sets whether multiple items can be selected by the
         * user in the list.
         *
         * \param bMulti if true multiselect will be enabled
         *
         * By default multiselection is turned off.
         */
        void SetMultiSelect(bool bMulti);

        /**
         * \returns true if multi selection is enabled
         *               for this list
         */
        bool IsMultiSelect() const;

        void SetCommitOnSelectionChange(bool bCommit);
        bool IsCommitOnSelectionChange() const;
    };
}

#endif // PDF_CHOICE_FIELD_H

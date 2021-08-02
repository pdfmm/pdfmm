/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OUTLINE_H
#define PDF_OUTLINE_H

#include "podofo/base/PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfDestination;
class PdfAction;
class PdfObject;
class PdfOutlineItem;
class PdfString;
class PdfVecObjects;

/**
 * The title of an outline item can be displayed
 * in different formating styles since PDF 1.4.
 */
enum class PdfOutlineFormat
{
    Default    = 0x00,   ///< Default format
    Italic     = 0x01,   ///< Italic
    Bold       = 0x02,   ///< Bold
    BoldItalic = 0x03,   ///< Bold Italic
};

/**
 * A PDF outline item has an title and a destination.
 * It is an element in the documents outline which shows
 * its hierarchical structure.
 *
 * \see PdfDocument
 * \see PdfOutlines
 * \see PdfDestination
 */
class PODOFO_DOC_API PdfOutlineItem : public PdfElement
{
public:
    virtual ~PdfOutlineItem();

    /** Create a PdfOutlineItem that is a child of this item
     *  \param sTitle title of this item
     *  \param rDest destination of this item
     */
    PdfOutlineItem* CreateChild(const PdfString& sTitle, const std::shared_ptr<PdfDestination>& dest);

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param sTitle title of this item
     *  \param rDest destination of this item
     */
    PdfOutlineItem* CreateNext(const PdfString& sTitle, const std::shared_ptr<PdfDestination>& dest);

    /** Create a PdfOutlineItem that is on the same level and follows the current item.
     *  \param sTitle title of this item
     *  \param rAction action of this item
     */
    PdfOutlineItem* CreateNext(const PdfString& sTitle, const std::shared_ptr<PdfAction>& action);

    /** Inserts a new PdfOutlineItem as a child of this outline item.
     *  The former can't be in the same tree as this one, as the tree property
     *  would be broken. If this prerequisite is violated, a PdfError
     *  exception (code EPdfError::OutlineItemAlreadyPresent) is thrown and
     *  nothing is changed.
     *  The item inserted is not copied, i.e. Erase() calls affect the original!
     *  Therefore also shared ownership is in effect, i.e. deletion by where it
     *  comes from damages the data structure it's inserted into.
     *
     *  \param pItem an existing outline item
     */
    void InsertChild(PdfOutlineItem* pItem);

    /**
     * \returns the previous item or nullptr if this is the first on the current level
     */
    inline PdfOutlineItem* Prev() const { return m_pPrev; }

    /**
     * \returns the next item or nullptr if this is the last on the current level
     */
    inline PdfOutlineItem* Next() const { return m_pNext; }

    /**
     * \returns the first outline item that is a child of this item
     */
    inline PdfOutlineItem* First() const { return m_pFirst; }

    /**
     * \returns the last outline item that is a child of this item
     */
    inline PdfOutlineItem* Last() const { return m_pLast; }

    /**
     * \returns the parent item of this item or nullptr if it is
     *          the top level outlines dictionary
     */
    inline PdfOutlineItem* GetParentOutline() const { return m_pParentOutline; }

    /** Deletes this outline item and all its children from
     *  the outline hierarchy and removes all objects from
     *  the list of PdfObjects
     *  All pointers to this item will be invalid after this function
     *  call.
     */
    void Erase();

    /** Set the destination of this outline.
     *  \param rDest the destination
     */
    void SetDestination(const std::shared_ptr<PdfDestination>& dest);

    /** Get the destination of this outline.
     *  \param pDoc a PdfDocument owning this annotation.
     *         This is required to resolve names and pages.
     *  \returns the destination, if there is one, or nullptr
     */
    std::shared_ptr<PdfDestination> GetDestination() const;

    /** Set the action of this outline.
     *  \param rAction the action
     */
    void SetAction(const std::shared_ptr<PdfAction>& action);

    /** Get the action of this outline.
     *  \returns the action, if there is one, or nullptr
     */
    std::shared_ptr<PdfAction> GetAction() const;

    /** Set the title of this outline item
     *  \param sTitle the title to use
     */
    void SetTitle(const PdfString& sTitle);

    /** Get the title of this item
     *  \returns the title as PdfString
     */
    const PdfString& GetTitle() const;

    /** Set the text format of the title.
     *  Supported since PDF 1.4.
     *
     *  \param eFormat the formatting options
     *                 for the title
     */
    void SetTextFormat(PdfOutlineFormat eFormat);

    /** Get the text format of the title
     *  \returns the text format of the title
     */
    PdfOutlineFormat GetTextFormat() const;

    /** Set the color of the title of this item.
     *  This property is supported since PDF 1.4.
     *  \param r red color component
     *  \param g green color component
     *  \param b blue color component
     */
    void SetTextColor(double r, double g, double b);

    /** Get the color of the title of this item.
     *  Supported since PDF 1.4.
     *  \returns the red color component
     *
     *  \see SetTextColor
     */
    double GetTextColorRed() const;

    /** Get the color of the title of this item.
     *  Supported since PDF 1.4.
     *  \returns the red color component
     *
     *  \see SetTextColor
     */
    double GetTextColorBlue() const;

    /** Get the color of the title of this item.
     *  Supported since PDF 1.4.
     *  \returns the red color component
     *
     *  \see SetTextColor
     */
    double GetTextColorGreen() const;

private:
    std::shared_ptr<PdfAction> getAction();
    std::shared_ptr<PdfDestination> getDestination();
    void SetPrevious(PdfOutlineItem* pItem);
    void SetNext(PdfOutlineItem* pItem);
    void SetLast(PdfOutlineItem* pItem);
    void SetFirst(PdfOutlineItem* pItem);

    void InsertChildInternal(PdfOutlineItem* pItem, bool bCheckParent);

protected:
    /** Create a new PdfOutlineItem dictionary
     *  \param pParent parent vector of objects
     */
    PdfOutlineItem(PdfDocument& doc);

    /** Create a new PdfOutlineItem from scratch
     *  \param sTitle title of this item
     *  \param rDest destination of this item
     *  \param pParentOutline parent of this outline item
     *                        in the outline item hierarchie
     *  \param pParent parent vector of objects which is required
     *                 to create new objects
     */
    PdfOutlineItem(PdfDocument& doc, const PdfString& sTitle, const std::shared_ptr<PdfDestination>& dest,
        PdfOutlineItem* pParentOutline);

    /** Create a new PdfOutlineItem from scratch
     *  \param sTitle title of this item
     *  \param rAction action of this item
     *  \param pParentOutline parent of this outline item
     *                        in the outline item hierarchie
     *  \param pParent parent vector of objects which is required
     *                 to create new objects
     */
    PdfOutlineItem(PdfDocument& doc, const PdfString& sTitle, const std::shared_ptr<PdfAction>& action,
        PdfOutlineItem* pParentOutline);

    /** Create a PdfOutlineItem from an existing PdfObject
     *  \param pObject an existing outline item
     *  \param pParentOutline parent of this outline item
     *                        in the outline item hierarchie
     *  \param pPrevious previous item of this item
     */
    PdfOutlineItem(PdfObject& pObject, PdfOutlineItem* pParentOutline, PdfOutlineItem* pPrevious);

private:
    PdfOutlineItem* m_pParentOutline;

    PdfOutlineItem* m_pPrev;
    PdfOutlineItem* m_pNext;

    PdfOutlineItem* m_pFirst;
    PdfOutlineItem* m_pLast;

    std::shared_ptr<PdfDestination> m_destination;
    std::shared_ptr<PdfAction> m_action;
};

/** The main PDF outlines dictionary.
 *  
 *  Do not create it by yourself but 
 *  use PdfDocument::GetOutlines() instead.
 *
 *  \see PdfDocument
 */
class PODOFO_DOC_API PdfOutlines : public PdfOutlineItem
{
public:
    /** Create a new PDF outlines dictionary
     *  \param pParent parent vector of objects
     */
    PdfOutlines(PdfDocument& doc);

    /** Create a PDF outlines object from an existing dictionary
     *  \param pObject an existing outlines dictionary
     */
    PdfOutlines(PdfObject& obj);

    /** Create the root node of the
     *  outline item tree.
     *
     *  \param sTitle the title of the root node
     */
    PdfOutlineItem* CreateRoot(const PdfString& sTitle);
};

};

#endif // PDF_OUTLINE_H

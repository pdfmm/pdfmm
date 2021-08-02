/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */


#ifndef PDF_DIFFERENCE_ENCODING_H
#define PDF_DIFFERENCE_ENCODING_H

#include "PdfEncodingMap.h"
#include "PdfArray.h"

namespace PoDoFo {

/** A helper class for PdfDifferenceEncoding that
 *  can be used to create a differences array.
 */
class PODOFO_DOC_API PdfEncodingDifference
{
    struct Difference
    {
        unsigned char Code = 0;
        PdfName Name;
        char32_t CodePoint = U'\0';
    };

    typedef std::vector<Difference> DifferenceList;
    typedef std::vector<Difference>::iterator iterator;
    typedef std::vector<Difference>::const_iterator const_iterator;

public:
    /** Create a PdfEncodingDifference object.
     */
    PdfEncodingDifference();

    /** Copy a PdfEncodingDifference object.
     */
    PdfEncodingDifference(const PdfEncodingDifference& rhs);

    /** Copy a PdfEncodingDifference object.
     */
    const PdfEncodingDifference& operator=(const PdfEncodingDifference& rhs);

    /** Add a difference to the object.
     *
     *  \param nCode unicode code point of the difference (0 to 255 are legal values)
     *  \param unicodeValue actual unicode value for nCode; can be 0
     *
     *  \see AddDifference if you know the name of the code point
     *       use the overload below which is faster
     */
    void AddDifference(unsigned char code, char32_t codePoint);

    /** Add a difference to the object.
     *
     *  \param nCode unicode code point of the difference (0 to 255 are legal values)
     *  \param unicodeValue actual unicode value for nCode; can be 0
     *  \param rName name of the different code point or .notdef if none
     *  \param bExplicitKeys if true, the unicode value is set to nCode as rName is meaningless (Type3 fonts)
     */
    void AddDifference(unsigned char code, char32_t codePoint, const PdfName& name, bool explicitNames = false);

    /** Tests if the specified code is part of the
     *  differences.
     *
     *  \param nCode test if the given code is part of the differences
     *  \param rName write the associated name into this object if the
     *               code is part of the difference
     *  \param rValue write the associated unicode value of the name to this value
     *
     *  \returns true if the code is part of the difference
     */
    bool Contains(unsigned char code, PdfName& name, char32_t& codePoint) const;

    bool ContainsUnicodeValue(char32_t codePoint, unsigned char& code) const;

    /** Convert the PdfEncodingDifference to an array
     *
     *  \param rArray write to this array
     */
    void ToArray(PdfArray& arr) const;

    /** Get the number of differences in this object.
     *  If the user added .notdef as a difference it is
     *  counted, even it is no real difference in the final encoding.
     *
     *  \returns the number of differences in this object
     */
    size_t GetCount() const;

    const_iterator begin() const { return m_vecDifferences.begin(); }

    const_iterator end() const { return m_vecDifferences.end(); }

private:
    bool contains(unsigned char code, PdfName& name, char32_t& codePoint);

    struct DifferenceComparatorPredicate
    {
    public:
        inline bool operator()(const Difference& diff1, const Difference& diff2) const
        {
            return diff1.Code < diff2.Code;
        }
    };

    DifferenceList m_vecDifferences;
};

/**
 * Defines the base encoding from which a
 * PdfDifferenceEncoding differs.
 */
enum class EBaseEncoding
{
    Implicit,  ///< See Table 5.11 PdfRefence 1.7
    WinAnsi,   ///< Use WinAnsiEncoding as base encoding
    MacRoman,  ///< Use MacRomanEncoding as base encoding
    MacExpert  ///< Use MacExpertEncoding as base encoding
};

/** PdfDifferenceEncoding is an encoding, which is based
 *  on either the fonts encoding or a predefined encoding
 *  and defines differences to this base encoding.
 */
class PODOFO_DOC_API PdfDifferenceEncoding final : public PdfEncodingMapSimple
{
public:
    /** Create a new PdfDifferenceEncoding which is based on 
     *  a predefined encoding.
     *
     *  \param difference the differences in this encoding
     *  \param eBaseEncoding the base encoding of this font
     */
    PdfDifferenceEncoding(const PdfEncodingDifference& difference,
        EBaseEncoding eBaseEncoding);

    /** Create a new PdfDifferenceEncoding from an existing object
     *  in a PDF file.
     *
     *  \param pObject an existing differences encoding
     *  \param bAutoDelete if true the encoding is deleted by its owning font
     *  \param bExplicitNames if true, glyph names are meaningless explicit keys on the font (used for Type3 fonts)
     */
    PdfDifferenceEncoding(const PdfObject& obj, bool explicitNames = false);

    /** Convert a standard character name to a unicode code point
     * 
     *  \param rName a standard character name
     *  \returns an unicode code point
     */
    static char32_t NameToUnicodeID(const PdfName& name);
    static char32_t NameToUnicodeID(const std::string_view& name);

    /** Convert an unicode code point to a standard character name
     * 
     *  \param inCodePoint a code point
     *  \returns a standard character name of /.notdef if none could be found
     */
    static PdfName UnicodeIDToName(char32_t inCodePoint);

    /**
     * Get read-only access to the object containing the actual
     * differences.
     *
     * \returns the container with the actual differences
     */
    inline const PdfEncodingDifference& GetDifferences() const { return m_differences; }

protected:
    void getExportObject(PdfVecObjects& objects, PdfName& name, PdfObject*& obj) const override;
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

private:
    /** Get an object of type baseencoding 
     * 
     *  \returns a base encoding
     */
    const PdfEncodingMap& GetBaseEncoding() const;

 private:
    PdfEncodingDifference m_differences;
    EBaseEncoding m_baseEncoding; // The base encoding of this font
};

};

#endif // PDF_DIFFERENCE_ENCODING_H

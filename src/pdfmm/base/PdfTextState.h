/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_TEXT_STATE_H
#define PDF_TEXT_STATE_H

#include "PdfDefines.h"

namespace mm
{
    class PDFMM_API PdfTextState final
    {
    public:
        PdfTextState();

    public:
        /** Set the font size (operator Tf, controlling Tfs)
         *
         *  \param size font size in points
         */
        inline void SetFontSize(double size) { m_FontSize = size; }

        /** Retrieve the current font size (operator Tf, controlling Tfs)
         *  \returns the current font size
         */
        inline double GetFontSize() const { return m_FontSize; }

        /** Set the current horizontal scaling (operator Tz)
         *
         *  \param scale scaling in [0,1]
         */
        inline void SetFontScale(double scale) { m_FontScale = scale; }

        /** Retrieve the current horizontal scaling (operator Tz)
         *  \returns the current font scaling in [0,1]
         */
        inline double GetFontScale() const { return m_FontScale; }

        /** Set the character spacing (operator Tc)
         *  \param charSpace character spacing in percent
         */
        inline void SetCharSpace(double charSpace) { m_CharSpace = charSpace; }

        /** Retrieve the character spacing (operator Tc)
         *  \returns the current font character spacing
         */
        inline double GetCharSpace() const { return m_CharSpace; }

        /** Set the word spacing (operator Tw)
         *  \param fWordSpace word spacing in PDF units
         */
        inline void SetWordSpace(double wordSpace) { m_WordSpace = wordSpace; }

        /** Retrieve the current word spacing (operator Tw)
         *  \returns the current font word spacing in PDF units
         */
        inline double GetWordSpace() const { return m_WordSpace; }

        inline bool SetUnderlined(bool underlined) { m_Underlined = underlined; }

        inline bool IsUnderlined() const { return m_Underlined; }

        inline bool SetStrikeOut(bool strikedOut) { m_StrikedOut = strikedOut; }

        inline bool IsStrikeOut() const { return m_StrikedOut; }

    private:
        double m_FontSize;
        double m_FontScale;
        double m_CharSpace;
        double m_WordSpace;
        bool m_Underlined;
        bool m_StrikedOut;
    };
}

#endif // PDF_TEXT_STATE_H

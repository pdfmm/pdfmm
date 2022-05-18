/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_TEXT_STATE_H
#define PDF_TEXT_STATE_H

#include "PdfDeclarations.h"

namespace mm
{
    class PdfFont;

    enum class PdfTextStateProperty
    {
        Font,
        FontScale,
        CharSpacing,
        WordSpacing,
        RenderingMode,
    };

    class PDFMM_API PdfTextState final
    {
        friend class PdfPainter;

    public:
        PdfTextState();

    public:
        void SetFont(const PdfFont* font, double fontSize);

        /** Set the current horizontal scaling (operator Tz)
         *
         *  \param scale scaling in [0,1]
         */
        void SetFontScale(double scale);

        /** Set the character spacing (operator Tc)
         *  \param charSpace character spacing in percent
         */
        void SetCharSpacing(double charSpacing);

        /** Set the word spacing (operator Tw)
         *  \param fWordSpace word spacing in PDF units
         */
        void SetWordSpacing(double wordSpacing);

        void SetRenderingMode(PdfTextRenderingMode mode);

    public:
        inline const PdfFont* GetFont() const { return m_Font; }

        /** Retrieve the current font size (operator Tf, controlling Tfs)
         *  \returns the current font size
         */
        inline double GetFontSize() const { return m_FontSize; }

        /** Retrieve the current horizontal scaling (operator Tz)
         *  \returns the current font scaling in [0,1]
         */
        inline double GetFontScale() const { return m_FontScale; }

        /** Retrieve the character spacing (operator Tc)
         *  \returns the current font character spacing
         */
        inline double GetCharSpacing() const { return m_CharSpacing; }

        /** Retrieve the current word spacing (operator Tw)
         *  \returns the current font word spacing in PDF units
         */
        inline double GetWordSpacing() const { return m_WordSpacing; }

        inline PdfTextRenderingMode GetRenderingMode() const { return m_RenderingMode; }

        inline void SetUnderlined(bool underlined) { m_Underlined = underlined; }

        inline bool IsUnderlined() const { return m_Underlined; }

        inline void SetStrikeOut(bool strikedOut) { m_StrikedOut = strikedOut; }

        inline bool IsStrikeOut() const { return m_StrikedOut; }

    private:
        void SetPropertyChangedCallback(const std::function<void(PdfTextStateProperty)>& callback);

    private:
        std::function<void(PdfTextStateProperty)> m_PropertyChanged;
        const PdfFont* m_Font;
        double m_FontSize;
        double m_FontScale;
        double m_CharSpacing;
        double m_WordSpacing;
        PdfTextRenderingMode m_RenderingMode;
        bool m_Underlined;
        bool m_StrikedOut;
    };
}

#endif // PDF_TEXT_STATE_H

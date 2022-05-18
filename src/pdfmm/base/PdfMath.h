/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_MATH_H
#define PDF_MATH_H

#include "PdfDeclarations.h"
#include "PdfMathBase.h"

namespace mm
{
    class PdfRect;
    class PdfArray;

    class Matrix;

    class PDFMM_API Vector2 final
    {
    public:
        Vector2();
        Vector2(double x, double y);

    public:
        Vector2 operator+(const Vector2& v) const;
        Vector2 operator-(const Vector2& v) const;
        Vector2 operator*(const Matrix& m) const;

    public:
        Vector2(const Vector2&) = default;
        Vector2& operator=(const Vector2&) = default;

    public:
        double X;
        double Y;
    };

    class PDFMM_API Matrix final
    {
    public:
        /** Constructs an identity matrix
         */
        Matrix();

    public:
        static Matrix FromArray(const double arr[6]);
        static Matrix FromArray(const PdfArray& arr);
        static Matrix FromCoefficients(double a, double b, double c, double d, double e, double f);
        static Matrix CreateTranslation(const Vector2& tx);
        static Matrix CreateScale(const Vector2& scale);
        static Matrix CreateRotation(double teta);
        static Matrix CreateRotation(const Vector2& center, double teta);

    public:
        Matrix& Translate(const Vector2& tx);
        Matrix Translated(const Vector2& tx) const;

        // TODO: Rotate/Scale

    public:
        template <AlgebraicTrait trait>
        double Get() const
        {
            return MatrixTraits<trait>::Get(m_mat);
        }

        template <AlgebraicTrait trait>
        void Set(double value)
        {
            MatrixTraits<trait>::Set(m_mat, value);
        }

        template <AlgebraicTrait trait>
        Matrix& Apply(double value)
        {
            MatrixTraits<trait>::Apply(m_mat, value);
            return *this;
        }

    public:
        Matrix operator*(const Matrix& m) const;

    public:
        Vector2 GetScale() const;
        Vector2 GetTranslation() const;
        void ToArray(double arr[6]) const;
        void ToArray(PdfArray& arr) const;

    public:
        Matrix(const Matrix&) = default;
        Matrix& operator=(const Matrix&) = default;
        bool operator==(const Matrix& m) const;
        bool operator!=(const Matrix& m) const;

    public:
        const double& operator[](unsigned idx) const;

    private:
        Matrix(const double arr[6]);
        Matrix(double a, double b, double c, double d, double e, double f);

    private:
        double m_mat[6];
    };

    /** Get a rotation trasformation that aligns the rectangle to the axis after the rotation
     */
    Matrix PDFMM_API GetFrameRotationTransform(const PdfRect& rect, double teta);

    /** Get an inverse rotation trasformation that aligns the rectangle to the axis after the rotation
     */
    Matrix PDFMM_API GetFrameRotationTransformInverse(const PdfRect& rect, double teta);
}

#endif // PDF_MATH_H

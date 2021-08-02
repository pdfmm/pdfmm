/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfPostScriptTokenizer.h"

using namespace std;
using namespace PoDoFo;

PdfPostScriptTokenizer::PdfPostScriptTokenizer()
    : PdfTokenizer(false) { }

PdfPostScriptTokenizer::PdfPostScriptTokenizer(const PdfRefCountedBuffer& buffer)
    : PdfTokenizer(buffer, false) { }

void PdfPostScriptTokenizer::ReadNextVariant(PdfInputDevice& device, PdfVariant& variant)
{
    if (!TryReadNextVariant(device, variant))
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected variant");
}

bool PdfPostScriptTokenizer::TryReadNextVariant(PdfInputDevice& device, PdfVariant& variant)
{
    EPdfTokenType tokenType;
    string_view pszToken;
    if (!PdfTokenizer::TryReadNextToken(device, pszToken, tokenType))
        return false;

    return PdfTokenizer::TryReadNextVariant(device, pszToken, tokenType, variant, nullptr);
}

bool PdfPostScriptTokenizer::TryReadNext(PdfInputDevice& device, EPdfPostScriptTokenType& psTokenType, string_view& keyword, PdfVariant& variant)
{
    EPdfTokenType tokenType;
    string_view token;
    keyword = { };
    bool gotToken = PdfTokenizer::TryReadNextToken(device, token, tokenType);
    if (!gotToken)
    {
        psTokenType = EPdfPostScriptTokenType::Unknown;
        return false;
    }

    // Try first to detect PS procedures delimiters
    switch (tokenType)
    {
        case EPdfTokenType::BraceLeft:
            psTokenType = EPdfPostScriptTokenType::ProcedureEnter;
            return true;
        case EPdfTokenType::BraceRight:
            psTokenType = EPdfPostScriptTokenType::ProcedureExit;
            return true;
        default:
            // Continue evaluating data type
            break;
    }

    EPdfLiteralDataType eDataType = DetermineDataType(device, token, tokenType, variant);

    // asume we read a variant unless we discover otherwise later.
    psTokenType = EPdfPostScriptTokenType::Variant;
    switch (eDataType)
    {
        case EPdfLiteralDataType::Null:
        case EPdfLiteralDataType::Bool:
        case EPdfLiteralDataType::Number:
        case EPdfLiteralDataType::Real:
            // the data was already read into rVariant by the DetermineDataType function
            break;

        case EPdfLiteralDataType::Dictionary:
            this->ReadDictionary(device, variant, nullptr);
            break;
        case EPdfLiteralDataType::Array:
            this->ReadArray(device, variant, nullptr);
            break;
        case EPdfLiteralDataType::String:
            this->ReadString(device, variant, nullptr);
            break;
        case EPdfLiteralDataType::HexString:
            this->ReadHexString(device, variant, nullptr);
            break;
        case EPdfLiteralDataType::Name:
            this->ReadName(device, variant);
            break;
        case EPdfLiteralDataType::Reference:
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Unsupported reference datatype at this context");
        default:
            // Assume we have a keyword
            keyword = token;
            psTokenType = EPdfPostScriptTokenType::Keyword;
            break;
    }

    return true;
}

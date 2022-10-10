/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfPage.h"

#include <regex>
#include <deque>
#include <stack>

#include <utfcpp/utf8.h>

#include "PdfDocument.h"
#include "PdfTextState.h"
#include "PdfMath.h"
#include "PdfXObjectForm.h"
#include "PdfContentsReader.h"
#include "PdfFont.h"

#include <pdfmm/private/outstringstream.h>

using namespace std;
using namespace cmn;
using namespace mm;

constexpr double SAME_LINE_THRESHOLD = 0.01;
constexpr double SEPARATION_EPSILON = 0.0000001;
// Inferred empirically on Adobe Acrobat Pro
constexpr unsigned HARD_SEPARATION_SPACING_MULTIPLIER = 6;
#define ASSERT(condition, message, ...) if (!condition)\
    mm::LogMessage(PdfLogSeverity::Warning, message, ##__VA_ARGS__);

static constexpr float NaN = numeric_limits<float>::quiet_NaN();

// 5.2 Text State Parameters and Operators
// 5.3 Text Objects
struct TextState
{
    Matrix T_rm;  // Current T_rm
    Matrix CTM;   // Current CTM
    Matrix T_m;   // Current T_m
    Matrix T_lm;  // Current T_lm
    double T_l = 0;             // Leading text Tl
    PdfTextState PdfState;
    Vector2 WordSpacingVectorRaw;
    double WordSpacingLength = 0;
    void ComputeDependentState();
    void ComputeSpaceLength();
    void ComputeT_rm();

    double GetWordSpacingLength() const;
    double GetCharLength(char32_t ch) const;
    double GetStringLength(const string_view& str) const;        // utf8 string
    double GetStringLength(const PdfString& encodedStr) const;   // pdf encoded
};

class StatefulString
{
public:
    StatefulString(const string_view &str, double lengthRaw, const TextState &state);
public:
    bool BeginsWithWhiteSpace() const;
    bool EndsWithWhiteSpace() const;
    StatefulString GetTrimmedBegin() const;
    StatefulString GetTrimmedEnd() const;
public:
    const string String;
    const TextState State;
    Vector2 Position;
    const double LengthRaw;
    const double Length;
    const bool IsWhiteSpace;
};

struct EntryOptions
{
    bool IgnoreCase;
    bool TrimSpaces;
    bool TokenizeWords;
    bool MatchWholeWord;
    bool RegexPattern;
    bool ComputeBoundingBox;
    bool RawCoordinates;
};

using StringChunk = list<StatefulString>;
using StringChunkPtr = unique_ptr<StringChunk>;
using StringChunkList = list<StringChunkPtr>;
using StringChunkListPtr = unique_ptr<StringChunkList>;

class TextStateStack
{
private:
    stack<TextState> m_states;
    TextState *m_current;
public:
    TextState * const & Current;
public:
    TextStateStack();
    void Push();
    void Pop(unsigned popCount = 1);
    unsigned GetSize() const;
private:
    void push(const TextState &state);
};

struct XObjectState
{
    const PdfXObjectForm* Form;
    unsigned TextStateIndex;
};

struct ExtractionContext
{
public:
    ExtractionContext(vector<PdfTextEntry> &entries, const PdfPage &page, const string_view &pattern,
        const EntryOptions &options, const nullable<PdfRect> &clipRect);
public:
    void BeginText();
    void EndText();
    void Tf_Operator(const PdfName &fontname, double fontsize);
    void cm_Operator(double a, double b, double c, double d, double e, double f);
    void Tm_Operator(double a, double b, double c, double d, double e, double f);
    void TdTD_Operator(double tx, double ty);
    void AdvanceSpace(double ty);
    void TStar_Operator();
public:
    void PushString(const StatefulString &str, bool pushchunk = false);
    void TryPushChunk();
    void TryAddLastEntry();
private:
    bool areChunksSpaced(double& distance);
    void pushChunk();
    void addEntry();
    void tryAddEntry(const StatefulString& currStr);
    const PdfCanvas& getActualCanvas();
private:
    const PdfPage& m_page;
public:
    const int PageIndex;
    const string Pattern;
    const EntryOptions Options;
    const nullable<PdfRect> ClipRect;
    unique_ptr<Matrix> Rotation;
    vector<PdfTextEntry> &Entries;
    StringChunkPtr Chunk = std::make_unique<StringChunk>();
    StringChunkList Chunks;
    TextStateStack States;
    vector<XObjectState> XObjectStateIndices;
    double CurrentEntryT_rm_y = NaN;    // Tracks line changing
    Vector2 PrevChunkT_rm_Pos;          // Tracks space separation
    bool BlockOpen = false;
};


static bool decodeString(const PdfString &str, TextState &state, string &decoded, double &length);
static bool areEqual(double lhs, double rhs);
static bool isWhiteSpaceChunk(const StringChunk &chunk);
static void splitChunkBySpaces(vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk);
static void splitStringBySpaces(vector<StatefulString> &separatedStrings, const StatefulString &string);
static void trimSpacesBegin(StringChunk &chunk);
static void trimSpacesEnd(StringChunk &chunk);
static double getStringLength(const string_view &str, const TextState &state);
static void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &strings,
    const string_view &pattern, const EntryOptions &options, const nullable<PdfRect> &clipRect,
    int pageIndex, const Matrix* rotation);
static void addEntryChunk(vector<PdfTextEntry> &textEntries, StringChunkList &strings,
    const string_view &pattern, const EntryOptions& options, const nullable<PdfRect> &clipRect,
    int pageIndex, const Matrix* rotation);
static void processChunks(StringChunkList& chunks, string& str, double& length);
static void processChunks(StringChunkList& chunks, string& str, double& length, PdfRect& bbox);
static void read(const PdfVariantStack& stack, double &tx, double &ty);
static void read(const PdfVariantStack& stack, double &a, double &b, double &c, double &d, double &e, double &f);
static EntryOptions fromFlags(PdfTextExtractFlags flags);

void PdfPage::ExtractTextTo(vector<PdfTextEntry>& entries, const PdfTextExtractParams& params) const
{
    ExtractTextTo(entries, { }, params);
}

void PdfPage::ExtractTextTo(vector<PdfTextEntry>& entries, const string_view& pattern,
    const PdfTextExtractParams& params) const
{
    ExtractionContext context(entries, *this, pattern, fromFlags(params.Flags), params.ClipRect);

    // Look FIGURE 4.1 Graphics objects
    PdfContentsReader reader(*this);
    PdfContent content;
    while (reader.TryReadNext(content))
    {
        switch (content.Type)
        {
            case PdfContentType::Operator:
            {
                if ((content.Warnings & PdfContentWarnings::InvalidOperator)
                    != PdfContentWarnings::None)
                {
                    // Ignore invalid operators
                    continue;
                }

                // T_l TL: Set the text leading, T_l
                switch (content.Operator)
                {
                    case PdfOperator::TL:
                    {
                        context.States.Current->T_l = content.Stack[0].GetReal();
                        break;
                    }
                    case PdfOperator::cm:
                    {
                        double a, b, c, d, e, f;
                        read(content.Stack, a, b, c, d, e, f);
                        context.cm_Operator(a, b, c, d, e, f);
                        break;
                    }
                    // t_x t_y Td     : Move to the start of the next line
                    // t_x t_y TD     : Move to the start of the next line
                    // a b c d e f Tm : Set the text matrix, T_m , and the text line matrix, T_lm
                    case PdfOperator::Td:
                    case PdfOperator::TD:
                    case PdfOperator::Tm:
                    {
                        if (content.Operator == PdfOperator::Td || content.Operator == PdfOperator::TD)
                        {
                            double tx, ty;
                            read(content.Stack, tx, ty);
                            context.TdTD_Operator(tx, ty);

                            if (content.Operator == PdfOperator::TD)
                                context.States.Current->T_l = -ty;
                        }
                        else if (content.Operator == PdfOperator::Tm)
                        {
                            double a, b, c, d, e, f;
                            read(content.Stack, a, b, c, d, e, f);
                            context.Tm_Operator(a, b, c, d, e, f);
                        }
                        else
                        {
                            throw runtime_error("Invalid flow");
                        }

                        break;
                    }
                    // T*: Move to the start of the next line
                    case PdfOperator::T_Star:
                    {
                        // NOTE: Errata for the PDF Reference, sixth edition, version 1.7
                        // Section 5.3, Text Objects:
                        // This operator has the same effect as the code
                        //    0 -Tl Td
                        context.TStar_Operator();
                        break;
                    }
                    // BT: Begin a text object
                    case PdfOperator::BT:
                    {
                        context.BeginText();
                        break;
                    }
                    // ET: End a text object
                    case PdfOperator::ET:
                    {
                        context.EndText();
                        break;
                    }
                    // font size Tf : Set the text font, T_f
                    case PdfOperator::Tf:
                    {
                        double fontSize = content.Stack[0].GetReal();
                        auto& fontName = content.Stack[1].GetName();
                        context.Tf_Operator(fontName, fontSize);
                        break;
                    }
                    // string Tj : Show a text string
                    // string '  : Move to the next line and show a text string
                    // a_w a_c " : Move to the next line and show a text string,
                    //             using a_w as the word spacing and a_c as the
                    //             character spacing
                    case PdfOperator::Tj:
                    case PdfOperator::Quote:
                    case PdfOperator::DoubleQuote:
                    {
                        ASSERT(context.BlockOpen, "No text block open");

                        auto str = content.Stack[0].GetString();
                        if (content.Operator == PdfOperator::DoubleQuote)
                        {
                            // Operator " arguments: aw ac string "
                            context.States.Current->PdfState.CharSpacing = content.Stack[1].GetReal();
                            context.States.Current->PdfState.WordSpacing = content.Stack[2].GetReal();
                        }

                        string decoded;
                        double length;
                        if (decodeString(str, *context.States.Current, decoded, length)
                            && decoded.length() != 0)
                        {
                            context.PushString(StatefulString(decoded, length, *context.States.Current), true);
                        }

                        if (content.Operator == PdfOperator::Quote
                            || content.Operator == PdfOperator::DoubleQuote)
                        {
                            context.TStar_Operator();
                        }

                        break;
                    }
                    // array TJ : Show one or more text strings
                    case PdfOperator::TJ:
                    {
                        ASSERT(context.BlockOpen, "No text block open");

                        auto& array = content.Stack[0].GetArray();
                        for (unsigned i = 0; i < array.GetSize(); i++)
                        {
                            auto& obj = array[i];
                            if (obj.IsString())
                            {
                                string decoded;
                                double length;
                                if (decodeString(obj.GetString(), *context.States.Current, decoded, length)
                                    && decoded.length() != 0)
                                {
                                    context.PushString(StatefulString(decoded, length, *context.States.Current));
                                }
                            }
                            else if (obj.IsNumberOrReal())
                            {
                                // pg. 408, Pdf Reference 1.7: "The number is expressed in thousandths of a unit
                                // of text space. [...] This amount is subtracted from from the current horizontal or
                                // vertical coordinate, depending on the writing mode"
                                // It must be scaled by the font size
                                double space = (-obj.GetReal() / 1000) * context.States.Current->PdfState.FontSize;
                                context.AdvanceSpace(space);
                            }
                            else
                            {
                                mm::LogMessage(PdfLogSeverity::Warning, "Invalid array object type {}", obj.GetDataTypeString());
                            }
                        }

                        context.TryPushChunk();
                        break;
                    }
                    // Tc : word spacing
                    case PdfOperator::Tc:
                    {
                        context.States.Current->PdfState.CharSpacing = content.Stack[0].GetReal();
                        break;
                    }
                    case PdfOperator::Tw:
                    {
                        context.States.Current->PdfState.WordSpacing = content.Stack[0].GetReal();
                        break;
                    }
                    // q : Save the current graphics state
                    case PdfOperator::q:
                    {
                        ASSERT(!context.BlockOpen, "Text block must be not open");
                        context.States.Push();
                        break;
                    }
                    // Q : Restore the graphics state by removing
                    // the most recently saved state from the stack
                    case PdfOperator::Q:
                    {
                        ASSERT(!context.BlockOpen, "Text block must be not open");
                        context.States.Pop();
                        break;
                    }
                    default:
                    {
                        // Ignore all the other operators
                        break;
                    }
                }

                break;
            }
            case PdfContentType::ImageDictionary:
            case PdfContentType::ImageData:
            {
                // Ignore image data token
                break;
            }
            case PdfContentType::DoXObject:
            {
                if (content.XObject->GetType() == PdfXObjectType::Form)
                {
                    context.XObjectStateIndices.push_back({
                        (const PdfXObjectForm*)content.XObject.get(),
                        context.States.GetSize()
                    });
                    context.States.Push();
                }

                break;
            }
            case PdfContentType::EndXObjectForm:
            {
                PDFMM_ASSERT(context.XObjectStateIndices.size() != 0);
                context.States.Pop(context.States.GetSize() - context.XObjectStateIndices.back().TextStateIndex);
                context.XObjectStateIndices.pop_back();
                break;
            }
            default:
            {
                throw runtime_error("Unsupported PdfContentType");
            }
        }
    }

    // After finishing processing tokens, one entry may still
    // be inside the chunks
    context.TryAddLastEntry();
}

void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &chunks, const string_view &pattern,
    const EntryOptions &options, const nullable<PdfRect> &clipRect, int pageIndex, const Matrix* rotation)
{
    if (options.TokenizeWords)
    {
        // Split lines into chunks separated by at char space
        // NOTE: It doesn't trim empty strings, leading and trailing,
        // white characters yet!
        vector<StringChunkListPtr> batches;
        vector<StringChunkPtr> previousWhiteChunks;
        vector<StringChunkPtr> sepratedChunks;
        auto currentBatch = std::make_unique<StringChunkList>();
        while (true)
        {
            if (chunks.size() == 0)
            {
                // Chunks analysis finished. Try to push last batch
                if (currentBatch->size() != 0)
                    batches.push_back(std::move(currentBatch));

                break;
            }

            auto chunk = std::move(chunks.front());
            chunks.pop_front();

            splitChunkBySpaces(sepratedChunks, *chunk);
            for (auto &sepratedChunk : sepratedChunks)
            {
                if (isWhiteSpaceChunk(*sepratedChunk))
                {
                    // A white space chunk is separating words. Try to push a batch
                    if (currentBatch->size() != 0)
                    {
                        batches.push_back(std::move(currentBatch));
                        currentBatch = std::make_unique<StringChunkList>();
                    }

                    previousWhiteChunks.push_back(std::move(sepratedChunk));
                }
                else
                {
                    // Reinsert previous white space chunks, they won't be trimmed yet
                    for (auto &whiteChunk : previousWhiteChunks)
                        currentBatch->push_back(std::move(whiteChunk));

                    previousWhiteChunks.clear();
                    currentBatch->push_back(std::move(sepratedChunk));
                }
            }
        }

        for (auto& batch : batches)
        {
            addEntryChunk(textEntries, *batch, pattern, options,
                clipRect, pageIndex, rotation);
        }
    }
    else
    {
        addEntryChunk(textEntries, chunks, pattern, options,
            clipRect, pageIndex, rotation);
    }
}

void addEntryChunk(vector<PdfTextEntry> &textEntries, StringChunkList &chunks, const string_view &pattern,
    const EntryOptions& options, const nullable<PdfRect> &clipRect, int pageIndex, const Matrix* rotation)
{
    if (options.TrimSpaces)
    {
        // Trim spaces at the begin of the string
        while (true)
        {
            if (chunks.size() == 0)
                return;

            auto &front = chunks.front();
            if (isWhiteSpaceChunk(*front))
            {
                chunks.pop_front();
                continue;
            }

            trimSpacesBegin(*front);
            break;
        }

        // Trim spaces at the end of the string
        while (true)
        {
            auto &back = chunks.back();
            if (isWhiteSpaceChunk(*back))
            {
                chunks.pop_back();
                continue;
            }

            trimSpacesEnd(*back);
            break;
        }
    }

    PDFMM_ASSERT(chunks.size() != 0);
    auto &firstChunk = *chunks.front();
    PDFMM_ASSERT(firstChunk.size() != 0);
    auto &firstStr = firstChunk.front();
    if (clipRect.has_value() && !clipRect->Contains(firstStr.Position.X, firstStr.Position.Y))
    {
        chunks.clear();
        return;
    }

    nullable<PdfRect> bbox;
    string str;
    double length;
    if (options.ComputeBoundingBox)
    {
        PdfRect bbox_;
        processChunks(chunks, str, length, bbox_);
        bbox = bbox_;
    }
    else
    {
        processChunks(chunks, str, length);
    }

    if (pattern.length() != 0)
    {
        bool match;
        if (options.RegexPattern)
        {
            auto flags = regex_constants::ECMAScript;
            if (options.IgnoreCase)
                flags |= regex_constants::icase;

            // NOTE: regex_search returns true when a sub-part of the string
            // matches the regex
            regex pieces_regex((string)pattern, flags);
            match = std::regex_search(str, pieces_regex);
        }
        else
        {
            if (options.MatchWholeWord)
            {
                if (options.IgnoreCase)
                    match = utls::ToLower(str) == utls::ToLower(pattern);
                else
                    match = str == pattern;
            }
            else
            {
                if (options.IgnoreCase)
                    match = utls::ToLower(str).find(utls::ToLower(pattern)) != string::npos;
                else
                    match = str.find(pattern) != string::npos;
            }
        }

        if (!match)
        {
            chunks.clear();
            return;
        }
    }

    // Rotate to canonical frame
    if (rotation == nullptr || options.RawCoordinates)
    {
        textEntries.push_back(PdfTextEntry{ str, pageIndex,
            firstStr.Position.X, firstStr.Position.Y, length, bbox });
    }
    else
    {
        Vector2 rawp(firstStr.Position.X, firstStr.Position.Y);
        auto p_1 = rawp * (*rotation);
        textEntries.push_back(PdfTextEntry{ str, pageIndex,
            p_1.X, p_1.Y, length, bbox });
    }

    chunks.clear();
}

void read(const PdfVariantStack& tokens, double & tx, double & ty)
{
    ty = tokens[0].GetReal();
    tx = tokens[1].GetReal();
}

void read(const PdfVariantStack& tokens, double & a, double & b, double & c, double & d, double & e, double & f)
{
    f = tokens[0].GetReal();
    e = tokens[1].GetReal();
    d = tokens[2].GetReal();
    c = tokens[3].GetReal();
    b = tokens[4].GetReal();
    a = tokens[5].GetReal();
}

bool decodeString(const PdfString &str, TextState &state, string &decoded, double &length)
{
    if (state.PdfState.Font == nullptr)
    {
        length = 0;
        if (!str.IsHex())
        {
            // As a fallback try to retrieve the raw string
            decoded = str.GetString();
            return true;
        }

        return false;
    }

    decoded = state.PdfState.Font->GetEncoding().ConvertToUtf8(str);
    length = state.GetStringLength(str);
    return true;
}

StatefulString::StatefulString(const string_view &str, double lengthRaw, const TextState &state)
    : String((string)str), State(state), Position(state.T_rm.GetTranslationVector()),
      LengthRaw(lengthRaw), Length((Vector2(lengthRaw, 0) * state.CTM).GetLength()), IsWhiteSpace(utls::IsStringEmptyOrWhiteSpace(str))
{
    PDFMM_ASSERT(str.length() != 0);
}

StatefulString StatefulString::GetTrimmedBegin() const
{
    auto &str = String;
    auto it = str.begin();
    auto end = str.end();
    auto prev = it;
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (!utls::IsWhiteSpace(cp))
            break;

        prev = it;
    }

    // First, advance the x position by finding the space string size with the current font
    auto state = State;
    double spacestrLength = 0;
    size_t trimmedLen = prev - str.begin();
    if (trimmedLen != 0)
    {
        auto spacestr = str.substr(0, trimmedLen);
        spacestrLength = getStringLength(spacestr, state);
        state.T_m.Apply<Tx>(spacestrLength);
        state.ComputeDependentState();
    }

    // After, rewrite the string without spaces
    return StatefulString(str.substr(trimmedLen), LengthRaw - spacestrLength, state);
}

bool StatefulString::BeginsWithWhiteSpace() const
{
    auto& str = String;
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (utls::IsWhiteSpace(cp))
            return true;
    }

    return false;
}

bool StatefulString::EndsWithWhiteSpace() const
{
    bool isPrevWhiteSpace = false;
    auto& str = String;
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        isPrevWhiteSpace = utls::IsWhiteSpace(cp);
    }

    return isPrevWhiteSpace;
}

StatefulString StatefulString::GetTrimmedEnd() const
{
    string trimmedStr = utls::TrimSpacesEnd(String);
    return StatefulString(trimmedStr, getStringLength(trimmedStr, State), State);
}

TextStateStack::TextStateStack()
    : m_current(nullptr), Current(m_current)
{
    push({ });
}

void TextStateStack::Push()
{
    push(m_states.top());
}

void TextStateStack::Pop(unsigned popCount)
{
    if (popCount >= m_states.size())
        throw runtime_error("Can't pop out all the states in the stack");

    for (unsigned i = 0; i < popCount; i++)
        m_states.pop();

    m_current = &m_states.top();
}

unsigned TextStateStack::GetSize() const
{
    return (unsigned)m_states.size();
}

void TextStateStack::push(const TextState & state)
{
    m_states.push(state);
    m_current = &m_states.top();
}

ExtractionContext::ExtractionContext(vector<PdfTextEntry> &entries, const PdfPage &page, const string_view &pattern,
    const EntryOptions & options, const nullable<PdfRect>& clipRect) :
    m_page(page),
    PageIndex(page.GetPageNumber() - 1),
    Pattern(pattern),
    Options(options),
    ClipRect(clipRect),
    Entries(entries)
{
    // Determine page rotation transformation
    double teta;
    if (page.HasRotation(teta))
        Rotation = std::make_unique<Matrix>(mm::GetFrameRotationTransform(page.GetRect(), teta));
}

void ExtractionContext::BeginText()
{
    ASSERT(!BlockOpen, "Text block already open");

    // NOTE: BT doesn't reset font
    BlockOpen = true;
}

void ExtractionContext::EndText()
{
    ASSERT(BlockOpen, "No text block open");
    States.Current->T_m = Matrix();
    States.Current->T_lm = Matrix();
    States.Current->ComputeDependentState();
    BlockOpen = false;
}

void ExtractionContext::Tf_Operator(const PdfName &fontname, double fontsize)
{
    auto fontObj = getActualCanvas().GetFromResources("Font", fontname);
    auto &doc = m_page.GetDocument();
    double spacingLengthRaw = 0;
    States.Current->PdfState.FontSize = fontsize;
    if (fontObj == nullptr || (States.Current->PdfState.Font = doc.GetFontManager().GetLoadedFont(*fontObj)) == nullptr)
    {
        mm::LogMessage(PdfLogSeverity::Warning, "Unable to find font object {}", fontname.GetString());
    }
    else
    {
        spacingLengthRaw = States.Current->GetWordSpacingLength();
    }

    States.Current->WordSpacingVectorRaw = Vector2(spacingLengthRaw, 0);
    if (spacingLengthRaw == 0)
    {
        mm::LogMessage(PdfLogSeverity::Warning, "Unable to provide a space size, setting default font size");
        States.Current->WordSpacingVectorRaw = Vector2(fontsize, 0);
    }
}

void ExtractionContext::cm_Operator(double a, double b, double c, double d, double e, double f)
{
    // TABLE 4.7: "cm" Modify the current transformation
    // matrix (CTM) by concatenating the specified matrix
    Matrix cm = Matrix::FromCoefficients(a, b, c, d, e, f);
    States.Current->CTM = cm * States.Current->CTM;
    States.Current->ComputeT_rm();
}

void ExtractionContext::Tm_Operator(double a, double b, double c, double d, double e, double f)
{
    States.Current->T_lm = Matrix::FromCoefficients(a, b, c, d, e, f);
    States.Current->T_m = States.Current->T_lm;
    States.Current->ComputeDependentState();
}

void ExtractionContext::TdTD_Operator(double tx, double ty)
{
    // 5.5 Text-positioning operators, Td/TD
    States.Current->T_lm = States.Current->T_lm.Translate(Vector2(tx, ty));
    States.Current->T_m = States.Current->T_lm;
    States.Current->ComputeDependentState();
}

void ExtractionContext::TStar_Operator()
{
    States.Current->T_lm.Apply<Ty>(-States.Current->T_l);
    States.Current->T_m = States.Current->T_lm;
    States.Current->ComputeDependentState();
}

void ExtractionContext::AdvanceSpace(double tx)
{
    States.Current->T_m.Apply<Tx>(tx);
    States.Current->ComputeDependentState();
}

void ExtractionContext::PushString(const StatefulString &str, bool pushchunk)
{
    PDFMM_ASSERT(str.String.length() != 0);
    if (std::isnan(CurrentEntryT_rm_y))
    {
        // Initalize tracking for line
        CurrentEntryT_rm_y = States.Current->T_rm.Get<Ty>();
    }

    tryAddEntry(str);

    // Set current line tracking
    CurrentEntryT_rm_y = States.Current->T_rm.Get<Ty>();
    Chunk->push_back(str);
    if (pushchunk)
        pushChunk();

    States.Current->T_m.Apply<Tx>(str.LengthRaw);
    States.Current->ComputeDependentState();
    PrevChunkT_rm_Pos = States.Current->T_rm.GetTranslationVector();
}

void ExtractionContext::TryPushChunk()
{
    if (Chunk->size() == 0)
        return;

    pushChunk();
}

void ExtractionContext::pushChunk()
{
    Chunks.push_back(std::move(Chunk));
    Chunk = std::make_unique<StringChunk>();
}

void ExtractionContext::TryAddLastEntry()
{
    if (Chunks.size() > 0)
        addEntry();
}

const PdfCanvas& ExtractionContext::getActualCanvas()
{
    if (XObjectStateIndices.size() == 0)
        return m_page;

    return *XObjectStateIndices.back().Form;
}

void ExtractionContext::addEntry()
{
    ::addEntry(Entries, Chunks, Pattern, Options, ClipRect, PageIndex, Rotation.get());
}

void ExtractionContext::tryAddEntry(const StatefulString& currStr)
{
    PDFMM_INVARIANT(Chunk != nullptr);
    if (Chunks.size() > 0 || Chunk->size() > 0)
    {
        if (areEqual(States.Current->T_rm.Get<Ty>(), CurrentEntryT_rm_y))
        {
            double distance;
            if (areChunksSpaced(distance))
            {
                if (Options.TokenizeWords
                    || distance + SEPARATION_EPSILON >
                        States.Current->WordSpacingLength * HARD_SEPARATION_SPACING_MULTIPLIER)
                {
                    // Current entry is space separated and either we
                    //  tokenize words, or it's an hard entry separation
                    TryPushChunk();
                    addEntry();
                }
                else
                {
                    const StatefulString* prevString;
                    if (Chunk->size() > 0)
                    {
                        prevString = &Chunk->back();
                    }
                    else
                    {
                        PDFMM_INVARIANT(Chunks.back()->size() != 0);
                        prevString = &Chunks.back()->back();
                    }

                    // Add "fake" space
                    if (!(prevString->EndsWithWhiteSpace() || currStr.BeginsWithWhiteSpace()))
                        Chunk->push_back(StatefulString(" ", distance, prevString->State));
                }
            }
        }
        else
        {
            // Current entry is not on same line
            TryPushChunk();
            addEntry();

        }
    }
}

bool ExtractionContext::areChunksSpaced(double& distance)
{
    // TODO
    // 1) Handle arbitraries rotations
    // 2) Handle the word spacing Tw state
    // 3) Handle the char spacing Tc state (is it actually needed?)
    // 4) Handle vertical scripts (HARD)
    distance = (States.Current->T_rm.GetTranslationVector() - PrevChunkT_rm_Pos).GetLength();
    return distance + SEPARATION_EPSILON >= States.Current->WordSpacingLength;
}

// Separate chunk words by spaces
void splitChunkBySpaces(vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk)
{
    PDFMM_ASSERT(chunk.size() != 0);
    splittedChunks.clear();

    vector<StatefulString> separatedStrings;
    for (auto &str : chunk)
    {
        auto separatedChunk = std::make_unique<StringChunk>();
        bool previousWhiteSpace = true;
        splitStringBySpaces(separatedStrings, str);
        for (auto &separatedStr : separatedStrings)
        {
            if (separatedChunk->size() != 0 && separatedStr.IsWhiteSpace != previousWhiteSpace)
            {
                splittedChunks.push_back(std::move(separatedChunk));
                separatedChunk = std::make_unique<StringChunk>();
            }

            separatedChunk->push_back(separatedStr);
            previousWhiteSpace = separatedStr.IsWhiteSpace;
        }

        // Push back last chunk, if present
        if (separatedChunk->size() != 0)
            splittedChunks.push_back(std::move(separatedChunk));
    }
}

// Separate string words by spaces
void splitStringBySpaces(vector<StatefulString> &separatedStrings, const StatefulString &str)
{
    PDFMM_ASSERT(str.String.length() != 0);
    separatedStrings.clear();

    string separatedStr;
    auto state = str.State;

    auto pushString = [&]() {
        double length = getStringLength(separatedStr, state);
        separatedStrings.push_back({ separatedStr , length, state });
        separatedStr.clear();
        state.T_m.Apply<Tx>(length);
        state.ComputeDependentState();
    };

    bool isPreviousWhiteSpace = true;
    for (int i = 0; i < (int)str.String.size(); i++)
    {
        char ch = str.String[i];
        bool isCurrentWhiteSpace = std::isspace((unsigned char)ch) != 0;
        if (separatedStr.length() != 0 && isCurrentWhiteSpace != isPreviousWhiteSpace)
            pushString();

        separatedStr.push_back(ch);
        isPreviousWhiteSpace = isCurrentWhiteSpace;
    }

    // Push back last string, if present
    if (separatedStr.length() != 0)
        pushString();
}

void trimSpacesBegin(StringChunk &chunk)
{
    while (true)
    {
        if (chunk.size() == 0)
            break;

        auto &front = chunk.front();
        if (!front.IsWhiteSpace)
        {
            auto trimmed = front.GetTrimmedBegin();

            chunk.pop_front();
            chunk.push_front(trimmed);
            break;
        }

        chunk.pop_front();
    }
}

void trimSpacesEnd(StringChunk &chunk)
{
    while (true)
    {
        if (chunk.size() == 0)
            break;

        auto &back = chunk.back();
        if (!back.IsWhiteSpace)
        {
            auto trimmed = back.GetTrimmedEnd();
            chunk.pop_back();
            chunk.push_back(trimmed);
            break;
        }

        chunk.pop_back();
    }
}

double getStringLength(const string_view &str, const TextState &state)
{
    if (state.PdfState.Font == nullptr)
        return 0;

    return state.GetStringLength(str);
}

bool isWhiteSpaceChunk(const StringChunk &chunk)
{
    for (auto &str : chunk)
    {
        if (!str.IsWhiteSpace)
            return false;
    }

    return true;
}

bool areEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < SAME_LINE_THRESHOLD;
}

void TextState::ComputeDependentState()
{
    ComputeSpaceLength();
    ComputeT_rm();
}

void TextState::ComputeSpaceLength()
{
    WordSpacingLength = (WordSpacingVectorRaw * T_m.GetScalingRotation()).GetLength();
}

void TextState::ComputeT_rm()
{
    T_rm = T_m * CTM;
}

double TextState::GetWordSpacingLength() const
{
    return PdfState.Font->GetWordSpacingLength(PdfState);
}

double TextState::GetCharLength(char32_t ch) const
{
    return PdfState.Font->GetCharLength(ch, PdfState);
}

double TextState::GetStringLength(const string_view& str) const
{
    return PdfState.Font->GetStringLength(str, PdfState);
}

double TextState::GetStringLength(const PdfString& encodedStr) const
{
    return PdfState.Font->GetStringLength(encodedStr, PdfState);
}

EntryOptions fromFlags(PdfTextExtractFlags flags)
{
    EntryOptions ret;
    ret.IgnoreCase = (flags & PdfTextExtractFlags::IgnoreCase) != PdfTextExtractFlags::None;
    ret.MatchWholeWord = (flags & PdfTextExtractFlags::MatchWholeWord) != PdfTextExtractFlags::None;
    ret.RegexPattern = (flags & PdfTextExtractFlags::RegexPattern) != PdfTextExtractFlags::None;
    ret.TokenizeWords = (flags & PdfTextExtractFlags::TokenizeWords) != PdfTextExtractFlags::None;
    ret.TrimSpaces = (flags & PdfTextExtractFlags::KeepWhiteTokens) == PdfTextExtractFlags::None || ret.TokenizeWords;
    ret.ComputeBoundingBox = (flags & PdfTextExtractFlags::ComputeBoundingBox) != PdfTextExtractFlags::None;
    ret.RawCoordinates = (flags & PdfTextExtractFlags::RawCoordinates) != PdfTextExtractFlags::None;
    return ret;
}

void processChunks(StringChunkList& chunks, string& dest, double& length)
{
    for (auto& chunk : chunks)
    {
        for (auto& str : *chunk)
            dest.append(str.String.data(), str.String.length());
    }
    auto& first = chunks.front()->front();
    auto& last = chunks.back()->back();
    length = (last.Position - first.Position).GetLength() + last.Length;
}

void processChunks(StringChunkList& chunks, string& dest, double& length, PdfRect& bbox)
{
    for (auto& chunk : chunks)
    {
        for (auto& str : *chunk)
            dest.append(str.String.data(), str.String.length());
    }
    auto& first = chunks.front()->front();
    auto& last = chunks.back()->back();
    length = (last.Position - first.Position).GetLength() + last.Length;
    auto font = first.State.PdfState.Font;
    // NOTE: This is very inaccurate
    double descend = 0;
    double ascent = 0;
    if (font != nullptr)
    {
        descend = ((Vector2(0, font->GetMetrics().GetDescent()) * first.State.T_m) * first.State.CTM).GetLength();
        ascent = ((Vector2(0, font->GetMetrics().GetAscent()) * first.State.T_m) * first.State.CTM).GetLength();
    }

    bbox = PdfRect(first.Position.X, first.Position.Y - descend, length, descend + ascent);
}

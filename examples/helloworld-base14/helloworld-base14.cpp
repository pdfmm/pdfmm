/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// Include the standard headers for cout to write
// some output to the console.
#include <iostream>

// Now include all pdfmm header files, to have access
// to all functions of pdfmm and so that you do not have
// to care about the order of includes.
// 
// You should always use pdfmm.h and not try to include
// the required headers on your own.
#include <pdfmm/pdfmm.h>

// All pdfmm classes are member of the pdfmm namespace.
using namespace std;
using namespace mm;

void PrintHelp()
{
    std::cout << "This is a example application for the pdfmm PDF library." << std::endl
        << "It creates a small PDF file containing the text >Hello World!<" << std::endl
        << "Please see https://github.com/pdfmm/pdfmm for more information" << std::endl << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  examplehelloworld [outputfile.pdf]" << std::endl << std::endl;
}

const char* GetBase14FontName(unsigned i);
void DemoBase14Fonts(PdfPainter& painter, PdfPage& page, PdfDocument& document, const PdfFontSearchParams& params);

void HelloWorld(const string_view& filename)
{
    PdfMemDocument document;

    // PdfPainter is the class which is able to draw text and graphics
    // directly on a PdfPage object.
    PdfPainter painter;

    // This pointer will hold the page object later.
    // PdfSimpleWriter can write several PdfPage's to a PDF file.
    PdfPage* page;

    // A PdfFont object is required to draw text on a PdfPage using a PdfPainter.
    // pdfmm will find the font using fontconfig on your system and embedd truetype
    // fonts automatically in the PDF file.
    PdfFont* font;

    try
    {
        // The PdfDocument object can be used to create new PdfPage objects.
        // The PdfPage object is owned by the PdfDocument will also be deleted automatically
        // by the PdfDocument object.
        // 
        // You have to pass only one argument, i.e. the page size of the page to create.
        // There are predefined enums for some common page sizes.
        page = document.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));

        // If the page cannot be created because of an error (e.g. ePdfError_OutOfMemory )
        // a nullptr pointer is returned.
        // We check for a nullptr pointer here and throw an exception using the RAISE_ERROR macro.
        // The raise error macro initializes a PdfError object with a given error code and
        // the location in the file in which the error ocurred and throws it as an exception.

        // Set the page as drawing target for the PdfPainter.
        // Before the painter can draw, a page has to be set first.
        painter.SetCanvas(page);

        // Create a PdfFont object using the font "Arial".
        // The font is found on the system using fontconfig and embedded into the
        // PDF file. If Arial is not available, a default font will be used.
        // 
        // The created PdfFont will be deleted by the PdfDocument.
        PdfFontSearchParams params;
        params.AutoSelect = PdfFontAutoSelectBehavior::Standard14;
        font = document.GetFontManager().GetFont("Helvetica", params);

        // If the PdfFont object cannot be allocated return an error.
        if (font == nullptr)
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

        // Set the font size
        painter.GetTextState().SetFontSize(18.0);

        // Set the font as default font for drawing.
        // A font has to be set before you can draw text on
        // a PdfPainter.
        painter.SetFont(font);

        // You could set a different color than black to draw
        // the text.
        // 
        // painter.SetColor(1.0, 0.0, 0.0);

        // Actually draw the line "Hello World!" on to the PdfPage at
        // the position 2cm,2cm from the top left corner.
        // Please remember that PDF files have their origin at the
        // bottom left corner. Therefore we substract the y coordinate
        // from the page height.
        //
        // The position specifies the start of the baseline of the text.
        //
        // All coordinates in pdfmm are in PDF units.
        // You can also use PdfPainterMM which takes coordinates in 1/1000th mm.
        painter.DrawText(56.69, page->GetRect().GetHeight() - 56.69, "Hello World!");

        DemoBase14Fonts(painter, *page, document, params);

        painter.FinishDrawing();

        // The last step is to close the document.
        document.Save(filename);

    }
    catch (PdfError& e)
    {
        // All pdfmm methods may throw exceptions
        // make sure that painter.FinishPage() is called
        // or who will get an assert in its destructor
        try
        {
            painter.FinishDrawing();
        }
        catch (...)
        {
            // Ignore errors this time
        }

        throw e;
    }
}

int main(int argc, char* argv[])
{
    // Check if a filename was passed as commandline argument.
    // If more than 1 argument or no argument is passed,
    // a help message is displayed and the example application
    // will quit.
    if (argc != 2)
    {
        PrintHelp();
        return -1;
    }

    // All pdfmm functions will throw an exception in case of an error.
    // 
    // You should catch the exception to either fix it or report
    // back to the user.
    // 
    // All exceptions pdfmm throws are objects of the class PdfError.
    // Thats why we simply catch PdfError objects.
    try
    {
        // Call the drawing routing which will create a PDF file
        // with the filename of the output file as argument.
        HelloWorld(argv[1]);
    }
    catch (PdfError& err)
    {
        // We have to check if an error has occurred.
        // If yes, we return and print an error message
        // to the commandline.
        err.PrintErrorMsg();
        return (int)err.GetError();
    }

    // The PDF was created sucessfully.
    std::cout << std::endl
        << "Created a PDF file containing the line \"Hello World!\": " << argv[1] << std::endl << std::endl;

    return 0;
}

// Base14 + other non-Base14 fonts for comparison

static const char* s_base14fonts[] =
{
    "Times-Roman",
    "Times-Italic",
    "Times-Bold",
    "Times-BoldItalic",
    "Helvetica",
    "Helvetica-Oblique",
    "Helvetica-Bold",
    "Helvetica-BoldOblique",
    "Courier",
    "Courier-Oblique",
    "Courier-Bold",
    "Courier-BoldOblique",
    "Symbol",
    "ZapfDingbats",
    "Arial",
    "Verdana"
};

const char* GetBase14FontName(unsigned i)
{
    if (i >= std::size(s_base14fonts))
        return nullptr;

    return s_base14fonts[i];
}

void DrawRedFrame(PdfPainter& painter, double x, double y, double width, double height)
{
    // draw red box
    painter.SetColor(PdfColor(1.0f, 0.0f, 0.0f));
    painter.SetStrokingColor(PdfColor(1.0f, 0.0f, 0.0f));
    painter.DrawLine(x, y, x + width, y);
    if (height > 0.0f)
    {
        painter.DrawLine(x, y, x, y + height);
        painter.DrawLine(x + width, y, x + width, y + height);
        painter.DrawLine(x, y + height, x + width, y + height);
    }
    // restore to black
    painter.SetColor(PdfColor(0.0f, 0.0f, 0.0f));
    painter.SetStrokingColor(PdfColor(0.0f, 0.0f, 0.0f));
}

void DemoBase14Fonts(PdfPainter& painter, PdfPage& page, PdfDocument& document, const PdfFontSearchParams& params)
{
    double x = 56, y = page.GetRect().GetHeight() - 56.69;
    string_view demo_text = "abcdefgABCDEFG12345!#$%&+-@?        ";
    double height = 0.0f, width = 0.0f;

    // draw sample of all types
    for (unsigned i = 0; i < std::size(s_base14fonts); i++)
    {
        x = 56; y = y - 25;
        string text = (string)demo_text;
        text.append(GetBase14FontName(i));

        PdfFont* font = document.GetFontManager().GetFont(GetBase14FontName(i), params);
        if (font == nullptr)
            throw runtime_error("Font not found");

        painter.SetFont(font);
        painter.GetTextState().SetFontSize(12.0);

        width = font->GetStringWidth(text, painter.GetTextState());
        height = font->GetMetrics().GetLineSpacing();

        std::cout << GetBase14FontName(i) << " Width = " << width << " Height = " << height << std::endl;

        // draw red box
        DrawRedFrame(painter, x, y, width, height);

        // draw text
        painter.DrawText(x, y, text);
    }

    // draw some individual characters:
    string_view demo_text2 = " @_1jiPlg .;";

    // draw  individuals
    for (unsigned i = 0; i < demo_text2.length(); i++)
    {
        x = 56; y = y - 25;
        string text;
        if (i == 0)
            text = "Helvetica / Arial Comparison:";
        else
            text = (string)demo_text2.substr(i, 1);

        PdfFont* font = document.GetFontManager().GetFont("Helvetica", params);
        painter.SetFont(font);
        height = font->GetMetrics().GetLineSpacing();
        width = font->GetStringWidth(text, painter.GetTextState());

        // draw red box
        DrawRedFrame(painter, x, y, width, height);

        // draw text
        painter.DrawText(x, y, text);

        if (i > 0)
        {
            // draw again, with non-Base14 font
            PdfFont* font2 = document.GetFontManager().GetFont("Arial", params);
            painter.SetFont(font2);
            height = font2->GetMetrics().GetLineSpacing();
            width = font2->GetStringWidth((string_view)text, painter.GetTextState());

            // draw red box
            DrawRedFrame(painter, x + 100, y, width, height);

            // draw text
            painter.DrawText(x + 100, y, text);
        }
    }
}

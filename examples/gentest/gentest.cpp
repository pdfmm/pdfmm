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
#include <csignal>
#include "custom_painter.h"

// All pdfmm classes are member of the pdfmm namespace.
using namespace std;
using namespace mm;

int totalCols = 4;
int totalRows = 9;

std::string colHeadTexts[] = {"Req#", "Status", "Image", "Result"};
std::string colValues[9][4] = {
    {"1", "OK", "IMG_Small_07.jpg", "ERROR"},
    {"2", "OK", "OneGirl.jpg", "ERROR"},
    {"3", "OK", "HD500.jpg", "ERROR"},
    {"4", "OK", "IMG_59941.jpg", "ERROR"},
    {"5", "OK", "Asiya_20150316_1.jpg", "ERROR"},
    {"6", "OK", "BrotherSister.jpg", "ERROR"},
    {"7", "OK", "IMG_60181.jpg", "ERROR"},
    {"8", "OK", "IMG_6012.jpg", "ERROR"},
    {"9", "OK", "sidepose4.jpg", "ERROR"},
};
float colLineWidths[] = {0.5f, 0.72f, 4.8f, 1.57f};


void PrintHelp()
{
    std::cout << "This is a example application for the pdfmm PDF library." << std::endl
        << "It creates a small PDF file containing the text >Hello World!<" << std::endl
        << "Please see https://github.com/pdfmm/pdfmm for more information" << std::endl << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  gentest [outputfile.pdf]" << std::endl << std::endl;
}

void GeneratePdfFile(const string_view& filename)
{
  char currentFolder[256];
  getcwd(currentFolder, 256);
  cout << "Current working directory: " << currentFolder << endl;

  CustomPainter customPainter;
    try
    {
      customPainter.AddNewPage();

      customPainter.SetTotalCols(totalCols); // top most start position
      customPainter.SetTopStart(11.45f); // top most start position
      customPainter.SetFirstColumnStart(0.26f); // left most start position
      customPainter.SetColWidths(colLineWidths);

      customPainter.OutputTableColHeaders(colHeadTexts, 12.96f);

      // output data
      customPainter.SetTableRowHeight(0.25f); // vertical difference between each data row
      customPainter.SetTableRowTopPadding(0.25f); // top padding for each data row
      customPainter.SetMaxImageWidthPerRow(4.05f);
      customPainter.SetMaxImageHeightPerRow(2.4f);
      customPainter.SetImageColumnIndex(2); // zero-based index
      customPainter.SetImagesFolder(currentFolder);
      for (int i = 0; i < totalRows; ++i)
      {
        if (i % 4 == 0 && i > 0)
        {
          // output outer lines for completed previous page
          customPainter.OutputTableOuterLines();

          // start next page
          customPainter.AddNewPage();
          customPainter.OutputTableColHeaders(colHeadTexts, 12.96f);
        }
        customPainter.OutputTableRowValues(colValues[i], 11.04f);
      }

      // output outer lines for last page
      customPainter.OutputTableOuterLines();

      customPainter.WriteDocumentToFile(filename.data());
    }
    catch (PdfError& e)
    {
        // All pdfmm methods may throw exceptions
        // make sure that painter.FinishPage() is called
        // or who will get an assert in its destructor
        try
        {
          customPainter.Terminate();
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
        GeneratePdfFile(argv[1]);
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

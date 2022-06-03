//
// Created by mgr on 6/1/22.
//

#include <iostream>
#include "custom_painter.h"

CustomPainter::CustomPainter()
{
  maxImageHeightPerRow = 0.0f;
}

void CustomPainter::addNewPage()
{
  PdfPage* page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
  pageHeight = page->GetRect().GetHeight();
  pageWidth = page->GetRect().GetWidth();
  painter.SetCanvas(page);
  font = document.GetFontManager().GetFont("Arial");
  if (font == nullptr)
    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
//  painter.GetTextState().SetFont(font, 12.96);
  pages.emplace_back(page);
}

void CustomPainter::insertText(const string_view &str, double x, double y, double fontSize)
{
  painter.GetTextState().SetFont(font, fontSize);
  painter.DrawText(str, x * 72, y * 72);
}

void CustomPainter::insertLine(double startX, double startY, double endX, double endY)
{
  painter.DrawLine(startX * 72, startY * 72, endX * 72, endY * 72);
}

void CustomPainter::insertRect(double x1, double y1, double x2, double y2, bool drawLeftEdge)
{
  insertLine(x1, y1, x2, y1);
  insertLine(x2, y1, x2, y2);
  insertLine(x2, y2, x1, y2);
  if (drawLeftEdge)
    insertLine(x1, y2, x1, y1);
}

void CustomPainter::insertImage(const std::string_view& imagePath, double posX, double posY)
{
  posX *= 72;
  posY *= 72;
//  float actualImgWidth = 600.0f;
//  float actualImgHeight = 338.0f;

//  PdfStringStream m_tmpStream;
//  m_tmpStream << posX << " "
//              << posY << " "
//              << actualImgWidth/2 << " "
//              << actualImgHeight/2
//              << " re" << endl
//  PdfObject obj(m_tmpStream);
//  PdfImage pdfImage(obj);

  PdfImage pdfImage(document);
  pdfImage.LoadFromFile(imagePath);
  unsigned int oWidth = pdfImage.GetWidth();
//  cout << "image-width = " << oWidth << endl;
  unsigned int oHeight = pdfImage.GetHeight();
//  cout << "image-height = " << oHeight << endl;

  double scaleX = 1.0f;
  double scaleY = 1.0f;
  if (oWidth > 300)
  {
    scaleX = 300.0f / oWidth;
    scaleY = scaleX; // maintain aspect ratio
//    cout << "scale-x = " << scaleX << endl;
    double scaleXHeight = oHeight * scaleX;
//    double scaleXWidth = oWidth * scaleX;
//    cout << "scale-x width = " << scaleXWidth << endl;
//    cout << "scale-x height = " << scaleXHeight << endl;
    if (scaleXHeight > 169)
    {
      scaleY = 169.0f / scaleXHeight;
//      cout << "scale-y = " << scaleY << endl;
//      cout << "scale-y width = " << (scaleXWidth * scaleY) << endl;
//      cout << "scale-y height = " << (scaleXHeight * scaleY) << endl;
      scaleY = (scaleXHeight * scaleY) / oHeight;
      scaleX = scaleY; // maintain aspect ratio
//      cout << "scale final = " << scaleY << endl;
    }
  }

//  Matrix matrix;
//  matrix = matrix.CreateScale({scaleX, scaleX});
//  pdfImage.SetMatrix(matrix);
//  matrix.

  painter.DrawImage(pdfImage, posX, posY, scaleX, scaleY);
}

void CustomPainter::terminate()
{
  painter.FinishDrawing();
}

int CustomPainter::writeDocumentToFile(const char *filepath)
{
  terminate();
  document.GetInfo().SetCreator(PdfString("pdfmm"));
  document.GetInfo().SetAuthor(PdfString("Umar Ali Khan - FutureIT"));
  document.GetInfo().SetTitle(PdfString("Image Processing Results"));
  document.GetInfo().SetSubject(PdfString("Image Processing Requests Report"));
  document.GetInfo().SetKeywords(PdfString("Image;Processing;Requests;Report;"));
  document.Save(filepath);
  return 0;
}

double CustomPainter::getPageHeight() const
{
  return pageHeight;
}

double CustomPainter::getPageWidth() const
{
  return pageWidth;
}

void CustomPainter::outputTableColHeaders(const std::string *headingTexts, double fontSize, float rowTop)
{
  if (rowTop == -1.0f)
  {
    rowTop = topRowStart;
  }

  float runningColStart = 0;
  float previousColStart = firstColumnStart;
  for (int i = 0; i < totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? colWidths[i - 1] + 0.05f : 0.0f);
    insertText(headingTexts[i], runningColStart, rowTop, fontSize);
    insertLine(runningColStart - 0.06f, rowTop - 0.07f, runningColStart + colWidths[i], rowTop - 0.07f);
    previousColStart = runningColStart;
  }
  currentTableRowOffset = rowTop;
}

void CustomPainter::outputTableRowValues(const std::string *valueTexts, double fontSize, const bool outputBottomLine)
{
  float runningColStart = 0;
  float previousColStart = firstColumnStart;
  currentTableRowOffset -= tableRowPositionOffset;
  for (int i = 0; i < totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? colWidths[i - 1] + 0.05f : 0.0f);
    insertText(valueTexts[i], runningColStart, currentTableRowOffset, fontSize);
    if (i == imageColumnIndex) // image column
    {
      cout << "start-value for image column: " << runningColStart << endl;
      string imageFullPath = imagesFolder + "/" + valueTexts[i];
      cout << "image-full-path: " << imageFullPath << endl;
      insertImage(imageFullPath, runningColStart, currentTableRowOffset - maxImageHeightPerRow);
    }
    else
    {
      cout << "start-value for column " << (i + 1) << " : " << runningColStart << endl;
    }
    if (outputBottomLine)
      insertLine(runningColStart - 0.06f, currentTableRowOffset - maxImageHeightPerRow - 0.075f, runningColStart + colWidths[i], currentTableRowOffset - maxImageHeightPerRow - 0.075f);
    previousColStart = runningColStart;
  }
  currentTableRowOffset -= maxImageHeightPerRow;
}

void CustomPainter::outputTableOuterLines()
{
  float runningColStart = 0;
  float previousColStart = firstColumnStart;
  float colsTopStart = topRowStart + 0.2;
  currentTableRowOffset -= 0.07f;
  for (int i = 0; i < totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? colWidths[i - 1] + 0.05f : 0.0f);
    insertRect(runningColStart - 0.06f, currentTableRowOffset, runningColStart + colWidths[i], colsTopStart, i == 0);
    previousColStart = runningColStart;
  }
}

void CustomPainter::setTotalCols(const int value)
{
  totalCols = value;
}

void CustomPainter::setFirstColumnStart(float value)
{
  firstColumnStart = value;
}

void CustomPainter::setTopRowStart(float value)
{
  topRowStart = value;
}

void CustomPainter::setColWidths(float *values)
{
  colWidths = values;
}

void CustomPainter::setTableRowPositionOffset(float value)
{
  tableRowPositionOffset = value;
}

void CustomPainter::setMaxImageHeightPerRow(float value)
{
  maxImageHeightPerRow = value;
}

void CustomPainter::setImageColumnIndex(float value)
{
  imageColumnIndex = value;
}

void CustomPainter::setImagesFolder(const char* value)
{
  imagesFolder = string(value);
}

void CustomPainter::setImagesFolder(const string &value)
{
  imagesFolder = value;
}

//
// Created by mgr on 6/1/22.
//

#include <iostream>
#include "custom_painter.h"

CustomPainter::CustomPainter()
{
  m_maxImageHeightPerRow = 0.0f;
}

void CustomPainter::AddNewPage()
{
  PdfPage* page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
  m_pageHeight = page->GetRect().GetHeight();
  m_pageWidth = page->GetRect().GetWidth();
  painter.SetCanvas(page);
  font = document.GetFontManager().GetFont("Arial");
  if (font == nullptr)
    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
//  painter.GetTextState().SetFont(font, 12.96);
  pages.emplace_back(page);
}

void CustomPainter::InsertText(const string_view &str, double x, double y, double fontSize)
{
  painter.GetTextState().SetFont(font, fontSize);
  painter.DrawText(str, x * 72, y * 72);
}

void CustomPainter::InsertLine(double startX, double startY, double endX, double endY)
{
  painter.DrawLine(startX * 72, startY * 72, endX * 72, endY * 72);
}

void CustomPainter::InsertRect(double x1, double y1, double x2, double y2, bool drawLeftEdge)
{
  InsertLine(x1, y1, x2, y1);
  InsertLine(x2, y1, x2, y2);
  InsertLine(x2, y2, x1, y2);
  if (drawLeftEdge)
    InsertLine(x1, y2, x1, y1);
}

void CustomPainter::InsertImage(const std::string_view& imagePath, double posX, double posY)
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

void CustomPainter::Terminate()
{
  painter.FinishDrawing();
}

int CustomPainter::WriteDocumentToFile(const char *filepath)
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

double CustomPainter::GetPageHeight() const
{
  return m_pageHeight;
}

double CustomPainter::GetPageWidth() const
{
  return m_pageWidth;
}

void CustomPainter::OutputTableColHeaders(const std::string *headingTexts, double fontSize, float rowTop)
{
  if (rowTop == -1.0f)
  {
    rowTop = m_topRowStart;
  }

  float runningColStart = 0;
  float previousColStart = m_firstColumnStart;
  for (int i = 0; i < m_totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? m_colWidths[i - 1] + 0.05f : 0.0f);
    InsertText(headingTexts[i], runningColStart, rowTop, fontSize);
    InsertLine(runningColStart - 0.06f, rowTop - 0.07f, runningColStart + m_colWidths[i], rowTop - 0.07f);
    previousColStart = runningColStart;
  }
  currentTableRowOffset = rowTop;
}

void CustomPainter::OutputTableRowValues(const std::string *valueTexts, double fontSize, const bool outputBottomLine)
{
  float runningColStart = 0;
  float previousColStart = m_firstColumnStart;
  currentTableRowOffset -= m_tableRowPositionOffset;
  for (int i = 0; i < m_totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? m_colWidths[i - 1] + 0.05f : 0.0f);
    InsertText(valueTexts[i], runningColStart, currentTableRowOffset, fontSize);
    if (i == m_imageColumnIndex) // image column
    {
      cout << "start-value for image column: " << runningColStart << endl;
      string imageFullPath = m_imagesFolder + "/" + valueTexts[i];
      cout << "image-full-path: " << imageFullPath << endl;
      InsertImage(imageFullPath, runningColStart, currentTableRowOffset - m_maxImageHeightPerRow);
    }
    else
    {
      cout << "start-value for column " << (i + 1) << " : " << runningColStart << endl;
    }
    if (outputBottomLine)
      InsertLine(runningColStart - 0.06f, currentTableRowOffset - m_maxImageHeightPerRow - 0.075f, runningColStart + m_colWidths[i], currentTableRowOffset - m_maxImageHeightPerRow - 0.075f);
    previousColStart = runningColStart;
  }
  currentTableRowOffset -= m_maxImageHeightPerRow;
}

void CustomPainter::OutputTableOuterLines()
{
  float runningColStart = 0;
  float previousColStart = m_firstColumnStart;
  float colsTopStart = m_topRowStart + 0.2;
  currentTableRowOffset -= 0.07f;
  for (int i = 0; i < m_totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? m_colWidths[i - 1] + 0.05f : 0.0f);
    InsertRect(runningColStart - 0.06f, currentTableRowOffset, runningColStart + m_colWidths[i], colsTopStart, i == 0);
    previousColStart = runningColStart;
  }
}

void CustomPainter::SetTotalCols(const int value)
{
  m_totalCols = value;
}

void CustomPainter::SetFirstColumnStart(float value)
{
  m_firstColumnStart = value;
}

void CustomPainter::SetTopRowStart(float value)
{
  m_topRowStart = value;
}

void CustomPainter::SetColWidths(float *values)
{
  m_colWidths = values;
}

void CustomPainter::SetTableRowPositionOffset(float value)
{
  m_tableRowPositionOffset = value;
}

void CustomPainter::SetMaxImageHeightPerRow(float value)
{
  m_maxImageHeightPerRow = value;
}

void CustomPainter::SetImageColumnIndex(float value)
{
  m_imageColumnIndex = value;
}

void CustomPainter::SetImagesFolder(const char* value)
{
  m_imagesFolder = string(value);
}

void CustomPainter::SetImagesFolder(const string &value)
{
  m_imagesFolder = value;
}

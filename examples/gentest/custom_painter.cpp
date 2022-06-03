//
// Created by mgr on 6/1/22.
//

#include <iostream>
#include "custom_painter.h"

CustomPainter::CustomPainter()
{
  m_maxImageHeightPerRow = 0.0f;
  m_imageColumnIndex = -1;
  m_topStart = 11.55f;
}

void CustomPainter::AddNewPage()
{
  PdfPage *page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
  m_pageHeight = page->GetRect().GetHeight();
  m_pageWidth = page->GetRect().GetWidth();
  painter.SetCanvas(page);
  font = document.GetFontManager().GetFont("Arial");
  if (font == nullptr)
    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
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

void CustomPainter::InsertImage(const std::string_view &imagePath, double posX, double posY)
{
  double adjustmentForLettersWithHangingPart = 1.0f;
  float imageTopPadding = 0.05f; // 0.05f

  PdfImage pdfImage(document);
  pdfImage.LoadFromFile(imagePath);
  unsigned int oWidth = pdfImage.GetWidth();
  unsigned int oHeight = pdfImage.GetHeight();

  double scaleX = 1.0f;
  double scaleY = 1.0f;
  float finalImageHeight = oHeight;
  double maxWidth = m_maxImageWidthPerRow * 72;
  double maxHeight = (m_maxImageHeightPerRow - 0.2) * 72;
  if (oWidth > maxWidth)
  {
    scaleX = maxWidth / oWidth;
    scaleY = scaleX; // maintain aspect ratio
    double scaleXHeight = oHeight * scaleY;
    finalImageHeight = scaleXHeight;
    double scaleXWidth = (oWidth * scaleX);
    if (scaleXHeight > maxHeight)
    {
      scaleY = maxHeight / scaleXHeight;
      scaleY = (scaleXHeight * scaleY) / oHeight;
      scaleX = scaleY; // maintain aspect ratio
      finalImageHeight = oHeight * scaleY;
    }
  }

  float tblRowHeightPixels = m_tableRowHeight * 72;
  float imageHeightAdjustment = (finalImageHeight - tblRowHeightPixels) / 72;
  posY = currentTableRowOffset - (finalImageHeight / 72) - imageTopPadding;

//  Matrix matrix;
//  matrix = matrix.CreateScale({scaleX, scaleX});
//  pdfImage.SetMatrix(matrix);

  posX *= 72;
  posY *= 72;
  painter.DrawImage(pdfImage, posX, posY - adjustmentForLettersWithHangingPart, scaleX, scaleY);
}

void CustomPainter::Terminate()
{
  painter.FinishDrawing();
}

int CustomPainter::WriteDocumentToFile(const char *filepath)
{
  Terminate();
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
    rowTop = m_topStart;
  }

  float headerHeight = 0.25f;
  float runningColStart = 0;
  float previousColStart = m_firstColumnStart;
  float leftPadding = 0.05f;
  float topPadding = 0.05f;
  float bottomPadding = 0.07f;
  float rowTop00 = rowTop - headerHeight + topPadding + bottomPadding;
  for (int i = 0; i < m_totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? m_colWidths[i - 1] : 0.0f);
    InsertLine(runningColStart, rowTop, runningColStart + m_colWidths[i], rowTop);
    InsertText(headingTexts[i], runningColStart + leftPadding, rowTop00 - topPadding, fontSize);
    InsertLine(runningColStart, rowTop - headerHeight, runningColStart + m_colWidths[i], rowTop - headerHeight);
    previousColStart = runningColStart;
  }
  currentTableRowOffset = rowTop - headerHeight;
}

void CustomPainter::OutputTableRowValues(const std::string *valueTexts, double fontSize)
{
  float runningColStart = 0;
  float previousColStart = m_firstColumnStart;
  float rowHeight = m_imageColumnIndex > -1 ? max(m_tableRowHeight, m_maxImageHeightPerRow) : m_tableRowHeight;
  float leftPadding = 0.05f;
  float bottomPadding = 0.07f;
  float rowTop00 = currentTableRowOffset - m_tableRowHeight + bottomPadding;
  for (int i = 0; i < m_totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? m_colWidths[i - 1] : 0.0f);
    InsertText(valueTexts[i], runningColStart + leftPadding, rowTop00, fontSize);
    if (i == m_imageColumnIndex) // image column
    {
      string imageFullPath = m_imagesFolder + "/" + valueTexts[i];
      InsertImage(imageFullPath, runningColStart + leftPadding, rowTop00);
    }
    double bottomLineOffset = currentTableRowOffset - rowHeight;
    InsertLine(runningColStart, bottomLineOffset, runningColStart + m_colWidths[i], bottomLineOffset);
    previousColStart = runningColStart;
  }
  currentTableRowOffset -= rowHeight;
}

void CustomPainter::OutputTableOuterLines()
{
  float runningColStart = 0;
  float previousColStart = m_firstColumnStart;
  float colsTopStart = m_topStart;
  InsertLine(m_firstColumnStart, currentTableRowOffset, m_firstColumnStart, colsTopStart);
  for (int i = 0; i < m_totalCols; ++i)
  {
    runningColStart = previousColStart + (i > 0 ? m_colWidths[i - 1] : 0.0f);
    InsertLine(runningColStart + m_colWidths[i], currentTableRowOffset, runningColStart + m_colWidths[i], colsTopStart);
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

void CustomPainter::SetTopStart(float value)
{
  m_topStart = value;
}

void CustomPainter::SetColWidths(float *values)
{
  m_colWidths = values;
}

void CustomPainter::SetTableRowHeight(float value)
{
  m_tableRowHeight = value;
}

void CustomPainter::SetMaxImageHeightPerRow(float value)
{
  m_maxImageHeightPerRow = value;
}

void CustomPainter::SetImageColumnIndex(int value)
{
  m_imageColumnIndex = value;
}

void CustomPainter::SetImagesFolder(const char *value)
{
  m_imagesFolder = string(value);
}

void CustomPainter::SetImagesFolder(const string &value)
{
  m_imagesFolder = value;
}

void CustomPainter::SetMaxImageWidthPerRow(float value)
{
  m_maxImageWidthPerRow = value;
}

void CustomPainter::SetTableRowTopPadding(float value)
{
  m_tableRowTopPadding = value;
}

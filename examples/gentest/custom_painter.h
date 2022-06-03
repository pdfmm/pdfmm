//
// Created by mgr on 6/1/22.
//

#ifndef PDFMM_CUSTOM_PAINTER_H
#define PDFMM_CUSTOM_PAINTER_H

#include <pdfmm/pdfmm.h>

// All pdfmm classes are member of the pdfmm namespace.
using namespace std;
using namespace mm;

class CustomPainter
{
public:

  // generic methods
  CustomPainter();

  void AddNewPage();

  void InsertText(const std::string_view& str, double x, double y, double fontSize);
  void InsertLine(double startX, double startY, double endX, double endY);
  void InsertRect(double x, double y, double x2, double y2, bool drawLeftEdge = true);
  void InsertImage(const std::string_view& imagePath, double posX, double posY);

  void Terminate();
  int WriteDocumentToFile(const char *filepath);

  double GetPageHeight() const;
  double GetPageWidth() const;

  // example/sample specific methods
  void OutputTableColHeaders(const std::string *headingTexts, double fontSize, float rowTop = -1.0f);
  void OutputTableRowValues(const std::string *valueTexts, double fontSize, const bool outputBottomLine = true);
  void OutputTableOuterLines();

  // generic setter methods
  void SetTotalCols(const int totalCols);
  void SetFirstColumnStart(float value);
  void SetTopRowStart(float value);
  void SetColWidths(float *values);
  void SetTableRowPositionOffset(float value);
  void SetMaxImageHeightPerRow(float maxImageHeightPerRow);
  void SetImageColumnIndex(float imageColumnIndex);
  void SetImagesFolder(const char* imagesFolder);
  void SetImagesFolder(const string &imagesFolder);

private:
  // fields only used within this class
  PdfMemDocument document;
  PdfPainter painter;
  PdfFont* font;
  std::vector<PdfPage*> pages;
  float currentTableRowOffset;

  // fields with getters
  double m_pageHeight;
  double m_pageWidth;

  // fields with setters
  int m_totalCols;
  float m_firstColumnStart;
  float m_topRowStart;
  float* m_colWidths;
  float m_tableRowPositionOffset;
  float m_maxImageHeightPerRow;
  float m_imageColumnIndex;
  std::string m_imagesFolder;

};


#endif //PDFMM_CUSTOM_PAINTER_H

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

  CustomPainter();

  void addNewPage();

  void insertText(const std::string_view& str, double x, double y, double fontSize);
  void insertLine(double startX, double startY, double endX, double endY);
  void insertRect(double x, double y, double x2, double y2, bool drawLeftEdge = true);
  void insertImage(const std::string_view& imagePath, double posX, double posY);

  void terminate();
  int writeDocumentToFile(const char *filepath);

  double getPageHeight() const;
  double getPageWidth() const;

  void outputTableColHeaders(const std::string *headingTexts, double fontSize, float rowTop = -1.0f);
  void outputTableRowValues(const std::string *valueTexts, double fontSize, const bool outputBottomLine = true);
  void outputTableOuterLines();

  void setTotalCols(const int totalCols);
  void setFirstColumnStart(float value);
  void setTopRowStart(float value);
  void setColWidths(float *values);
  void setTableRowPositionOffset(float value);
  void setMaxImageHeightPerRow(float maxImageHeightPerRow);
  void setImageColumnIndex(float imageColumnIndex);
  void setImagesFolder(const char* imagesFolder);
  void setImagesFolder(const string &imagesFolder);

private:
  PdfMemDocument document;
  PdfPainter painter;
  PdfFont* font;
  std::vector<PdfPage*> pages;

  double pageHeight;
  double pageWidth;

  int totalCols;
  float firstColumnStart;
  float topRowStart;
  float* colWidths;
  float tableRowPositionOffset;
  float currentTableRowOffset;
  float maxImageHeightPerRow;
  float imageColumnIndex;
  std::string imagesFolder;

};


#endif //PDFMM_CUSTOM_PAINTER_H

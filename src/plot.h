#ifndef __REPLACE_CIMG_PLOT__
#define __REPLACE_CIMG_PLOT__

#include <vector>
#include <memory>
#include "coordi.h"

// 
// The following structure/header will be removed.
//
// This is temporal implementation with CImg
//
namespace replace {

void SaveCellPlotAsJPEG(std::string imgName, bool isGCell, std::string imgPosition);
void SaveBinPlotAsJPEG(std::string imgName, std::string imgPosition);
void SaveArrowPlotAsJPEG(std::string imgName, std::string imgPosition);
void SavePlot(std::string imgName = "", bool isGCell = false);
//void ShowPlot(std::string circuitName = "");

class NesterovBase;

enum { BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE };

struct PlotColor {
  private:
  int r_;
  int g_;
  int b_;

  public:
  PlotColor() : r_(0), g_(0), b_(0) {};
  PlotColor(int r, int g, int b) :
    r_(r), g_(g), b_(b) {};

  int r() { return r_; };
  int g() { return g_; };
  int b() { return b_; };
};


// for X11 drawing
class PlotEnv {
 public:
  int minLength;
  int imageWidth;
  int imageHeight;
  int xMargin;
  int yMargin;
  float origWidth;
  float origHeight;
  float unitX;
  float unitY;

  // for showPlot
  float dispWidth;
  float dispHeight;

  bool hasCellColor;

  // color information for each cells.
  // Needed for cell coloring
  std::vector<PlotColor> colors;

  // constructor
  PlotEnv(
      std::shared_ptr<PlacerBase> pb, 
      std::shared_ptr<NesterovBase> nb);

  void Init();
  void InitCellColors(std::string colorFile);
  int GetTotalImageWidth();
  int GetTotalImageHeight();
  int GetX(FloatCoordi &coord);
  int GetX(float coord);
  int GetY(FloatCoordi &coord);
  int GetY(float coord);

 private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
};

}
#endif

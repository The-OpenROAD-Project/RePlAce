///////////////////////////////////////////////////////////////////////////////
// Authors: Ilgweon Kang and Lutong Wang
//          (respective Ph.D. advisors: Chung-Kuan Cheng, Andrew B. Kahng),
//          based on Dr. Jingwei Lu with ePlace and ePlace-MS
//
//          Many subsequent improvements were made by Mingyu Woo
//          leading up to the initial release.
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "plot.h"
#include "bin.h"
#include "global.h"
#include "opt.h"

#include "CImg.h"
#include "bookShelfIO.h"

#include <sstream>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
//#include <tgmath.h>
#include <error.h>
#include <cmath>

using namespace cimg_library;

// to save snapshot of the circuit.
static vector< CImg< unsigned char > > imgStor;

// possible color settings
static const unsigned char yellow[] = {255, 255, 0}, white[] = {255, 255, 255},
                           green[] = {0, 255, 0}, blue[] = {120, 200, 255},
                           darkblue[] = {69, 66, 244},
                           purple[] = {255, 100, 255}, black[] = {0, 0, 0},
                           red[] = {255, 0, 0};

static PlotEnv pe;
static bool isPlotEnvInit = false;

void plotFinalLayout() {
  GCellPinCoordiUpdate();
  plot("S9-Final-Layout", 9999, 1.0, 0);
}

// original plot function
void plot(string fileName, int idx, prec scale, int lab) {
  char mkdir_cmd[BUF_SZ] = {
      0,
  };

  // making directory
  sprintf(mkdir_cmd, "mkdir -p %s/cell", dir_bnd);
  system(mkdir_cmd);

  char str[BUF_SZ];
  char fig[BUF_SZ];
  char jpg[BUF_SZ];
  char cmd[BUF_SZ];

  // idx -> zero-filled 4 digit string
  idx2str4digits(idx, str);

  // fig, jpg
  sprintf(fig, "%s/cell/%s%s.fig", dir_bnd, fileName.c_str(), str);
  sprintf(jpg, "%s/cell/%s%s.jpg", dir_bnd, fileName.c_str(), str);

  xfig_gp(scale, scale, fig, lab);

  // fig2dev..
  sprintf(cmd, "fig2dev -L jpeg %s %s", fig, jpg);
  system(cmd);

  sprintf(cmd, "rm -rvf %s >/dev/null 2>&1", fig);
  // sprintf(cmd, "rm -rvf %s >> rm.log ", fig);
  system(cmd);
}

// inline int GetX(FPOS _coord, float unitX) {
//    return (_coord.x - gmin.x) * unitX;
//}

// inline int GetX(float _coordX, float unitX) {
//    return (_coordX - gmin.x) * unitX;
//}

//// the Y-axis must be mirrored
// inline int GetY(FPOS _coord, float unitY, float origHeight) {
//    return ( origHeight - (_coord.y - gmin.y)) * unitY;
//}

// inline int GetY(float _coordY, float unitY, float origHeight ) {
//    return ( origHeight - (_coordY - gmin.y)) * unitY;
//}

////////////////////////////////////////////////////////////////////////
//
// below is for the CImg drawing
//

PlotEnv::PlotEnv() {
  Init();
}

void PlotEnv::Init() {
  origWidth = gmax.x - gmin.x;
  origHeight = gmax.y - gmin.y;
  minLength = 1000;
  xMargin = yMargin = 30;

  // imageWidth & height setting
  // Set minimum length of picture as minLength
  if(origWidth < origHeight) {
    imageHeight = 1.0 * origHeight / (origWidth / minLength);
    imageWidth = minLength;
  }
  else {
    imageWidth = 1.0 * origWidth / (origHeight / minLength);
    imageHeight = minLength;
  }
  //        imageWidth = origWidth*0.5;
  //        imageHeight = origHeight*0.5;
  //        imageWidth = origWidth;
  //        imageHeight = origHeight;
  //        imageWidth = origWidth*1.3;
  //        imageHeight = origHeight*1.3;

  // scaling
  unitX = imageWidth / origWidth;
  unitY = imageHeight / origHeight;

  dispWidth = imageWidth * 0.2;
  dispHeight = imageHeight * 0.2;
}

int PlotEnv::GetTotalImageWidth() {
  return imageWidth + 2 * xMargin;
}

int PlotEnv::GetTotalImageHeight() {
  return imageHeight + 2 * yMargin;
}

int PlotEnv::GetX(FPOS &coord) {
  return (coord.x - gmin.x) * unitX + xMargin;
}

int PlotEnv::GetX(prec coord) {
  return (coord - gmin.x) * unitX + xMargin;
}

int PlotEnv::GetY(FPOS &coord) {
  return (origHeight - (coord.y - gmin.y)) * unitY + yMargin;
}

int PlotEnv::GetY(prec coord) {
  return (origHeight - (coord - gmin.y)) * unitY + yMargin;
}

typedef CImg< unsigned char > CImgObj;

void DrawTerminal(CImgObj &img, const unsigned char color[], float opacity) {
  // FIXED CELL
  for(int i = 0; i < terminalCNT; i++) {
    TERM *curTerminal = &terminalInstance[i];

    // skip for drawing terminalNI pins
    if(curTerminal->isTerminalNI) {
      continue;
    }

    if(shapeMap.find(curTerminal->name) == shapeMap.end()) {
      int x1 = pe.GetX(curTerminal->pmin);
      int x3 = pe.GetX(curTerminal->pmax);
      int y1 = pe.GetY(curTerminal->pmin);
      int y3 = pe.GetY(curTerminal->pmax);
      img.draw_rectangle(x1, y1, x3, y3, color, opacity);
      //            img.draw_text((x1+x3)/2, (y1+y3)/2, curTerminal->name,
      //            black, NULL, 1, 30);
    }
    else {
      for(auto &curIdx : shapeMap[curTerminal->name]) {
        int x1 = pe.GetX(shapeStor[curIdx].llx);
        int y1 = pe.GetY(shapeStor[curIdx].lly);

        int x3 = pe.GetX(shapeStor[curIdx].llx + shapeStor[curIdx].width);
        int y3 = pe.GetY(shapeStor[curIdx].lly + shapeStor[curIdx].height);

        img.draw_rectangle(x1, y1, x3, y3, color, opacity);
      }
    }
  }
}

void DrawGcell(CImgObj &img, const unsigned char fillerColor[],
               const unsigned char cellColor[],
               const unsigned char macroColor[], float opacity) {
  for(int i = 0; i < gcell_cnt; i++) {
    CELLx *curGCell = &gcell_st[i];

    curGCell->pmin.x = curGCell->center.x - 0.5 * curGCell->size.x;
    curGCell->pmax.x = curGCell->center.x + 0.5 * curGCell->size.x;

    curGCell->pmin.y = curGCell->center.y - 0.5 * curGCell->size.y;
    curGCell->pmax.y = curGCell->center.y + 0.5 * curGCell->size.y;

    int x1 = pe.GetX(curGCell->pmin);
    int x3 = pe.GetX(curGCell->pmax);
    int y1 = pe.GetY(curGCell->pmin);
    int y3 = pe.GetY(curGCell->pmax);

    if(curGCell->flg == FillerCell) {
      continue;
    }

    // img.draw_rectangle( x1, y1, x3, y3,
    //        (curGCell->flg == FillerCell)? fillerColor: cellColor, opacity );
    img.draw_rectangle(x1, y1, x3, y3,
                       (curGCell->flg == Macro) ? macroColor : cellColor,
                       opacity);

    if(curGCell->flg == Macro) {
      img.draw_rectangle(x1, y1, x3, y3, black, opacity, ~0U);
      // img.draw_text((x1+x3)/2, (y1+y3)/2, curGCell->name, black, NULL, 1,
      // 20);
    }
  }
}

void DrawModule(CImgObj &img, const unsigned char color[], float opacity) {
  for(int i = 0; i < moduleCNT; i++) {
    MODULE *curModule = &moduleInstance[i];

    // update pmin & pmax
    curModule->pmin.x = curModule->center.x - 0.5 * curModule->size.x;
    curModule->pmax.x = curModule->center.x + 0.5 * curModule->size.x;

    curModule->pmin.y = curModule->center.y - 0.5 * curModule->size.y;
    curModule->pmax.y = curModule->center.y + 0.5 * curModule->size.y;

    int x1 = pe.GetX(curModule->pmin);
    int x3 = pe.GetX(curModule->pmax);
    int y1 = pe.GetY(curModule->pmin);
    int y3 = pe.GetY(curModule->pmax);
    img.draw_rectangle(x1, y1, x3, y3, color, opacity);
  }
}

void DrawBinDensity(CImgObj &img, float opacity) {
  for(int i = 0; i < tier_st[0].tot_bin_cnt; i++) {
    BIN *curBin = &tier_st[0].bin_mat[i];
    int x1 = pe.GetX(curBin->pmin);
    int x3 = pe.GetX(curBin->pmax);
    int y1 = pe.GetY(curBin->pmin);
    int y3 = pe.GetY(curBin->pmax);

    int color = curBin->den * 50 + 20;
    color = (color > 255) ? 255 : (color < 20) ? 20 : color;
    color = 255 - color;

    char denColor[3] = {(char)color, (char)color, (char)color};
    img.draw_rectangle(x1, y1, x3, y3, denColor, opacity);
    //        img.draw_text((x1+x3)/2-5, (y1+y3)/2,
    //        to_string(curBin->den).c_str(), black, NULL, 1, 15);
  }
}

void CimgDrawArrow(CImgObj &img, int x1, int y1, int x3, int y3, int thick,
                   const unsigned char color[], float opacity) {
  // ARROW HEAD DRAWING
  float arrowHeadSize = thick;
  float theta = atan(1.0 * (y3 - y1) / (x3 - x1));
  float invTheta = atan(-1.0 * (x3 - x1) / (y3 - y1));

  // ARROW RECT DRAWING
  int ldX = x1 - 1.0 * thick / 4 * cos(invTheta);
  int ldY = y1 - 1.0 * thick / 4 * sin(invTheta);
  int rdX = x1 + 1.0 * thick / 4 * cos(invTheta);
  int rdY = y1 + 1.0 * thick / 4 * sin(invTheta);

  int luX = x3 - 1.0 * thick / 4 * cos(invTheta);
  int luY = y3 - 1.0 * thick / 4 * sin(invTheta);
  int ruX = x3 + 1.0 * thick / 4 * cos(invTheta);
  int ruY = y3 + 1.0 * thick / 4 * sin(invTheta);

  cimg_library::CImg< int > rectPoints(4, 2);
  rectPoints(0, 0) = ldX;
  rectPoints(0, 1) = ldY;
  rectPoints(1, 0) = rdX;
  rectPoints(1, 1) = rdY;
  rectPoints(2, 0) = ruX;
  rectPoints(2, 1) = ruY;
  rectPoints(3, 0) = luX;
  rectPoints(3, 1) = luY;

  img.draw_polygon(rectPoints, color);

  cimg_library::CImg< int > headPoints(3, 2);
  int lPointX = x3 - 1.0 * thick / 2 * cos(invTheta);
  int lPointY = y3 - 1.0 * thick / 2 * sin(invTheta);
  int rPointX = x3 + 1.0 * thick / 2 * cos(invTheta);
  int rPointY = y3 + 1.0 * thick / 2 * sin(invTheta);

  int uPointX = (1.0 * (x3 - x1) >= 0) ? x3 + 1.0 * thick * cos(theta)
                                       : x3 - 1.0 * thick * cos(theta);
  int uPointY = (1.0 * (x3 - x1) >= 0) ? y3 + 1.0 * thick * sin(theta)
                                       : y3 - 1.0 * thick * sin(theta);

  //    int uPointX = x3 + 1.0*thick*cos(theta);
  //    int uPointY = y3 + 1.0*thick*sin(theta);

  headPoints(0, 0) = lPointX;
  headPoints(0, 1) = lPointY;
  headPoints(1, 0) = rPointX;
  headPoints(1, 1) = rPointY;
  headPoints(2, 0) = uPointX;
  headPoints(2, 1) = uPointY;

  //    cout << x3 << " " << y3 << endl;
  //    cout << lPointX << " " << lPointY << endl;
  //    cout << rPointX << " " << rPointY << endl;
  //    cout << uPointX << " " << uPointY << endl << endl;

  img.draw_polygon(headPoints, color);
  //    img.draw_arrow( x1, y1, x3, y3, color, opacity );
}

void DrawArrowDensity(CImgObj &img, float opacity) {
  int binMaxX = (STAGE == cGP2D) ? dim_bin_cGP2D.x
                                 : (STAGE == mGP2D) ? dim_bin_mGP2D.x : INT_MIN;
  int binMaxY = (STAGE == cGP2D) ? dim_bin_cGP2D.y
                                 : (STAGE == mGP2D) ? dim_bin_mGP2D.y : INT_MIN;

  int arrowSpacing = (binMaxX / 16 <= 0) ? 1 : binMaxX / 16;

  // below is essential for extracting e?Max
  prec exMax = PREC_MIN, eyMax = PREC_MIN, ezMax = PREC_MIN;
  for(int i = 0; i < binMaxX; i += arrowSpacing) {
    for(int j = 0; j < binMaxY; j += arrowSpacing) {
      int binIdx = binMaxX * j + i;
      BIN *curBin = &tier_st[0].bin_mat[binIdx];

      prec newEx = fabs(curBin->e.x);
      prec newEy = fabs(curBin->e.y);

      exMax = (exMax < newEx) ? newEx : exMax;
      eyMax = (eyMax < newEy) ? newEy : eyMax;
    }
  }

  for(int i = 0; i < binMaxX; i += arrowSpacing) {
    for(int j = 0; j < binMaxY; j += arrowSpacing) {
      int binIdx = binMaxX * j + i;
      BIN *curBin = &tier_st[0].bin_mat[binIdx];

      int signX = (curBin->e.x > 0) ? 1 : -1;
      int signY = (curBin->e.y > 0) ? 1 : -1;

      prec newVx = fabs(curBin->e.x);
      prec newVy = fabs(curBin->e.y);

      int x1 = curBin->center.x;
      int y1 = curBin->center.y;

      prec dx = signX * newVx / exMax;
      prec dy = signY * newVy / eyMax;

      //        prec theta = atan(dy / dx);
      prec length = sqrt(pow(tier_st[0].bin_stp.x, 2.0) +
                         pow(tier_st[0].bin_stp.y, 2.0)) *
                    5;

      int x3 = x1 + dx * length;
      int y3 = y1 + dy * length;

      int drawX1 = pe.GetX(x1);
      int drawY1 = pe.GetY(y1);
      int drawX3 = pe.GetX(x3);
      int drawY3 = pe.GetY(y3);

      //            img.draw_arrow( drawX1, drawY1, drawX3, drawY3, black,
      //            opacity );
      CimgDrawArrow(img, drawX1, drawY1, drawX3, drawY3, 20, red, opacity);
      //        cout << "sign: " <<signX << endl;
      //        cout << "newVx: " << newVx << endl;
      //        cout << "exMax: " << exMax << endl;
      //        cout << "bin_stp: " << bin_stp.x << endl;
      //        cout << binCoordi.x << " " << binCoordi.y << " "
      //            << curBin->center.x << " " << curBin->center.y << " "
      //            << signX*newVx/exMax * bin_stp.x * 10 << " "
      //            << signY*newVy/eyMax * bin_stp.y * 10 << endl;
    }
  }

  //    cout << "theta: " << theta << endl;
}

void SaveCellPlot(CImgObj &img, bool isGCell) {
  float opacity = 0.7;
  // FIXED CELL
  DrawTerminal(img, blue, opacity);

  // STD CELL
  if(!isGCell) {
    DrawModule(img, red, opacity);
  }
  else {
    DrawGcell(img, purple, darkblue, red, opacity);
  }
}

void SaveBinPlot(CImgObj &img) {
  float opacity = 1;
  DrawBinDensity(img, opacity);
}

void SaveArrowPlot(CImgObj &img) {
  float opacity = 1;
  DrawBinDensity(img, opacity);
  DrawArrowDensity(img, opacity);
}

//
// using X11
// isGCell : GCell drawing mode. true->gcell_st, false->moduleInstance
//
void SavePlot(string imgName, bool isGCell) {
  // if gcell is exist, then update module's information
  // before drawing
  //    if( gcell_st ) {
  //        GCellPinCoordiUpdate();
  //    }
  if(!isPlotEnvInit) {
    pe.Init();
    isPlotEnvInit = true;
  }

  CImg< unsigned char > img(pe.GetTotalImageWidth(), pe.GetTotalImageHeight(),
                            1, 3, 255);

  SaveCellPlot(img, isGCell);
  //    SaveBinPlot(img);

  // Finally draw image info
  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 100);
  imgStor.push_back(img);
}

//
// save current circuit's as BMP file in imgPosition & iternumber
// isGCell : GCell drawing mode. true->gcell_st, false->moduleInstance
//
void SaveCellPlotAsJPEG(string imgName, bool isGCell, string imgPosition) {
  // if gcell is exist, then update module's information
  // before drawing
  //    if( gcell_st ) {
  //        GCellPinCoordiUpdate();
  //    }

  if(!isPlotEnvInit) {
    pe.Init();
    isPlotEnvInit = true;
  }

  float opacity = 0.7;
  CImg< unsigned char > img(pe.GetTotalImageWidth(), pe.GetTotalImageHeight(),
                            1, 3, 255);

  //    int cnt = (lab == 0)? moduleCNT : gcell_cnt;

  /*
  for(int i=0; i<row_cnt; i++) {
      ROW* curRow = &row_st[i];
      int x1 = GetX( curRow->pmin.x, unitX ) + xMargin;
      int x3 = GetX( curRow->pmax.x, unitX ) + xMargin;
      int y1 = GetY( curRow->pmin.y, unitY, origHeight ) + yMargin;
      int y3 = GetY( curRow->pmax.y, unitY, origHeight ) + yMargin;
      img.draw_rectangle( x1, y1, x3, y3, black, 0.025 );
  }*/

  SaveCellPlot(img, isGCell);

  /*
  for(int i=0; i<tier_st[0].tot_bin_cnt; i++) {
      BIN* curBin = &tier_st[0].bin_mat[i];
      int x1 = GetX( curBin->pmin.x, unitX ) + xMargin;
      int x3 = GetX( curBin->pmax.x, unitX ) + xMargin;
      int y1 = GetY( curBin->pmin.y, unitY, origHeight ) + yMargin;
      int y3 = GetY( curBin->pmax.y, unitY, origHeight ) + yMargin;
//        img.draw_rectangle( x1, y1, x3, y3, purple, opacity );
      img.draw_text( x1, (y1 + y3)/2 , to_string(int(100*curBin->den)).c_str(),
black, NULL, 1, 25);
      img.draw_text( x1 + 35, (y1 + y3)/2 ,
to_string(int(100*curBin->den2)).c_str(), black, NULL, 1, 25);


//        if( i > 5)
//            break;
  }
  */

  //    cout << "current imgName: " << imgName << endl;
  // Finally draw image info
  string saveName = imgPosition + string(".jpg");

  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 30);
  img.save_jpeg(saveName.c_str(), 70);
  //  img.save_bmp( string(imgPosition + string(".bmp")).c_str() );
  cout << "INFO:  JPEG HAS BEEN SAVED: " << saveName << endl;
}

//
// save current circuit's as BMP file in imgPosition & iternumber
void SaveBinPlotAsJPEG(string imgName, string imgPosition) {
  if(!isPlotEnvInit) {
    pe.Init();
    isPlotEnvInit = true;
  }

  CImg< unsigned char > img(pe.GetTotalImageWidth(), pe.GetTotalImageHeight(),
                            1, 3, 255);

  SaveBinPlot(img);

  //    cout << "current imgName: " << imgName << endl;
  // Finally draw image info
  string saveName = imgPosition + string(".jpg");

  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 30);
  img.save_jpeg(saveName.c_str(), 70);
  //  img.save_bmp( string(imgPosition + string(".bmp")).c_str() );
  cout << "INFO:  JPEG HAS BEEN SAVED: " << saveName << endl;
}
//
// save current circuit's as BMP file in imgPosition & iternumber
void SaveArrowPlotAsJPEG(string imgName, string imgPosition) {
  if(!isPlotEnvInit) {
    pe.Init();
    isPlotEnvInit = true;
  }

  CImg< unsigned char > img(pe.GetTotalImageWidth(), pe.GetTotalImageHeight(),
                            1, 3, 255);

  SaveArrowPlot(img);

  //    cout << "current imgName: " << imgName << endl;
  // Finally draw image info
  string saveName = imgPosition + string(".jpg");

  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 30);
  img.save_jpeg(saveName.c_str(), 70);
  //  img.save_bmp( string(imgPosition + string(".bmp")).c_str() );
  cout << "INFO:  JPEG HAS BEEN SAVED: " << saveName << endl;
}

// control vector's index to plot
int IncreaseIdx(vector< CImg< unsigned char > > *imgStor, unsigned *curIdx) {
  return (imgStor->size() - 1 > *curIdx) ? ++(*curIdx) : imgStor->size() - 1;
}

int DecreaseIdx(unsigned *curIdx) {
  return (*curIdx > 0) ? --(*curIdx) : 0;
}

// using X11
void ShowPlot(string circuitName) {
  // imgStor must have at least one image.
  assert(imgStor.size() > 1);

  if(!isPlotEnvInit) {
    pe.Init();
    isPlotEnvInit = true;
  }

  CImgDisplay disp(
      (int)pe.dispWidth, (int)pe.dispHeight,
      string(circuitName + " Circuit viewer (Made by mgwoo)").c_str(), 0);
  CImgDisplay dispz(
      500, 500, string(circuitName + " Circuit zoomer (Made by mgwoo)").c_str(),
      0);

  disp.move((CImgDisplay::screen_width() - disp.width()) / 2,
            (CImgDisplay::screen_height() - disp.height()) / 2);
  dispz.move(0, 0);

  /*
      unsigned itercnt = 0;
      // Enter interactive loop
      //------------------------
      while (!disp.is_closed() && !disp.is_keyESC() && !disp.is_keyQ()) {
          if (disp.key()) {
              switch (disp.key()) {
                  case cimg::keyARROWRIGHT:
                                      imgStor[IncreaseIdx(&imgStor,
     &itercnt)].display(disp); break;
                  case cimg::keyARROWLEFT:
                                      imgStor[DecreaseIdx(&itercnt)].display(disp);
     break;

                  case cimg::keyA:    itercnt = 0;
                                      imgStor[itercnt].display(disp); break;

                  case cimg::keyS:    itercnt = imgStor.size()-1;
                                      imgStor[itercnt].display(disp); break;

                  case cimg::keyZ:    dispWidth *= 1.2; dispHeight *= 1.2;
                                      disp.resize( (int)dispWidth,
     (int)dispHeight ); break;

                  case cimg::keyX:    dispWidth /= 1.2; dispHeight /= 1.2;
                                      disp.resize( (int)dispWidth,
     (int)dispHeight ); break;
              }
              disp.set_key();
          }
          disp.wait();
      }
  */
  unsigned itercnt = 0;
  float factor = 200;
  int x = 0, y = 0;
  float xUnit = pe.imageWidth / pe.dispWidth,
        yUnit = pe.imageHeight / pe.dispHeight;

  bool redraw = false;
  while(!disp.is_closed() && !dispz.is_closed() && !disp.is_keyQ() &&
        !dispz.is_keyQ() && !disp.is_keyESC() && !dispz.is_keyESC()) {
    if(disp.mouse_x() >= 0) {
      x = disp.mouse_x() * xUnit;
      y = disp.mouse_y() * yUnit;
      redraw = true;
    }
    if(redraw) {
      const int x0 = x - factor, y0 = y - factor, x1 = x + factor,
                y1 = y + factor;

      (+imgStor[itercnt])
          .draw_rectangle(x0, y0, x1, y1, yellow, 0.7f)
          .display(disp);

      CImg< unsigned char > visu = imgStor[itercnt]
                                       .get_crop(x0, y0, x1, y1)
                                       .draw_point(x - x0, y - y0, red, 0.2f)
                                       .resize(dispz);
      visu.draw_text(10, 10, "Coords (%d,%d)", black, 0, 1, 13, x, y)
          .display(dispz);
    }
    if(disp.is_resized())
      disp.resize(disp);
    if(dispz.is_resized()) {
      dispz.resize();
      redraw = true;
    }

    if(disp.key()) {
      switch(disp.key()) {
        case cimg::keyARROWRIGHT:
          imgStor[IncreaseIdx(&imgStor, &itercnt)].display(disp);
          break;
        case cimg::keyARROWLEFT:
          imgStor[DecreaseIdx(&itercnt)].display(disp);
          break;

        case cimg::keyA:
          itercnt = 0;
          imgStor[itercnt].display(disp);
          break;

        case cimg::keyS:
          itercnt = imgStor.size() - 1;
          imgStor[itercnt].display(disp);
          break;

        case cimg::keyZ:
          factor /= 1.4;
          redraw = true;
          break;

        case cimg::keyX:
          factor *= 1.4;
          redraw = true;
          break;
      }
      disp.set_key();
    }
    CImgDisplay::wait(disp, dispz);
  }
}

// Update gcell_st's pin info before plotting
void GCellPinCoordiUpdate() {
  FPOS *y_st = (struct FPOS *)malloc(sizeof(struct FPOS) * moduleCNT);
  for(int i = 0; i < moduleCNT; i++) {
    gcell_st[i].center = y_st[i] = moduleInstance[i].center;
  }

  // gcell_st -> pmin, pmax update by given center coordinates (y_st)
  cell_update(y_st, moduleCNT);
  free(y_st);
}

// integer -> zero-filled 4 digits string
void idx2str4digits(int idx, char *str) {
  if(idx < 10) {
    strcpy(str, "000");
    itoa(idx, str + 3);
  }
  else if(idx < 100) {
    strcpy(str, "00");
    itoa(idx, str + 2);
  }
  else if(idx < 1000) {
    strcpy(str, "0");
    itoa(idx, str + 1);
  }
  else {
    strcpy(str, "");
    itoa(idx, str);
  }
}

string intoFourDigit(int idx) {
  std::stringstream ss;
  ss << std::setw(4) << std::setfill('0') << idx;
  return ss.str();
}

void plot_bin(string fn, int idx, prec scale, int lab) {
  char fig[BUF_SZ];
  char jpg[BUF_SZ];
  char cmd[BUF_SZ];
  char str[BUF_SZ];
  char dir1[BUF_SZ];
  char dir2[BUF_SZ];
  char mkdir_cmd[BUF_SZ];

  switch(lab) {
    case 0:
      // bin density
      strcpy(dir1, "den");
      strcpy(dir2, "2d");
      break;

    case 1:
      // gradient arrow
      strcpy(dir1, "den");
      strcpy(dir2, "grad");
      break;

    case 2:
      // bin density + gradient arrow
      strcpy(dir1, "den");
      strcpy(dir2, "grad2");
      break;

    case 3:
      // bin dft
      strcpy(dir1, "den");
      strcpy(dir2, "dft");
      break;

    case 4:
      // field + bin density
      strcpy(dir1, "field");
      strcpy(dir2, "arrowed");
      break;

    case 5:
      // bin potential
      strcpy(dir1, "phi");
      strcpy(dir2, "2d");
      break;

    default:
      return;
      break;
  }

  sprintf(mkdir_cmd, "mkdir -p %s/%s/%s", dir_bnd, dir1, dir2);
  system(mkdir_cmd);
  idx2str4digits(idx, str);
  sprintf(fig, "%s/%s/%s/%s%s.fig", dir_bnd, dir1, dir2, fn.c_str(), str);
  sprintf(jpg, "%s/%s/%s/%s%s.jpg", dir_bnd, dir1, dir2, fn.c_str(), str);

  if(numLayer == 1)
    xfig_cGP2D(scale, scale, fig, lab);
  else
    xfig_cGP2D_3d(scale, scale, fig, lab);

  sprintf(cmd, "fig2dev -L jpeg %s %s", fig, jpg);
  system(cmd);
  sprintf(cmd, "rm -rvf %s >/dev/null 2>&1", fig);
  system(cmd);
}

void xfig_gp(prec x_stp, prec y_stp, char *file, int lab) {
  FILE *fp_fig = fopen(file, "w");
  int i = 0, cnt = 0;
  int x1 = 0, y1 = 0;  // left;
  int x2 = 0, y2 = 0;  // right;
  int x3 = 0, y3 = 0;  // up;
  int x4 = 0, y4 = 0;  // down;

  prec min_x = gmin.x;
  prec min_y = gmin.y;
  prec xwd = gmax.x - gmin.x;
  prec ywd = gmax.y - gmin.y;
  prec x_A2_len = 23.3;  // 23.39 inches ;
  prec y_A2_len = 16.5;  // 16.54 inches ;
  prec x_len = x_A2_len;
  prec y_len = y_A2_len;
  prec tot_x = (prec)((prec)x_len * (prec)1200);
  prec tot_y = (prec)((prec)y_len * (prec)1200);
  prec unit_x = tot_x / xwd;
  prec unit_y = tot_y / ywd;

  struct MODULE *mdp = NULL;
  struct TERM *term = NULL;
  struct CELLx *cell = NULL;
  struct FPOS p0 = zeroFPoint, p1 = zeroFPoint;

  fprintf(fp_fig, "#FIG 3.2  Produced by xfig version 3.2.5\n");
  fprintf(fp_fig, "Landscape\n");
  fprintf(fp_fig, "Center\n");
  fprintf(fp_fig, "Inches\n");
  fprintf(fp_fig, "A2\n");
  fprintf(fp_fig, "100.00\n");
  fprintf(fp_fig, "Single\n");
  fprintf(fp_fig, "-2\n");
  fprintf(fp_fig, "1200 1\n");

  p0 = gmin;
  p1 = gmax;

  fprintf(fp_fig, "2 1 0 8 %d 7 50 -1 -1 0.000 0 0 -1 0 0 5\n", BLACK);
  fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", (int)(p0.x),
          (int)(tot_y - p0.y), (int)(p0.x),
          (int)(tot_y - p1.y / y_stp * unit_y), (int)(p1.x / x_stp * unit_x),
          (int)(tot_y - p1.y / y_stp * unit_y), (int)(p1.x / x_stp * unit_x),
          (int)(tot_y - p0.y), (int)(p0.x), (int)(tot_y - p0.y));

  cnt = lab == 0 ? moduleCNT : gcell_cnt;

  if(STAGE == mGP3D || STAGE == INITIAL_PLACE || STAGE == cGP3D) {
    ////// FILLERS

    if(lab == 1) {
      for(i = cnt - 1; i >= 0; i--) {
        cell = &gcell_st[i];

        cell->pmin.x = cell->center.x - 0.5 * cell->size.x;
        cell->pmin.y = cell->center.y - 0.5 * cell->size.y;

        cell->pmax.x = cell->center.x + 0.5 * cell->size.x;
        cell->pmax.y = cell->center.y + 0.5 * cell->size.y;

        x1 = (int)((cell->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        x2 = (int)((cell->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x3 = (int)((cell->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x4 = (int)((cell->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        if(cell->flg == FillerCell) {
          fprintf(fp_fig, "2 3 0 0 5 %d 500 -1 %d 0.000 0 0 -1 0 0 5\n", 12,
                  30);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }

    ////// MACROS

    if(lab == 0) {
      for(i = cnt - 1; i >= 0; i--) {
        mdp = &moduleInstance[i];

        mdp->pmin.x = mdp->center.x - 0.5 * mdp->size.x;
        mdp->pmin.y = mdp->center.y - 0.5 * mdp->size.y;

        mdp->pmax.x = mdp->center.x + 0.5 * mdp->size.x;
        mdp->pmax.y = mdp->center.y + 0.5 * mdp->size.y;

        x1 = (int)((mdp->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        x2 = (int)((mdp->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x3 = (int)((mdp->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x4 = (int)((mdp->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        if(mdp->flg == Macro) {
          fprintf(fp_fig, "2 1 0 7 %d 7 500 -1 -1 0.000 0 0 -1 0 0 5\n", 9);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }
    else {
      for(i = cnt - 1; i >= 0; i--) {
        cell = &gcell_st[i];

        cell->pmin.x = cell->center.x - 0.5 * cell->size.x;
        cell->pmin.y = cell->center.y - 0.5 * cell->size.y;

        cell->pmax.x = cell->center.x + 0.5 * cell->size.x;
        cell->pmax.y = cell->center.y + 0.5 * cell->size.y;

        x1 = (int)((cell->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        x2 = (int)((cell->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x3 = (int)((cell->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x4 = (int)((cell->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        if(cell->flg == Macro) {
          fprintf(fp_fig, "2 1 0 7 %d 7 500 -1 -1 0.000 0 0 -1 0 0 5\n", 9);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }

    ////// STD CELLS

    if(lab == 0) {
      for(i = cnt - 1; i >= 0; i--) {
        mdp = &moduleInstance[i];

        mdp->pmin.x = mdp->center.x - 0.5 * mdp->size.x;
        mdp->pmin.y = mdp->center.y - 0.5 * mdp->size.y;

        mdp->pmax.x = mdp->center.x + 0.5 * mdp->size.x;
        mdp->pmax.y = mdp->center.y + 0.5 * mdp->size.y;

        x1 = (int)((mdp->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        x2 = (int)((mdp->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x3 = (int)((mdp->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x4 = (int)((mdp->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        if(mdp->flg == StdCell) {
          fprintf(fp_fig, "2 3 0 0 5 %d 500 -1 %d 0.000 0 0 -1 0 0 5\n", 19,
                  15);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }
    else {
      for(i = cnt - 1; i >= 0; i--) {
        cell = &gcell_st[i];

        cell->pmin.x = cell->center.x - 0.5 * cell->size.x;
        cell->pmin.y = cell->center.y - 0.5 * cell->size.y;

        cell->pmax.x = cell->center.x + 0.5 * cell->size.x;
        cell->pmax.y = cell->center.y + 0.5 * cell->size.y;

        x1 = (int)((cell->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        x2 = (int)((cell->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x3 = (int)((cell->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x4 = (int)((cell->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        if(cell->flg == StdCell) {
          fprintf(fp_fig, "2 3 0 0 5 %d 500 -1 %d 0.000 0 0 -1 0 0 5\n", 19,
                  15);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }
  }
  else {
    ////// FILLERS

    if(lab == 1) {
      for(i = cnt - 1; i >= 0; i--) {
        cell = &gcell_st[i];

        cell->pmin.x = cell->center.x - 0.5 * cell->size.x;
        cell->pmin.y = cell->center.y - 0.5 * cell->size.y;

        cell->pmax.x = cell->center.x + 0.5 * cell->size.x;
        cell->pmax.y = cell->center.y + 0.5 * cell->size.y;

        x1 = (int)((cell->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        x2 = (int)((cell->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x3 = (int)((cell->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x4 = (int)((cell->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        if(cell->flg == FillerCell) {
          fprintf(fp_fig, "2 3 0 0 5 %d 500 -1 %d 0.000 0 0 -1 0 0 5\n", 12,
                  30);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }

    ////// STD CELLS

    if(lab == 0) {
      for(i = cnt - 1; i >= 0; i--) {
        mdp = &moduleInstance[i];

        mdp->pmin.x = mdp->center.x - 0.5 * mdp->size.x;
        mdp->pmin.y = mdp->center.y - 0.5 * mdp->size.y;

        mdp->pmax.x = mdp->center.x + 0.5 * mdp->size.x;
        mdp->pmax.y = mdp->center.y + 0.5 * mdp->size.y;

        x1 = (int)((mdp->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        x2 = (int)((mdp->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x3 = (int)((mdp->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x4 = (int)((mdp->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        if(mdp->flg == StdCell) {
          fprintf(fp_fig, "2 3 0 0 5 %d 500 -1 %d 0.000 0 0 -1 0 0 5\n", 19,
                  15);

          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }
    else {
      for(i = cnt - 1; i >= 0; i--) {
        cell = &gcell_st[i];

        cell->pmin.x = cell->center.x - 0.5 * cell->size.x;
        cell->pmin.y = cell->center.y - 0.5 * cell->size.y;

        cell->pmax.x = cell->center.x + 0.5 * cell->size.x;
        cell->pmax.y = cell->center.y + 0.5 * cell->size.y;

        x1 = (int)((cell->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        x2 = (int)((cell->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x3 = (int)((cell->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x4 = (int)((cell->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        if(cell->flg == StdCell) {
          fprintf(fp_fig, "2 3 0 0 5 %d 500 -1 %d 0.000 0 0 -1 0 0 5\n", 19,
                  15);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }

    ////// MACROS

    if(lab == 0) {
      for(i = cnt - 1; i >= 0; i--) {
        mdp = &moduleInstance[i];

        mdp->pmin.x = mdp->center.x - 0.5 * mdp->size.x;
        mdp->pmin.y = mdp->center.y - 0.5 * mdp->size.y;

        mdp->pmax.x = mdp->center.x + 0.5 * mdp->size.x;
        mdp->pmax.y = mdp->center.y + 0.5 * mdp->size.y;

        x1 = (int)((mdp->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        x2 = (int)((mdp->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x3 = (int)((mdp->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (mdp->pmax.y - min_y) * unit_y);

        x4 = (int)((mdp->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (mdp->pmin.y - min_y) * unit_y);

        if(mdp->flg == Macro) {
          fprintf(fp_fig, "2 1 0 7 %d 7 500 -1 -1 0.000 0 0 -1 0 0 5\n", 9);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }
    else {
      for(i = cnt - 1; i >= 0; i--) {
        cell = &gcell_st[i];

        cell->pmin.x = cell->center.x - 0.5 * cell->size.x;
        cell->pmin.y = cell->center.y - 0.5 * cell->size.y;

        cell->pmax.x = cell->center.x + 0.5 * cell->size.x;
        cell->pmax.y = cell->center.y + 0.5 * cell->size.y;

        x1 = (int)((cell->pmin.x - min_x) * unit_x);
        y1 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        x2 = (int)((cell->pmin.x - min_x) * unit_x);
        y2 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x3 = (int)((cell->pmax.x - min_x) * unit_x);
        y3 = (int)(tot_y - (cell->pmax.y - min_y) * unit_y);

        x4 = (int)((cell->pmax.x - min_x) * unit_x);
        y4 = (int)(tot_y - (cell->pmin.y - min_y) * unit_y);

        if(cell->flg == Macro) {
          fprintf(fp_fig, "2 1 0 7 %d 7 500 -1 -1 0.000 0 0 -1 0 0 5\n", 9);
          fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                  y3, x4, y4, x1, y1);
        }
      }
    }
  }

  // fixed blocks

  for(i = 0; i < terminalCNT; i++) {
    term = &terminalInstance[i];

    x1 = (int)((term->pmin.x - min_x) / x_stp * unit_x);
    y1 = (int)(tot_y - (term->pmin.y - min_y) / y_stp * unit_y);

    x2 = (int)((term->pmin.x - min_x) / x_stp * unit_x);
    y2 = (int)(tot_y - (term->pmax.y - min_y) / y_stp * unit_y);

    x3 = (int)((term->pmax.x - min_x) / x_stp * unit_x);
    y3 = (int)(tot_y - (term->pmax.y - min_y) / y_stp * unit_y);

    x4 = (int)((term->pmax.x - min_x) / x_stp * unit_x);
    y4 = (int)(tot_y - (term->pmin.y - min_y) / y_stp * unit_y);

    if(!term->IO) {
      fprintf(fp_fig, "2 1 0 7 %d 7 500 -1 -1 0.000 0 0 -1 0 0 5\n",
              9 /* color */);
      /* fprintf(fp_fig, */
      /*    "2 3 0 0 7 %d 50 -1 %d 0.000 0 0 -1 0 0 5\n",color,10); */
      fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3, y3,
              x4, y4, x1, y1);
    }
    else {
      fprintf(fp_fig, "2 3 0 0 7 %d 50 -1 %d 0.000 0 0 -1 0 0 5\n",
              7 /* color */, /* 12 */ 8);
      fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3, y3,
              x4, y4, x1, y1);
    }
  }

  fclose(fp_fig);
}

void xfig_cGP2D(prec x_stp, prec y_stp, char *file, int lab) {
  FILE *fp_fig = fopen(file, "w");

  prec min_x0 = place.org.x;
  prec min_y0 = place.org.y;
  prec max_x0 = place.end.x;
  prec max_y0 = place.end.y;
  prec xwd0 = max_x0 - min_x0;
  prec ywd0 = max_y0 - min_y0;
  prec ex_wd = xwd0 * 0.05;
  prec ey_wd = ywd0 * 0.05;
  prec xwd = xwd0 + ex_wd * 2.0;
  prec ywd = ywd0 + ey_wd * 2.0;
  prec min_x = min_x0 - ex_wd;
  prec min_y = min_y0 - ey_wd;

  prec x_A2_len = 23.3;  // 23.39 inches ;
  prec y_A2_len = 16.5;  // 16.54 inches ;

  prec x_len = x_A2_len;
  prec y_len = y_A2_len;

  prec tot_x = (prec)((prec)x_len * (prec)1200);
  prec tot_y = (prec)((prec)y_len * (prec)1200);

  prec unit_x = tot_x / xwd;
  prec unit_y = tot_y / ywd;

  int i = 0;
  int x1 = 0, y1 = 0;  // left;
  int x2 = 0, y2 = 0;  // right;
  int x3 = 0, y3 = 0;  // up;
  int x4 = 0, y4 = 0;  // down;
  int x_idx;
  int y_idx;
  struct BIN *bp = NULL;

  struct FPOS end_pt = zeroFPoint;
  struct POS p = zeroPoint;
  struct TIER *tier = &tier_st[0];

  int bin_color = 0, bin_power = 0;
  int resolutionControl = 1024;

  prec sgn_x = 0, sgn_y = 0;

  fprintf(fp_fig, "#FIG 3.2  Produced by xfig version 3.2.5\n");
  fprintf(fp_fig, "Landscape\n");
  fprintf(fp_fig, "Center\n");
  fprintf(fp_fig, "Inches\n");
  fprintf(fp_fig, "A2\n");
  fprintf(fp_fig, "100.00\n");
  fprintf(fp_fig, "Single\n");
  fprintf(fp_fig, "-2\n");
  fprintf(fp_fig, "1200 1\n");

  struct FPOS fp0 = zeroFPoint, fp1 = zeroFPoint;

  fp0.x = gmin.x - gwid.x * 0.03;
  fp0.y = gmin.y - gwid.y * 0.03;
  fp1.x = gmax.x + gwid.x * 0.03;
  fp1.y = gmax.y + gwid.y * 0.03;

  fprintf(fp_fig, "2 1 0 8 %d 7 50 -1 -1 0.000 0 0 -1 0 0 5\n", BLACK);
  fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n",
          (int)((fp0.x - min_x) / x_stp * unit_x),
          (int)(tot_y - (fp0.y - min_y) / y_stp * unit_y),
          (int)((fp0.x - min_x) / x_stp * unit_x),
          (int)(tot_y - (fp1.y - min_y) / y_stp * unit_y),
          (int)((fp1.x - min_x) / x_stp * unit_x),
          (int)(tot_y - (fp1.y - min_y) / y_stp * unit_y),
          (int)((fp1.x - min_x) / x_stp * unit_x),
          (int)(tot_y - (fp0.y - min_y) / y_stp * unit_y),
          (int)((fp0.x - min_x) / x_stp * unit_x),
          (int)(tot_y - (fp0.y - min_y) / y_stp * unit_y));

  switch(lab) {
    case 0:  // bin density

      for(i = 0; i < tier->tot_bin_cnt; i++) {
        bp = &tier->bin_mat[i];

        //  get_bin_color ( bp,&color,&power );

        get_bin_power3(bp, &bin_color, &bin_power, lab);

        x1 = (int)((bp->pmin.x - min_x) / x_stp * unit_x);
        y1 = (int)(tot_y - (bp->pmin.y - min_y) / y_stp * unit_y);

        x2 = (int)((bp->pmin.x - min_x) / x_stp * unit_x);
        y2 = (int)(tot_y - (bp->pmax.y - min_y) / y_stp * unit_y);

        x3 = (int)((bp->pmax.x - min_x) / x_stp * unit_x);
        y3 = (int)(tot_y - (bp->pmax.y - min_y) / y_stp * unit_y);

        x4 = (int)((bp->pmax.x - min_x) / x_stp * unit_x);
        y4 = (int)(tot_y - (bp->pmin.y - min_y) / y_stp * unit_y);

        fprintf(fp_fig, "2 3 0 0 7 %d 50 -1 %d 0.000 0 0 -1 0 0 5\n", 0,
                (20 - bin_power));
        fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                y3, x4, y4, x1, y1);
      }

      break;

    case 4: {
      // field + bin density
      for(i = 0; i < tier->tot_bin_cnt; i++) {
        bp = &tier->bin_mat[i];
        get_bin_power2(bp, &bin_color, &bin_power, lab);

        x1 = (int)((bp->pmin.x - min_x) / x_stp * unit_x);
        y1 = (int)(tot_y - (bp->pmin.y - min_y) / y_stp * unit_y);

        x2 = (int)((bp->pmin.x - min_x) / x_stp * unit_x);
        y2 = (int)(tot_y - (bp->pmax.y - min_y) / y_stp * unit_y);

        x3 = (int)((bp->pmax.x - min_x) / x_stp * unit_x);
        y3 = (int)(tot_y - (bp->pmax.y - min_y) / y_stp * unit_y);

        x4 = (int)((bp->pmax.x - min_x) / x_stp * unit_x);
        y4 = (int)(tot_y - (bp->pmin.y - min_y) / y_stp * unit_y);

        fprintf(fp_fig, "2 3 0 0 7 %d 50 -1 %d 0.000 0 0 -1 0 0 5\n",
                0 /* bin_color */, 20 - bin_power);
        fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                y3, x4, y4, x1, y1);
      }

      if(STAGE == mGP2D) {
        x_idx = (int)((prec)max_bin.x /
                      ((prec)resolutionControl / (prec)dim_bin_mGP2D.x));
        y_idx = (int)((prec)max_bin.y /
                      ((prec)resolutionControl / (prec)dim_bin_mGP2D.y));
      }
      else if(STAGE == cGP2D) {
        x_idx = (int)((prec)max_bin.x /
                      ((prec)resolutionControl / (prec)dim_bin_cGP2D.x));
        y_idx = (int)((prec)max_bin.y /
                      ((prec)resolutionControl / (prec)dim_bin_cGP2D.y));
      }
      else {
        x_idx = max_bin.x / 16;
        y_idx = max_bin.y / 16;
      }

      if(x_idx <= 0)
        x_idx = 1;
      if(y_idx <= 0)
        y_idx = 1;

      prec max_ex = FLT_MIN;
      prec max_ey = FLT_MIN;

      for(i = 0; i < tier->tot_bin_cnt; i++) {
        bp = &tier->bin_mat[i];
        p = get_bin_idx(i);

        if((p.x % x_idx != 0 && p.x != max_bin.x - 1) ||
           (p.y % y_idx != 0 && p.y != max_bin.y - 1))
          continue;

        // igkang
        max_ex = max(max_ex, (prec)fabs(bp->e.x));
        max_ey = max(max_ey, (prec)fabs(bp->e.y));
      }

      for(i = 0; i < tier->tot_bin_cnt; i++) {
        bp = &tier->bin_mat[i];
        p = get_bin_idx(i);

        if((p.x % x_idx != 0 && p.x != max_bin.x - 1) ||
           (p.y % y_idx != 0 && p.y != max_bin.y - 1))
          continue;
        // igkang
        sgn_x = bp->e.x > 0 ? 1.0 : -1.0;
        sgn_y = bp->e.y > 0 ? 1.0 : -1.0;
        // sgn_x = bp->e_local.x > 0 ? 1.0 : -1.0;
        // sgn_y = bp->e_local.y > 0 ? 1.0 : -1.0;

        prec mag_e = 1.0;

        // igkang
        prec mag_vx = fabs(bp->e.x) / mag_e;
        prec mag_vy = fabs(bp->e.y) / mag_e;
        // prec mag_vx = fabs(bp->e_local.x) / mag_e;
        // prec mag_vy = fabs(bp->e_local.y) / mag_e;

        end_pt.x =
            bp->center.x + sgn_x * mag_vx / max_ex * 0.5 * bin_stp.x * 10.0;
        end_pt.y =
            bp->center.y + sgn_y * mag_vy / max_ey * 0.5 * bin_stp.y * 10.0;

        if(end_pt.x < gmin.x)
          end_pt.x = gmin.x + 1.0;
        if(end_pt.x > gmax.x)
          end_pt.x = gmax.x - 1.0;

        if(end_pt.y < gmin.y)
          end_pt.y = gmin.y + 1.0;
        if(end_pt.y > gmax.y)
          end_pt.y = gmax.y - 1.0;

        x1 = (int)((bp->center.x - min_x) / x_stp * unit_x);
        y1 = (int)(tot_y - (bp->center.y - min_y) / y_stp * unit_y);

        x2 = (int)((end_pt.x - min_x) / x_stp * unit_x);
        y2 = (int)(tot_y - (end_pt.y - min_y) / y_stp * unit_y);

        x3 = x2;
        y3 = y2;

        fprintf(fp_fig, "2 1 0 6 %d 7 50 -1 -1 0.000 0 0 -1 1 0 3\n", 20);
        fprintf(fp_fig, "1 1 1 200 200 \n");
        fprintf(fp_fig, "%d %d %d %d %d %d\n", x2, y2, x1, y1, x2, y2);
      }
    } break;

    case 5:  // bin potential

      for(i = 0; i < tier->tot_bin_cnt; i++) {
        bp = &tier->bin_mat[i];

        //  get_bin_color ( bp,&color,&power );

        get_bin_power(bp, &bin_color, &bin_power, lab);

        x1 = (int)((bp->pmin.x - min_x) / x_stp * unit_x);
        y1 = (int)(tot_y - (bp->pmin.y - min_y) / y_stp * unit_y);

        x2 = (int)((bp->pmin.x - min_x) / x_stp * unit_x);
        y2 = (int)(tot_y - (bp->pmax.y - min_y) / y_stp * unit_y);

        x3 = (int)((bp->pmax.x - min_x) / x_stp * unit_x);
        y3 = (int)(tot_y - (bp->pmax.y - min_y) / y_stp * unit_y);

        x4 = (int)((bp->pmax.x - min_x) / x_stp * unit_x);
        y4 = (int)(tot_y - (bp->pmin.y - min_y) / y_stp * unit_y);

        fprintf(fp_fig, "2 3 0 0 7 %d 50 -1 %d 0.000 0 0 -1 0 0 5\n", bin_color,
                bin_power);
        fprintf(fp_fig, "%d %d %d %d %d %d %d %d %d %d\n", x1, y1, x2, y2, x3,
                y3, x4, y4, x1, y1);
      }

      break;

    default:

      break;
  }

  fclose(fp_fig);
}

void get_bin_power2(struct BIN *bp, int *color, int *power, int lab) {
  int c = 0, p = 0;

  switch(lab) {
    case 0:  // bin density
      c = GREEN;
      p = (int)(bp->den * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    /*     case 1: // gradient arrow  */
    /*       c = GREEN ; */
    /*       p = (int)(bp->den * 30.0) ; */
    /*       p = 40 - p < 0 ? 0 : 40 - p ; */
    /*       break; */

    case 2:  // bin density + gradient arrow
      c = GREEN;
      p = (int)(bp->den * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    case 4:  // field
      c = GREEN;
      p = (int)(bp->den * 10.0);
      p = 20 - p < 0 ? 0 : 20 - p;
      break;

    case 5:  // potential
      c = RED;
      p = (int)(bp->phi * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    default:

      break;
  }

  *color = c;
  *power = p;
}

void get_bin_power3(struct BIN *bp, int *color, int *power, int lab) {
  int c = 0, p = 0;

  switch(lab) {
    case 0:  // bin density
      c = GREEN;
      p = (int)(bp->den * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    /*     case 1: // gradient arrow  */
    /*       c = GREEN ; */
    /*       p = (int)(bp->den * 30.0) ; */
    /*       p = 40 - p < 0 ? 0 : 40 - p ; */
    /*       break; */

    case 2:  // bin density + gradient arrow
      c = GREEN;
      p = (int)(bp->den * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    case 4:  // field
      c = GREEN;
      p = (int)(bp->den * 10.0);
      p = 20 - p < 0 ? 0 : 20 - p;
      break;

    case 5:  // potential
      c = RED;
      p = (int)(bp->phi * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    case 7:  // field
      c = GREEN;
      p = (int)(bp->den * 10.0);
      p = 20 - p < 0 ? 0 : 20 - p;
      break;

    default:

      break;
  }

  *color = c;
  *power = p;
}

void get_bin_power(struct BIN *bp, int *color, int *power, int lab) {
  int c = 0, p = 0;

  switch(lab) {
    case 0:  // bin density
      c = GREEN;
      p = (int)(bp->den * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    case 2:  // bin density + gradient arrow
      c = GREEN;
      p = (int)(bp->den * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    case 4:  // field
      c = GREEN;
      p = (int)(bp->den * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    case 5:  // potential
      c = RED;
      p = (int)(bp->phi * 30.0);
      p = 40 - p < 0 ? 0 : 40 - p;
      break;

    default:

      break;
  }

  *color = c;
  *power = p;
}

void get_bin_color(struct BIN *bp, int *color, int *power) {
  *power = 20;

  if(bp->den < 0.2)
    *color = WHITE;
  else if(bp->den < 0.4)
    *color = YELLOW;
  else if(bp->den < 0.6)
    *color = GREEN;
  else if(bp->den < 0.8)
    *color = BLUE;
  else if(bp->den < 1.0)
    *color = RED;
  else if(bp->den < 1.5)
    *color = MAGENTA;
  else
    *color = BLACK;
}

void mkdirPlot() {
  char mkdir_cmd[BUF_SZ] = {
      0,
  };

  sprintf(mkdir_cmd, "mkdir -p %s/initPlace", dir_bnd);
  system(mkdir_cmd);

  sprintf(mkdir_cmd, "mkdir -p %s/cell", dir_bnd);
  system(mkdir_cmd);

  sprintf(mkdir_cmd, "mkdir -p %s/bin", dir_bnd);
  system(mkdir_cmd);

  sprintf(mkdir_cmd, "mkdir -p %s/arrow", dir_bnd);
  system(mkdir_cmd);
  /*
  sprintf(mkdir_cmd, "mkdir -p %s/den", dir_bnd);
  system(mkdir_cmd);
  sprintf(mkdir_cmd, "mkdir -p %s/field", dir_bnd);
  system(mkdir_cmd);
  sprintf(mkdir_cmd, "mkdir -p %s/phi", dir_bnd);
  system(mkdir_cmd);
  */
}

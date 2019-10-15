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

#ifndef __PL_PLOT__
#define __PL_PLOT__

#include <cstring>

#include "replace_private.h"
#include "bin.h"

void mkdirPlot();

std::string intoFourDigit(int idx);
void idx2str4digits(int idx, char *str);
void GCellPinCoordiUpdate();

// X11 drawing function
// below function is added by mgwoo.
void SaveCellPlotAsJPEG(std::string imgName, bool isGCell, std::string imgPosition);
void SaveBinPlotAsJPEG(std::string imgName, std::string imgPosition);
void SaveArrowPlotAsJPEG(std::string imgName, std::string imgPosition);
void SavePlot(std::string imgName = "", bool isGCell = false);
void ShowPlot(std::string circuitName = "");

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
  PlotEnv();
  void Init();
  void InitCellColors(std::string colorFile);
  int GetTotalImageWidth();
  int GetTotalImageHeight();
  int GetX(FPOS &coord);
  int GetX(prec coord);
  int GetY(FPOS &coord);
  int GetY(prec coord);
};

#endif

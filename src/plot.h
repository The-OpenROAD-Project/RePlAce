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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
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

#include "global.h"
#include "bin.h"

void mkdirPlot();
void plotFinalLayout();

struct PLOT_MATRIX {
    prec val;
    int color;
};

void plot(string fn, int idx, prec scale, int lab);
void xfig_gp(prec x_stp, prec y_stp, char *file, int lab);
void xfig_gp_3d(prec x_stp, prec y_stp, char *file, int lab);

void plot_bin(string fn, int idx, prec scale, int lab);
void xfig_cGP2D(prec x_stp, prec y_stp, char *file, int lab);
void xfig_cGP2D_3d(prec x_stp, prec y_stp, char *file, int lab);


void get_bin_color(struct BIN *bp, int *color, int *power);
void get_bin_power(struct BIN *bp, int *color, int *power, int lab);
void get_bin_power2(struct BIN *bp, int *color, int *power, int lab);
void get_bin_power3(struct BIN *bp, int *color, int *power, int lab);

string intoFourDigit(int idx);
void idx2str4digits(int idx, char *str);
void GCellPinCoordiUpdate();

// X11 drawing function
// below function is added by mgwoo.
void SaveCellPlotAsJPEG(string imgName, bool isGCell, string imgPosition);
void SaveBinPlotAsJPEG(string imgName, string imgPosition);
void SaveArrowPlotAsJPEG(string imgName, string imgPosition);
void SavePlot(string imgName = "", bool isGCell = false);
void ShowPlot(string circuitName = "");


enum {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    WHITE
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

        PlotEnv();
        void Init();
        int GetTotalImageWidth();
        int GetTotalImageHeight();
        int GetX( FPOS& coord );
        int GetX( prec coord );
        int GetY( FPOS& coord );
        int GetY( prec coord );
}; 


#endif

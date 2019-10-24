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

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <ctime>

#include "replace_private.h"
#include "initPlacement.h"
#include "wlen.h"
#include "plot.h"

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <Eigen/IterativeLinearSolvers>
#include <unsupported/Eigen/IterativeSolvers>

using std::vector;
using std::string;
using std::to_string;
using std::max;
using std::min;

void initial_placement() {
  using namespace Eigen;
  printf("PROC:  Conjugate Gradient (CG) method to obtain the IP\n");

  int itmax = 100;

  prec target_tol = 0.000001;
  prec x_err = 0, y_err = 0;

  double time_s = 0;

  auto hpwl = GetUnscaledHpwl(); 

  if(isSkipIP) {
    return;
  }

  printf("INFO:  The Initial HPWL is %.6lf\n", hpwl.first + hpwl.second);

  if(hpwl.first + hpwl.second <= 0) {
    printf("ERROR: HPWL <= 0, skip initial QP\n");
    return;
  }

  printf("INFO:  The Matrix Size is %d\n", moduleCNT);
  fflush(stdout);

  // malloc to solve PCG
  //
  // Ax = b
  //
  // x : variable vector to solve
  // b : constant vector
  //

  setNbThreads(numThread);

  // BCGSTAB settings
  SMatrix eMatX(moduleCNT, moduleCNT), eMatY(moduleCNT, moduleCNT);

  VectorXf xcg_x(moduleCNT), xcg_b(moduleCNT), ycg_x(moduleCNT),
      ycg_b(moduleCNT);

  for(int i = 0;; i++) {
    if(i >= numInitPlaceIter) {
      break;
    }

    time_start(&time_s);
    CreateSparseMatrix(xcg_x, xcg_b, ycg_x, ycg_b, eMatX, eMatY);

    BiCGSTAB< SMatrix, IdentityPreconditioner > solver;
    solver.setMaxIterations(itmax);

    solver.compute(eMatX);
    xcg_x = solver.solveWithGuess(xcg_b, xcg_x);
    x_err = solver.error();

    solver.compute(eMatY);
    ycg_x = solver.solveWithGuess(ycg_b, ycg_x);
    y_err = solver.error();

    update_module(xcg_x, ycg_x);
    update_pin_by_module();
    update_net_by_pin();

    if(isPlot && i % 5 == 0) {
      SaveCellPlotAsJPEG(string("FIP - Iter: ") + to_string(i), false,
                         string(dir_bnd) + string("/initPlace/initPlacement_") +
                             intoFourDigit(i));
      // SavePlot( string("FIP - Iter: ") + to_string(i) );
    }

    time_end(&time_s);

    hpwl = GetUnscaledHpwl();

    printf("INFO:  IP%3d,  CG Error %.6lf,  HPWL %.6lf,  CPUtime %.2lf\n", i,
           max(x_err, y_err), hpwl.first + hpwl.second, time_s);
    fflush(stdout);

    if(fabs(x_err) < target_tol && fabs(y_err) < target_tol && i > 4) {
      break;
    }
  }
}

void build_data_struct(bool initCoordi) {
  MODULE *mdp = NULL;
  TERM *term = NULL;
  PIN *pin = NULL;
  NET *curNet = NULL;
  FPOS pof;

  prec min_x = 0;
  prec min_y = 0;
  prec max_x = 0;
  prec max_y = 0;

  for(int i = 0; i < moduleCNT; i++) {
    mdp = &moduleInstance[i];

    if(initCoordi) {
      mdp->center = place.center;
    }

    mdp->pmin.x = mdp->center.x - 0.5 * mdp->size.x;
    mdp->pmin.y = mdp->center.y - 0.5 * mdp->size.y;

    mdp->pmax.x = mdp->center.x + 0.5 * mdp->size.x;
    mdp->pmax.y = mdp->center.y + 0.5 * mdp->size.y;

    for(int j = 0; j < mdp->pinCNTinObject; j++) {
      pof = mdp->pof[j];
      pin = mdp->pin[j];

      pin->fp.x = mdp->center.x + pof.x;
      pin->fp.y = mdp->center.y + pof.y;

      pin->X_MIN = 0;
      pin->X_MAX = 0;
      pin->Y_MIN = 0;
      pin->Y_MAX = 0;
    }
  }

  term_pmin.x = PREC_MAX;
  term_pmin.x = PREC_MAX;
  term_pmax.x = 0;
  term_pmax.y = 0;

  for(int i = 0; i < terminalCNT; i++) {
    term = &terminalInstance[i];

    term->pmin.x = term->center.x - 0.5 * term->size.x;
    term->pmin.y = term->center.y - 0.5 * term->size.y;

    term->pmax.x = term->center.x + 0.5 * term->size.x;
    term->pmax.y = term->center.y + 0.5 * term->size.y;

    if(term_pmin.x > term->pmin.x)
      term_pmin.x = term->pmin.x;
    if(term_pmin.y > term->pmin.y)
      term_pmin.y = term->pmin.y;
    if(term_pmax.x < term->pmax.x)
      term_pmax.x = term->pmax.x;
    if(term_pmax.y < term->pmax.y)
      term_pmax.y = term->pmax.y;

    for(int j = 0; j < term->pinCNTinObject; j++) {
      pof = term->pof[j];
      pin = term->pin[j];

      pin->fp.x = term->center.x + pof.x;
      pin->fp.y = term->center.y + pof.y;

      pin->X_MIN = 0;
      pin->X_MAX = 0;
      pin->Y_MIN = 0;
      pin->Y_MAX = 0;
    }
  }

  for(int i = 0; i < netCNT; i++) {
    curNet = &netInstance[i];

    min_x = PREC_MAX;
    min_y = PREC_MAX;
    max_x = PREC_MIN;
    max_y = PREC_MIN;

    PIN *pin_xmin = NULL;
    PIN *pin_ymin = NULL;
    PIN *pin_xmax = NULL;
    PIN *pin_ymax = NULL;

    for(int j = 0; j < curNet->pinCNTinObject; j++) {
      pin = curNet->pin[j];
      if(pin_xmin) {
        if(min_x > pin->fp.x) {
          min_x = pin->fp.x;
          pin_xmin->X_MIN = 0;
          pin_xmin = pin;
          pin->X_MIN = 1;
        }
      }
      else {
        min_x = pin->fp.x;  // mdp->center.x ;
        pin_xmin = pin;
        pin->X_MIN = 1;
      }

      if(pin_ymin) {
        if(min_y > pin->fp.y)  // mdp->center.y)
        {
          min_y = pin->fp.y;  // mdp->center.y ;
          pin_ymin->Y_MIN = 0;
          pin_ymin = pin;
          pin->Y_MIN = 1;
        }
      }
      else {
        min_y = pin->fp.y;  // mdp->center.y ;
        pin_ymin = pin;
        pin->Y_MIN = 1;
      }

      if(pin_xmax) {
        if(max_x < pin->fp.x)  // mdp->center.x)
        {
          max_x = pin->fp.x;  // mdp->center.x ;
          pin_xmax->X_MAX = 0;
          pin_xmax = pin;
          pin->X_MAX = 1;
        }
      }
      else {
        max_x = pin->fp.x;  // mdp->center.x ;
        pin_xmax = pin;
        pin->X_MAX = 1;
      }

      if(pin_ymax) {
        if(max_y < pin->fp.y)  // mdp->center.y)
        {
          max_y = pin->fp.y;  // mdp->center.y ;
          pin_ymax->Y_MAX = 0;
          pin_ymax = pin;
          pin->Y_MAX = 1;
        }
      }
      else {
        max_y = pin->fp.y;  // mdp->center.y ;
        pin_ymax = pin;
        pin->Y_MAX = 1;
      }
    }

    curNet->min_x = min_x;
    curNet->min_y = min_y;

    curNet->max_x = max_x;
    curNet->max_y = max_y;
  }
}

void update_module(VectorXf &xcg_x, VectorXf &ycg_x) {
  MODULE *mdp = NULL;
  for(int i = 0; i < moduleCNT; i++) {
    mdp = &moduleInstance[i];

    mdp->center.x = xcg_x(i);
    mdp->center.y = ycg_x(i);

    if((mdp->center.x + 0.5 * mdp->size.x) > place.end.x)
      mdp->center.x = place.end.x - 0.5 * mdp->size.x - Epsilon;

    if((mdp->center.y + 0.5 * mdp->size.y) > place.end.y)
      mdp->center.y = place.end.y - 0.5 * mdp->size.y - Epsilon;

    if((mdp->center.x - 0.5 * mdp->size.x) < place.org.x)
      mdp->center.x = place.org.x + 0.5 * mdp->size.x + Epsilon;

    if((mdp->center.y - 0.5 * mdp->size.y) < place.org.y)
      mdp->center.y = place.org.y + 0.5 * mdp->size.y + Epsilon;

    mdp->pmin.x = mdp->center.x - 0.5 * mdp->size.x;
    mdp->pmin.y = mdp->center.y - 0.5 * mdp->size.y;

    mdp->pmax.x = mdp->center.x + 0.5 * mdp->size.x;
    mdp->pmax.y = mdp->center.y + 0.5 * mdp->size.y;
  }
}

void update_pin_by_module(void) {
  FPOS pof;
  PIN *pin = NULL;
  MODULE *mdp = NULL;

  for(int i = 0; i < moduleCNT; i++) {
    mdp = &moduleInstance[i];
    for(int j = 0; j < mdp->pinCNTinObject; j++) {
      pof = mdp->pof[j];
      pin = mdp->pin[j];

      if(pin->moduleID != i || pin->pinIDinModule != j || pin->term == 1) {
        exit(1);
      }

      pin->fp.x = mdp->center.x + pof.x;
      pin->fp.y = mdp->center.y + pof.y;

      pin->X_MIN = 0;
      pin->X_MAX = 0;
      pin->Y_MIN = 0;
      pin->Y_MAX = 0;
    }
  }
}

void update_net_by_pin() {
  for(int i = 0; i < netCNT; i++) {
    NET *curNet = &netInstance[i];

    PIN *pin_xmin = NULL, *pin_ymin = NULL;
    PIN *pin_xmax = NULL, *pin_ymax = NULL;

    prec min_x = PREC_MAX, min_y = PREC_MAX;
    prec max_x = PREC_MIN, max_y = PREC_MIN;

    for(int j = 0; j < curNet->pinCNTinObject; j++) {
      PIN *pin = curNet->pin[j];

      if(pin_xmin) {
        if(min_x > pin->fp.x)  // mdp->center.x)
        {
          min_x = pin->fp.x;  // mdp->center.x ;
          pin_xmin->X_MIN = 0;
          pin_xmin = pin;
          pin->X_MIN = 1;
        }
      }
      else {
        min_x = pin->fp.x;  // mdp->center.x ;
        pin_xmin = pin;
        pin->X_MIN = 1;
      }

      if(pin_ymin) {
        if(min_y > pin->fp.y)  // mdp->center.y)
        {
          min_y = pin->fp.y;  // mdp->center.y ;
          pin_ymin->Y_MIN = 0;
          pin_ymin = pin;
          pin->Y_MIN = 1;
        }
      }
      else {
        min_y = pin->fp.y;  // mdp->center.y ;
        pin_ymin = pin;
        pin->Y_MIN = 1;
      }

      if(pin_xmax) {
        if(max_x < pin->fp.x)  // mdp->center.x)
        {
          max_x = pin->fp.x;  // mdp->center.x ;
          pin_xmax->X_MAX = 0;
          pin_xmax = pin;
          pin->X_MAX = 1;
        }
      }
      else {
        max_x = pin->fp.x;  // mdp->center.x ;
        pin_xmax = pin;
        pin->X_MAX = 1;
      }

      if(pin_ymax) {
        if(max_y < pin->fp.y)  // mdp->center.y)
        {
          max_y = pin->fp.y;  // mdp->center.y ;
          pin_ymax->Y_MAX = 0;
          pin_ymax = pin;
          pin->Y_MAX = 1;
        }
      }
      else {
        max_y = pin->fp.y;  // mdp->center.y ;
        pin_ymax = pin;
        pin->Y_MAX = 1;
      }
    }
    curNet->min_x = min_x;
    curNet->min_y = min_y;
    curNet->max_x = max_x;
    curNet->max_y = max_y;
  }
}

//
// CreateSparseMatrix Routine
//
// using current Pin's structure,
// based on the B2B models,
// it genereates Sparsematrix into Eigen formats.
//
void CreateSparseMatrix(VectorXf &xcg_x, VectorXf &xcg_b, VectorXf &ycg_x,
                        VectorXf &ycg_b, SMatrix &eMatX, SMatrix &eMatY) {
  int pinCNTinObject = 0;
  int moduleID1 = 0;
  int moduleID2 = 0;
  int is_term1 = 0;
  int is_term2 = 0;

  prec common1;
  prec common2;

  NET *tempNet = NULL;
  PIN *pin1 = NULL;
  PIN *pin2 = NULL;

  MODULE *mdp1 = NULL;
  MODULE *mdp2 = NULL;
  TERM *term1 = NULL;
  TERM *term2 = NULL;

  FPOS center1, center2;
  FPOS fp1, fp2;

  // to easily convert (i, j, value) -> CSR sparse Matrix.
  // using Eigen library..
  vector< T > tripletListX, tripletListY;
  tripletListX.reserve(10000000);
  tripletListY.reserve(10000000);

  // xcg_x & ycg_x update
  MODULE *curModule = NULL;
  for(int i = 0; i < moduleCNT; i++) {
    curModule = &moduleInstance[i];

    // 1d prec array
    xcg_x(i) = curModule->center.x;
    ycg_x(i) = curModule->center.y;

    xcg_b(i) = ycg_b(i) = 0;
  }

  for(int i = 0; i < netCNT; i++) {
    tempNet = &netInstance[i];
    pinCNTinObject = tempNet->pinCNTinObject;
    common1 = 1.0 / ((prec)pinCNTinObject - 1.0);

    for(int j = 0; j < pinCNTinObject; j++) {
      pin1 = tempNet->pin[j];
      moduleID1 = pin1->moduleID;
      fp1 = pin1->fp;

      // if is not Terminal -> moduleInstance
      if(!pin1->term) {
        mdp1 = &moduleInstance[moduleID1];
        center1 = mdp1->center;
        is_term1 = 0;
      }
      // is terminal -> terminalInstance
      else {
        term1 = &terminalInstance[moduleID1];
        center1 = term1->center;
        is_term1 = 1;
      }

      for(int k = j + 1; k < pinCNTinObject; k++) {
        pin2 = tempNet->pin[k];
        moduleID2 = pin2->moduleID;
        fp2 = pin2->fp;

        if(!pin2->term) {
          mdp2 = &moduleInstance[moduleID2];
          center2 = mdp2->center;
          is_term2 = 0;
        }
        else {
          term2 = &terminalInstance[moduleID2];
          center2 = term2->center;
          is_term2 = 1;
        }

        // there is no need to calculate (for same nodes)
        if(moduleID1 == moduleID2 && is_term1 == is_term2) {
          continue;
        }

        if(pin1->X_MIN || pin1->X_MAX || pin2->X_MIN || pin2->X_MAX) {
          prec len_x = fabs(fp1.x - fp2.x);

          prec wt_x = 0.0f;
          if(dge(len_x, MIN_LEN)) {
            wt_x = common1 / len_x;
          }
          else {
            wt_x = common1 / MIN_LEN;
          }

          common2 = (-1.0) * wt_x;

          if(wt_x < 0)
            printf("ERROR WEIGHT\n");

          // both is module
          if(!is_term1 && !is_term2) {
            tripletListX.push_back(T(moduleID1, moduleID1, wt_x));
            tripletListX.push_back(T(moduleID2, moduleID2, wt_x));
            tripletListX.push_back(T(moduleID1, moduleID2, common2));
            tripletListX.push_back(T(moduleID2, moduleID1, common2));

            xcg_b(moduleID1) +=
                common2 * ((fp1.x - center1.x) - (fp2.x - center2.x));
            xcg_b(moduleID2) +=
                common2 * ((fp2.x - center2.x) - (fp1.x - center1.x));
          }
          // 1 is terminal, 2 is module
          else if(is_term1 && !is_term2) {
            tripletListX.push_back(T(moduleID2, moduleID2, wt_x));
            xcg_b(moduleID2) += wt_x * (fp1.x - (fp2.x - center2.x));
          }
          // 2 is terminal, 1 is module
          else if(!is_term1 && is_term2) {
            tripletListX.push_back(T(moduleID1, moduleID1, wt_x));
            xcg_b(moduleID1) += wt_x * (fp2.x - (fp1.x - center1.x));
          }
        }

        if(pin1->Y_MIN || pin1->Y_MAX || pin2->Y_MIN || pin2->Y_MAX) {
          prec len_y = fabs(fp1.y - fp2.y);

          prec wt_y = 0.0f;
          if(dge(len_y, MIN_LEN)) {
            wt_y = common1 / len_y;
          }
          else {
            wt_y = common1 / MIN_LEN;
          }
          common2 = (-1.0) * wt_y;

          // both is module
          if(!is_term1 && !is_term2) {
            tripletListY.push_back(T(moduleID1, moduleID1, wt_y));
            tripletListY.push_back(T(moduleID2, moduleID2, wt_y));
            tripletListY.push_back(T(moduleID1, moduleID2, common2));
            tripletListY.push_back(T(moduleID2, moduleID1, common2));

            ycg_b(moduleID1) +=
                common2 * ((fp1.y - center1.y) - (fp2.y - center2.y));
            ycg_b(moduleID2) +=
                common2 * ((fp2.y - center2.y) - (fp1.y - center1.y));
          }
          // 1 is terminal, 2 is module
          else if(is_term1 && !is_term2) {
            tripletListY.push_back(T(moduleID2, moduleID2, wt_y));
            ycg_b(moduleID2) += wt_y * (fp1.y - (fp2.y - center2.y));
          }
          // 2 is terminal, 1 is module
          else if(!is_term1 && is_term2) {
            tripletListY.push_back(T(moduleID1, moduleID1, wt_y));
            ycg_b(moduleID1) += wt_y * (fp2.y - (fp1.y - center1.y));
          }
        }
      }
    }
  }

  eMatX.setFromTriplets(tripletListX.begin(), tripletListX.end());
  eMatY.setFromTriplets(tripletListY.begin(), tripletListY.end());
}

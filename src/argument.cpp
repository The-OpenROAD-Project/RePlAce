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
#include <cstring>
#include <ctime>

#include "replace_private.h"
#include "opt.h"
#include "routeOpt.h"


using std::string;
using std::cout;
using std::endl;

void initGlobalVars() {
  bmFlagCMD = "etc";       // string
  target_cell_den = PREC_MAX;

  gVerbose = 0;
  globalWns = 0.0f;
  globalTns = 0.0f;

  dim_bin.x = dim_bin.y = 32;
  UPPER_PCOF = 1.05;
  LOWER_PCOF = 0.95;
  refDeltaWL = 346000;
  INIT_LAMBDA_COF_GP = PREC_MIN;

  racntiCMD = "10";         // lutong
  maxinflCMD = "2.5";       // lutong
  inflcoefCMD = "2.33";     // lutong
  filleriterCMD = "0";

  auxCMD = "";
  outputCMD = "";
  experimentCMD = "";
  defMacroCnt = 0;

  isSkipPlacement = false;
  hasDensityDP = false;
  densityDP = 0.0f;
  routeMaxDensity = 0.99f;
  isBinSet = false;
  isSkipIP = false;

  isVerbose = false;
  isPlot = false;               // bool
  plotCellCMD = false;          // bool
  plotMacroCMD = false;         // bool
  plotDensityCMD = false;       // bool
  plotFieldCMD = false;         // bool
  constraintDrivenCMD = false;  // bool
  isRoutability = false;       // bool
  stnCMD = false;               // lutong
  lambda2CMD = false;           // bool
  dynamicStepCMD = false;       // bool
  isOnlyGlobalPlace = false;   // bool
  isSkipIP = false;             // bool
  isTiming = false;
  isDummyFill =  true;

  isInitSeed = false;
  plotColorFile = "";

  thermalAwarePlaceCMD = false;  // bool
  trialRunCMD = false;           // bool
  autoEvalRC_CMD = false;        // bool

  isOnlyLGinDP = (isRoutability) ? true : false;

  numThread = 1;  // default
  hasUnitNetWeight = false;
  hasCustomNetWeight = false;
  netWeight = 1.00;
  netWeightBase = 1.2f;
  netWeightBound = 1.8f;
  netWeightScale = 500.0f;
  netWeightApply = true;

  capPerMicron = PREC_MIN;
  resPerMicron = PREC_MIN;

  isClockGiven = false;
  timingClock = PREC_MIN;
  clockPinName = "";

  netCut = 1;
  timingUpdateIter = 10;

  globalRouterPosition = "../router/NCTUgr.ICCAD2012";
  globalRouterSetPosition = "../router/ICCAD12.NCTUgr.set";
  globalRouterCapRatio = 1.0;

  conges_eval_methodCMD =
      global_router_based;  // int (enum: defined in replace_private.h)
  onlyLG_CMD = (isRoutability) ? true : false;

  overflowMin = PREC_MAX;
}

void initGlobalVarsAfterParse() {
  // density & overflowMin settings
  target_cell_den = 
    (target_cell_den == PREC_MAX)? ((isRoutability) ? 0.9 : 1.0 ) : target_cell_den;
  target_cell_den_orig = target_cell_den;

  overflowMin = (overflowMin == PREC_MAX)? 
                       ((isRoutability) ? 0.17 : 0.1)
                       : overflowMin;
  
  INIT_LAMBDA_COF_GP = (INIT_LAMBDA_COF_GP == PREC_MIN)? 
    ((isRoutability)? 0.001 : 0.00008) : INIT_LAMBDA_COF_GP ;

  numInitPlaceIter = (isRoutability)? 1 : 30;

  /// !!!
  if( isFastMode ) {
    UPPER_PCOF = 1.2;
    overflowMin = 0.3;
    refDeltaWL = 1000000;
    numInitPlaceIter = 1;
    INIT_LAMBDA_COF_GP = 1; 
  } 

  DEN_ONLY_PRECON = false;

  numLayer = 1;

  ALPHAmGP = 1e-12;
  BETAmGP = 1e-13;

  ALPHAcGP = 1e-6;
  BETAcGP = 1e-8;

  dampParam = 0.999999;

  maxALPHA = 1E5;
  stn_weight = 0.1;  // lutong
  bloating_max_count = 1;
  routepath_maxdist = 0;

  edgeadj_coef = 1.19;
  pincnt_coef = 1.66;
  gRoute_pitch_scal = 1.09;

  max_inflation_ratio = atof(maxinflCMD.c_str());    // lutong
  inflation_ratio_coef = atof(inflcoefCMD.c_str());  // lutong
  NUM_ITER_FILLER_PLACE = atoi(filleriterCMD.c_str());
  inflation_max_cnt = atof(racntiCMD.c_str());  // lutong

  // newly set RealPath
  globalRouterPosition = GetRealPath( globalRouterPosition );
  globalRouterSetPosition = GetRealPath( globalRouterSetPosition );

  ExtraWSfor3D = 0;     //.12; //0.1;
  MaxExtraWSfor3D = 0;  //.20; //0.2;

  isOnlyGlobalPlace = true;
  isFirst_gp_opt = true;

  ///////////////////////////////////////////////////////
  // flg_3dic is always to be 1...
  flg_3dic = 1;
  flg_3dic_io = 0;
  ///////////////////////////////////////////////////////
}


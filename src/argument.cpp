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
#include "lefdefIO.h"
#include "routeOpt.h"


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
  defName = "";
  sdcName = "";
  verilogName = "";
  outputCMD = "";
  experimentCMD = "";
  verilogTopModule = "";
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

  detailPlacerFlag = "";  // mgwoo
  detailPlacerLocation = "";
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
  detailPlacerLocation = GetRealPath( detailPlacerLocation );

  // newline escape
  ReplaceStringInPlace(verilogName, "\r", "");
  ReplaceStringInPlace(verilogName, "\r", "");
  ReplaceStringInPlace(sdcName, "\r", "");
  ReplaceStringInPlace(sdcName, "\n", "");
  
  for(auto& libName: libStor) {
    ReplaceStringInPlace(libName, "\r", "");
    ReplaceStringInPlace(libName, "\n", "");
  } 


  ExtraWSfor3D = 0;     //.12; //0.1;
  MaxExtraWSfor3D = 0;  //.20; //0.2;

  // detailPlacer settings
  detailPlacer = NoneDp;
  if(!strcmp(detailPlacerFlag.c_str(), "NTU3")) {
    detailPlacer = NTUplace3;
  }
  else if(!strcmp(detailPlacerFlag.c_str(), "NTU4")) {
    detailPlacer = NTUplace4h;
  }
  else {
    printf(
        "\n** WARNING: Your Detail Placement Step must be skipped.\n"
        "              (i.e. this program will be executed as -onlyGP)\n"
        "     If you want to have DP after GP, please specify -dpflag and "
        "-dploc.\n\n");
    isOnlyGlobalPlace = true;
  }

  ///////////////////////////////////////////////////////
  // flg_3dic is always to be 1...
  flg_3dic = 1;
  flg_3dic_io = 0;
  ///////////////////////////////////////////////////////
}

void initArgument(int argc, char *argv[]) {
  initGlobalVars();
  // parse all argument here.
  if(argument(argc, argv) == false) {
    printUsage();
    exit(0);
  }

  if(criticalArgumentError() == true) {
    exit(0);
  }
  initGlobalVarsAfterParse();
}

bool argument(int argc, char *argv[]) {
  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-bmflag")) {
      i++;
      if(argv[i][0] != '-') {
        bmFlagCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires benchmark's flag.\n",
               argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-aux")) {
      i++;
      if(argv[i][0] != '-') {
        auxCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires *.aux.\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-lef")) {
      i++;
      if(argv[i][0] != '-') {
        lefStor.push_back(string(argv[i]));
      }
      else {
        printf("\n**ERROR: Option %s requires *.lef.\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-def")) {
      i++;
      if(argv[i][0] != '-') {
        defName = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires *.def.\n", argv[i - 1]);
        return false;
      }
    }
    // for SDC
    else if(!strcmp(argv[i], "-sdc")) {
      i++;
      if(argv[i][0] != '-') {
        sdcName = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires *.sdc.\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-verilog")) {
      i++;
      if(argv[i][0] != '-') {
        verilogName = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires *.v.\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-lib")) {
      i++;
      if(argv[i][0] != '-') {
        libStor.push_back(string(argv[i]));
      }
      else {
        printf("\n**ERROR: Option %s requires *.lib.\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-output")) {
      i++;
      if(argv[i][0] != '-') {
        outputCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires output's directory.\n",
               argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-experi")) {
      i++;
      if(argv[i][0] != '-') {
        experimentCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires output's directory.\n",
               argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-t")) {
      i++;
      if(argv[i][0] != '-') {
        numThread = atoi(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires thread number\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-den")) {
      i++;
      if(argv[i][0] != '-') {
        target_cell_den = atof(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires density (FLT).\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-routeMaxDensity")) {
      i++;
      if(argv[i][0] != '-') {
        routeMaxDensity = atof(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires density (FLT).\n", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-verbose")) {
      isVerbose = true;
    }
    // timing-related param; NetCut
    else if(!strcmp(argv[i], "-nc")) {
      i++;
      if(argv[i][0] != '-') {
        netCut = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    // timing-related param; NetWeight
    else if(!strcmp(argv[i], "-nw")) {
      i++;
      if(argv[i][0] != '-') {
        hasUnitNetWeight= true;
        netWeight = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    // timing-related param; NetWeightBase
    else if(!strcmp(argv[i], "-nwb")) {
      i++;
      if(argv[i][0] != '-') {
        netWeightBase = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    // timing-related param; NetWeightBoundary
    else if(!strcmp(argv[i], "-nwbd")) {
      i++;
      if(argv[i][0] != '-') {
        netWeightBound = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    // timing-related param; NetWeightScale
    else if(!strcmp(argv[i], "-nws")) {
      i++;
      if(argv[i][0] != '-') {
        netWeightScale = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    // timing-related param; Clock
    else if(!strcmp(argv[i], "-clock")) {
      i++;
      if(argv[i][0] != '-') {
        timingClock = atof(argv[i]);
        isClockGiven = true;
      }
      else {
        return false;
      }
    }
    // timing-related param;
    else if(!strcmp(argv[i], "-capPerMicron")) {
      i++;
      if(argv[i][0] != '-') {
        capPerMicron = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    // timing-related param;
    else if(!strcmp(argv[i], "-resPerMicron")) {
      i++;
      if(argv[i][0] != '-') {
        resPerMicron = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    // timing-related param; timingUpdateIter
    else if(!strcmp(argv[i], "-tIter")) {
      i++;
      if(argv[i][0] != '-') {
        timingUpdateIter = atoi(argv[i]);
      }
      else {
        return false;
      }
    }
    else if(!strcmp(argv[i], "-gr_cap_ratio")) {
      i++;
      if(argv[i][0] != '-') {
        globalRouterCapRatio = atof(argv[i]);
      }
      else {
        return false;
      }
    }
    else if(!strcmp(argv[i], "-gr_cap_ratio_file")) {
      i++;
      if(argv[i][0] != '-') {
        routeInst.FillLayerCapacityRatio( string(argv[i]) );
      }
      else {
        return false;
      }
    }
    else if(!strcmp(argv[i], "-gr_binary")) {
      i++;
      if(argv[i][0] != '-') {
        globalRouterPosition = argv[i];
      }
      else {
        return false;
      }
    }
    else if(!strcmp(argv[i], "-gr_set")) {
      i++;
      if(argv[i][0] != '-') {
        globalRouterSetPosition = argv[i];
      }
      else {
        return false;
      }
    }
    else if(!strcmp(argv[i], "-plotColor")) {
      i++;
      if(argv[i][0] != '-') {
        plotColorFile = argv[i]; 
      }
      else {
        return false;
      }
    }

    /*
    else if(!strcmp(argv[i], "-numLayer")) {
        i++;
        if(argv[i][0] != '-') {
            numLayerCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires the number of layers ",
                   argv[i - 1]);
            printf("(INT).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-gtsvcof")) {
        i++;
        if(argv[i][0] != '-') {
            gTSVcofCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires gTSV coefficient ",
                   argv[i - 1]);
            printf("(FLT).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-alphaM")) {
        i++;
        if(argv[i][0] != '-') {
            ALPHAmGP_CMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires ALPHAmGP coefficient ",
                   argv[i - 1]);
            printf("(FLT, e.g., 1E-12).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-alphaC")) {
        i++;
        if(argv[i][0] != '-') {
            ALPHAcGP_CMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires ALPHAcGP coefficient ",
                   argv[i - 1]);
            printf("(FLT, e.g., 1E-5).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-betaM")) {
        i++;
        if(argv[i][0] != '-') {
            BETAmGP_CMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires BETAmGP coefficient ",
                   argv[i - 1]);
            printf("(FLT, e.g., 1E-13).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-betaC")) {
        i++;
        if(argv[i][0] != '-') {
            BETAcGP_CMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires BETAcGP coefficient ",
                   argv[i - 1]);
            printf("(FLT, e.g., 1E-8).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-damp")) {
        i++;
        if(argv[i][0] != '-') {
            dampParam_CMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires dampParam coefficient ",
                   argv[i - 1]);
            printf("(FLT, e.g., 0.999999).\n");
            return false;
        }
    }
    */
    else if(!strcmp(argv[i], "-overflow")) {
      i++;
      if(argv[i][0] != '-') {
        overflowMin = atof(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires overflow tolerance ",
               argv[i - 1]);
        printf("(FLT<0.3).\n");
        return false;
      }
    }
    else if(!strcmp(argv[i], "-pcofmin")) {
      i++;
      if(argv[i][0] != '-') {
        LOWER_PCOF = atof(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires pCof_Min ", argv[i - 1]);
        printf("(FLT>0.9).\n");
        return false;
      }
    }
    else if(!strcmp(argv[i], "-pcofmax")) {
      i++;
      if(argv[i][0] != '-') {
        UPPER_PCOF = atof(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires pCof_Max", argv[i - 1]);
        printf("(FLT<1.1).\n");
        return false;
      }
    }
    else if(!strcmp(argv[i], "-initCoef")) {
      i++;
      if(argv[i][0] != '-') {
        INIT_LAMBDA_COF_GP = atof(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s init_Coef", argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-fast")) {
      isFastMode = true;
    }
    else if(!strcmp(argv[i], "-initSeed")) {
      isInitSeed = true;
    }
    /*
    else if(!strcmp(argv[i], "-stnweight")) {
        i++;
        if(argv[i][0] != '-') {
            stnweightCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires steiner_weight",
                   argv[i - 1]);
            printf("(FLT<1.1).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-hpitch")) {
        i++;
        if(argv[i][0] != '-') {
            hpitchCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires hpitch", argv[i - 1]);
            printf("(FLT<1.1).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-vpitch")) {
        i++;
        if(argv[i][0] != '-') {
            vpitchCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires vpitch", argv[i - 1]);
            printf("(FLT<1.1).\n");
            return false;
        }
    }
    */
    else if(!strcmp(argv[i], "-racnti")) {
      i++;
      if(argv[i][0] != '-') {
        racntiCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires racnti", argv[i - 1]);
        printf("(FLT<1.1).\n");
        return false;
      }
    }
    /*
    else if(!strcmp(argv[i], "-racnto")) {
        i++;
        if(argv[i][0] != '-') {
            racntoCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires racnto", argv[i - 1]);
            printf("(FLT<1.1).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-rpmaxdist")) {
        i++;
        if(argv[i][0] != '-') {
            routepath_maxdistCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires rpmaxdist", argv[i - 1]);
            printf("(INT>=0).\n");
            return false;
        }
    }
    */
    else if(!strcmp(argv[i], "-maxinfl")) {
      i++;
      if(argv[i][0] != '-') {
        maxinflCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires maxinfl", argv[i - 1]);
        printf("(FLT<1.1).\n");
        return false;
      }
    }
    else if(!strcmp(argv[i], "-inflcoef")) {
      i++;
      if(argv[i][0] != '-') {
        inflcoefCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires inflcoef", argv[i - 1]);
        printf("(FLT<1.1).\n");
        return false;
      }
    }
    /*
    else if(!strcmp(argv[i], "-edgeadjcoef")) {
        i++;
        if(argv[i][0] != '-') {
            edgeadjcoefCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires edgeadjcoef",
                   argv[i - 1]);
            printf("(FLT<1.1).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-pincntcoef")) {
        i++;
        if(argv[i][0] != '-') {
            pincntcoefCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires pincntcoef", argv[i - 1]);
            printf("(FLT<1.1).\n");
            return false;
        }
    }
    else if(!strcmp(argv[i], "-grps")) {
        i++;
        if(argv[i][0] != '-') {
            gRoute_pitch_scalCMD = argv[i];
        }
        else {
            printf("\n**ERROR: Option %s requires GR pitch scale",
                   argv[i - 1]);
            printf("(FLT<1.1).\n");
            return false;
        }
    }
    */
    else if(!strcmp(argv[i], "-filleriter")) {
      i++;
      if(argv[i][0] != '-') {
        filleriterCMD = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires filleriter", argv[i - 1]);
        printf("(FLT<1.1).\n");
        return false;
      }
    }
    else if(!strcmp(argv[i], "-stepScale")) {
      i++;
      if(argv[i][0] != '-') {
        refDeltaWL = atof(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires refDeltaWL", argv[i - 1]);
        printf("ref is 346000 (INT).\n");
        return false;
      }
    }
    // detailPlacer Settings : flag
    else if(!strcmp(argv[i], "-dpflag")) {
      i++;
      if(argv[i][0] != '-') {
        detailPlacerFlag = argv[i];
      }
      else {
        printf(
            "\n**ERROR: Option %s requires which Detailed Placer you want to "
            "use",
            argv[i - 1]);
        printf("currently support 3 detail placer : NTU3, and NTU4.\n");
        printf("You must specify Detail Placer between these two.\n");
        printf(
            "example : ./replace -input @@ -output @@ -dpflag NTU4 -dploc "
            "./ntuplacer4h\n");
        return false;
      }
    }
    // detailPlacer Settings : location
    else if(!strcmp(argv[i], "-dploc")) {
      i++;
      if(argv[i][0] != '-') {
        detailPlacerLocation = argv[i];
      }
      else {
        printf("\n**ERROR: Option %s requires your Detailed Placer's location ",
               argv[i - 1]);
        printf("currently support 2 detail placer : NTU3, and NTU4.\n");
        printf("You must specify Detail Placer between these two.\n");
        printf(
            "example : ./replace -input @@ -output @@ -dpflag NTU4 -dploc "
            "./ntuplacer4h\n");
        return false;
      }
    }
    // custom scaledown input parameter setting
    else if(!strcmp(argv[i], "-unitX")) {
      if(i + 1 >= argc || argv[i + 1][0] == '-') {
        printf(
            "\n**ERROR: Option %s requires your custom scale-down parameter; "
            "unitX\n",
            argv[i]);
        return false;
      }
      SetUnitX(atof(argv[++i]));
    }
    // custom scaledown input parameter setting; (like ASAP-N7 library)
    else if(!strcmp(argv[i], "-unitY")) {
      if(i + 1 >= argc || argv[i + 1][0] == '-') {
        printf(
            "\n**ERROR: Option %s requires your custom scale-down parameter; "
            "unitY\n",
            argv[i]);
        return false;
      }
      SetUnitY(atof(argv[++i]));
    }
    // custom scaledown input parameter setting; (like ASAP-N7 library)
    else if(!strcmp(argv[i], "-offsetX")) {
      if(i + 1 >= argc || argv[i + 1][0] == '-') {
        printf(
            "\n**ERROR: Option %s requires your custom offset parameter; "
            "offsetX\n",
            argv[i]);
        return false;
      }
      SetOffsetX(atof(argv[++i]));
    }
    // custom scaledown input parameter setting; (like ASAP-N7 library)
    else if(!strcmp(argv[i], "-offsetY")) {
      if(i + 1 >= argc || argv[i + 1][0] == '-') {
        printf(
            "\n**ERROR: Option %s requires your custom offset parameter; "
            "offsetY\n",
            argv[i]);
        return false;
      }
      SetOffsetY(atof(argv[++i]));
    }
    else if(!strcmp(argv[i], "-onlyDP")) {
      isSkipPlacement = true;
    }
    else if(!strcmp(argv[i], "-skipIP")) {
      isSkipIP = true;
    }
    // set target density manually for dp
    else if(!strcmp(argv[i], "-denDP")) {
      if(i + 1 >= argc || argv[i + 1][0] == '-') {
        printf("\n**ERROR: Option %s requires target density for DP\n",
               argv[i]);
        return false;
      }
      hasDensityDP = true;
      densityDP = atof(argv[++i]);
    }
    // detail placer with only Legalization modes
    else if(!strcmp(argv[i], "-onlyLG")) {
      isOnlyLGinDP = true;
      onlyLG_CMD = true;
    }
    // Dummy Cell Create For NtuPlacer3
    else if(!strcmp(argv[i], "-fragmentedRow")) {
      isDummyFill = true;
    }
    else if(!strcmp(argv[i], "-bin")) {
      i++;
      if(argv[i][0] != '-') {
        dim_bin.x = atoi(argv[i]);
        dim_bin.y = atoi(argv[i]);
        isBinSet = true;
      }
      else {
        printf("\n**ERROR: Option %s requires # bins in axis (INT).\n",
               argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-x")) {
      i++;
      if(argv[i][0] != '-') {
        dim_bin.x = atoi(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires the maximum X (INT).\n",
               argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-y")) {
      i++;
      if(argv[i][0] != '-') {
        dim_bin.y = atoi(argv[i]);
      }
      else {
        printf("\n**ERROR: Option %s requires the maximum Y (INT).\n",
               argv[i - 1]);
        return false;
      }
    }
    else if(!strcmp(argv[i], "-plot")) {
      isPlot = true;
      plotCellCMD = true;
      plotMacroCMD = true;
      plotDensityCMD = true;
      plotFieldCMD = true;
    }
    else if(!strcmp(argv[i], "-plotCell")) {
      plotCellCMD = true;
    }
    else if(!strcmp(argv[i], "-plotMacro")) {
      plotMacroCMD = true;
    }
    else if(!strcmp(argv[i], "-plotDensity")) {
      plotDensityCMD = true;
    }
    else if(!strcmp(argv[i], "-plotField")) {
      plotFieldCMD = true;
    }
    else if(!strcmp(argv[i], "-LD")) {
      constraintDrivenCMD = true;
      lambda2CMD = true;
    }
    else if(!strcmp(argv[i], "-routability")) {
      isRoutability = true;
    }
    else if(!strcmp(argv[i], "-DS")) {
      dynamicStepCMD = true;
      trialRunCMD = true;
    }
    else if(!strcmp(argv[i], "-timing")) {
      isTiming = true;
    }
    else if(!strcmp(argv[i], "-onlyGP")) {
      isOnlyGlobalPlace = true;
    }
    else if(!strcmp(argv[i], "-auto_eval_RC")) {
      autoEvalRC_CMD = true;
    }
    else {
      printf("\n**ERROR: Option %s is NOT available.\n", argv[i]);
      return false;
    }
  }
  return true;
}

void printCMD(int argc, char *argv[]) {
  printf("CMD :  ");
  for(int i = 0; i < argc; i++) {
    printf("%s ", argv[i]);
  }
  printf("\n");
}

void printUsage() {
  cout << endl;
  cout << "[Lef/Def/Verilog]" << endl;
  cout << " Usage: replace -bmflag etc -lef <*.lef> -def "
          "<*.def> [-verilog <*.v>] -output <outputLocation> -dpflag "
          "<NTU3/NTU4> -dploc <dpLocation> [Options]"
       << endl
       << endl;
  cout << "  -bmflag     : Specify which Benchmark is Used" << endl;
  cout << "  -lef        : *.lef Location" << endl;
  cout << "  -def        : *.def Location" << endl;
  cout << "  -verilog    : *.v Location (Required for Timing-Driven)" << endl << endl;
  cout << "[Options]" << endl;
  cout << " Placement " << endl;
  cout << "  -onlyGP     : Only Global Placement mode" << endl;
  cout << "  -den        : Target Density, Floating number, Default = 1 "
          "[0.00,1.00]"
       << endl;
  cout << "  -bin        : #bins (in power of 2) for x, y, z Directions"
       << endl;
  cout << "              : Unsigned Integer[3], Default = 32 32 32" << endl;
  cout << "  -overflow   : Overflow Termination Condition, Floating Number, "
          "Default = 0.1 [0.00, 1.00]"
       << endl;
  cout << "  -pcofmin    : µ_k Lower Bound, Float Number, Default = 0.95"
       << endl;
  cout << "  -pcofmax    : µ_k Upper Bound, Float Number, Default = 1.05"
       << endl;
  cout << "  -rancti     : Max #Global Router Calls and Cell Inflation" << endl;
  cout << "                No Restore of Cell Size, Keep Increasing, Unsigned "
          "Integer, Default = 10"
       << endl;
  cout << "  -maxinfl    : Max Cell Inflation for Each Cell Inflation Per "
          "Global Router Call"
       << endl;
  cout << "              : Floating Number, Default = 2.5" << endl;
  cout << "  -inflcoef   : γ_super, Floating Number, Default = 2.33" << endl;
  cout << "  -filleriter : #Filler Only Placement Iterations, Floating Number, "
          "Default = 20"
       << endl;
  cout << "  -stepScale  : ∆HPWL_REF, Floating Number, Default=346000" << endl
       << endl;
  cout << " Plot" << endl;
  cout << "  -plot       : Plot Layout Every 10 Iterations" << endl << endl;
  cout << " Detail Placer" << endl;
  cout << "  -dpflag     : Specify which Detailed Placer is Used" << endl;
  cout << "  -dploc      : Specify the Location of Detailed Placer" << endl;
  cout << "  -onlyLG     : Call Detailed Placement in Legalization Mode" << endl
       << endl;
  cout << " Router" << endl;
  cout << "  -routabiliy : Enable Routability Flow" << endl << endl;
}

bool criticalArgumentError() {
  // mgwoo
  if(auxCMD == "" && lefStor.size() == 0 && defName == "") {
    printf(
        "\n** ERROR: lef/def pair or aux files are needed, use (-lef/-def) or "
        "(-aux) options.\n");
    printUsage();
    return true;
  }

  // mgwoo
  if(auxCMD == "" && !(lefStor.size() != 0 && defName != "")) {
    printf("\n** ERROR: Both of lef/def files are needed.\n");
    printUsage();
    return true;
  }

  // mgwoo
  if(outputCMD == "") {
    printf(
        "\n** ERROR: Missing output result directory, use -output option.\n");
    printUsage();
    return true;
  }

  if(isTiming) {
    if(libStor.size() == 0) {
      printf(
          "\n** ERROR: Timing mode must contain at least one liberty file.\n");
      printUsage();
      return true;
    }
    if(verilogName == "") {
      printf("\n** ERROR: Timing mode must contain verilog.\n");
      printUsage();
      return true;
    }

    if(sdcName == "" && !isClockGiven) {
      printf("\n** ERROR: Timing mode must contain sdc file.\n");
      printUsage();
      return true;
    }

    if(isClockGiven) {
      if(clockPinName == "") {
        printf(
            "\n** ERROR: If the clock is given, argv must contain "
            "clockPinName\n");
        printUsage();
        return true;
      }
      if(timingClock == PREC_MIN) {
        printf(
            "\n** ERROR: If the clock is given, argv must contain "
            "clockPeriod\n");
        printUsage();
        return true;
      }
    }

    if(capPerMicron == PREC_MIN || resPerMicron == PREC_MIN) {
      printf(
          "\n** ERROR: Timing mode must specify Resistor and Cap per "
          "micron.\n");
      printUsage();
      return true;
    }
  }

  if(constraintDrivenCMD == false && lambda2CMD == true) {
    printf("\n** ERROR: condtDriven should be enabled with diffLambda.\n");
    return true;
  }
  return false;
}


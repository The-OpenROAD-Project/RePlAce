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

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "bookShelfIO.h"
#include "lefdefIO.h"
#include "bin.h"
#include "charge.h"
#include "fft.h"
#include "replace_private.h"
#include "initPlacement.h"
#include "macro.h"
#include "opt.h"
#include "plot.h"
#include "wlen.h"
#include "timing.h"

#include <tcl.h>

#define compileDate __DATE__
#define compileTime __TIME__

prec globalWns;
prec globalTns;

prec netCut;
bool hasUnitNetWeight;
bool hasCustomNetWeight;
prec netWeight;
prec netWeightMin;
prec netWeightMax;

prec netWeightBase;
prec netWeightBound;
prec netWeightScale;
bool netWeightApply;

prec capPerMicron;
prec resPerMicron;

bool isClockGiven;
prec timingClock;
string clockPinName;

bool isInitSeed;
string plotColorFile;

int timingUpdateIter;
PIN *pinInstance;
MODULE *moduleInstance;
int pinCNT;
int moduleCNT;

string globalRouterPosition;
string globalRouterSetPosition;
prec globalRouterCapRatio;

// for moduleInst's pinName
vector< vector< string > > mPinName;

// for termInst's pinName
vector< vector< string > > tPinName;

vector<string> moduleNameStor;
vector<string> terminalNameStor;
vector<string> netNameStor;
vector<string> cellNameStor;

TERM *terminalInstance;
NET *netInstance;
HASH_MAP< string, int > netNameMap;
int terminalCNT;
int netCNT;

int gcell_cnt;
int row_cnt;
int place_st_cnt;

int gVerbose;

int STAGE;
int placementStdcellCNT;
int gfiller_cnt;
int placementMacroCNT;
int msh_yz;
int INPUT_FLG;
int gmov_mac_cnt;
int cGP3D_buf_iter;
int cGP2D_buf_iter;
int mGP3D_tot_iter;
int mGP2D_tot_iter;
int cGP3D_tot_iter;
int cGP2D_tot_iter;
int flg_3dic;
int flg_3dic_io;
int trial_iterCNT = 0;
int mGP3D_iterCNT = 0;
int mGP2D_iterCNT = 0;
int cGP3D_iterCNT = 0;
int cGP2D_iterCNT = 0;
int bloating_max_count = 1;
int bloatCNT = 0;
int potnPhaseDS;

// routability
prec h_pitch = 0.80;
prec v_pitch = 0.72;
prec max_inflation_ratio = 2.5;
prec inflation_ratio_coef = 1.0;
prec edgeadj_coef = 1.0;
prec pincnt_coef = 1.0;
prec gRoute_pitch_scal = 1.0;
prec ignoreEdgeRatio = 0.8;
prec inflation_threshold;
prec total_inflation_ratio;
prec h_max_inflation_ratio;
prec v_max_inflation_ratio;
prec total_inflatedNewAreaDelta;
bool isRoutabilityInit = false;
bool flg_noroute = false;
int routepath_maxdist;
int inflation_cnt = 0;
int inflation_max_cnt = 4;
prec inflation_area_over_whitespace = 0.25;
prec curr_filler_area = 0;
prec adjust_ratio = 0;
prec currTotalInflation;
bool is_inflation_h = false;
vector< pair< int, prec > > inflationList;

bool isTrial = false;
bool isFirst_gp_opt = true;
bool DEN_ONLY_PRECON;

int orderHPWL;
vector< pair< prec, prec > > trial_HPWLs;
vector< prec > trial_POTNs;
vector< pair< prec, prec > > hpwlEPs_1stOrder;
vector< pair< prec, prec > > hpwlEPs_2ndOrder;
vector< prec > potnEPs;

// .route
std::map< string, vector< int > > routeBlockageNodes;
int nXgrids, nYgrids, nMetLayers;
vector< int > verticalCapacity;
vector< int > horizontalCapacity;
vector< prec > minWireWidth;
vector< prec > minWireSpacing;
vector< prec > viaSpacing;
vector< tuple< int, int, int, int, int, int, int > > edgeCapAdj;
prec gridLLx, gridLLy;
prec tileWidth, tileHeight;
prec blockagePorosity;

prec gtsv_cof;
prec ALPHAmGP;
prec ALPHAcGP;
prec ALPHA;
prec BETAmGP;
prec BETAcGP;
prec BETA;
prec dampParam;
prec stn_weight;  // lutong
prec maxALPHA;
prec ExtraWSfor3D;
prec MaxExtraWSfor3D;
prec rowHeight;
prec SITE_SPA;

prec layout_area;
prec total_std_area;
prec total_std_den;
prec total_modu_area;
prec inv_total_modu_area;
prec total_cell_area;
prec curr_cell_area;  // lutong
prec total_term_area;
prec total_move_available_area;
prec total_filler_area;
prec total_PL_area;
prec total_termPL_area;  // mgwoo
prec total_WS_area;      // mgwoo
prec curr_WS_area;       // lutong
prec filler_area;
prec target_cell_den;
prec target_cell_den_orig;  // lutong
prec total_macro_area;
prec grad_stp;
prec gsum_phi;
prec gsum_ovfl;
prec gsum_ovf_area;
prec overflowMin;
prec mGP3D_opt_phi_cof;
prec mGP2D_opt_phi_cof;
prec cGP3D_opt_phi_cof;
prec cGP2D_opt_phi_cof;
prec inv_RAND_MAX;

unsigned extPt1_2ndOrder;
unsigned extPt2_2ndOrder;
unsigned extPt1_1stOrder;
unsigned extPt2_1stOrder;
unsigned extPt3_1stOrder;

char defOutput[BUF_SZ];
char defGpOutput[BUF_SZ];

char gbch_dir[BUF_SZ];
char gbch_aux[BUF_SZ];
char gbch[BUF_SZ];
char gGP_pl[BUF_SZ];
char gIP_pl[BUF_SZ];
char gGP_pl_file[BUF_SZ];
char gmGP2D_pl[BUF_SZ];
char gGP3_pl[BUF_SZ];
char gLG_pl[BUF_SZ];
char gDP_log[BUF_SZ];
char gDP_pl[BUF_SZ];
char gDP_tmp[BUF_SZ];
char gDP2_pl[BUF_SZ];
char gDP3_pl[BUF_SZ];
char gGR_dir[BUF_SZ];
char gGR_log[BUF_SZ];
char gGR_tmp[BUF_SZ];
char gFinal_DP_pl[BUF_SZ];
char bench_aux[BUF_SZ];
char dir_bnd[BUF_SZ];
char global_router[1023];
char output_dir[BUF_SZ];
char currentDir[BUF_SZ];

string sourceCodeDir;

// opt.cpp -> main.cpp
CELL *gcell_st;
ROW *row_st;

PLACE *place_st;
PLACE *place_backup_st;
PLACE place;
PLACE place_backup;
FPOS term_pmax;
FPOS term_pmin;
FPOS filler_size;
POS n1p;
POS msh;
FPOS gmin;
FPOS gmax;
TIER *tier_st;
POS dim_bin;
POS dim_bin_mGP2D;

// .shapes
// SHAPE's storage
vector< SHAPE > shapeStor;

// nodes String -> shapeStor's index.
HASH_MAP< string, vector< int > > shapeMap;
POS dim_bin_cGP2D;

///  ARGUMENTS  ///////////////////////////////////////////
int numLayer;
prec aspectRatio;
string bmFlagCMD;
string auxCMD;             // mgwoo
string defName;             // mgwoo
string sdcName;             // mgwoo
string verilogName;         // mgwoo
vector< string > libStor;  // mgwoo
string outputCMD;          // mgwoo
string experimentCMD;      // mgwoo
vector< string > lefStor;  // mgwoo
string verilogTopModule;
int defMacroCnt;
int numInitPlaceIter;
prec refDeltaWL;

int numThread;
InputMode inputMode;

string benchName;

string dimCMD;
string gTSVcofCMD;
string ALPHAmGP_CMD;
string ALPHAcGP_CMD;
string BETAmGP_CMD;
string BETAcGP_CMD;
string dampParam_CMD;
string aspectRatioCMD;
string stnweightCMD;    // lutong
string hpitchCMD;       // lutong
string vpitchCMD;       // lutong
string racntiCMD;       // lutong
string racntoCMD;       // lutong
string maxinflCMD;      // lutong
string inflcoefCMD;     // lutong
string edgeadjcoefCMD;  // lutong
string pincntcoefCMD;   // lutong
string routepath_maxdistCMD;
string gRoute_pitch_scalCMD;
string filleriterCMD;

// for detail Placer
int detailPlacer;
string detailPlacerLocation;
string detailPlacerFlag;

prec densityDP;
prec routeMaxDensity; 
bool hasDensityDP;
bool isSkipPlacement;
bool isOnlyLGinDP;
bool isPlot;
bool isSkipIP;
bool isBinSet;
bool isDummyFill;

int conges_eval_methodCMD;
bool isVerbose;
bool plotCellCMD;
bool plotMacroCMD;
bool plotDensityCMD;
bool plotFieldCMD;
bool constraintDrivenCMD;
bool isRoutability;
bool stnCMD;  // lutong
bool lambda2CMD;
bool dynamicStepCMD;
bool isOnlyGlobalPlace;
bool isTiming;
bool thermalAwarePlaceCMD;
bool trialRunCMD;
bool autoEvalRC_CMD;
bool onlyLG_CMD;
bool isFastMode;
///////////////////////////////////////////////////////////

Tcl_Interp* _interp;

extern "C" {
extern int Replace_Init(Tcl_Interp *interp);
}


int 
replaceTclAppInit(Tcl_Interp *interp) {

  _interp = interp;

  if (Tcl_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  if( Replace_Init(interp) == TCL_ERROR) {
    return TCL_ERROR; 
  }
  
  string command = "";

  command = "";
  command += "puts \"RePlAce Version: 1.0.0\"";
  Tcl_Eval(interp, command.c_str());

//  command = "";
//  command += "if {$tcl_interactive} {\n";
//  command += "package require tclreadline\n";
//  command += "proc ::tclreadline::prompt1 {} {\n";
//  command += " return \"replace-[lindex [split [pwd] \"/\"] end] % \"\n";
//  command += "}\n";
//  command += "::tclreadline::Loop\n";
//  command += "}";
  
  // register tclreadline 
//  Tcl_Eval(interp, command.c_str());
  
  return TCL_OK;
}

int main(int argc, char *argv[]) {
 
  Tcl_Main(1, argv, replaceTclAppInit);

  double tot_cpu = 0;
  double time_ip = 0;
  double time_tp = 0;
  double time_mGP2D = 0;
  double time_mGP = 0;
  double time_lg = 0;
  double time_cGP2D = 0;
  double time_cGP = 0;
  double time_dp = 0;
  
  time_t rawtime;
  struct tm *timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  total_hpwl.SetZero();

  printCMD(argc, argv);
  cout << endl;
  PrintInfoString("CompileDate", compileDate);
  PrintInfoString("CompileTime", compileTime);
  PrintInfoString("StartingTime", asctime(timeinfo));

  ///////////////////////////////////////////////////////////////////////
  ///// Parse Arguments (defined in argument.cpp) ///////////////////////
  initArgument(argc, argv);
  ///////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////
  ///// Placement Initialization  ///////////////////////////////////////

  init();
  PrintProcBegin("Importing Placement Input");
  ParseInput();

//  if(numLayer > 1)
//    calcTSVweight();

  net_update_init();
  init_tier();
  PrintProcEnd("Importing Placement Input");
  ///////////////////////////////////////////////////////////////////////

  time_start(&tot_cpu);
  // Normal cases
  if(!isSkipPlacement) {
    ///////////////////////////////////////////////////////////////////////
    ///// IP:  INITIAL PLACEMENT //////////////////////////////////////////
    PrintProcBegin("Initial Placement");
    time_start(&time_ip);
    build_data_struct(!isInitSeed);
    initialPlacement_main();
    time_end(&time_ip);
    PrintProcEnd("Initial Placement");
    ///////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////
    ///// Setup before Placement Optimization  ////////////////////////////
    setup_before_opt();
    ///////////////////////////////////////////////////////////////////////

    if(trialRunCMD == true) {
      isTrial = true;
      ///////////////////////////////////////////////////////////////////////
      ///// PP:  Pre-Placement (Trial Run to Catch Parameters) //////////////
      time_start(&time_tp);
      trial_main();
      time_end(&time_tp);
      ///////////////////////////////////////////////
      free_trial_mallocs();
      ParseInput();
      build_data_struct();
      net_update_init();
      init_tier();
      initialPlacement_main();
      setup_before_opt();
      ///////////////////////////////////////////////////////////////////////
      isTrial = false;
    }

    time_start(&time_mGP);
    /*
    if(numLayer > 1 && placementMacroCNT > 0) {
      ///////////////////////////////////////////////////////////////////////
      ///// mGP3D:  MIXED_SIZE_3D_GLOBAL_PLACE //////////////////////////////
      printf("PROC:  BEGIN GLOBAL 3D PLACEMENT\n");
      printf("PROC:  Mixed-Size 3D Global Placement (mGP3D)\n");
      time_start(&time_mGP3D);
      mGP3DglobalPlacement_main();
      time_end(&time_mGP3D);
      printf("   RUNTIME(s) : %.4f\n\n\n", time_mGP3D);
      printf("PROC:  END GLOBAL 3D PLACEMENT\n\n\n");
      ///////////////////////////////////////////////////////////////////////
    }*/

    if(placementMacroCNT > 0) {
      ///////////////////////////////////////////////////////////////////////
      ///// mGP2D:  MIXED_SIZE_2D_GLOBAL_PLACE //////////////////////////////
    
      PrintProc("Begin Mixed-Size Global Placement ...");
      time_start(&time_mGP2D);
      mGP2DglobalPlacement_main();
      time_end(&time_mGP2D);

      printf("   RUNTIME(s) : %.4f\n\n\n", time_mGP2D);
      PrintProc("End Mixed-Size Global Placement");

      WriteDef(defGpOutput);
      WriteDef(defOutput);

      // no need to run any other flow with MS-RePlAce
      return 0;
      ///////////////////////////////////////////////////////////////////////
    }
    time_end(&time_mGP);

    // if(placementMacroCNT > 0 /*&& INPUT_FLG != ISPD*/) {
    //   ///////////////////////////////////////////////////////////////////////
    //   ///// mLG:  MACRO_LEGALIZATION ////////////////////////////////////////
    //   cout << "PROC:  BEGIN MACRO LEGALIZATION IN EACH TIER" << endl;
    //   cout << "PROC:  SA-based Macro Legalization (mLG)" << endl;
    //   time_start(&time_lg);
    //   bin_init();
    //   macroLegalization_main();
    //   time_end(&time_lg);
    //   printf("   RUNTIME(s) : %.4f\n\n\n", time_lg);
    //   cout << "PROC:  END MACRO LEGALIZATION (mLG)" << endl << endl;
    //   ///////////////////////////////////////////////////////////////////////
    // }


    time_start(&time_cGP);
    /*
    if(numLayer > 1) {
      ///////////////////////////////////////////////////////////////////////
      ///// cGP3D:  STDCELL_ONLY_3D_GLOBAL_PLACE
      /////////////////////////////////
      printf("PROC:  Standard Cell 3D Global Placement (cGP3D)\n");
      time_start(&time_cGP3D);
      cGP3DglobalPlacement_main();
      time_end(&time_cGP3D);
      printf("   RUNTIME(s) : %.4f\n\n\n", time_cGP3D);
      printf("PROC:  END GLOBAL 3D PLACEMENT\n\n\n");
      ///////////////////////////////////////////////////////////////////////
    }*/


    ///////////////////////////////////////////////////////////////////////
    ///// cGP2D:  STDCELL_ONLY_2D_GLOBAL_PLACE //////////////////////////////
    printf("PROC:  Standard Cell 2D Global Placement (cGP2D)\n");
    fflush(stdout);
    time_start(&time_cGP2D);
    cGP2DglobalPlacement_main();
    time_end(&time_cGP2D);
    time_end(&time_cGP);
    printf("   RUNTIME(s) : %.4f\n\n\n", time_cGP2D);
    fflush(stdout);
    printf("PROC:  END GLOBAL 2D PLACEMENT\n\n");
    fflush(stdout);
    ///////////////////////////////////////////////////////////////////////

    //    SavePlot( "Final GP Result");
    //    ShowPlot( benchName );
  }
  else {
    routeInst.Init();
    build_data_struct(false);
    tier_assign(STDCELLonly);
    setup_before_opt();
  }
  
  PrintUnscaledHpwl("GlobalPlacer");

  // Write BookShelf format
  WriteBookshelf();

  if(isPlot) {
    SaveCellPlotAsJPEG("Final Global Placement Result", false,
                       string(dir_bnd) + string("/globalPlaceResult"));
  }

  if(inputMode == InputMode::lefdef) {
    WriteDef(defGpOutput);
  }

  ///////////////////////////////////////////////////////////////////////
  ///// GP:  DETAILED PLACEMENT /////////////////////////////////////////
  if(isOnlyGlobalPlace) {
    time_start(&time_dp);
    time_end(&time_dp);
  }
  else {
    time_start(&time_dp);
    STAGE = DETAIL_PLACE;

    CallDetailPlace();
    time_end(&time_dp);
  }
  time_end(&tot_cpu);
  ///////////////////////////////////////////////////////////////////////

  UpdateNetMinMaxPin2();

  output_final_pl(gDP3_pl);
  if(inputMode == InputMode::lefdef) {
    WriteDef(defOutput);
  }

  printf(" ### Numbers of Iterations: \n");
  if(trialRunCMD)
    printf("     Trial:  %d (%.4lf sec/iter)\n", trial_iterCNT,
           time_tp / (prec)trial_iterCNT);
  if(mGP2D_iterCNT != 0)
    printf("     mGP2D:  %d (%.4lf sec/iter)\n", mGP2D_iterCNT,
           time_mGP / ((prec)(mGP2D_iterCNT)));
  printf("     cGP2D:  %d (%.4lf sec/iter)\n", cGP2D_iterCNT,
         time_cGP / ((prec)(cGP3D_iterCNT + cGP2D_iterCNT)));
  printf("     ____________________________________\n");
  printf("     TOTAL:  %d\n\n", trial_iterCNT + mGP2D_iterCNT +
                                    cGP2D_iterCNT);

  printf(
      " ### CPU_{IP, TP, mGP, LG, cGP, DP} is %.2lf, %.2lf, %.2lf, %.2lf, "
      "%.2lf, %.2lf.\n",
      time_ip, time_tp, time_mGP, time_lg, time_cGP, time_dp);
  printf(" ### CPU_TOT is %.2lf seconds (%.2lf min).\n\n", tot_cpu,
         tot_cpu / 60);
  fflush(stdout);
  
  PrintUnscaledHpwl("Final");

  //    SavePlot( "Final Placement Result");
  if(isPlot) {
    SaveCellPlotAsJPEG("Final Placement Result", false,
                       string(dir_bnd) + string("/finalResult"));
  }

  // DP Spef Generation
  if(inputMode == InputMode::lefdef && isTiming) {
    // for final DP SPEF generating
    Timing::Timing TimingInst(moduleInstance, terminalInstance, netInstance,
                              netCNT, pinInstance, pinCNT, mPinName, tPinName,
                              clockPinName, timingClock);

    TimingInst.BuildSteiner(true);

    string spefName = string(dir_bnd) + "/" + gbch + "_dp.spef";
    TimingInst.WriteSpef(spefName);
  }

  //    ShowPlot( benchName );
  return 0;
}

// mgwoo
void init() {
  char str_lg[BUF_SZ] = {
      0,
  };
  char str_dp[BUF_SZ] = {
      0,
  };
  char str_dp2[BUF_SZ] = {
      0,
  };
  char str_dp3[BUF_SZ] = {
      0,
  };
  char result_file[BUF_SZ] = {
      0,
  };
  int ver_num = 0;

  inv_RAND_MAX = (prec)1.0 / RAND_MAX;

  sprintf(global_router, "NCTUgr.ICCAD2012");

  switch(detailPlacer) {
    case FastPlace:
      sprintf(str_lg, "%s", "_eplace_lg");
      sprintf(str_dp, "%s", "_FP_dp");
      break;

    case NTUplace3:
      sprintf(str_lg, "%s", "_eplace_lg");
      sprintf(str_dp, "%s", ".eplace-gp.ntup");
      sprintf(str_dp2, "%s", ".eplace-cGP3D.ntup");
      break;

    case NTUplace4h:
      sprintf(str_lg, "%s", "_eplace_lg");
      sprintf(str_dp, "%s", ".eplace-gp.ntup");
      sprintf(str_dp2, "%s", ".eplace-cGP3D.ntup");
      break;
  }

  sprintf(str_dp3, ".%s.%s", bmFlagCMD.c_str(), "eplace");

  INPUT_FLG = ETC;

  global_macro_area_scale = target_cell_den;
  PrintInfoPrec("TargetDensity", target_cell_den, 0);

  wcof_flg = /* 1 */ 2 /* 3 */;

  // see ePlace-MS 
  // 8 * binSize.
  //
  // see: wlen.cpp: wcof_init function also
  //
  switch(WLEN_MODEL) {
    case LSE:
      // 10 !!!
      wcof00.x = wcof00.y = 0.1;
      break;

    case WA:
      if(INPUT_FLG == ISPD05 || INPUT_FLG == ISPD06 || INPUT_FLG == ISPD ||
         INPUT_FLG == MMS || INPUT_FLG == SB || INPUT_FLG == ETC) {

        // 8 !!!
        wcof00.x = wcof00.y = 0.125;
      }
      else if(INPUT_FLG == IBM) {
        // 2 !!!
        wcof00.x = wcof00.y = 0.50;
      }
      break;
  }

  SetMAX_EXP_wlen();

  // benchMark directory settings
  getcwd(currentDir, BUF_SZ);
  sourceCodeDir = getexepath();

  //
  //
  // benchName(== gbch) : benchmark name
  // gbch_dir : benchmark directory
  //
  //

  string fileCMD = (auxCMD != "") ? auxCMD : defName;

  //
  // check '/' from back side
  //
  int slashPos = fileCMD.rfind('/');

  //
  // extract benchName ~~~~/XXXX.aux
  //                        <------>
  benchName =
      (slashPos == -1)
          ? fileCMD
          : fileCMD.substr(slashPos + 1, fileCMD.length() - slashPos - 1);
  sprintf(gbch_dir, "%s",
          (slashPos == -1) ? currentDir : fileCMD.substr(0, slashPos).c_str());

  // if benchName has [.aux] extension
  // remove [.aux]
  int dotPos = benchName.rfind(".");
  if(benchName.length() - dotPos == 4) {
    benchName = benchName.substr(0, dotPos);
  }

  sprintf(gbch, "%s", benchName.c_str());
  sprintf(output_dir, "%s/%s/%s", outputCMD.c_str(), bmFlagCMD.c_str(),
          benchName.c_str());

  // generate folder if output folders not exist.
  struct stat fileStat;
  if(stat(output_dir, &fileStat) < 0) {
    string mkdir_cmd = "mkdir -p " + string(output_dir);
    system(mkdir_cmd.c_str());
  }

  // experiment folder could be given by user.
  // original settings
  if(experimentCMD == "") {
    // 'experimentXX' -1 checker.
    for(ver_num = 0;; ver_num++) {
      sprintf(dir_bnd, "%s/experiment%03d", output_dir, ver_num);

      // until not exists.
      struct stat fileStat;
      if(stat(dir_bnd, &fileStat) < 0) {
        break;
      }
    }
  }
  // follow user settings
  else {
    sprintf(dir_bnd, "%s/%s", output_dir, experimentCMD.c_str());
  }

  // output 'experimentXX' directory generator.
  string mkdir_cmd = "mkdir -p " + string(dir_bnd);
  system(mkdir_cmd.c_str());

  // PL file writer info update
  sprintf(gIP_pl, "%s/%s.eplace-ip.pl", dir_bnd, gbch);
  sprintf(gGP_pl, "%s/%s.eplace-gp.pl", dir_bnd, gbch);
  sprintf(gmGP2D_pl, "%s/%s.eplace-mGP2D.pl", dir_bnd, gbch);
  sprintf(gGP3_pl, "%s/%s.eplace-cGP2D.pl", dir_bnd, gbch);
  sprintf(gGP_pl_file, "%s.eplace-gp.pl", gbch);
  sprintf(gLG_pl, "%s/%s%s.pl", dir_bnd, gbch, str_lg);
  sprintf(gDP_pl, "%s/%s%s.pl", dir_bnd, gbch, str_dp);
  sprintf(gDP2_pl, "%s/%s%s.pl", dir_bnd, gbch, str_dp2);
  sprintf(gDP3_pl, "%s/%s%s.pl", dir_bnd, gbch, str_dp3);
  sprintf(result_file, "%s/res", dir_bnd);

  sprintf(defGpOutput, "%s/%s_gp.def", dir_bnd, gbch);
  sprintf(defOutput, "%s/%s_final.def", dir_bnd, gbch);

  PrintInfoInt("ExperimentIndex", ver_num);
  PrintInfoString("DirectoryPath", dir_bnd);

  sprintf(bench_aux, "%s/%s.aux", gbch_dir, gbch);
  sprintf(gbch_aux, "%s.aux", gbch);

  strcpy(gDP_log, gbch);
  strcat(gDP_log, "_DP.log");

  // mgwoo
  mkdirPlot();
}

void calcTSVweight() {
  TSV_WEIGHT = TSV_CAP * ((prec)numLayer) /
               (ROW_CAP * (prec)(tier_st[0].row_cnt)) * gtsv_cof;
  if(INPUT_FLG == IBM)
    TSV_WEIGHT = 0.73 * gtsv_cof;
  printf("INFO:  TSV_WEIGHT = %.6E\n", TSV_WEIGHT);
}

void initialPlacement_main() {
  STAGE = INITIAL_PLACE;
  initial_placement();
  UpdateNetAndGetHpwl();

  PrintUnscaledHpwl("Initial Placement");
  place_backup = place;
}

void tmGP3DglobalPlacement_main() {
  STAGE = mGP3D;
  // ALPHAmGP = initialALPHA;
  gp_opt();
  UpdateNetAndGetHpwl();
  printf("RESULT:\n");
  printf("   HPWL(tGP3D): %.4f (%.4f, %.4f)\n\n",
         total_hpwl.x + total_hpwl.y, total_hpwl.x, total_hpwl.y);
  fflush(stdout);
}

void tmGP2DglobalPlacement_main() {
  STAGE = mGP2D;
  // ALPHA = initialALPHA;
  tier_assign(MIXED);
  calc_tier_WS();
  setup_before_opt_mGP2D();
  gp_opt();
  post_mGP2D_delete();
  fflush(stdout);
  UpdateNetAndGetHpwl();
  printf("RESULT:\n");
  printf("   HPWL(tGP2D): %.4f (%.4f, %.4f)\n\n", total_hpwl.x + total_hpwl.y,
         total_hpwl.x, total_hpwl.y);
  fflush(stdout);
}

void mGP3DglobalPlacement_main() {
  STAGE = mGP3D;
  // ALPHA = initialALPHA;
  gp_opt();
  isFirst_gp_opt = false;
  UpdateNetAndGetHpwl();
  if(dynamicStepCMD) {
    reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y);
  }
  printf("RESULT:\n");
  printf("   HPWL(mGP3D): %.4f (%.4f, %.4f)\n\n",
         total_hpwl.x + total_hpwl.y, total_hpwl.x, total_hpwl.y);
  fflush(stdout);
}

void mGP2DglobalPlacement_main() {
  STAGE = mGP2D;
  // ALPHA = initialALPHA;
  tier_assign(MIXED);
  calc_tier_WS();
  setup_before_opt_mGP2D();
  gp_opt();
  isFirst_gp_opt = false;
  /*if (INPUT_FLG != ISPD)*/ post_mGP2D_delete();
  fflush(stdout);
  UpdateNetAndGetHpwl();
  if(dynamicStepCMD) {
    reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y);
  }
  auto hpwl = GetUnscaledHpwl(); 
  PrintInfoPrec("Nesterov: HPWL", hpwl.first + hpwl.second, 1);
}

void tcGP3DglobalPlacement_main() {
  STAGE = cGP3D;
  // ALPHA = initialALPHA;
  UpdateGcellCoordiFromModule();  // cell_macro_copy ();
  gp_opt();
  UpdateNetAndGetHpwl();
  printf("RESULT:\n");
  printf("   HPWL(tGP3D): %.4f (%.4f, %.4f)\n\n",
         total_hpwl.x + total_hpwl.y, total_hpwl.x, total_hpwl.y);
  fflush(stdout);
}

void cGP3DglobalPlacement_main() {
  STAGE = cGP3D;
  // ALPHA = initialALPHA;
  UpdateGcellCoordiFromModule();  // cell_macro_copy ();
  gp_opt();
  isFirst_gp_opt = false;
  UpdateNetAndGetHpwl();
  if(dynamicStepCMD) {
    reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y);
  }
  printf("RESULT:\n");
  printf("   HPWL(cGP3D): %.4f (%.4f, %.4f)\n\n",
         total_hpwl.x + total_hpwl.y, total_hpwl.x, total_hpwl.y);
  fflush(stdout);
}

void tcGP2DglobalPlacement_main() {
  STAGE = cGP2D;
  // ALPHA = initialALPHA;
  tier_assign(STDCELLonly);
  setup_before_opt_cGP2D();
  gp_opt();
  UpdateNetAndGetHpwl();
  printf("RESULT:\n");
  printf("   HPWL(tGP2D): %.4f (%.4f, %.4f)\n\n", total_hpwl.x + total_hpwl.y,
         total_hpwl.x, total_hpwl.y);
  fflush(stdout);
}

void cGP2DglobalPlacement_main() {
  STAGE = cGP2D;
  // ALPHA = initialALPHA;

  tier_assign(STDCELLonly);
  setup_before_opt_cGP2D();
  gp_opt();

  isFirst_gp_opt = false;
  UpdateNetAndGetHpwl();

  auto hpwl = GetUnscaledHpwl(); 
  PrintInfoPrec("Nesterov: HPWL", hpwl.first + hpwl.second, 1);
}

void macroLegalization_main() {
  STAGE = mLG3D;
  pre_mac_tier();
  sa_macro_lg();
  UpdateNetAndGetHpwl();
  if(dynamicStepCMD) {
    reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y);
  }
  printf("RESULT:\n");
  printf("   HPWL(mLG): %.4f (%.4f, %.4f)\n\n", total_hpwl.x + total_hpwl.y,
         total_hpwl.x, total_hpwl.y);
  fflush(stdout);
  post_mac_tier();
}

void WriteBookshelfForGR() {
  PrintProcBegin("Write Bookshelf");
  // temporary update net->pin2 to write bookshelf
  update_pin2();

  char targetDir[BUF_SZ] = {0, };
  sprintf( targetDir, "%s/router_base/", dir_bnd);
  cout << targetDir << endl;

  char cmd[BUF_SZ] = {0, };
  sprintf( cmd, "mkdir -p %s", targetDir);
  system(cmd);

    // call Write Bookshelf function by its tier
    WriteBookshelfWithTier(
        targetDir, 
        // tier number
        0, 
        // *.shape support
//        (detailPlacer == NTUplace3 || shapeMap.size() == 0) ? false : true);
        true, true, true);
  PrintProcEnd("Write Bookshelf");
}

void WriteBookshelf() {
  printf("INFO:  WRITE BOOKSHELF...");
  // temporary update net->pin2 to write bookshelf
  update_pin2();
  
  char targetDir[BUF_SZ] = {0, };
  sprintf( targetDir, "%s/tiers/0", dir_bnd);
  cout << targetDir << endl;

  char cmd[BUF_SZ] = {0, };
  sprintf( cmd, "mkdir -p %s", targetDir);
  system(cmd);

    // call Write Bookshelf function by its tier
    WriteBookshelfWithTier(
        targetDir,
        // tier number
        0, 
        // *.shape support
        (detailPlacer == NTUplace3) ? false : true,
        false);
  printf("PROC:  END WRITE BOOKSHELF\n\n");
  fflush(stdout);
}

void free_trial_mallocs() {
  free(moduleInstance);
  free(terminalInstance);
  free(netInstance);
  free(pinInstance);
  free(gcell_st);
  free(row_st);
  free(tier_st);
  free(place_st);

}

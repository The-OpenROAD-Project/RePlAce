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

#include <error.h>
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
#include "global.h"
#include "initPlacement.h"
#include "macro.h"
#include "opt.h"
#include "plot.h"
#include "wlen.h"
#include "timing.h"

#define compileDate __DATE__
#define compileTime __TIME__


prec netCut;
bool hasNetWeight;
prec netWeight;
prec netWeightMin;
prec netWeightMax;

prec netWeightBase;
prec netWeightBound;
prec netWeightScale;

prec capPerMicron;
prec resPerMicron;

bool isClockGiven;
prec timingClock;
string clockPinName;


int timingUpdateIter;
PIN *pinInstance;
MODULE *moduleInstance;
int pinCNT;
int moduleCNT;

// for moduleInst's pinName
vector< vector<string> > mPinName;

// for termInst's pinName
vector< vector<string> > tPinName;

TERM *terminalInstance;
NET *netInstance;
dense_hash_map<string, int> netNameMap;
int terminalCNT;
int netCNT;

int gcell_cnt;
int row_cnt;
int place_st_cnt;

int g_rrr;

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
int GP_DIM_ONE;
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
bool isBloatingBegin = false;
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
map< string, vector< int > > routeBlockageNodes;
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
prec global_ovfl;
prec maxALPHA;
prec ExtraWSfor3D;
prec MaxExtraWSfor3D;
prec rowHeight;
prec SITE_SPA;

prec layout_area;
double tot_HPWL;
prec tx_HPWL;
prec ty_HPWL;
prec tz_HPWL;
prec tot_overlap;
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
long total_termPL_area; // mgwoo
long total_WS_area; // mgwoo
prec curr_WS_area;  // lutong
prec filler_area;
prec target_cell_den;
prec target_cell_den_orig;  // lutong
prec total_macro_area;
prec grad_stp;
prec gsum_phi;
prec gsum_ovfl;
prec gsum_ovf_area;
prec overflowMin;
prec overflowMin_initial;
prec mGP3D_opt_phi_cof;
prec mGP2D_opt_phi_cof;
prec cGP3D_opt_phi_cof;
prec cGP2D_opt_phi_cof;
prec inv_RAND_MAX;
prec theta;
prec dp_margin_per_tier;

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
char gGP_dir[BUF_SZ];
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
char gTMP_bch_dir[BUF_SZ];
char gTMP_bch_aux[BUF_SZ];
char gTMP_bch_nodes[BUF_SZ];
char gTMP_bch_nets[BUF_SZ];
char gTMP_bch_wts[BUF_SZ];
char gTMP_bch_pl[BUF_SZ];
char gTMP_bch_scl[BUF_SZ];
char sing_fn_aux[BUF_SZ];
char sing_fn_nets[BUF_SZ];
char sing_fn_nodes[BUF_SZ];
char sing_fn_pl[BUF_SZ];
char sing_fn_wts[BUF_SZ];
char sing_fn_scl[BUF_SZ];
char bench_aux[BUF_SZ];
char dir_bnd[BUF_SZ];
char global_router[1023];
char output_dir[BUF_SZ];
char currentDir[BUF_SZ];

string sourceCodeDir;

//RECT cur_rect;

// opt.cpp -> main.cpp
CELLx *gcell_st;
ROW *row_st;

PLACE *place_st;
PLACE *place_backup_st;
PLACE place;
PLACE place_backup;
FPOS term_pmax;
FPOS term_pmin;
FPOS filler_size;
FPOS zeroFPoint;
POS zeroPoint;
POS n1p;
POS msh;
FPOS gmin;
FPOS gmax;
FPOS gwid;
TIER *tier_st;
POS dim_bin;
POS dim_bin_mGP2D;

// .shapes
// SHAPE's storage
vector< SHAPE > shapeStor;

// nodes String -> shapeStor's index.
dense_hash_map< string, vector< int > > shapeMap;
POS dim_bin_cGP2D;


///  ARGUMENTS  ///////////////////////////////////////////
int numLayer;
prec aspectRatio;
string bmFlagCMD;
string auxCMD;   // mgwoo
string defCMD;   // mgwoo
string sdcCMD; // mgwoo
string verilogCMD; // mgwoo
vector<string> libStor;   // mgwoo
string outputCMD;  // mgwoo
string experimentCMD;  // mgwoo
vector<string> lefStor;   // mgwoo
string verilogTopModule;
int defMacroCnt;

int numThread;
InputMode inputMode;

string benchName;

string denCMD;
string dimCMD;
string bxMaxCMD;
string byMaxCMD;
string bzMaxCMD;
string gTSVcofCMD;
string ALPHAmGP_CMD;
string ALPHAcGP_CMD;
string BETAmGP_CMD;
string BETAcGP_CMD;
string dampParam_CMD;
string aspectRatioCMD;
string overflowMinCMD;
string pcofmaxCMD;
string pcofminCMD;
string refdWLCMD;
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
string detailPlacerLocationCMD;
string detailPlacerFlagCMD;

prec densityDP;
bool hasDensityDP;
bool isSkipPlacement;
bool isOnlyLGinDP;
bool isPlot;
bool isSkipIP;
bool isBinSet;
bool isNtuDummyFill;

int conges_eval_methodCMD;
bool isVerbose;
bool plotCellCMD;
bool plotMacroCMD;
bool plotDensityCMD;
bool plotFieldCMD;
bool constraintDrivenCMD;
bool routabilityCMD;
bool stnCMD;  // lutong
bool lambda2CMD;
bool dynamicStepCMD;
bool onlyGlobalPlaceCMD;
bool isTiming;
bool isARbyUserCMD;
bool thermalAwarePlaceCMD;
bool trialRunCMD;
bool autoEvalRC_CMD;
bool onlyLG_CMD;
///////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {

    double tot_cpu = 0;
    double time_ip = 0;
    double time_tp = 0;
    double time_mGP3D = 0;
    double time_mGP2D = 0;
    double time_mGP = 0;
    double time_lg = 0;
    double time_cGP3D = 0;
    double time_cGP2D = 0;
    double time_cGP = 0;
    double time_dp = 0;

    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    printCMD(argc, argv);
    cout << "INFO:  VERSION, Compiled at " << compileDate << " " << compileTime
         << endl;
    cout << "INFO:  Now is " << asctime(timeinfo);

    ///////////////////////////////////////////////////////////////////////
    ///// Parse Arguments (defined in argument.cpp) ///////////////////////
    initArgument(argc, argv);
    ///////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////
    ///// Placement Initialization  ///////////////////////////////////////

    init();
    printf("PROC:  BEGIN IMPORTING PLACEMENT INPUT\n");
    
    ParseInput();

    if(numLayer > 1)
        calcTSVweight();
    build_data_struct();
    
    net_update_init();
    init_tier();
    printf("PROC:  END IMPORTING PLACEMENT INPUT\n\n\n");
    fflush(stdout);
    ///////////////////////////////////////////////////////////////////////

    time_start(&tot_cpu);
    // Normal cases
    if( !isSkipPlacement ) {
        ///////////////////////////////////////////////////////////////////////
        ///// IP:  INITIAL PLACEMENT //////////////////////////////////////////
        printf("PROC:  BEGIN INITIAL PLACEMENT (IP)\n");
        time_start(&time_ip);
        build_data_struct();
        initialPlacement_main();
        time_end(&time_ip);
        printf("PROC:  END INITIAL PLACEMENT (IP)\n\n\n");
        fflush(stdout);
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
        }

        if(placementMacroCNT > 0) {
            ///////////////////////////////////////////////////////////////////////
            ///// mGP2D:  MIXED_SIZE_2D_GLOBAL_PLACE //////////////////////////////
            printf("PROC:  BEGIN GLOBAL 2D PLACEMENT IN EACH TIER\n");
            printf("PROC:  Mixed-Size 2D Global Placement (mGP2D)\n");
            time_start(&time_mGP2D);
            mGP2DglobalPlacement_main();
            time_end(&time_mGP2D);
            printf("   RUNTIME(s) : %.4f\n\n\n", time_mGP2D);
            printf("PROC:  END GLOBAL 2D PLACEMENT (mGP2D)\n\n\n");
            if( inputMode == InputMode::lefdef )  {
              WriteDef( defGpOutput );
            }
            ///////////////////////////////////////////////////////////////////////
        }
        time_end(&time_mGP);

        if(placementMacroCNT > 0 /*&& INPUT_FLG != ISPD*/) {
            ///////////////////////////////////////////////////////////////////////
            ///// mLG:  MACRO_LEGALIZATION ////////////////////////////////////////
            printf("PROC:  BEGIN MACRO LEGALIZATION IN EACH TIER\n");
            printf("PROC:  SA-based Macro Legalization (mLG)\n");
            time_start(&time_lg);
            // mgwoo
            bin_init();
            macroLegalization_main();
            time_end(&time_lg);
            printf("   RUNTIME(s) : %.4f\n\n\n", time_lg);
            printf("PROC:  END MACRO LEGALIZATION (mLG)\n\n\n");
            ///////////////////////////////////////////////////////////////////////
        }

        time_start(&time_cGP);
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
        }

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
        build_data_struct(false);
        tier_assign(STDCELLonly);
        setup_before_opt();
    }

    // Write BookShelf format
    WriteBookshelf();
    printf("PROC:  END WRITE BOOKSHELF\n\n");
    fflush(stdout);
   
    if( isPlot ) { 
        SaveCellPlotAsJPEG( "Final Global Placement Result", false, 
                string(dir_bnd) 
                + string("/globalPlaceResult") );
    }
   
    if( inputMode == InputMode::lefdef )  {
        WriteDef( defGpOutput );
    }

    ///////////////////////////////////////////////////////////////////////
    ///// DP:  DETAILED PLACEMENT /////////////////////////////////////////
    if(onlyGlobalPlaceCMD) {
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

    get_modu_hpwl();
    output_final_pl( gDP3_pl );
    if( inputMode == InputMode::lefdef )  {
        WriteDef( defOutput );
    }

    printf("\n\n");
    printf(
        "=== SUMMARY "
        "=========================================================\n");
    printf(" ### HPWL (x, y) of the design is %.2lf (%.2lf, %.2lf).\n\n",
           tot_HPWL, tx_HPWL, ty_HPWL);

    printf(" ### Numbers of Iterations: \n");
    if(trialRunCMD)
        printf("     Trial:  %d (%.4lf sec/iter)\n", trial_iterCNT,
               time_tp / (prec)trial_iterCNT);
    if(numLayer > 1)
        printf("     mGP3D:  %d\n", mGP3D_iterCNT);
    if(mGP2D_iterCNT != 0)
        printf("     mGP2D:  %d (%.4lf sec/iter)\n", mGP2D_iterCNT,
               time_mGP / ((prec)(mGP3D_iterCNT + mGP2D_iterCNT)));
    if(numLayer > 1)
        printf("     cGP3D:  %d\n", cGP3D_iterCNT);
    printf("     cGP2D:  %d (%.4lf sec/iter)\n", cGP2D_iterCNT,
           time_cGP / ((prec)(cGP3D_iterCNT + cGP2D_iterCNT)));
    printf("     ____________________________________\n");
    printf("     TOTAL:  %d\n\n", trial_iterCNT + mGP3D_iterCNT +
                                      mGP2D_iterCNT + cGP3D_iterCNT +
                                      cGP2D_iterCNT);

    printf(
        " ### CPU_{IP, TP, mGP, LG, cGP, DP} is %.2lf, %.2lf, %.2lf, %.2lf, "
        "%.2lf, %.2lf.\n",
        time_ip, time_tp, time_mGP, time_lg, time_cGP, time_dp);
    printf(" ### CPU_TOT is %.2lf seconds (%.2lf min).\n\n", tot_cpu,
           tot_cpu / 60);
    fflush(stdout);
    
//    SavePlot( "Final Placement Result");
    if( isPlot ) { 
        SaveCellPlotAsJPEG( "Final Placement Result", false, 
                string(dir_bnd) 
                + string("/finalResult") );
    }

    // DP Spef Generation 
    if( inputMode == InputMode::lefdef && isTiming ) {
        // for final DP SPEF generating
        Timing::Timing TimingInst(moduleInstance, terminalInstance,
            netInstance, netCNT, pinInstance, pinCNT, 
            mPinName, tPinName, "clk", timingClock);

        TimingInst.BuildSteiner(true);

        string spefName = string(dir_bnd) + "/" + gbch + "_dp.spef";
        TimingInst.WriteSpef( spefName );
    }

//    ShowPlot( benchName );   
    return 0;
}

// mgwoo
void init() {
    char str_lg[BUF_SZ] = {0, };
    char str_dp[BUF_SZ] = {0, };
    char str_dp2[BUF_SZ] = {0, };
    char str_dp3[BUF_SZ] = {0, };
    char result_file[BUF_SZ] = {0, };
    int ver_num = 0;

    inv_RAND_MAX = (prec)1.0 / RAND_MAX;

    theta = THETA_XY_3D_PLOT;
    target_cell_den = atof(denCMD.c_str());
    target_cell_den_orig = atof(denCMD.c_str());  // lutong
    // overflowMin     = overflowMin/atof(den);

    zeroFPoint.SetZero();
    zeroPoint.SetZero();

//    gfft_flg = DDCT;

//    sprintf(FastDP_cmd, "FastPlace3.0_DP");
//    sprintf(NTUplace3_cmd, "ntuplace3");
//    sprintf(NTUplace4h_cmd, "ntuplace4h");
    sprintf(global_router, "NCTUgr.ICCAD2012");
    // sprintf (global_router, "NCTUgr2_fast");
    // sprintf (global_router, "NCTUgr2");

    switch(detailPlacer) {
        case FastPlace:
#ifdef SA_LG
            sprintf(str_lg, "%s", "_eplace_lg");
#else
            sprintf(str_lg, "%s", "_FP_lg");
#endif
            sprintf(str_dp, "%s", "_FP_dp");
            break;

        case NTUplace3:
#ifdef SA_LG
            sprintf(str_lg, "%s", "_eplace_lg");
#else
            sprintf(str_lg, "%s", ".eplace-gp.lg");
#endif
            sprintf(str_dp, "%s", ".eplace-gp.ntup");
            sprintf(str_dp2, "%s", ".eplace-cGP3D.ntup");
            break;

        case NTUplace4h:
#ifdef SA_LG
            sprintf(str_lg, "%s", "_eplace_lg");
#else
            sprintf(str_lg, "%s", ".eplace-gp.lg");
#endif
            sprintf(str_dp, "%s", ".eplace-gp.ntup");
            sprintf(str_dp2, "%s", ".eplace-cGP3D.ntup");
            break;
    }

    sprintf(str_dp3, ".%s.%s", bmFlagCMD.c_str(), "eplace");
    g_rrr = 0;
    tot_overlap = 0;

    if(strcmp(bmFlagCMD.c_str(), "mms") == 0) {
        INPUT_FLG = MMS;
    }
    else if(strcmp(bmFlagCMD.c_str(), "ispd") == 0) {
        INPUT_FLG = ISPD;
    }
    else if(strcmp(bmFlagCMD.c_str(), "sb") == 0) {
        INPUT_FLG = SB;
    }
    else if(strcmp(bmFlagCMD.c_str(), "etc") == 0) {
        INPUT_FLG = ETC;
    }
    else if(strcmp(bmFlagCMD.c_str(), "ibm") == 0) {
        INPUT_FLG = IBM;
    }
    else {
        printf("***ERROR: Please check your benchmark flag, i.e., bmflag!\n");
        printf(" SUGGEST: mms, ispd, sb, etc, ibm\n\n");
        exit(1);
    }

    global_macro_area_scale = target_cell_den;
    printf("INFO:  Target Density = %.6lf\n", target_cell_den);

    wcof_flg = /* 1 */ 2 /* 3 */;

    switch(WLEN_MODEL) {
        case LSE:
            wcof00.x = wcof00.y = wcof00.z = 0.1;
            break;

        case WA:
            if(INPUT_FLG == ISPD05 || INPUT_FLG == ISPD06 ||
               INPUT_FLG == ISPD || INPUT_FLG == MMS || INPUT_FLG == SB ||
               INPUT_FLG == ETC) {
                wcof00_dim1.x = wcof00_dim1.y = wcof00_dim1.z = 0.50;  // 500.0;
                wcof00_org.x = wcof00_org.y = wcof00_org.z = 0.125;
            }
            else if(INPUT_FLG == IBM) {
                wcof00_dim1.x = wcof00_dim1.y = wcof00_dim1.z = 0.50;
                wcof00_org.x = wcof00_org.y = wcof00_org.z = 0.50;
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
    

    string fileCMD = (auxCMD != "")? auxCMD : defCMD;

    //
    // check '/' from back side
    //
    int slashPos = fileCMD.rfind('/');
     
    // 
    // extract benchName ~~~~/XXXX.aux
    //                        <------>   
    benchName = (slashPos == -1)? fileCMD : fileCMD.substr(slashPos+1, fileCMD.length() - slashPos - 1);
    sprintf(gbch_dir, "%s", (slashPos == -1)? currentDir : fileCMD.substr(0, slashPos).c_str());

    // if benchName has [.aux] extension
    // remove [.aux] 
    int dotPos = benchName.rfind(".");
    if( benchName.length() - dotPos == 4 ) {
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

    if( experimentCMD == "" ) {
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
    sprintf(gGP_dir, "%s", dir_bnd);
    sprintf(gLG_pl, "%s/%s%s.pl", dir_bnd, gbch, str_lg);
    sprintf(gDP_pl, "%s/%s%s.pl", dir_bnd, gbch, str_dp);
    sprintf(gDP2_pl, "%s/%s%s.pl", dir_bnd, gbch, str_dp2);
    sprintf(gDP3_pl, "%s/%s%s.pl", dir_bnd, gbch, str_dp3);
    sprintf(result_file, "%s/res", dir_bnd);

    sprintf(defGpOutput, "%s/%s_gp.def", dir_bnd, gbch); 
    sprintf(defOutput, "%s/%s_final.def", dir_bnd, gbch); 

    printf("INFO:  Experiment Index %d\n", ver_num);
    printf("INFO:  DIR_PATH = %s\n\n", dir_bnd);
    fflush(stdout);

    sprintf(bench_aux, "%s/%s.aux", gbch_dir, gbch);
    sprintf(gbch_aux, "%s.aux", gbch);

    strcpy(gGP_dir, "./");

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
    GP_DIM_ONE = GP_DIM_ONE_FOR_IP;
    initial_placement();
    UpdateNetAndGetHpwl();
    printf("RESULT:\n");
    printf("   HPWL(IP): %.4f (%.4f, %.4f, %.4f)\n\n",
           total_hpwl.x + total_hpwl.y + total_hpwl.z, total_hpwl.x,
           total_hpwl.y, total_hpwl.z);
    place_backup = place;
    fflush(stdout);
}

void tmGP3DglobalPlacement_main() {
    STAGE = mGP3D;
    // ALPHAmGP = initialALPHA;
    GP_DIM_ONE = GP_DIM_ONE_FOR_GP1;
    gp_opt();
    UpdateNetAndGetHpwl();
    printf("RESULT:\n");
    printf("   HPWL(tGP3D): %.4f (%.4f, %.4f, %.4f)\n\n",
           total_hpwl.x + total_hpwl.y + total_hpwl.z, total_hpwl.x,
           total_hpwl.y, total_hpwl.z);
    fflush(stdout);
}

void tmGP2DglobalPlacement_main() {
    STAGE = mGP2D;
    // ALPHA = initialALPHA;
    GP_DIM_ONE = GP_DIM_ONE_FOR_mGP2D;
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
    GP_DIM_ONE = GP_DIM_ONE_FOR_GP1;
    gp_opt();
    isFirst_gp_opt = false;
    UpdateNetAndGetHpwl();
    if(dynamicStepCMD) {
        reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y +
                                       total_hpwl.z);
    }
    printf("RESULT:\n");
    printf("   HPWL(mGP3D): %.4f (%.4f, %.4f, %.4f)\n\n",
           total_hpwl.x + total_hpwl.y + total_hpwl.z, total_hpwl.x,
           total_hpwl.y, total_hpwl.z);
    fflush(stdout);
}

void mGP2DglobalPlacement_main() {
    STAGE = mGP2D;
    // ALPHA = initialALPHA;
    GP_DIM_ONE = GP_DIM_ONE_FOR_mGP2D;
    tier_assign(MIXED);
    calc_tier_WS();
    setup_before_opt_mGP2D();
    gp_opt();
    isFirst_gp_opt = false;
    /*if (INPUT_FLG != ISPD)*/ post_mGP2D_delete();
    fflush(stdout);
    UpdateNetAndGetHpwl();
    if(dynamicStepCMD) {
        reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y +
                                       total_hpwl.z);
    }
    printf("RESULT:\n");
    printf("   HPWL(mGP2D): %.4f (%.4f, %.4f)\n\n", total_hpwl.x + total_hpwl.y,
           total_hpwl.x, total_hpwl.y);
    fflush(stdout);
}

void tcGP3DglobalPlacement_main() {
    STAGE = cGP3D;
    // ALPHA = initialALPHA;
    GP_DIM_ONE = GP_DIM_ONE_FOR_GP2;
    cell_copy();  // cell_macro_copy ();
    gp_opt();
    UpdateNetAndGetHpwl();
    printf("RESULT:\n");
    printf("   HPWL(tGP3D): %.4f (%.4f, %.4f, %.4f)\n\n",
           total_hpwl.x + total_hpwl.y + total_hpwl.z, total_hpwl.x,
           total_hpwl.y, total_hpwl.z);
    fflush(stdout);
}

void cGP3DglobalPlacement_main() {
    STAGE = cGP3D;
    // ALPHA = initialALPHA;
    GP_DIM_ONE = GP_DIM_ONE_FOR_GP2;
    cell_copy();  // cell_macro_copy ();
    gp_opt();
    isFirst_gp_opt = false;
    UpdateNetAndGetHpwl();
    if(dynamicStepCMD) {
        reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y +
                                       total_hpwl.z);
    }
    printf("RESULT:\n");
    printf("   HPWL(cGP3D): %.4f (%.4f, %.4f, %.4f)\n\n",
           total_hpwl.x + total_hpwl.y + total_hpwl.z, total_hpwl.x,
           total_hpwl.y, total_hpwl.z);
    fflush(stdout);
}

void tcGP2DglobalPlacement_main() {
    STAGE = cGP2D;
    // ALPHA = initialALPHA;
    GP_DIM_ONE = GP_DIM_ONE_FOR_GP3;
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
    GP_DIM_ONE = false;

    tier_assign(STDCELLonly);
    setup_before_opt_cGP2D();
    gp_opt();

    isFirst_gp_opt = false;
    UpdateNetAndGetHpwl();
    printf("RESULT:\n");
    printf("   HPWL(cGP2D): %.4f (%.4f, %.4f)\n\n", total_hpwl.x + total_hpwl.y,
           total_hpwl.x, total_hpwl.y);
    fflush(stdout);
}

void macroLegalization_main() {
    STAGE = mLG3D;
    GP_DIM_ONE = GP_DIM_ONE_FOR_MAC;
    pre_mac_tier();
    sa_macro_lg();
    UpdateNetAndGetHpwl();
    if(dynamicStepCMD) {
        reassign_trial_2ndOrder_lastEP(total_hpwl.x + total_hpwl.y +
                                       total_hpwl.z);
    }
    printf("RESULT:\n");
    printf("   HPWL(mLG): %.4f (%.4f, %.4f)\n\n", total_hpwl.x + total_hpwl.y,
           total_hpwl.x, total_hpwl.y);
    fflush(stdout);
    post_mac_tier();
}

void WriteBookshelf() {
    // temporary update net->pin2 to write bookshelf
    update_pin2();
    
    for(int i = 0; i < numLayer; i++) {
        // call Write Bookshelf function by its tier
        WriteBookshelfWithTier(i, 
            (placementMacroCNT == 0)? STDCELLonly : MIXED,
            (detailPlacer == NTUplace3 || shapeMap.size() == 0)? false : true );
    }
}

void free_trial_mallocs() {
    mkl_free(moduleInstance);
    mkl_free(terminalInstance);
    mkl_free(netInstance);
    mkl_free(pinInstance);
    mkl_free(gcell_st);
    free(row_st);
    mkl_free(tier_st);
    free(place_st);

//    mkl_free(modu_map);
//    mkl_free(term_map);
}

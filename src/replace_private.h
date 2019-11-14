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

#ifndef __GLOBAL__
#define __GLOBAL__

#include <stdint.h>
#include <algorithm>
#include <climits>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <iomanip>
#include <cmath>

#ifdef USE_GOOGLE_HASH
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#else
#include <unordered_map>
#include <unordered_set>
#endif

#include <cfloat>
#include <cstring>
#include <cassert>

#include <tcl.h>

#define PI 3.141592653589793238462L
#define SQRT2 1.414213562373095048801L

// for PREC_MODE variable => required for different codes.
#define IS_FLOAT 0
#define IS_DOUBLE 1

// precision settings
#define PREC_MODE IS_FLOAT

#if PREC_MODE == IS_FLOAT

typedef float prec;
#define PREC_MAX FLT_MAX
#define PREC_MIN FLT_MIN
#define PREC_EPSILON numeric_limits< float >::epsilon()

#elif PREC_MODE == IS_DOUBLE

typedef double prec;
#define PREC_MAX DBL_MAX
#define PREC_MIN DBL_MIN
#define PREC_EPSILON numeric_limits< double >::epsilon()

#endif

#define INT_CONVERT(a) (int)(1.0 * (a) + 0.5f)
#define INT_DOWN(a) (int)(a)
#define INT_UP(a) (int)(a) + 1
#define UNSIGNED_CONVERT(a) (unsigned)(1.0 * (a) + 0.5f)

#define IS_PRECISION_EQUAL(a, b) (fabs((a)-(b)) <= PREC_EPSILON)

#ifdef USE_GOOGLE_HASH
#define HASH_MAP google::dense_hash_map
#define HASH_SET google::dense_hash_set
#else
#define HASH_MAP std::unordered_map
#define HASH_SET std::unordered_set
#endif

#define TSV_CAP 30.0  // fF
// data from M. Jung et al. "How to Reduce Power in 3D IC Designs:
// A Case Study with OpenSPARC T2 Core", CICC 2013, Section 2.B
#define MIV_CAP 0.1  // fF
// data from S. Panth et al. "Power-Performance Study of Block-Level
// Monolithc 3D-ICs Considering Inter-Tier Performance Variations",
// DAC 2014, Section 4.
#define ROW_CAP 0.3  // fF
// row_hei is 1.4um at 45nm tech-node with 70nm M1 half-pitch and
// 10 M1 tracks per placement row, unit capacitance per centimeter
// at intermediate routing layers are constantly 2pF/cm from ITRS
// INTC2 tabls between 2008 and 2013, which makes ROW_CAP roughly
// to be 0.3fF/row

#define BUF_SZ 511
#define Epsilon 1.0E-15
#define MIN_AREA3 /* 1.0 */ 1.0e-12
#define MIN_LEN 25.0 /* 10.0 */ /* 5.0 */ /* 1.0 */
#define LS_DEN
#define DetailPlace
#define MSH_Z_RES /* 8 */ 1
#define THETA_XY_3D_PLOT PI / 6.0
#define Z_SCAL 1.00

#define DEN_GRAD_SCALE 1.0 /* 0.1 */ /* 0.5 */ /* 0.25 */ /* 0.125 */
#define LAYER_ASSIGN_3DIC MIN_TIER_ORDER                  /* MAX_AREA_DIS_DIV */
///////////////////////////////////////////////////////////////////////////


struct POS;

struct FPOS {
  prec x;
  prec y;

  FPOS();
  FPOS(prec xloc, prec yloc);
  void Set(prec a);
  void Set(FPOS a);

  void Set(prec xloc, prec yloc);
  void Set(POS a);
  void SetZero();

  prec GetX();
  prec GetY(); 

  void Add(FPOS a);
  void SetAdd(FPOS a, FPOS b);

  void Min(FPOS a);
  void SetMin(FPOS a, FPOS b);

  void Max(FPOS a);
  void SetMax(FPOS a, FPOS b);

  prec GetProduct();
  void Dump();
  void Dump(std::string a);
};

struct POS {
  int x;
  int y;

  POS();
  POS(int xloc, int yloc);

  void Set(int a);
  void Set(POS a);

  void Set(int xloc, int yloc);

  void Set(FPOS fp);
  void SetZero();
  void Add(POS a);

  void SetAdd(POS a, POS b);
  void Min(POS a);
  void SetMin(POS a, POS b);
  void Max(POS a); 
  void SetMax(POS a, POS b);
  int GetProduct();
  void SetXProjection(int a, int b);
  void SetYProjection(int a, int b);
  void SetProjection(POS a, POS b);
  void SetXYProjection(POS a, POS b);
  void Dump(); 
  void Dump(std::string a);
};


// for saving pinName
// If it's lied in the PIN structure, 
// it'll enlarge the runtime
extern std::vector< std::vector< std::string > > mPinName;
extern std::vector< std::vector< std::string > > tPinName;

extern std::vector<std::string> moduleNameStor; 
extern std::vector<std::string> terminalNameStor;
extern std::vector<std::string> cellNameStor;
extern std::vector<std::string> netNameStor;

struct RECT {
  FPOS pmin;
  FPOS pmax;
  RECT();
  void Dump();
};

// Pin Instance
struct PIN {
  FPOS fp;
  FPOS e1;
  FPOS e2;
  POS flg1;
  POS flg2;
  int moduleID;
  int pinIDinModule;
  int netID;
  int pinIDinNet;
  int gid;   // current Pin's idx
  int IO;    // I -> 0; O -> 1
  int term;  // term -> 1, move -> 0
  int X_MIN;
  int Y_MIN;
  int X_MAX;
  int Y_MAX;
  PIN(); 
};


// *.nodes -> not isTerminal
// Module Instance
struct MODULE {
  FPOS pmin;
  FPOS pmax;
  FPOS size;
  FPOS half_size;
  FPOS center;
  FPOS *pof;
  PIN **pin;
  prec area;
  int idx;
  int netCNTinObject;
  int pinCNTinObject;
  int flg;
  int tier;
  int mac_idx;
  int ovlp_flg;
  POS pmin_lg;
  POS pmax_lg;

  const char* Name();
  MODULE();
  void Dump(std::string a);
};



// *.nodes -> isTerminal // isTerminalNI
// Terminal Instance
struct TERM {
  // struct  POS p;   // left_x,  low_y
  // struct  POS mp;  // right_x, up_y
  FPOS pmin;
  FPOS pmax;
  prec area;
  FPOS size;
  FPOS center;
  FPOS *pof;
  PIN **pin;
  int idx;
  int netCNTinObject;
  int pinCNTinObject;
  int IO;  // I -> 0; O -> 1
           //    int tier;
  bool isTerminalNI;
  prec PL_area;
  const char* Name();

  TERM();
  void Dump();
};

struct CELL {
  FPOS pmin;
  FPOS pmax;
  FPOS den_pmin;
  FPOS den_pmax;
  FPOS center;
  prec area;
  FPOS *pof;
  PIN **pin;
  prec x_grad;
  prec y_grad;
  int tier;
  FPOS size;
  prec den_scal;
  FPOS half_size;
  FPOS half_den_size;
  int idx;
  int pinCNTinObject;
  int netCNTinObject;
  int flg;
  prec area_org_befo_bloating;
  prec inflatedNewArea;
  prec inflatedNewAreaDelta;
  prec den_scal_org_befo_bloating;
  FPOS size_org_befo_bloating;
  FPOS half_size_org_befo_bloating;
  FPOS half_den_size_org_befo_bloating;
  FPOS *pof_tmp;
  PIN **pin_tmp;
  const char* Name();
  void Dump();
};

class SHAPE {
 public:
  //    char *name;
  //    char *prefix;

  std::string name, instName;

  int idx;
  prec llx;
  prec lly;
  prec width;
  prec height;

  SHAPE(std::string _name, std::string _instName, 
      int _idx, prec _llx, prec _lly,
      prec _width, prec _height);
  void Dump();
};

class UFPin {
 public:
  // int id;
  int parent;
  int rank;
  int modu;

  UFPin();
  UFPin(int moduleID);
};

class TwoPinNets {
 public:
  bool selected;
  int start_modu;
  int end_modu;
  int rect_dist;
  int i;
  int j;

  TwoPinNets();
  TwoPinNets(int start_modu, int end_modu, 
      prec rect_dist, int i, int j);

};


class ROUTRACK {
 public:
  struct FPOS from;
  struct FPOS to;
  int layer;  // 1:M_Layer1, 2:M_Layer2, ..., etc.
  int netIdx;
  ROUTRACK();
  ROUTRACK(struct FPOS _from, struct FPOS _to, 
      int _layer, int _netIdx);
  void Dump();
};

struct NET {
  public:
  std::map< int, UFPin > mUFPin;
  std::vector< TwoPinNets > two_pin_nets;
  std::vector< ROUTRACK > routing_tracks;

  prec min_x;
  prec min_y;
  prec max_x;
  prec max_y;
  FPOS sum_num1;
  FPOS sum_num2;
  FPOS sum_denom1;
  FPOS sum_denom2;
  PIN **pin;  // will have modified pin info. see opt.cpp
  PIN **pin2; // will store original pin info. used for bookshelf writing.
  FPOS terminalMin;
  FPOS terminalMax;

  prec hpwl_x;
  prec hpwl_y;
  prec hpwl;
  int outPinIdx;            // determine outpin's index
  int pinCNTinObject;       // for pin
  int pinCNTinObject2;      // for pin2
  int pinCNTinObject_tier;  // used for writing bookshelf
  int idx;
  int mod_idx;
  prec timingWeight;
  prec customWeight;
  prec wl_rsmt;             // lutong

  const char* Name();
  NET();
};

int UFFind(NET *net, int moduleID);
void UFUnion(NET *net, int idx, int idy);

// for *.scl files
struct ROW {
  prec site_wid;  // SiteWidth
  prec site_spa;  // SiteSpacing

  std::string ori;
  //    string sym;
  bool isXSymmetry;
  bool isYSymmetry;
  bool isR90Symmetry;

  int x_cnt;  // NumSites
  FPOS pmin;
  FPOS pmax;
  FPOS size;

  ROW();
  void Dump(std::string a);
};

// for *.scl files
// ROW -> PLACE structure
struct PLACE {
  FPOS org;
  FPOS end;
  FPOS center;
  FPOS stp;
  FPOS cnt;
  prec area;
  void Dump(std::string a);
};

// for multi-rows
struct TIER {
  struct FPOS bin_org;
  struct FPOS inv_bin_stp;
  struct POS dim_bin;
  struct FPOS pmin;
  struct FPOS pmax;
  struct BIN *bin_mat;
  struct FPOS center;
  struct FPOS size;
  struct ROW *row_st;
  struct MODULE **modu_st;
  struct TERM **term_st;
  struct MODULE **mac_st;
  struct CELL **cell_st;
  struct FPOS bin_stp;
  prec area;
  prec modu_area;
  prec term_area;
  prec virt_area;
  prec filler_area;
  prec pl_area;
  prec ws_area;
  prec bin_area;
  prec tot_bin_area;
  prec inv_bin_area;
  prec sum_ovf;
  int row_cnt;
  int modu_cnt;
  int filler_cnt;
  int cell_cnt;
  int mac_cnt;
  int term_cnt;
  int tot_bin_cnt;
  prec temp_mac_area;
  struct FPOS bin_off;
  struct FPOS half_bin_stp;
  struct MODULE *max_mac;
  struct CELL **cell_st_tmp;

  // routability
  struct FPOS tile_stp;
  prec tile_area;
  struct POS dim_tile;
  int tot_tile_cnt;
  struct FPOS half_tile_stp;
  struct FPOS inv_tile_stp;
  struct FPOS tile_org;
  struct FPOS tile_off;
  struct TILE *tile_mat;
};

enum { STDCELLonly, MIXED };


extern int gVerbose;

// for timing
extern prec globalWns;
extern prec globalTns;
extern prec netCut;
extern bool hasUnitNetWeight;
extern bool hasCustomNetWeight;
extern prec netWeight;
extern prec netWeightMin;
extern prec netWeightMax;

extern prec netWeightBase;
extern prec netWeightBound;
extern prec netWeightScale;
extern bool netWeightApply;

extern prec capPerMicron;
extern prec resPerMicron;

extern bool isClockGiven;
extern prec timingClock;
extern std::string clockPinName;

extern bool isInitSeed;
extern std::string plotColorFile;

extern int timingUpdateIter;
extern int pinCNT;
extern int moduleCNT;
extern int gcell_cnt;

extern std::string globalRouterPosition;
extern std::string globalRouterSetPosition;
extern prec globalRouterCapRatio;

enum { FillerCell, StdCell, Macro };
extern int terminalCNT;
extern int netCNT;
extern int numNonRectangularNodes;
extern int totalShapeCount;

extern int g_rrr;
extern int STAGE;

extern std::vector< SHAPE > shapeStor;
extern HASH_MAP< std::string, std::vector< int > > shapeMap;

// STAGE's possible inner variables
enum {
  INITIAL_PLACE,  // INITIAL_PLACE
  tGP,            // TRIAL
  mGP3D,          // MIXED_SIZE_3D_GLOBAL_PLACE
  mGP2D,          // MIXED_SIZE_2D_GLOBAL_PLACE
  mLG3D,          // MACRO_LEGALIZATION
  cGP3D,          // STDCELL_ONLY_3D_GLOBAL_PLACE
  cGP2D,          // STDCELL_ONLY_2D_GLOBAL_PLACE
  DETAIL_PLACE
};

// these variable is required to have detailPlacer settings
extern int detailPlacer;
enum { NoneDp, FastPlace, NTUplace3, NTUplace4h };
extern std::string detailPlacerLocation;
extern std::string detailPlacerFlag;

extern bool isOnlyLGinDP;
extern bool isSkipPlacement;
extern bool hasDensityDP;
extern bool isSkipIP;
extern bool isBinSet;
extern bool isDummyFill;
extern prec densityDP;
extern prec routeMaxDensity;

extern int placementStdcellCNT;
extern int gfiller_cnt;
extern int placementMacroCNT;
enum { CDFT, RDFT, DDCT };
extern int msh_yz;
extern int INPUT_FLG;
enum { ISPD05, ISPD06, ISPD06m, ISPD, MMS, SB, ETC, IBM };
extern int gmov_mac_cnt;
extern int cGP3D_buf_iter;
extern int cGP2D_buf_iter;
extern int mGP3D_tot_iter;
extern int mGP2D_tot_iter;
extern int cGP3D_tot_iter;
extern int cGP2D_tot_iter;
extern int flg_3dic;
extern int flg_3dic_io;
extern int numLayer;
enum { NoneAdj, RandomAdj, SmartAdj };
enum { zCenterPlace, zCenterTierZero, zCenterTierMax };

enum { global_router_based, prob_ripple_based };

enum { FastDP, NTUpl3, NTUpl4h };
enum { MAX_PCNT_ORDER, MIN_TIER_ORDER, MAX_AREA_DIS_DIV };
enum { secondOrder, thirdOrder };
enum {
  potnPhase1 = 1,
  potnPhase2 = 2,
  potnPhase3 = 3,
  potnPhase4 = 4,
  potnPhase5 = 5,
  potnPhase6 = 6,
  potnPhase7 = 7,
  potnPhase8 = 8
};

extern int potnPhaseDS;
extern int bloating_max_count;
extern int bloatCNT;
extern int trial_iterCNT;
extern int mGP3D_iterCNT;
extern int mGP2D_iterCNT;
extern int cGP3D_iterCNT;
extern int cGP2D_iterCNT;

// routability
extern prec row_height;   // lutong
extern prec gcell_x_cof;  // lutong
extern prec gcell_y_cof;  // lutong
extern prec gcell_cof;
extern prec eq_pitch;
extern prec max_inflation_ratio;
extern prec inflation_ratio_coef;
extern prec edgeadj_coef;
extern prec pincnt_coef;
extern prec gRoute_pitch_scal;
extern prec inflation_threshold;
extern prec total_inflation_ratio;
extern prec h_max_inflation_ratio;
extern prec v_max_inflation_ratio;
extern prec total_inflatedNewAreaDelta;
extern prec currTotalInflation;
extern int inflation_cnt;
extern int inflation_max_cnt;
extern int routepath_maxdist;
extern prec inflation_area_over_whitespace;
extern prec curr_filler_area;
extern prec adjust_ratio;
extern bool is_inflation_h;
extern bool flg_noroute;


extern prec ALPHAmGP;
extern prec ALPHAcGP;
extern prec ALPHA;
extern prec BETAmGP;
extern prec BETAcGP;
extern prec BETA;

extern prec dampParam;
extern prec maxALPHA;
extern prec ExtraWSfor3D;
extern prec MaxExtraWSfor3D;
extern prec rowHeight;
extern prec SITE_SPA;
extern prec layout_area;
extern prec total_std_area;
extern prec total_std_den;
extern prec total_modu_area;
extern prec inv_total_modu_area;
extern prec total_cell_area;
extern prec curr_cell_area;  // lutong

extern prec total_term_area;
extern prec total_move_available_area;
extern prec total_filler_area;
extern prec total_PL_area;
extern prec total_termPL_area;
extern prec total_WS_area;

extern prec curr_WS_area;  // lutong
extern prec filler_area;
extern prec target_cell_den;
extern prec target_cell_den_orig;  // lutong
extern prec total_macro_area;
extern prec ignoreEdgeRatio;
extern prec grad_stp;
extern prec gsum_phi;
extern prec gsum_ovfl;
extern prec gsum_ovf_area;
extern prec overflowMin;
extern prec mGP3D_opt_phi_cof;
extern prec mGP2D_opt_phi_cof;
extern prec cGP3D_opt_phi_cof;
extern prec cGP2D_opt_phi_cof;
extern prec inv_RAND_MAX;
extern prec stn_weight;  // lutong
extern prec opt_w_cof;   // lutong

extern unsigned extPt1_2ndOrder;
extern unsigned extPt2_2ndOrder;
extern unsigned extPt1_1stOrder;
extern unsigned extPt2_1stOrder;
extern unsigned extPt3_1stOrder;

extern char gbch_dir[BUF_SZ];
extern char gbch[BUF_SZ];
extern char gGP_pl[BUF_SZ];
extern char gIP_pl[BUF_SZ];
extern char gGP_pl_file[BUF_SZ];
extern char gmGP2D_pl[BUF_SZ];
extern char gGP3_pl[BUF_SZ];
extern char gLG_pl[BUF_SZ];
extern char gDP_log[BUF_SZ];
extern char gDP_pl[BUF_SZ];
extern char gDP_tmp[BUF_SZ];
extern char gDP2_pl[BUF_SZ];
extern char gDP3_pl[BUF_SZ];
extern char gGR_dir[BUF_SZ];
extern char gGR_log[BUF_SZ];
extern char gGR_tmp[BUF_SZ];
extern char gFinal_DP_pl[BUF_SZ];
extern char bench_aux[BUF_SZ];
extern char dir_bnd[BUF_SZ];
extern char global_router[1023];
extern char output_dir[BUF_SZ];
extern char currentDir[BUF_SZ];

extern std::string sourceCodeDir;

extern bool isRoutabilityInit;
extern bool isTrial;
extern bool isFirst_gp_opt;
extern bool DEN_ONLY_PRECON;
extern int orderHPWL;

extern std::vector< std::pair< int, prec > > inflationList;
extern std::vector< std::pair< prec, prec > > trial_HPWLs;
extern std::vector< prec > trial_POTNs;
extern std::vector< std::pair< prec, prec > > hpwlEPs_1stOrder;
extern std::vector< std::pair< prec, prec > > hpwlEPs_2ndOrder;
extern std::vector< prec > potnEPs;
extern std::map< std::string, std::vector< int > > routeBlockageNodes;
extern int nXgrids, nYgrids, nMetLayers;
extern std::vector< int > verticalCapacity;
extern std::vector< int > horizontalCapacity;
extern std::vector< prec > minWireWidth;
extern std::vector< prec > minWireSpacing;
extern std::vector< prec > viaSpacing;
extern std::vector< std::tuple< int, int, int, int, int, int, int > > edgeCapAdj;
extern prec gridLLx, gridLLy;
extern prec tileWidth, tileHeight;
extern prec blockagePorosity;

extern PIN *pinInstance;
extern MODULE *moduleInstance;
extern CELL *gcell_st;
extern TERM *terminalInstance;

extern NET *netInstance;
extern HASH_MAP< std::string, int > netNameMap;

// structure for *.scl
extern ROW *row_st;
extern int row_cnt;

// ??
extern PLACE *place_st;
extern PLACE *place_backup_st;  // why?
extern int place_st_cnt;

// ??
extern PLACE place;
extern PLACE place_backup;  //  why?

extern FPOS avgCellSize;
extern FPOS avgTerminalSize;

extern FPOS term_pmax;
extern FPOS term_pmin;

extern FPOS filler_size;

extern POS msh;

// global Space Min/Max
extern FPOS gmin;
extern FPOS gmax;

extern FPOS gwid;
extern TIER *tier_st;
extern POS dim_bin;
extern POS dim_bin_mGP2D;
extern POS dim_bin_cGP2D;

extern FPOS grow_pmin;
extern FPOS grow_pmax;

///////////////////////////////////////////////////////////////////////////
/*  ARGUMENTS: main.cpp                                                  */
///////////////////////////////////////////////////////////////////////////
extern std::string bmFlagCMD;
extern std::string auxCMD;
extern std::string defName;
extern std::string verilogName;
extern std::string sdcName;
extern std::vector< std::string > lefStor;
extern std::string outputCMD;
extern std::string experimentCMD;
extern std::vector< std::string > libStor;
extern std::string verilogTopModule;
extern int defMacroCnt;
extern int numInitPlaceIter;

extern std::string benchName;

extern int numThread;
enum class InputMode { bookshelf, lefdef };
extern InputMode inputMode;

extern std::string racntiCMD;    // lutong
extern std::string maxinflCMD;   // lutong
extern std::string inflcoefCMD;  // lutong
extern std::string filleriterCMD;
extern prec refDeltaWL;
extern int conges_eval_methodCMD;  // grouter | prob

extern bool isVerbose;
extern bool isPlot;
extern bool plotCellCMD;
extern bool plotMacroCMD;
extern bool plotDensityCMD;
extern bool plotFieldCMD;
extern bool constraintDrivenCMD;
extern bool isRoutability;
extern bool lambda2CMD;
extern bool dynamicStepCMD;
extern bool thermalAwarePlaceCMD;
extern bool isOnlyGlobalPlace;
extern bool isSkipIP;
extern bool isTiming;
extern bool stnCMD;  // lutong
extern bool trialRunCMD;
extern bool autoEvalRC_CMD;
extern bool onlyLG_CMD;
extern bool isFastMode;

extern Tcl_Interp* _interp;

//////////////////////////////////////////////////////////////////////////
// Defined in main.cpp ///////////////////////////////////////////////////
void init();
void initialPlacement_main(void);
void trial_main(void);
void free_trial_mallocs(void);
void tmGP3DglobalPlacement_main(void);
void tmGP2DglobalPlacement_main(void);
void tcGP3DglobalPlacement_main(void);
void tcGP2DglobalPlacement_main(void);
void mGP3DglobalPlacement_main(void);
void mGP2DglobalPlacement_main(void);
void cGP3DglobalPlacement_main(void);
void cGP2DglobalPlacement_main(void);
void macroLegalization_main(void);
void calcRef_dWL(void);
void findMinHPWLduringTrial(void);
void findMaxHPWLduringTrial(void);
void get2ndOrderEP1(void);
void get2ndOrderEP2(void);
bool isLargestGapHPWL_1stEP(void);
void sort2ndOrderEPs(void);
void printTrend(void);
void get2ndOrderDiff_HPWL_LinearHPWLtrend(void);
void get1stOrderDiff_HPWL_LinearHPWLtrendforThird();
void get1stOrderDiff_HPWL_LinearHPWLtrendforSecond(void);
void get1stOrder_ExtremePointsforThird(void);
void get1stOrder_ExtremePointsforSecond(void);
void store2ndOrder_ExtremePoints(void);
void store_POTNs(void);
void store1stOrder_ExtremePointsforThird(void);
void store1stOrder_ExtremePointsforSecond(void);
void reassign_trial_2ndOrder_lastEP(prec);
void printEPs(void);

void printUsage(void);
void initArgument(int, char **);
void initGlobalVars();
void initGlobalVarsAfterParse();

void calcTSVweight(void);
bool argument(int, char **);
void printCMD(int, char **);
bool criticalArgumentError(void);

int pos_eqv(struct POS p1, struct POS p2);
void overlap_count(int iter);
void update_net_by_pin(void);

inline int dge(prec a, prec b) {
  return (a > b || a == b) ? 1 : 0;
}
inline int dle(prec a, prec b) {
  return (a < b || a == b) ? 1 : 0;
}

void OR_opt(void);
void rdft2dsort(int, int, int, prec **);

void read_macro(char *fn);


void tier_assign(int);
void tier_assign_with_macro(void);
void tier_assign_without_macro(void);
int prec_cmp(const void *a, const void *b);
int min_tier_cmp(const void *a, const void *b);
int max_area_dis_div_cmp(const void *a, const void *b);

void preprocess_SB_inputs(char *tier_dir);
void postprocess_SB_inputs(char *tier_dir);
void init_tier(void);
void tot_area_comp(void);
void calc_tier_WS(void);
void post_mac_tier(void);
void pre_mac_tier(void);

inline prec getStepSizefromEPs(prec hpwl, prec hpwlEP, prec hpwlSTD,
                               prec basePCOF, prec baseRange) {
  return std::min(basePCOF + baseRange, basePCOF +
                                       baseRange * ((prec)fabs(hpwl - hpwlSTD) /
                                                    (prec)fabs(hpwlEP - hpwlSTD)));
}

// writing Bookshelf function
void WriteBookshelf();
void WriteBookshelfForGR();

void CallDetailPlace();
void CallNtuPlacer3(const char *tier_dir, const char *tier_aux, const char *tier_pl);
void CallNtuPlacer4h(const char *tier_dir, const char *tier_aux, const char *tier_pl);


// 
// Some utils for RePlAce.
//
#include "util.h"

#endif

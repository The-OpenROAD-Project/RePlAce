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
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#include <cfloat>
#include <cstring>

#define PI 3.141592653589793238462L
#define SQRT2 1.414213562373095048801L
#define INV_SQRT2 0.707106781186547524401L

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
#define SA_LG
#define FILLER_ADJ RandomAdj
#define TIER_DEP /* 64.0 */ /* 600.0 */ 1
#define TIER_Z0 0
#define MSH_Z_RES /* 8 */ 1
#define THETA_XY_3D_PLOT PI / 6.0
#define Z_SCAL 1.00

// GP_Dimension_one,
// 1: true, 0: false
#define GP_DIM_ONE_FOR_IP 1
#define GP_DIM_ONE_FOR_GP1 1
#define GP_DIM_ONE_FOR_GP2 1
#define GP_DIM_ONE_FOR_MAC 1

#define GP_DIM_ONE_FOR_mGP2D 0
#define GP_DIM_ONE_FOR_GP3 0

#define GP_SCAL /* 1000.0 */ /* 1.0 */ /* 0.001 */ 0.0002
#define DEN_GRAD_SCALE 1.0 /* 0.1 */ /* 0.5 */ /* 0.25 */ /* 0.125 */
#define LAYER_ASSIGN_3DIC MIN_TIER_ORDER                  /* MAX_AREA_DIS_DIV */

///////////////////////////////////////////////////////////////////////////

using std::string;
using std::cout;
using std::endl;

using std::vector;
using std::pair;
using std::tuple;
using std::max;
using std::min;

using std::fixed;
using std::scientific;
using std::map;
using std::to_string;

using std::numeric_limits;
using std::make_pair;
using google::dense_hash_map;
using google::dense_hash_set;

struct POS;

struct FPOS {
  prec x;
  prec y;
//  prec z;

  FPOS() {
    SetZero();
  };

  FPOS(prec _x, prec _y) : x(_x), y(_y){};

  inline void Set(prec a) {
    x = y = a;
  }
  inline void Set(FPOS a) {
    x = a.x;
    y = a.y;
  }
  inline void Set(prec _x, prec _y, prec _z) {
    x = _x;
    y = _y;
  }

  inline void Set(POS a);

  inline void SetZero() {
    x = y = 0.0f;
  }
  inline void Add(FPOS a) {
    x += a.x;
    y += a.y;
  }
  inline void SetAdd(FPOS a, FPOS b) {
    x = a.x + b.x;
    y = a.y + b.y;
  }
  inline void Min(FPOS a) {
    x = min(x, a.x);
    y = min(y, a.y);
  }
  inline void SetMin(FPOS a, FPOS b) {
    x = min(a.x, b.x);
    y = min(a.y, b.y);
  }
  inline void Max(FPOS a) {
    x = max(x, a.x);
    y = max(y, a.y);
  }
  inline void SetMax(FPOS a, FPOS b) {
    x = max(a.x, b.x);
    y = max(a.y, b.y);
  }

  inline prec GetProduct() {
    return x * y;
  }
  inline void Dump() {
    cout << "(" << x << " " << y << ")" << endl;
  }
  inline void Dump(string a) {
    cout << a << ": (" << x << " " << y << ")" << endl;
  }
};

struct POS {
  int x;
  int y;

  POS() {
    SetZero();
  };

  POS(int _x, int _y) : x(_x), y(_y){};

  inline void Set(int a) {
    x = y = a;
  }
  inline void Set(POS a) {
    x = a.x;
    y = a.y;
  }

  inline void Set(int _x, int _y){
    x = _x;
    y = _y;
  }

  inline void Set(FPOS fp);

  inline void SetZero() {
    x = y = 0.0f;
  }
  inline void Add(POS a) {
    x += a.x;
    y += a.y;
  }

  inline void SetAdd(POS a, POS b) {
    x = a.x + b.x;
    y = a.y + b.y;
  }
  inline void Min(POS a) {
    x = min(x, a.x);
    y = min(y, a.y);
  }
  inline void SetMin(POS a, POS b) {
    x = min(a.x, b.x);
    y = min(a.y, b.y);
  }
  inline void Max(POS a) {
    x = max(x, a.x);
    y = max(y, a.y);
  }
  inline void SetMax(POS a, POS b) {
    x = max(a.x, b.x);
    y = max(a.y, b.y);
  }
  inline int GetProduct() {
    return x * y ;
  }
  inline void SetXProjection(int a, int b) {
    x = (x < a) ? a : (x > b) ? b : x;
  }
  inline void SetYProjection(int a, int b) {
    y = (y < a) ? a : (y > b) ? b : y;
  }
  inline void SetProjection(POS a, POS b) {
    SetXProjection(a.x, b.x);
    SetYProjection(a.y, b.y);
  }
  inline void SetXYProjection(POS a, POS b) {
    SetXProjection(a.x, b.x);
    SetYProjection(a.y, b.y);
  }
  inline void Dump() {
    cout << "(" << x << " " << y << ")" << endl;
  }
  inline void Dump(string a) {
    cout << a << ": (" << x << " " << y << ")" << endl;
  }
};

inline void FPOS::Set(POS p) {
  x = p.x;
  y = p.y;
}

inline void POS::Set(FPOS fp) {
  x = (int)(fp.x + 0.5f);
  y = (int)(fp.y + 0.5f);
}

struct RECT {
  FPOS pmin;
  FPOS pmax;
  RECT() {
    pmin.SetZero();
    pmax.SetZero();
  };
  void Dump() {
    pmin.Dump("RECT: pmin");
    pmax.Dump("RECT: pmax");
    cout << endl;
  }
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
  int tier;
  int X_MIN;
  int Y_MIN;
  int X_MAX;
  int Y_MAX;
  PIN() {
    fp.SetZero();
    e1.SetZero();
    e2.SetZero();
    flg1.SetZero();
    flg2.SetZero();
  }
};

// for saving pinName
// If it's lied in the PIN structure, it'll enlarge the runtime
extern vector< vector< string > > mPinName;
extern vector< vector< string > > tPinName;

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
  char name[255];
  int idx;
  int netCNTinObject;
  int pinCNTinObject;
  int flg;
  int tier;
  int mac_idx;
  int ovlp_flg;
  POS pmin_lg;
  POS pmax_lg;

  MODULE()
      : pof(0),
        pin(0),
        area(0.0f),
        name(""),
        idx(0),
        netCNTinObject(0),
        pinCNTinObject(0),
        flg(0),
        tier(0),
        mac_idx(0),
        ovlp_flg(0) {
    pmin.SetZero();
    pmax.SetZero();
    size.SetZero();
    half_size.SetZero();
    center.SetZero();
    pmin_lg.SetZero();
    pmax_lg.SetZero();
  }
  void Dump(string a) {
    cout << idx << ", name: " << name << endl;
    cout << "tier: " << tier << endl;
    cout << "mac_idx: " << mac_idx << endl;
    cout << "ovlp_flg: " << ovlp_flg << endl;
    pmin.Dump("pmin");
    pmax.Dump("pmax");
    size.Dump("size");
    half_size.Dump("half_size");
    center.Dump("center");
    cout << "area: " << area << endl;
    cout << "netCNTinObject: " << netCNTinObject << endl;
    cout << "pinCNTinObject: " << pinCNTinObject << endl;
    pmin_lg.Dump("pmin_lg");
    pmax_lg.Dump("pmax_lg");
    cout << endl;
  }
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
  char name[255];
  int idx;
  int netCNTinObject;
  int pinCNTinObject;
  int IO;  // I -> 0; O -> 1
           //    int tier;
  bool isTerminalNI;

  prec PL_area;

  TERM()
      : area(0.0f),
        pof(0),
        pin(0),
        name(""),
        idx(0),
        netCNTinObject(0),
        pinCNTinObject(0),
        IO(0),
        isTerminalNI(0),
        PL_area(0.0f) {
    pmin.SetZero();
    pmax.SetZero();
    size.SetZero();
    center.SetZero();
  }

  void Dump() {
    printf("terminal[%d]: name: %s \n", idx, name);
    fflush(stdout);
    cout << "isTerminalNI: " << (isTerminalNI ? "YES" : "NO") << endl;
    cout << "IO: " << ((IO == 0) ? "Input" : "Output") << endl;
    //        cout << "tier: " << tier << endl;
    pmin.Dump("pmin");
    pmax.Dump("pmax");
    cout << "area: " << area << endl;
    size.Dump("size");
    center.Dump("center");
    cout << "netCNTinObject: " << netCNTinObject << endl;
    cout << "pinCNTinObject: " << pinCNTinObject << endl;
    cout << "PL_area: " << PL_area << endl;
    cout << endl;
  }
};

struct CELLx {
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
  char name[255];
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
};

typedef struct CELLx CELL;

class SHAPE {
 public:
  //    char *name;
  //    char *prefix;

  string name, instName;

  int idx;
  prec llx;
  prec lly;
  prec width;
  prec height;

  SHAPE(string _name, string _instName, int _idx, prec _llx, prec _lly,
        prec _width, prec _height)
      : name(_name),
        instName(_instName),
        idx(_idx),
        llx(_llx),
        lly(_lly),
        width(_width),
        height(_height){};

  void Dump() {
    printf(
        "shape[%d]: name: %s, instName: %s, llx: %lf, lly: %lf, width: %lf, "
        "height: %lf\n",
        idx, name.c_str(), instName.c_str(), llx, lly, width, height);
    fflush(stdout);
  }
};

class UFPin {
 public:
  // int id;
  int parent;
  int rank;
  int modu;

  UFPin() {
  }

  UFPin(int moduleID) {
    // id = 0;
    parent = moduleID;
    rank = 0;
    modu = moduleID;
  }

  ~UFPin() {
  }
};

class TwoPinNets {
 public:
  bool selected;
  int start_modu;
  int end_modu;
  int rect_dist;
  int i;
  int j;

  TwoPinNets() {
  }

  TwoPinNets(int start_modu, int end_modu, prec rect_dist, int i, int j) {
    selected = false;
    this->start_modu = start_modu;
    this->end_modu = end_modu;
    this->rect_dist = rect_dist;
    this->i = i;
    this->j = j;
  }

  ~TwoPinNets() {
  }
};

bool TwoPinNets_comp(TwoPinNets x, TwoPinNets y);

int UFFind(struct NET *net, int moduleID);
void UFUnion(struct NET *net, int idx, int idy);

class ROUTRACK {
 public:
  struct FPOS from;
  struct FPOS to;
  int layer;  // 1:M_Layer1, 2:M_Layer2, ..., etc.
  int netIdx;
  ROUTRACK() : layer(INT_MAX), netIdx(INT_MAX) { from.SetZero(); to.SetZero(); };
  ROUTRACK(struct FPOS _from, struct FPOS _to, int _layer, int _netIdx) {
    from.Set(_from);
    to.Set(_to);
    layer = _layer;
    netIdx = _netIdx;
  };
  void Dump() {
    from.Dump("from"); 
    to.Dump("to"); 
    cout << "layer: " << layer << endl;
    cout << "netIdx: " << netIdx << endl << endl;
  }
};

class NET {
  public:
  char name[255];
  std::map< int, UFPin > mUFPin;
  vector< TwoPinNets > two_pin_nets;
  vector< ROUTRACK > routing_tracks;

  prec min_x;
  prec min_y;
  prec min_z;
  prec max_x;
  prec max_y;
  prec max_z;
  FPOS sum_num1;
  FPOS sum_num2;
  FPOS sum_denom1;
  FPOS sum_denom2;
  PIN **pin;
  PIN **pin2;
  FPOS terminalMin;
  FPOS terminalMax;

  prec hpwl_x;
  prec hpwl_y;
  prec hpwl_z;
  prec hpwl;
  int outPinIdx;  // determine outpin's index
  int pinCNTinObject;
  int pinCNTinObject2;
  int pinCNTinObject_tier;  // used for writing bookshelf
  int idx;
  int mod_idx;
  prec timingWeight;  // mgwoo
  prec stn_cof;       // lutong
  prec wl_rsmt;       // lutong

  NET() : name(""), min_x(PREC_MAX), min_y(PREC_MAX), 
  min_z(PREC_MAX), max_x(PREC_MIN), max_y(PREC_MIN), max_z(PREC_MIN),
  pin(0), pin2(0), hpwl_x(PREC_MIN), hpwl_y(PREC_MIN), hpwl_z(PREC_MIN), 
  outPinIdx(INT_MAX), pinCNTinObject(INT_MAX), pinCNTinObject2(INT_MAX),
  pinCNTinObject_tier(INT_MAX), idx(INT_MAX), mod_idx(INT_MAX), 
  timingWeight(0.0f), 
  stn_cof(0.0f),
  wl_rsmt(0.0f) { 
    sum_num1.SetZero();
    sum_num2.SetZero();
    sum_denom1.SetZero();
    sum_denom2.SetZero();
    terminalMin.SetZero();
    terminalMax.SetZero();
  };
};

struct T0 {
  int z;
  prec dis;
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
  void Dump(string a) {
    cout << a << endl;
    org.Dump("origin");
    end.Dump("end");
    center.Dump("center");
    stp.Dump("stp");
    cnt.Dump("cnt");
    cout << endl;
  }
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
  struct CELLx **cell_st;
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
  int row_term_cnt;
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
  struct CELLx **cell_st_tmp;

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

// for timing
extern prec netCut;
extern bool hasNetWeight;
extern prec netWeight;
extern prec netWeightMin;
extern prec netWeightMax;

extern prec netWeightBase;
extern prec netWeightBound;
extern prec netWeightScale;

extern prec capPerMicron;
extern prec resPerMicron;

extern bool isClockGiven;
extern prec timingClock;
extern string clockPinName;

extern int timingUpdateIter;
extern int pinCNT;
extern int moduleCNT;
extern int gcell_cnt;

extern string globalRouterPosition;
extern string globalRouterSetPosition;
extern prec globalRouterCapRatio;

enum { FillerCell, StdCell, Macro };
extern int terminalCNT;
extern int netCNT;
extern int numNonRectangularNodes;
extern int totalShapeCount;

extern int g_rrr;
extern int STAGE;

extern vector< SHAPE > shapeStor;
extern dense_hash_map< string, vector< int > > shapeMap;

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
enum { None, FastPlace, NTUplace3, NTUplace4h };
extern string detailPlacerLocationCMD;
extern string detailPlacerFlagCMD;

extern bool isOnlyLGinDP;
extern bool isSkipPlacement;
extern bool hasDensityDP;
extern bool isSkipIP;
extern bool isBinSet;
extern bool isNtuDummyFill;
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
extern int GP_DIM_ONE;
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
extern prec global_ovfl;  ////////////
extern prec maxALPHA;
extern prec ExtraWSfor3D;
extern prec MaxExtraWSfor3D;
extern prec rowHeight;
extern prec SITE_SPA;
extern prec layout_area;
extern double tot_HPWL;
extern prec tx_HPWL;
extern prec ty_HPWL;
extern prec tz_HPWL;
extern prec tot_overlap;
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
extern long total_termPL_area;
extern long total_WS_area;

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
extern prec theta;
extern prec dp_margin_per_tier;
extern prec stn_weight;  // lutong
extern prec opt_w_cof;   // lutong

extern unsigned extPt1_2ndOrder;
extern unsigned extPt2_2ndOrder;
extern unsigned extPt1_1stOrder;
extern unsigned extPt2_1stOrder;
extern unsigned extPt3_1stOrder;

extern char gbch_dir[BUF_SZ];
extern char gbch_aux[BUF_SZ];
extern char gbch[BUF_SZ];
extern char gGP_dir[BUF_SZ];
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
extern char gTMP_bch_dir[BUF_SZ];
extern char gTMP_bch_aux[BUF_SZ];
extern char gTMP_bch_nodes[BUF_SZ];
extern char gTMP_bch_nets[BUF_SZ];
extern char gTMP_bch_wts[BUF_SZ];
extern char gTMP_bch_pl[BUF_SZ];
extern char gTMP_bch_scl[BUF_SZ];
extern char sing_fn_aux[BUF_SZ];
extern char sing_fn_nets[BUF_SZ];
extern char sing_fn_nodes[BUF_SZ];
extern char sing_fn_pl[BUF_SZ];
extern char sing_fn_wts[BUF_SZ];
extern char sing_fn_scl[BUF_SZ];
extern char fn_bch_IP[BUF_SZ];
extern char fn_bch_GP[BUF_SZ];
extern char fn_bch_GP2[BUF_SZ];
extern char fn_bch_GP3[BUF_SZ];
extern char fn_bch_mac_LG[BUF_SZ];
extern char fn_bch_DP[BUF_SZ];
extern char bench_aux[BUF_SZ];
extern char dir_bnd[BUF_SZ];
extern char global_router[1023];
extern char output_dir[BUF_SZ];
extern char currentDir[BUF_SZ];

extern string sourceCodeDir;

extern bool isBloatingBegin;
extern bool isRoutabilityInit;
extern bool isTrial;
extern bool isFirst_gp_opt;
extern bool DEN_ONLY_PRECON;
extern int orderHPWL;

extern vector< std::pair< int, prec > > inflationList;
extern vector< std::pair< prec, prec > > trial_HPWLs;
extern vector< prec > trial_POTNs;
extern vector< std::pair< prec, prec > > hpwlEPs_1stOrder;
extern vector< std::pair< prec, prec > > hpwlEPs_2ndOrder;
extern vector< prec > potnEPs;
extern std::map< string, vector< int > > routeBlockageNodes;
extern int nXgrids, nYgrids, nMetLayers;
extern vector< int > verticalCapacity;
extern vector< int > horizontalCapacity;
extern vector< prec > minWireWidth;
extern vector< prec > minWireSpacing;
extern vector< prec > viaSpacing;
extern vector< tuple< int, int, int, int, int, int, int > > edgeCapAdj;
extern prec gridLLx, gridLLy;
extern prec tileWidth, tileHeight;
extern prec blockagePorosity;

extern RECT cur_rect;
extern PIN *pinInstance;
extern MODULE *moduleInstance;
extern CELLx *gcell_st;
extern TERM *terminalInstance;

extern NET *netInstance;
extern dense_hash_map< string, int > netNameMap;

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

extern FPOS zeroFPoint;
extern POS zeroPoint;

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
extern string bmFlagCMD;
extern string auxCMD;
extern string defCMD;
extern string verilogCMD;
extern string sdcCMD;
extern vector< string > lefStor;
extern string outputCMD;
extern string experimentCMD;
extern vector< string > libStor;
extern string verilogTopModule;
extern int defMacroCnt;
extern int numInitPlaceIter;

extern string benchName;

extern int numThread;
enum class InputMode { bookshelf, lefdef };
extern InputMode inputMode;

extern string denCMD;
extern string bxMaxCMD;
extern string byMaxCMD;
extern string bzMaxCMD;
extern string racntiCMD;    // lutong
extern string maxinflCMD;   // lutong
extern string inflcoefCMD;  // lutong
extern string filleriterCMD;
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
extern bool onlyGlobalPlaceCMD;
extern bool isSkipIP;
extern bool isTiming;
extern bool isARbyUserCMD;
extern bool stnCMD;  // lutong
extern bool trialRunCMD;
extern bool autoEvalRC_CMD;
extern bool onlyLG_CMD;

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
prec get_abs(prec a);
void store2ndOrder_ExtremePoints(void);
void store_POTNs(void);
void store1stOrder_ExtremePointsforThird(void);
void store1stOrder_ExtremePointsforSecond(void);
void reassign_trial_2ndOrder_lastEP(prec);
void printEPs(void);

void printUsage(void);
void initArgument(int, char **);
void calcTSVweight(void);
bool argument(int, char **);
void printCMD(int, char **);
bool criticalArgumentError(void);

string getexepath();
int pos_eqv(struct POS p1, struct POS p2);
int prec_eqv(prec x, prec y);
int prec2int(prec a);
unsigned prec2unsigned(prec a);
int find_non_zero(prec *a, int cnt);
int prec_le(prec x, prec y);
int prec_ge(prec x, prec y);
int prec_lt(prec x, prec y);
int prec_gt(prec x, prec y);
int HPWL_count(void);
void overlap_count(int iter);
void update_net_by_pin(void);
void time_start(double *time_cost);
void time_end(double *time_cost);
void time_calc(double last_time, double *curr_time, double *time_cost);
void itoa(int n, char k[]);

inline int dge(prec a, prec b) {
  return (a > b || a == b) ? 1 : 0;
}
inline int dle(prec a, prec b) {
  return (a < b || a == b) ? 1 : 0;
}

void OR_opt(void);
void rdft2dsort(int, int, int, prec **);

// void call_DP(void);
// void call_FastDP(void);
// void call_NTUpl3(void);
// void call_NTUpl4h(void);
// void call_DP_StdCell_NTUpl4h(void);
void read_macro(char *fn);

FPOS fp_mul(struct FPOS a, struct FPOS b);
inline FPOS fp_add(struct FPOS a, struct FPOS b) {
  struct FPOS c = zeroFPoint;
  c.x = a.x + b.x;
  c.y = a.y + b.y;
  return c;
}
FPOS fp_add_abs(struct FPOS a, struct FPOS b);
inline FPOS fp_scal(prec s, struct FPOS a) {
  struct FPOS c = a;
  c.x *= s;
  c.y *= s;
  return c;
}
FPOS fp_subt(struct FPOS a, struct FPOS b);
FPOS fp_subt_const(struct FPOS a, prec b);
prec fp_sum(struct FPOS a);
FPOS fp_exp(struct FPOS a);
prec fp_product(struct FPOS a);

int p_product(struct POS a);
int p_max(struct POS a);

FPOS fp_min2(struct FPOS a, struct FPOS b);
FPOS fp_max2(struct FPOS a, struct FPOS b);
FPOS fp_div(struct FPOS a, struct FPOS b);
FPOS fp_rand(void);
FPOS p2fp(struct POS a);
POS fp2p_floor(struct FPOS a);
POS fp2p_ceil(struct FPOS a);
FPOS fp_inv(struct FPOS a);

void tier_assign(int);
void tier_assign_with_macro(void);
void tier_assign_without_macro(void);
void find_close_tier(prec z, struct T0 *t0_st, int *z_st);
int prec_cmp(const void *a, const void *b);
int max_pinCNTinObject_cmp(const void *a, const void *b);
int min_tier_cmp(const void *a, const void *b);
int max_area_dis_div_cmp(const void *a, const void *b);

void call_FastDP_tier(char *tier_dir, char *tier_aux, char *tier_pl);
// void call_NTUpl3_tier(char *tier_dir, char *tier_aux, char *tier_pl);
// void call_NTUpl4h_tier(char *tier_dir, char *tier_aux, char *tier_pl);

void preprocess_SB_inputs(char *tier_dir);
void postprocess_SB_inputs(char *tier_dir);
void init_tier(void);
void tot_area_comp(void);
void calc_tier_WS(void);
void post_mac_tier(void);
void pre_mac_tier(void);
inline prec getStepSizefromEPs(prec hpwl, prec hpwlEP, prec hpwlSTD,
                               prec basePCOF, prec baseRange) {
  return min(basePCOF + baseRange, basePCOF +
                                       baseRange * (get_abs(hpwl - hpwlSTD) /
                                                    get_abs(hpwlEP - hpwlSTD)));
}

// writing Bookshelf function
void WriteBookshelf();
void WriteBookshelfForGR();

void CallDetailPlace();
void CallNtuPlacer3(const char *tier_dir, const char *tier_aux, const char *tier_pl);
void CallNtuPlacer4h(const char *tier_dir, const char *tier_aux, const char *tier_pl);

// useful function

// return Common Area
// between Rectangle A and Rectangle B.
// type : casted long from prec
inline long lGetCommonAreaXY(FPOS aLL, FPOS aUR, FPOS bLL, FPOS bUR) {
  long xLL = max(aLL.x, bLL.x), yLL = max(aLL.y, bLL.y),
       xUR = min(aUR.x, bUR.x), yUR = min(aUR.y, bUR.y);

  if(xLL >= xUR || yLL >= yUR) {
    return 0;
  }
  else {
    return (xUR - xLL) * (yUR - yLL);
  }
}

// return Common Area
// between Rectangle A and Rectangle B.
// type : integer
inline int iGetCommonAreaXY(POS aLL, POS aUR, POS bLL, POS bUR) {
  int xLL = max(aLL.x, bLL.x), yLL = max(aLL.y, bLL.y), xUR = min(aUR.x, bUR.x),
      yUR = min(aUR.y, bUR.y);

  if(xLL >= xUR || yLL >= yUR) {
    return 0;
  }
  else {
    return (xUR - xLL) * (yUR - yLL);
  }
}

// return Common Area
// between Rectangle A and Rectangle B.
// type : prec
inline prec pGetCommonAreaXY(FPOS aLL, FPOS aUR, FPOS bLL, FPOS bUR) {
  prec xLL = max(aLL.x, bLL.x), yLL = max(aLL.y, bLL.y),
       xUR = min(aUR.x, bUR.x), yUR = min(aUR.y, bUR.y);

  if(xLL >= xUR || yLL >= yUR) {
    return 0;
  }
  else {
    return (xUR - xLL) * (yUR - yLL);
  }
}

// for string escape
inline bool ReplaceStringInPlace(std::string &subject,
                                 const std::string &search,
                                 const std::string &replace) {
  size_t pos = 0;
  bool isFound = false;
  while((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
    isFound = true;
  }
  return isFound;
}

inline void SetEscapedStr(std::string &inp) {
  if(ReplaceStringInPlace(inp, "/", "\\/")) {
    ReplaceStringInPlace(inp, "[", "\\[");
    ReplaceStringInPlace(inp, "]", "\\]");
  }
}

inline char *GetEscapedStr(const char *name, bool isEscape = true) {
  std::string tmp(name);
  if( isEscape ) {
    SetEscapedStr(tmp);
  }
  return strdup(tmp.c_str());
}

inline string GetRealPath(string path ) {
  char tmp[PATH_MAX] = {0, };
  char* ptr = realpath(path.c_str(), tmp);
  return string(tmp);
}


#endif

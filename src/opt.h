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

#ifndef __PL_OPT__
#define __PL_OPT__

#include <vector>
#include "replace_private.h"

extern int FILLER_PLACE;
extern int NUM_ITER_FILLER_PLACE;  // fast filler pre-place, need tuning
extern prec opt_phi_cof;
extern prec opt_phi_cof_local;
extern prec gsum_pcnt;
extern prec gsum_area;
extern prec avg80p_cell_area;
extern prec refDeltaWL;
extern prec LOWER_PCOF, UPPER_PCOF;
extern prec INIT_LAMBDA_COF_GP;
extern prec MIN_PRE;
extern FPOS avg80p_cell_dim;

struct DST {
  int idx;
  prec dxy;
};

struct ABC {
  int idx;
  prec val;
  struct FPOS p;
};

struct ITER {
  struct FPOS wlen;
  struct FPOS hpwl;
  struct FPOS lc_dim;
  struct FPOS wcof;
  prec grad;
  prec tot_wlen;
  prec tot_hpwl;
  prec tot_stnwl;  // lutong
  prec tot_wwl;    // lutong
  prec potn;
  prec ovfl;
  prec pcof;
  int idx;
  prec beta;
  prec dis00;
  prec alpha00;
  prec lc;
  prec lc_w;
  prec lc_p;
  double cpu_curr, cpu_cost;
  struct FPOS alpha_dim;
};

// routability
void routability();
void routability_init();
void FLUTE_init();  // lutong
void clean_routing_tracks_in_net();
void restore_cells_info();
void bloating();
void bloat_prep();
void adjust_inflation();
void cell_calc_new_area_per_Cell(struct CELL *cell, struct TILE *bp);
void shrink_filler_cells(prec);
void calc_Total_inflate_ratio();
void calc_Total_inflated_cell_area();
void buildRSMT_FLUTE(struct FPOS *st);                          // lutong
void buildRSMT_FLUTE_PerNet(struct FPOS *st, struct NET *net);  // lutong
void buildRMST(struct FPOS *st);
void buildRMST2(struct FPOS *st);
void buildRMSTPerNet(struct FPOS *st, struct NET *net);
void buildRMSTPerNet_genTwoPinNets(struct FPOS *st, struct NET *net);
void buildRMSTPerNet_genRMST(struct NET *net);
void buildRMSTPerNet_printRMST(struct FPOS *st, struct NET *net);
void clearTwoPinNets();
void calcCong_print();
void calcCong_print_detail();
void calcCongPerNet_prob_based(struct FPOS *st, struct NET *net);
void calcCongPerNet_grouter_based(struct NET *net);
void calcCong(struct FPOS *st, int est_method);
void CalcPinDensity(struct FPOS *st);
void MergePinDen2Route();
void MergeBlkg2Route();
void CalcPinDensityPerCell(struct CELL *cell);
void calcInflationRatio_foreachTile();
void gen_sort_InflationList();
bool inflationList_comp(std::pair< int, prec > a, std::pair< int, prec > b);
void dynamicInflationAdjustment();
void printInflationRatio();
void cell_inflation_per_Cell(struct CELL *cell, struct TILE *bp);
void cell_den_scal_update_forNewGrad_inNSopt(struct CELL *cell);
void print_inflation_list();

// PIN
void backup_org_PIN_info(struct PIN *pin);
void prepare_bloat_PIN_info();
void restore_org_PIN_info();
// MODULE
void backup_org_MODULE_info(struct MODULE *module);
void prepare_bloat_MODULE_info();
void restore_org_MODULE_info();
// CELL
void backup_org_CELL_info(struct CELL *cell);
void prepare_bloat_CELL_info();
void restore_org_CELL_info(struct CELL *cell);
// TERM
void backup_org_TERM_info(struct TERM *term);
void prepare_bloat_TERM_info();
void restore_org_TERM_info();

prec get_norm(struct FPOS *st, int n, prec num);

void setup_before_opt(void);
int setup_before_opt_mGP2D(void);
int setup_before_opt_cGP2D(void);
inline bool isPOTNuphill(void);
int definePOTNphase(prec);
void stepSizeAdaptation_by2ndOrderEPs(prec);
void stepSizeAdaptation_by1stOrderEPs(prec);
void msh_init();
int post_mGP2D_delete(void);

void post_opt(void);

void cell_init(void);

void cell_filler_init();

void whitespace_init(void);

void cell_update(struct FPOS *st, int n);
void cell_delete(void);

void input_sol(struct FPOS *st, int N, char *fn);

void UpdateModuleCoordiFromGcell(void);
void UpdateGcellCoordiFromModule(void);

prec get_dis(struct FPOS *a, struct FPOS *b, int N);
struct FPOS get_dis2(struct FPOS *a, struct FPOS *b, int N, prec num);

prec dmax(prec a, prec b);
prec dmin(prec a, prec b);

prec get_beta(struct FPOS *dx_st, struct FPOS *ldx_st, struct FPOS *ls_st,
              int N, int flg, int iter);

prec get_phi_cof(prec x);
prec get_phi_cof1(prec x);

void iter_update(struct FPOS *lx_st, struct FPOS *x_st, struct FPOS *dx_st,
                 int N, struct FPOS max_mov_L2, struct FPOS stop_mov_L2,
                 int iter, struct ITER *it, int lab);

void cg_input(struct FPOS *x_st, int N, int input);

int area_sort(const void *a, const void *b);

enum {
  NONE_INPUT,
  QWL_ISOL,
  ISOL_INPUT,
  WL_SOL_INPUT,
  IP_CEN_SQR,
  RANDOM_INPUT
};

enum { NONE_OUTPUT, QWL_OUTPUT, ISOL_OUTPUT, WL_SOL_OUTPUT, IP_CEN_SQR_OUTPUT };

#define OPT_INPUT QWL_ISOL /* IP_CEN_SQR */ /* RANDOM_INPUT */

#define WLEN_SCALE_FACTOR 0.25

#define MAX_WL_ITER 10

void getCostFuncGradient3(FPOS *dst, FPOS *wdst, FPOS *pdst, FPOS *pdstl, int n,
                          prec *cellLambdaArr);
void getCostFuncGradient2(FPOS *dst, FPOS *wdst, FPOS *pdst, FPOS *pdstl, int n,
                          prec *cellLambdaArr);
void getCostFuncGradient2_filler(FPOS *dst, FPOS *wdst, FPOS *pdst, FPOS *pdstl,
                                 int start_idx, int end_idx,
                                 prec *cellLambdaArr);
void getCostFuncGradient2_DEN_ONLY_PRECON(FPOS *dst, FPOS *wdst, FPOS *pdst,
                                          FPOS *pdstl, int n,
                                          prec *cellLambdaArr);
void getCostFuncGradient2_filler_DEN_ONLY_PRECON(FPOS *dst, FPOS *wdst,
                                                 FPOS *pdst, FPOS *pdstl,
                                                 int start_idx, int end_idx,
                                                 prec *cellLambdaArr);

void get_lc(struct FPOS *y_st, struct FPOS *y_dst, struct FPOS *z_st,
            struct FPOS *z_dst, struct ITER *iter, int N);
void get_lc3(struct FPOS *y_st, struct FPOS *y_dst, struct FPOS *z_st,
             struct FPOS *z_dst, struct ITER *iter, int N);
void get_lc3_filler(struct FPOS *y_st, struct FPOS *y_dst, struct FPOS *z_st,
                    struct FPOS *z_dst, struct ITER *iter, int N);

void init_iter(struct ITER *it, int idx);

void cell_macro_copy(void);
void gp_opt(void);

void FillerCellRandPlace(int idx);
void sa_pl_shift(struct FPOS *x_st, int N);

void tier_delete_mGP2D(void);
void tier_init_2D(int);
void cell_init_2D(void);

//#define LC_REF_DIS
#define ref_yz_dis 50.0  // empirical from adaptec1, need deep tuning
#define z_ref_alpha      /* 0.0001 */ \
  0.01 /* 100.0 */       // empirical from adaptec1, need tuning

#define BACKTRACK

#define ratioVarPl 1.0

#define PRECON
//#define DEN_ONLY_PRECON  // prevents fillers from spreading // only SB uses
// DEN_ONLY_PRECON
// too fast in 3D-IC placement

#define MAX_BKTRK_CNT 10

#define tot_num_iter_var_pl 0

//#define INIT_LAMBDA_COF_GP 0.0001
// LW temp change

// inline prec   fastPow(prec   a, prec   b) {
//  // should be much more precise with large b
//  // calculate approximation with fraction of the exponent
//  int e = (int) b;
//  union {
//    prec   d;
//    int x[2];
//  } u = { a };
//  u.x[1] = (int)((b - e) * (u.x[1] - 1072632447) + 1072632447);
//  u.x[0] = 0;
//
//  // exponentiation by squaring with the exponent's integer part
//  // prec   r = u.d makes everything much slower, not sure why
//  prec   r = 1.0;
//  while (e) {
//    if (e & 1) {
//      r *= a;
//    }
//    a *= a;
//    e >>= 1;
//  }
//  return r * u.d;
//}

inline prec fastPow(prec a, prec b) {
  union {
    prec d;
    int x[2];
  } u = {a};
  u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
  u.x[0] = 0;
  return u.d;
}

inline prec fastExp(prec a) {
  a = 1.0 + a / 1024.0;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  return a;
}

#endif

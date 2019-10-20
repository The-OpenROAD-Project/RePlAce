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

#ifndef __PL_BIN__
#define __PL_BIN__

#include "replace_private.h"

extern int tot_bin_cnt;
extern int bin_cnt;
extern prec bin_area;
extern prec inv_bin_area;
extern prec global_macro_area_scale;

extern BIN *bin_mat;

extern FPOS bin_org;
extern FPOS bin_stp;
extern FPOS inv_bin_stp;
extern FPOS half_bin_stp;
extern FPOS bin_stp_mGP2D;
extern FPOS bin_stp_cGP2D;
extern POS max_bin;

// extern BIN **bin_list;
// extern int bin_list_cnt;

// extern POS *bin_st;

struct BIN {
  FPOS e;
  ////igkang
  //  FPOS    e_local;
  POS p;
  FPOS pmin;
  FPOS pmax;
  FPOS center;
  prec cell_area;
  prec cell_area2;

  prec virt_area;
  long term_area;  // mgwoo

  prec filler_area;
  prec phi;
  int flg;
  prec pl_area;
  prec macro_area;
  prec macro_area2;
  prec den;
  prec den2;
  prec no_mac_den;
  void dump(std::string a) {
    std::cout << a << std::endl;
    e.Dump("e");
    p.Dump("p");
    pmin.Dump("pmin");
    pmax.Dump("pmax");
    center.Dump("center");
    std::cout << std::endl;
  }
};

int idx_in_bin_rect(POS *p, POS pmin, POS pmax);

void bin_init();
void bin_init_2D(int);

void UpdateTerminalArea(TIER *tier, FPOS *pmin, FPOS *pmax);

void bin_update();
void bin_update7_cGP2D();
void bin_update7_mGP2D();

void bin_delete(void);

void get_bin_grad(BIN **bin, int max_x, int max_y);

void legal_bin_idx(POS *p);
POS get_bin_pt_from_point(FPOS p);

// calculating the da variable.
// using current da & half_size.
//
//
inline prec 
GetCoordiLayoutInsideAxis(prec center_coordi, prec half_size, int lab) {
  prec min_a = center_coordi - half_size;
  prec max_a = center_coordi + half_size;

  prec valid_center_coordi = center_coordi;

  // according to X
  if(lab == 0) {
    if(min_a < place.org.x)
      center_coordi += place.org.x - min_a;
    if(max_a > place.end.x)
      center_coordi -= max_a - place.end.x;
  }
  // accroding to Y
  else if(lab == 1) {
    if(min_a < place.org.y)
      center_coordi += place.org.y - min_a;
    if(max_a > place.end.y)
      center_coordi -= max_a - place.end.y;
  }
  return center_coordi;
}

// given center_coordi and half_size, 
// 
// return a valid center_coordi, 
// within place.org <= center_coordi <= place.end.
//
// half_size is considered.
//
inline FPOS 
GetCoordiLayoutInside(FPOS center_coordi, FPOS half_size) {
  FPOS v1;
  v1.x = GetCoordiLayoutInsideAxis(center_coordi.x, half_size.x, 0);
  v1.y = GetCoordiLayoutInsideAxis(center_coordi.y, half_size.y, 1);
  return v1;
}

int is_IO_block(TERM *term);

void get_bins(FPOS center, CELL *cell, POS *st, prec *share_st, int *bin_cnt);
//prec get_bins_mac(FPOS center, MODULE *mac);
void fft_test(void);

#define DEN_SMOOTH_COF 5.0
enum { SIN_SMOOTH, LIN_SMOOTH };
#define SMOOTH_LAB LIN_SMOOTH /* SIN_SMOOTH   */

int find_next_dbin(int *x0, int *y0);

void loc_ext(int x0, int y0, int dir, int flg);
int find_next_loc_opt(POS *p);

int point_in_range(prec a, prec min_a, prec max_a);

FPOS valid_coor4(FPOS center, FPOS half_sz);

//#define GRAD_SCAL
//#define GRAD_SCAL_COMB

prec get_ovfl();

void bin_den_update(FPOS *x_st, int N, int iter, int lab);
bool get_common_rect(FPOS pmin1, FPOS pmax1, FPOS pmin2, FPOS pmax2, FPOS *p1,
                     FPOS *p2);
prec get_den(prec area_num, prec area_denom);
prec get_den2(prec area_num, prec area_denom);

// return bin_mat's pointer
inline BIN *get_bin_from_idx(POS p) {
  return &bin_mat[ p.x * max_bin.y + p.y];
};

void add_net_to_bins(NET *ntp);
int point_in_rect(FPOS p0, FPOS pmin, FPOS pmax);

////// MACRO LG //////
void den_update();
void bin_den_mac_update(void);
void bin_cell_area_update(void);
void bin_clr(void);
prec get_mac_den(int idx);

POS get_bin_idx(int idx);

prec get_all_macro_den(void);

void bin_den_update_for_filler_adj(void);
//////-MACRO LG-//////

enum { NoneDpre, AreaDpre, DenDpre };
enum { NoneTemppre, AreaTemppre, DenTemppre };
#define CHARGE_PRE /* NoneDpre */ AreaDpre     /* DenDpre */
#define TEMP_PRE /* NoneTemppre */ AreaTemppre /* DenTemppre */

void den_comp(int cell_idx);
inline void den_comp_2d_mGP2D(CELL *cell, TIER *tier);
inline void den_comp_2d_cGP2D(CELL *cell, TIER *tier);
void den_comp_3d(int cell_idx);

// void    bin_zum_z ();
void bin_delete_mGP2D(void);
void bin_init_mGP2D(void);
void bin_init_cGP2D(void);

int compare(const void *p1, const void *p2);

#endif

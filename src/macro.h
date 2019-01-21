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

#ifndef __PL_MACRO__
#define __PL_MACRO__

extern int macro_cnt;
extern int *ovlp_y;
extern int org_mac_ovlp;
extern int tot_mac_ovlp;
extern int new_mac_ovlp;
extern int ovlp_free_flg;
extern int sa_max_iter;
extern int sa_max_iter2;
extern int sa_max_iter0;
extern int sa_iter_cof;
extern int tot_mac_area, tot_std_area;
extern int real_mac_area;
extern int tot_accept_cnt;
extern int tot_reject_cnt;
extern int cur_acc_cnt;
extern int cur_rej_cnt;
extern int sa_cnt;
extern int sa_x_gtz_cnt;
extern int sa_x_eqz_cnt;
extern int sa_x_ltz_cnt;
extern int sa_y_gtz_cnt;
extern int sa_y_eqz_cnt;
extern int sa_y_ltz_cnt;
extern int sa_z_gtz_cnt;
extern int sa_z_eqz_cnt;
extern int sa_z_ltz_cnt;
extern int sa_n_disp;
extern int ovlp_mac_cnt;
extern prec sa_hpwl_wgt, sa_hpwl_cof;
extern prec sa_den_wgt, sa_den_cof;
extern prec sa_ovlp_wgt, sa_ovlp_cof;
extern prec org_mac_hpwl, org_mac_den;
extern prec tot_mac_hpwl;
extern prec tot_mac_den;
extern prec new_mac_hpwl, new_mac_den;
extern prec mac_hpwl_xyz;
extern prec sa_init_neg_rate;
extern prec sa_last_neg_rate;
extern prec sa_t;
extern prec sa_init_t;
extern prec sa_t_cof;
extern prec MIN_SA_R_Z, MAX_SA_R_Z;

extern MODULE **macro_st;
extern FPOS sa_r;
extern FPOS max_sa_r;
extern FPOS min_sa_r;
extern FPOS sa_r_stp;
extern FPOS sa_ncof;
extern FPOS sa_n;
extern FPOS mac_hpwl;
extern FPOS sum_mac_sz, avg_mac_sz;
extern FPOS avg_mac_cnt_per_dim;
extern POS sa_u;

struct NODE {
  int x;
  int y1;
  int y2;
  int flg;
};
extern NODE *ovlp_node;

struct seg_tree_node {
  int l;
  int r;
  int ml;
  int mr;
  int s;
  int len;
};
extern seg_tree_node *ovlp_a;

void mac_ovlp_init(int n);
int ovlp_node_cmp(const void *a, const void *b);
int int_cmp(const void *a, const void *b);
void updata(int i, struct NODE b);
void callen(int i);
void build_seg_tree(int i, int left, int right);
int get_all_macro_ovlp(void);
int get_all_macro_ovlp_3d(void);
int get_mac_ovlp_top(void);
int get_mac_ovlp_3d_top(void);
int get_tot_mac_ovlp(void);

// prec    real_area_calc ( int lab ) ;

//////////////-OVERLAP COUNT-//////////////

void sa_macro_lg(void);
int get_mac_idx(void);
prec get_mac_cost(int idx, prec *hpwl_cost, prec *den_cost, int *ovlp_cost);
int sa_mac_leg_top(void);
void sa_mac_leg(int iter);
void sa_mac_leg_sub();

void sa_init_top(void);

void sa_mac_leg_init(void);
void sa_mac_leg_init_with_margin(void);

void sa_param_init_top(void);
void sa_param_init(int iter);

void sa_param_update();
void sa_param_update_sub();

int get_mac_ovlp(int idx);
int get_ovlp_area(struct MODULE *mac, struct MODULE *mac2);
struct POS get_mac_mov(struct FPOS r, struct POS u);
void do_mac_mov(int idx, struct POS *mov);
int mov_accept(prec cost_0, prec cost_1);

void post_mac_leg(void);
void sa_post(void);
void sa_delete(void);
int macro_area_cmp(const void *a, const void *b);
void mac_ovlp_mark();

#define SORT_MAC_BY_SZ

#define MAC_MARGIN_3DIC 0.2

#define MAX_SA_R_COF 100.0

#endif

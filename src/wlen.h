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

#ifndef __PL_WLEN__
#define __PL_WLEN__

#include "replace_private.h"
#include "opt.h"

enum { NoneWpre, PcntWpre, WlenWpre };
#define WLEN_PRE /* NoneWpre */ PcntWpre /* WlenWpre */

extern int wcof_flg;
extern prec hpwl_mGP3D;
extern prec hpwl_mGP2D;
extern prec hpwl_cGP3D;
extern prec hpwl_cGP2D;
extern prec TSV_WEIGHT;
extern int MAX_EXP;
extern int NEG_MAX_EXP;
extern FPOS wcof00;
extern FPOS total_hpwl;
extern FPOS total_stnwl;  // lutong
extern FPOS total_wlen;
extern FPOS gp_wlen_weight;
extern FPOS dp_wlen_weight;
extern FPOS base_wcof;
extern FPOS wlen_cof;
extern FPOS wlen_cof_inv;

struct EXP_ST {
  prec x;
  prec val;
  prec y_h;
};

extern EXP_ST *exp_st;

prec get_wlen();
prec get_wlen_wa();
prec get_wlen_lse(void);
void SetMAX_EXP_wlen();

FPOS get_net_wlen_wa(NET *net);
FPOS get_net_wlen_lse(NET *net);

void wlen_grad(int cell_idx, FPOS *grad);
void wlen_grad_lse(int cell_idx, FPOS *grad);
inline void wlen_grad_wa(int cell_idx, FPOS *grad);
void get_net_wlen_grad_lse(NET *net, PIN *pin, FPOS *grad);
inline void get_net_wlen_grad_wa(FPOS obj, NET *net, PIN *pin, FPOS *grad);


void initCustomNetWeight(std::string netWeightFile);

inline void wlen_pre(int cell_idx, FPOS *wpre) {
  wpre->x = wpre->y = 0;

#ifdef NO_WLEN
  return;
#endif

  CELL *cell = &gcell_st[cell_idx];

  switch(WLEN_PRE) {
    case NoneWpre:
      wpre->x = wpre->y = 0;
      break;

    case PcntWpre:
      wpre->x = wpre->y = (prec)(cell->pinCNTinObject);
      break;
  }

  wpre->x *= gp_wlen_weight.x /* / 2000.0 */;
  wpre->y *= gp_wlen_weight.y /* / 2000.0 */;
}

void wlen_grad2(int cell_idx, FPOS *grad2);
void wlen_grad2_lse(int cell_idx, FPOS *grad2);
void wlen_grad2_wa(FPOS *grad);
void get_net_wlen_grad2_lse(NET *net, PIN *pin, FPOS *grad2);

FPOS get_wlen_cof(prec ovf);
FPOS get_wlen_cof1(prec ovf);
FPOS get_wlen_cof2(prec ovf);
FPOS get_wlen_cof3(prec ovf);

void wlen_grad_debug(FPOS grad);

//#define MAX_EXP 300 //300 //40 // 10
#define EXP_RES 16           // 4
#define exp_st_cnt 640       // 40 // MAX_EXP * EXP_RES
#define exp_interval 0.0625  // 0.25 // MAX_EXP / exp_st_cnt

inline prec get_exp(prec a) {
  return fastExp(a);
}
void wlen_init(void);
void wlen_init_mGP2D(void);
void wlen_init_cGP2D(void);
void wcof_init(FPOS bstp);

void net_update_init(void);
void net_update(FPOS *st);
void net_update_lse(FPOS *st);
void net_update_wa(FPOS *st);

prec GetHpwl();
prec UpdateNetAndGetHpwl();

// enum {B2B,NTU};

// #define CELL_CENTER_WLEN_GRAD
// #define CELL_CENTER_WLEN
// #define CELL_CENTER_NET_UPDATE

//#define MIN_WGRAD_2 25.0

////// MACRO LG //////
prec get_mac_hpwl(int idx);
//////-MACRO LG-//////

prec net_update_hpwl_mac(void);
void UpdateNetMinMaxPin2();
void update_pin2(void);

enum { LSE, WA };


std::pair<double, double> GetUnscaledHpwl();
void PrintUnscaledHpwl(std::string mode);


#define WLEN_MODEL WA /* LSE */ /* NTU */
// #define TSV_WEIGHT /* 16.0 */ /* 1.00 */ /* 1.50 */ /* 0.73 */ /* 3.33 */ /*
// 10.00 */ /* 0.1 */ /* 0.01 */

#endif

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

#ifndef __PL_FFT__
#define __PL_FFT__

#include "replace_private.h"
//#include "mkl_service.h"
//#include "mkl_dfti.h"
//#include "mkl_trig_transforms.h"

extern int *charge_ip_2d;
extern int *charge_ip_3d;
extern int charge_dft_n_2d;
extern int charge_dft_n_3d;
extern int charge_dft_nbin_2d;
extern int charge_dft_nbin_3d;
extern int charge_dft_nbit_2d;
extern int charge_dft_nbit_3d;
extern int charge_dft_nw_2d;
extern int charge_dft_nw_3d;
extern prec **den_2d_st2;
extern prec ***den_3d_st3;
extern prec **phi_2d_st2;
extern prec ***phi_3d_st3;
extern prec **e_2d_st2;
extern prec ***e_3d_st3;
extern prec **ex_2d_st2;
extern prec ***ex_3d_st3;
extern prec **ey_2d_st2;
extern prec ***ey_3d_st3;
extern prec **ez_2d_st2;
extern prec ***ez_3d_st3;
// extern  prec            **powerDen_2d_st2;
// extern  prec            ***powerDen_3d_st3;
// extern  prec            **theta_2d_st2;
// extern  prec            ***theta_3d_st3;
// extern  prec            **heat_2d_st2;
// extern  prec            ***heat_3d_st3;
// extern  prec            **heat_x_2d_st2;
// extern  prec            ***heat_x_3d_st3;
// extern  prec            **heat_y_2d_st2;
// extern  prec            ***heat_y_3d_st3;
// extern  prec            **heat_z_2d_st2;
// extern  prec            ***heat_z_3d_st3;
extern prec *w_2d;
extern prec *w_3d;
extern prec *wx_2d_st;
extern prec *wx2_2d_st;
extern prec *wy_2d_st;
extern prec *wy2_2d_st;
extern prec *wz_2d_st;
extern prec *wz2_2d_st;
extern prec *wx_3d_st;
extern prec *wx2_3d_st;
extern prec *wy_3d_st;
extern prec *wy2_3d_st;
extern prec *wz_3d_st;
extern prec *wz2_3d_st;
extern prec *wx_2d_iL;
extern prec *wy_2d_iL;
extern prec *wx2_2d_iL;
extern prec *wy2_2d_iL;

// extern  prec            *thermal_w_2d;
// extern  prec            *thermal_w_3d;
// extern  prec            *thermal_wx_2d_st;
// extern  prec            *thermal_wx2_2d_st;
// extern  prec            *thermal_wy_2d_st;
// extern  prec            *thermal_wy2_2d_st;
// extern  prec            *thermal_wz_2d_st;
// extern  prec            *thermal_wz2_2d_st;
// extern  prec            *thermal_wx_3d_st;
// extern  prec            *thermal_wx2_3d_st;
// extern  prec            *thermal_wy_3d_st;
// extern  prec            *thermal_wy2_3d_st;
// extern  prec            *thermal_wz_3d_st;
// extern  prec            *thermal_wz2_3d_st;
extern prec DFT_SCALE_2D;
extern prec DFT_SCALE_3D;
extern struct POS dft_bin_2d;
extern struct POS dft_bin_3d;

/// Electrostatic-Based Functions /////////////////////////////////////////
void charge_fft_init(struct POS nbin, struct FPOS stp, int flg);
void charge_fft_init_2d(struct POS nbin, struct FPOS stp);
//void charge_fft_init_3d(struct POS nbin, struct FPOS stp);

void charge_fft_delete(int flg);
void charge_fft_delete_2d(void);
//void charge_fft_delete_3d(void);

void charge_fft_call(int flg);
void charge_fft_call_2d(void);
//void charge_fft_call_3d(void);

inline void copy_e_from_fft_2D(struct FPOS *e, struct POS p) {
  e->x = ex_2d_st2[p.x][p.y];
  e->y = ey_2d_st2[p.x][p.y];
}
//inline void copy_e_from_fft_3D(struct FPOS *e, struct POS p) {
//  e->x = ex_3d_st3[p.z][p.x][p.y];
//  e->y = ey_3d_st3[p.z][p.x][p.y];
//  e->z = ez_3d_st3[p.z][p.x][p.y];
//}
inline void copy_phi_from_fft_2D(prec *phi, struct POS p) {
  *phi = phi_2d_st2[p.x][p.y];
}
//inline void copy_phi_from_fft_3D(prec *phi, struct POS p) {
//  *phi = phi_3d_st3[p.z][p.x][p.y];
//}
inline void copy_den_to_fft_2D(prec den, struct POS p) {
  den_2d_st2[p.x][p.y] = den;
}
//inline void copy_den_to_fft_3D(prec den, struct POS p) {
//  den_3d_st3[p.z][p.x][p.y] = den;
//}

/// Thermal-Based Functions ///////////////////////////////////////////////
// void thermal_fft_init    (struct POS nbin, struct FPOS stp, int flg);
// void thermal_fft_init_2d (struct POS nbin, struct FPOS stp);
// void thermal_fft_init_3d (struct POS nbin, struct FPOS stp);
//
// void thermal_fft_delete    (int flg);
// void thermal_fft_delete_2d (void);
// void thermal_fft_delete_3d (void);
//
// void thermal_fft_call (int flg);
// void thermal_fft_call_2d (void);
// void thermal_fft_call_3d (void);
//
// void copy_heatflux_from_fft   (struct FPOS *heatflux, struct POS p, int flg);
// void copy_theta_from_fft      (prec   *phi, struct POS p, int flg);
// void copy_powerDensity_to_fft (prec   powerDensity, struct POS p, int flg);

/// 1D FFT ////////////////////////////////////////////////////////////////
void cdft(int n, int isgn, prec *a, int *ip, prec *w);
void ddct(int n, int isgn, prec *a, int *ip, prec *w);
void ddst(int n, int isgn, prec *a, int *ip, prec *w);

/// 2D FFT ////////////////////////////////////////////////////////////////
void cdft2d(int, int, int, prec **, prec *, int *, prec *);
void rdft2d(int, int, int, prec **, prec *, int *, prec *);
void ddct2d(int, int, int, prec **, prec *, int *, prec *);
void ddst2d(int, int, int, prec **, prec *, int *, prec *);
void ddsct2d(int n1, int n2, int isgn, prec **a, prec *t, int *ip, prec *w);
void ddcst2d(int n1, int n2, int isgn, prec **a, prec *t, int *ip, prec *w);

/// 3D FFT ////////////////////////////////////////////////////////////////
void cdft3d(int, int, int, int, prec ***, prec *, int *, prec *);
void rdft3d(int, int, int, int, prec ***, prec *, int *, prec *);
void ddct3d(int, int, int, int, prec ***, prec *, int *, prec *);
void ddst3d(int, int, int, int, prec ***, prec *, int *, prec *);
void ddscct3d(int, int, int, int isgn, prec ***, prec *, int *, prec *);
void ddcsct3d(int, int, int, int isgn, prec ***, prec *, int *, prec *);
void ddccst3d(int, int, int, int isgn, prec ***, prec *, int *, prec *);

//void fft_wxy_update_3d(struct FPOS stp);

#endif

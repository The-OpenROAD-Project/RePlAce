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

#include <cstdlib>
//#include        <tgmath.h>
#include <cmath>
#include <cfloat>

#include "fft.h"

int *charge_ip_2d;
int *charge_ip_3d;
int charge_dft_n_2d;
int charge_dft_n_3d;
int charge_dft_nbin_2d;
int charge_dft_nbin_3d;
int charge_dft_nbit_2d;
int charge_dft_nbit_3d;
int charge_dft_nw_2d;
int charge_dft_nw_3d;
prec **den_2d_st2;
prec ***den_3d_st3;
prec **phi_2d_st2;
prec ***phi_3d_st3;
prec **e_2d_st2;
prec ***e_3d_st3;
prec **ex_2d_st2;
prec ***ex_3d_st3;
prec **ey_2d_st2;
prec ***ey_3d_st3;
prec **ez_2d_st2;
prec ***ez_3d_st3;
prec *w_2d;
prec *w_3d;
prec *wx_2d_st;
prec *wx2_2d_st;
prec *wy_2d_st;
prec *wy2_2d_st;
prec *wz_2d_st;
prec *wz2_2d_st;
prec *wx_3d_st;
prec *wx2_3d_st;
prec *wy_3d_st;
prec *wy2_3d_st;
prec *wz_3d_st;
prec *wz2_3d_st;
prec *wx_2d_iL;
prec *wy_2d_iL;
prec *wx2_2d_iL;
prec *wy2_2d_iL;

prec DFT_SCALE_2D;
prec DFT_SCALE_3D;
struct POS dft_bin_2d;
struct POS dft_bin_3d;

void charge_fft_init(struct POS nbin, struct FPOS stp, int flg) {
  charge_fft_init_2d(nbin, stp);
}

void charge_fft_init_2d(struct POS nbin, struct FPOS stp) {
  // Descriptions for parameters are in fftsg2d.cpp.
  // See DCT section.  Line 200 in fftsg2d.cpp
  int x = 0;
  int y = 0;
  dft_bin_2d = nbin;
  charge_dft_n_2d = p_max(dft_bin_2d);
  charge_dft_nbin_2d = p_product(dft_bin_2d);
  DFT_SCALE_2D = 1.0 / ((prec)charge_dft_nbin_2d);

  den_2d_st2 = (prec **)malloc(sizeof(prec *) * dft_bin_2d.x);
  phi_2d_st2 = (prec **)malloc(sizeof(prec *) * dft_bin_2d.x);
  ex_2d_st2 = (prec **)malloc(sizeof(prec *) * dft_bin_2d.x);
  ey_2d_st2 = (prec **)malloc(sizeof(prec *) * dft_bin_2d.x);

  for(x = 0; x < dft_bin_2d.x; x++) {
    den_2d_st2[x] = (prec *)malloc(sizeof(prec) * dft_bin_2d.y);
    phi_2d_st2[x] = (prec *)malloc(sizeof(prec) * dft_bin_2d.y);
    ex_2d_st2[x] = (prec *)malloc(sizeof(prec) * dft_bin_2d.y);
    ey_2d_st2[x] = (prec *)malloc(sizeof(prec) * dft_bin_2d.y);
  }

  charge_dft_nbit_2d = 2 + (int)sqrt((prec)charge_dft_n_2d + 0.5);
  charge_ip_2d = (int *)malloc(sizeof(int) * charge_dft_nbit_2d);
  charge_dft_nw_2d = charge_dft_n_2d * 3 / 2;
  w_2d = (prec *)malloc(sizeof(prec) * charge_dft_nw_2d);
  charge_ip_2d[0] = 0;

  wx_2d_st = (prec *)malloc(sizeof(prec) * dft_bin_2d.x);
  wy_2d_st = (prec *)malloc(sizeof(prec) * dft_bin_2d.y);
  wx2_2d_st = (prec *)malloc(sizeof(prec) * dft_bin_2d.x);
  wy2_2d_st = (prec *)malloc(sizeof(prec) * dft_bin_2d.y);

  for(x = 0; x < dft_bin_2d.x; x++) {
    // LW 05/04/17
    wx_2d_st[x] = PI * (prec)x / ((prec)dft_bin_2d.x);
    // stp.x *= 1.0;
    // wx_2d_st [x]= PI * (prec  )x / ((prec  )dft_bin_2d.x * stp.x);
    wx2_2d_st[x] = wx_2d_st[x] * wx_2d_st[x];
  }

  for(y = 0; y < dft_bin_2d.y; y++) {
    // LW 05/05/17
    // wy_2d_st [y]= PI * (prec  ) y / ((prec  ) dft_bin_2d.y);
    wy_2d_st[y] = PI * (prec)y / ((prec)dft_bin_2d.y * stp.y / stp.x);
    // wy_2d_st [y]= PI * (prec  )y / ((prec  )dft_bin_2d.y * stp.y);
    wy2_2d_st[y] = wy_2d_st[y] * wy_2d_st[y];
  }
}

/*
void charge_fft_init_3d(struct POS nbin, struct FPOS stp) {
  int x = 0;
  int y = 0;
  int z = 0;

  dft_bin_3d = nbin;
  charge_dft_n_3d = p_max(dft_bin_3d);
  charge_dft_nbin_3d = p_product(dft_bin_3d);
  DFT_SCALE_3D = 1.0 / ((prec)charge_dft_nbin_3d);

  den_3d_st3 = (prec ***)malloc(sizeof(prec **) * dft_bin_3d.z);
  phi_3d_st3 = (prec ***)malloc(sizeof(prec **) * dft_bin_3d.z);
  ex_3d_st3 = (prec ***)malloc(sizeof(prec **) * dft_bin_3d.z);
  ey_3d_st3 = (prec ***)malloc(sizeof(prec **) * dft_bin_3d.z);
  ez_3d_st3 = (prec ***)malloc(sizeof(prec **) * dft_bin_3d.z);

  for(z = 0; z < dft_bin_3d.z; z++) {
    den_3d_st3[z] = (prec **)malloc(sizeof(prec *) * dft_bin_3d.x);
    phi_3d_st3[z] = (prec **)malloc(sizeof(prec *) * dft_bin_3d.x);
    ex_3d_st3[z] = (prec **)malloc(sizeof(prec *) * dft_bin_3d.x);
    ey_3d_st3[z] = (prec **)malloc(sizeof(prec *) * dft_bin_3d.x);
    ez_3d_st3[z] = (prec **)malloc(sizeof(prec *) * dft_bin_3d.x);

    for(x = 0; x < dft_bin_3d.x; x++) {
      den_3d_st3[z][x] = (prec *)malloc(sizeof(prec) * dft_bin_3d.y * 2);
      phi_3d_st3[z][x] = (prec *)malloc(sizeof(prec) * dft_bin_3d.y * 2);
      ex_3d_st3[z][x] = (prec *)malloc(sizeof(prec) * dft_bin_3d.y * 2);
      ey_3d_st3[z][x] = (prec *)malloc(sizeof(prec) * dft_bin_3d.y * 2);
      ez_3d_st3[z][x] = (prec *)malloc(sizeof(prec) * dft_bin_3d.y * 2);
    }
  }

  charge_dft_nbit_3d = 2 + (int)sqrt((prec)charge_dft_n_3d + 0.5);
  charge_ip_3d = (int *)malloc(sizeof(int) * charge_dft_nbit_3d);
  charge_dft_nw_3d = (int)(charge_dft_n_3d * 3 / 2) + 1;
  w_3d = (prec *)malloc(sizeof(prec) * charge_dft_nw_3d);

  charge_ip_3d[0] = 0;

  wx_3d_st = (prec *)malloc(sizeof(prec) * dft_bin_3d.x);
  wy_3d_st = (prec *)malloc(sizeof(prec) * dft_bin_3d.y);
  wz_3d_st = (prec *)malloc(sizeof(prec) * dft_bin_3d.z);

  wx2_3d_st = (prec *)malloc(sizeof(prec) * dft_bin_3d.x);
  wy2_3d_st = (prec *)malloc(sizeof(prec) * dft_bin_3d.y);
  wz2_3d_st = (prec *)malloc(sizeof(prec) * dft_bin_3d.z);

  for(x = 0; x < dft_bin_3d.x; x++) {
    wx_3d_st[x] = PI * (prec)x / ((prec)dft_bin_3d.x * stp.x);
    wx2_3d_st[x] = wx_3d_st[x] * wx_3d_st[x];
  }

  for(y = 0; y < dft_bin_3d.y; y++) {
    wy_3d_st[y] = PI * (prec)y / ((prec)dft_bin_3d.y * stp.y);
    wy2_3d_st[y] = wy_3d_st[y] * wy_3d_st[y];
  }

  for(z = 0; z < dft_bin_3d.z; z++) {
    wz_3d_st[z] = PI * (prec)z / ((prec)dft_bin_3d.z * stp.z);
    wz2_3d_st[z] = wz_3d_st[z] * wz_3d_st[z];
  }
}
*/
void charge_fft_call(int flg) {
  charge_fft_call_2d();
}

void charge_fft_call_2d(void) {
  // Descriptions for parameters are in fftsg2d.cpp.
  // See DCT section.  Line 200 in fftsg2d.cpp
  int x = 0;
  int y = 0;
  int n1 = dft_bin_2d.x;
  int n2 = dft_bin_2d.y;
  prec a_den = 0;
  prec a_phi = 0;
  prec denom = 0;
  prec a_ex = 0;
  prec a_ey = 0;
  prec wx = 0;
  prec wx2 = 0;
  prec wy = 0;
  prec wy2 = 0;

  ddct2d(n1, n2, -1, den_2d_st2, NULL, charge_ip_2d, w_2d);

  for(x = 0; x < n1; x++) {
    den_2d_st2[x][0] *= 0.5;
  }

  for(y = 0; y < n2; y++) {
    den_2d_st2[0][y] *= 0.5;
  }

  for(x = 0; x < n1; x++) {
    for(y = 0; y < n2; y++) {
      den_2d_st2[x][y] *= 4.0 / n1 / n2;
    }
  }

  for(x = 0; x < n1; x++) {
    wx = wx_2d_st[x];
    wx2 = wx2_2d_st[x];

    for(y = 0; y < n2; y++) {
      wy = wy_2d_st[y];
      wy2 = wy2_2d_st[y];

      a_den = den_2d_st2[x][y];

      if(x == 0 && y == 0) {
        a_phi = 0.0;
        a_ex = 0.0;
        a_ey = 0.0;
      }
      else {
        //////////// lutong
        //  denom =
        //  wx2 / 4.0 +
        //  wy2 / 4.0 ;
        // a_phi = a_den / denom ;
        ////b_phi = 0 ; // -1.0 * b / denom ;
        ////a_ex = 0 ; // b_phi * wx ;
        // a_ex = a_phi * wx / 2.0 ;
        ////a_ey = 0 ; // b_phi * wy ;
        // a_ey = a_phi * wy / 2.0 ;
        ///////////
        denom = wx2 + wy2;
        a_phi = a_den / denom;
        a_ex = a_phi * wx;
        a_ey = a_phi * wy;
      }
      phi_2d_st2[x][y] = a_phi;
      ex_2d_st2[x][y] = a_ex;
      ey_2d_st2[x][y] = a_ey;
    }
  }
  ddct2d(n1, n2, 1, phi_2d_st2, NULL, charge_ip_2d, w_2d);
  ddsct2d(n1, n2, 1, ex_2d_st2, NULL, charge_ip_2d, w_2d);
  ddcst2d(n1, n2, 1, ey_2d_st2, NULL, charge_ip_2d, w_2d);
}

/*
void charge_fft_call_3d(void) {
  int x = 0, y = 0, z = 0;
  int n1 = dft_bin_3d.x;
  int n2 = dft_bin_3d.y;
  prec a_den = 0, a_phi = 0, denom = 0;
  prec a_ex = 0, a_ey = 0, a_ez = 0;
  prec wx = 0, wx2 = 0;
  prec wy = 0, wy2 = 0;
  prec wz = 0, wz2 = 0;

  ddct3d(n3, n1, n2, -1, den_3d_st3, NULL, charge_ip_3d, w_3d);

  for(x = 0; x < n1; x++) {
    for(y = 0; y < n2; y++) {
      den_3d_st3[0][x][y] *= 0.5;
    }
  }

  for(z = 0; z < n3; z++) {
    for(x = 0; x < n1; x++) {
      den_3d_st3[z][x][0] *= 0.5;
    }
  }

  for(z = 0; z < n3; z++) {
    for(y = 0; y < n2; y++) {
      den_3d_st3[z][0][y] *= 0.5;
    }
  }

  for(z = 0; z < n3; z++) {
    for(x = 0; x < n1; x++) {
      for(y = 0; y < n2; y++) {
        den_3d_st3[z][x][y] *= 8.0 / n1 / n2 / n3;
      }
    }
  }

  for(x = 0; x < n1; x++) {
    wx = wx_3d_st[x];
    wx2 = wx2_3d_st[x];

    for(y = 0; y < n2; y++) {
      wy = wy_3d_st[y];
      wy2 = wy2_3d_st[y];

      for(z = 0; z < n3; z++) {
        wz = wz_3d_st[z];
        wz2 = wz2_3d_st[z];

        a_den = den_3d_st3[z][x][y];

        if(x == 0 && y == 0 && z == 0) {
          a_phi = 0.0;
          a_ex = 0.0;
          a_ey = 0.0;
          a_ez = 0.0;
        }
        else {
          denom = wx2 + wy2 + wz2;
          a_phi = a_den / denom;
          a_ex = a_phi * wx;
          a_ey = a_phi * wy;
          a_ez = a_phi * wz;
        }

        phi_3d_st3[z][x][y] = a_phi;
        ex_3d_st3[z][x][y] = a_ex;
        ey_3d_st3[z][x][y] = a_ey;
        ez_3d_st3[z][x][y] = a_ez;
      }
    }
  }
  ddct3d(n3, n1, n2, 1, phi_3d_st3, NULL, charge_ip_3d, w_3d);
  ddcsct3d(n3, n1, n2, 1, ex_3d_st3, NULL, charge_ip_3d, w_3d);
  ddccst3d(n3, n1, n2, 1, ey_3d_st3, NULL, charge_ip_3d, w_3d);
  ddscct3d(n3, n1, n2, 1, ez_3d_st3, NULL, charge_ip_3d, w_3d);
}
*/

// void thermal_fft_call_3d (void) {
//    int             x=0, y=0, z=0;
//    int             n1 = dft_bin_3d.x;
//    int             n2 = dft_bin_3d.y;
//    int             n3 = dft_bin_3d.z;
//    prec            a_powerDen = 0;
//    prec            a_theta = 0;
//    prec            denom = 0;
//    prec            a_heat_x = 0;
//    prec            a_heat_y = 0;
//    prec            a_heat_z = 0;
//    prec            wx = 0;
//    prec            wx2= 0;
//    prec            wy = 0;
//    prec            wy2= 0;
//    prec            wz = 0;
//    prec            wz2= 0;
//
//    ddct3d (n3, n1, n2, -1, powerDen_3d_st3, NULL, thermal_ip_3d,
// thermal_w_3d);
//
//    for (x=0; x<n1; x++) {
//        for(y=0; y<n2; y++) {
//            powerDen_3d_st3[0][x][y] *= 0.5;
//        }
//    }
//
//    for (z=0; z<n3; z++) {
//        for (x=0; x<n1; x++) {
//            powerDen_3d_st3[z][x][0] *= 0.5;
//        }
//    }
//
//    for (z=0; z<n3; z++) {
//        for(y=0;y<n2;y++) {
//            powerDen_3d_st3[z][0][y] *= 0.5;
//        }
//    }
//
//    for (z=0; z<n3; z++) {
//        for (x=0; x<n1; x++) {
//            for (y=0; y<n2; y++) {
//                powerDen_3d_st3[z][x][y] *= 8.0 / n1 / n2 / n3;
//            }
//        }
//    }
//
//    for (x=0; x<n1; x++) {
//        wx  = thermal_wx_3d_st[x];
//        wx2 = thermal_wx2_3d_st[x];
//
//        for (y=0;y<n2;y++) {
//            wy  = thermal_wy_3d_st[y];
//            wy2 = thermal_wy2_3d_st[y];
//
//            for(z=0; z<n3; z++) {
//                wz  = thermal_wz_3d_st[z];
//                wz2 = thermal_wz2_3d_st[z];
//
//                a_powerDen = powerDen_3d_st3[z][x][y];
//
//                if (x==0 && y==0 && z==0) {
//                    a_theta = 0.0;
//                    a_heat_x= 0.0;
//                    a_heat_y= 0.0;
//                    a_heat_z= 0.0;
//                } else {
//                    denom= wx2 + wy2 + wz2;
//                    a_theta = a_powerDen / denom;
//                    a_heat_x= a_theta * wx;
//                    a_heat_y= a_theta * wy;
//                    a_heat_z= a_theta * wz;
//                }
//                theta_3d_st3 [z][x][y] = a_theta;
//                heat_x_3d_st3[z][x][y] = a_heat_x;
//                heat_y_3d_st3[z][x][y] = a_heat_y;
//                heat_z_3d_st3[z][x][y] = a_heat_z;
//            }
//        }
//    }
//    ddscct3d (n3, n1, n2, 1, theta_3d_st3,  NULL, thermal_ip_3d,
// thermal_w_3d);
//    //ddssct3d (n3, n1, n2, 1, heat_x_3d_st3, NULL, thermal_ip_3d,
// thermal_w_3d);
//    //ddscst3d (n3, n1, n2, 1, heat_y_3d_st3, NULL, thermal_ip_3d,
// thermal_w_3d);
//    ddct3d   (n3, n1, n2, 1, heat_z_3d_st3, NULL, thermal_ip_3d,
// thermal_w_3d);
//}

// void thermal_fft_call_2d (void) {
//    int         x=0, y=0;
//    int         n1 = dft_bin_2d.x;
//    int         n2 = dft_bin_2d.y;
//    prec        a_den=0;
//    prec        a_phi=0;
//    prec        denom=0;
//    prec        a_ex=0;
//    prec        a_ey=0;
//    prec        wx=0;
//    prec        wx2=0;
//    prec        wy=0;
//    prec        wy2=0;
//
//    ddct2d (n1, n2, -1, den_2d_st2, NULL, charge_ip_2d, w_2d);
//
//    for (x=0; x<n1; x++) {
//        den_2d_st2 [x][0] *= 0.5 ;
//    }
//
//    for (y=0; y<n2; y++) {
//        den_2d_st2 [0][y] *= 0.5 ;
//    }
//
//    for(x=0;x<n1;x++)
//      {
//        for(y=0;y<n2;y++)
//          {
//            den_2d_st2 [x] [y] *= 4.0 / n1 / n2 ;
//          }
//      }
//
//    for(x=0;x<n1;x++)
//    {
//      wx = wx_2d_st [x] ;
//      wx2 = wx2_2d_st [x] ;
//
//      for(y=0;y<n2;y++)
//        {
//          wy = wy_2d_st [y] ;
//          wy2 = wy2_2d_st [y] ;
//
//          a_den = den_2d_st2 [x] [y] ;
//
//          if(x==0 && y==0)
//            {
//              a_phi = 0.0;
//              a_ex = 0.0;
//              a_ey = 0.0;
//            }
//          else
//            {
//              denom = wx2 + wy2;
//
//              a_phi = a_den / denom ;
//              a_ex = a_phi * wx;
//              a_ey = a_phi * wy;
//            }
//
//          phi_2d_st2 [x] [y] = a_phi ;
//          ex_2d_st2 [x] [y] = a_ex ;
//          ey_2d_st2 [x] [y] = a_ey ;
//        }
//    }
//
//  ddct2d ( n1      ,
//           n2      ,
//           1       ,
//           phi_2d_st2 ,
//           NULL    ,
//           charge_ip_2d      ,
//           w_2d      );
//
//  ddsct2d ( n1      ,
//            n2      ,
//            1       ,
//            ex_2d_st2  ,
//            NULL    ,
//            charge_ip_2d      ,
//            w_2d     );
//
//  ddcst2d ( n1      ,
//            n2      ,
//            1       ,
//            ey_2d_st2  ,
//            NULL    ,
//            charge_ip_2d    ,
//            w_2d     );
//
//}

// void thermal_fft_init (struct POS nbin, struct FPOS stp, int flg) {
//    if (flg)    thermal_fft_init_3d (nbin, stp);
//    else        thermal_fft_init_2d (nbin, stp);
//}

// void thermal_fft_init_2d (struct POS nbin, struct FPOS stp) {
//  int x=0,y=0;
//
//  dft_bin_2d = nbin;
//
//  charge_dft_n_2d = p_max ( dft_bin_2d);
//
//  charge_dft_nbin_2d = p_product ( dft_bin_2d );
//
//  DFT_SCALE_2D = 1.0 / ( (prec  ) charge_dft_nbin_2d ) ;
//
//  den_2d_st2 = ( prec   ** )
//    malloc ( sizeof(prec  *) * dft_bin_2d.x ) ;
//  phi_2d_st2 = ( prec   ** )
//    malloc ( sizeof(prec  *) * dft_bin_2d.x ) ;
//  ex_2d_st2  = ( prec   ** )
//    malloc ( sizeof(prec  *) * dft_bin_2d.x ) ;
//  ey_2d_st2  = ( prec   ** )
//    malloc ( sizeof(prec  *) * dft_bin_2d.x ) ;
//
//  for(x=0;x<dft_bin_2d.x;x++)
//    {
//      den_2d_st2 [x] =
//        ( prec   * )malloc ( sizeof(prec  ) * dft_bin_2d.y );
//      phi_2d_st2 [x] =
//        ( prec   * )malloc ( sizeof(prec  ) * dft_bin_2d.y );
//      ex_2d_st2 [x] =
//        ( prec   * )malloc ( sizeof(prec  ) * dft_bin_2d.y );
//      ey_2d_st2 [x] =
//        ( prec   * )malloc ( sizeof(prec  ) * dft_bin_2d.y );
//    }
//
//  charge_dft_nbit_2d = 2 + (int) sqrt((prec  ) charge_dft_n_2d + 0.5);
//
//  charge_ip_2d = (int*) malloc (sizeof(int) * charge_dft_nbit_2d);
//
//  charge_dft_nw_2d = charge_dft_n_2d * 3 / 2 ;
//
//  w_2d = ( prec   * ) malloc ( sizeof(prec  ) * charge_dft_nw_2d ) ;
//
//  charge_ip_2d [0] = 0;
//
//  wx_2d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_2d.x );
//  wy_2d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_2d.y );
//
//  wx2_2d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_2d.x );
//  wy2_2d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_2d.y );
//
//  for(x=0;x<dft_bin_2d.x;x++)
//    {
//      wx_2d_st [x] = PI * (prec  ) x /
//        ((prec  ) dft_bin_2d.x * stp.x);
//      wx2_2d_st [x] = wx_2d_st [x] * wx_2d_st [x] ;
//    }
//
//  for(y=0;y<dft_bin_2d.y;y++)
//    {
//      wy_2d_st [y] = PI * (prec  ) y /
//        ((prec  ) dft_bin_2d.y * stp.y);
//      wy2_2d_st [y] = wy_2d_st [y] * wy_2d_st [y] ;
//    }
//}

// void thermal_fft_init_3d (struct POS nbin, struct FPOS stp) {
//    int x=0, y=0, z=0;
//    dft_bin_3d = nbin;
//    thermal_dft_n_3d = p_max (dft_bin_3d);
//    thermal_dft_nbin_3d = p_product (dft_bin_3d);
//
//    DFT_SCALE_3D = 1.0 / ((prec  )charge_dft_nbin_3d);
//
//    powerDen_3d_st3= (prec  ***)malloc(sizeof(prec  **)*dft_bin_3d.z);
//    theta_3d_st3   = (prec  ***)malloc(sizeof(prec  **)*dft_bin_3d.z);
//    heat_x_3d_st3  = (prec  ***)malloc(sizeof(prec  **)*dft_bin_3d.z);
//    heat_y_3d_st3  = (prec  ***)malloc(sizeof(prec  **)*dft_bin_3d.z);
//    heat_z_3d_st3  = (prec  ***)malloc(sizeof(prec  **)*dft_bin_3d.z);
//
//    for (z=0; z<dft_bin_3d.z; z++) {
//        powerDen_3d_st3[z]= (prec  **)malloc(sizeof(prec  *)*dft_bin_3d.x);
//        theta_3d_st3[z]   = (prec  **)malloc(sizeof(prec  *)*dft_bin_3d.x);
//        ex_3d_st3[z] = ( prec   ** )malloc ( sizeof(prec  *) * dft_bin_3d.x );
//        ey_3d_st3[z] = ( prec   ** )malloc ( sizeof(prec  *) * dft_bin_3d.x );
//        ez_3d_st3[z] = ( prec   ** )malloc ( sizeof(prec  *) * dft_bin_3d.x );
// 	    for(x=0;x<dft_bin_3d.x;x++)
//	    {
//          den_3d_st3 [z][x] =
//            ( prec   *)malloc ( sizeof(prec  ) * dft_bin_3d.y * 2 );
//          phi_3d_st3 [z][x] =
//            ( prec   *)malloc ( sizeof(prec  ) * dft_bin_3d.y * 2 );
//          ex_3d_st3  [z][x] =
//            ( prec   *)malloc ( sizeof(prec  ) * dft_bin_3d.y * 2 );
//          ey_3d_st3  [z][x] =
//            ( prec   *)malloc ( sizeof(prec  ) * dft_bin_3d.y * 2 );
//          ez_3d_st3  [z][x] =
//            ( prec   *)malloc ( sizeof(prec  ) * dft_bin_3d.y * 2 );
//	    }
//    }
//
//  charge_dft_nbit_3d = 2 + (int) sqrt( (prec  ) charge_dft_n_3d + 0.5 ) ;
//  charge_ip_3d = ( int * ) malloc ( sizeof(int) * charge_dft_nbit_3d ) ;
//  charge_dft_nw_3d = (int) (charge_dft_n_3d * 3 / 2) + 1;
//  w_3d = ( prec   * ) malloc ( sizeof(prec  ) * charge_dft_nw_3d ) ;
//
//  charge_ip_3d[0] = 0;
//
//  wx_3d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_3d.x );
//  wy_3d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_3d.y );
//  wz_3d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_3d.z );
//
//  wx2_3d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_3d.x );
//  wy2_3d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_3d.y );
//  wz2_3d_st = ( prec   * ) malloc ( sizeof(prec  ) * dft_bin_3d.z );
//
//  for(x=0;x<dft_bin_3d.x;x++)
//    {
//      wx_3d_st  [x] = PI * (prec  ) x /
//        ((prec  ) dft_bin_3d.x * stp.x);
//      wx2_3d_st [x] = wx_3d_st [x] * wx_3d_st [x];
//    }
//
//  for(y=0;y<dft_bin_3d.y;y++)
//    {
//      wy_3d_st [y] = PI * (prec  ) y /
//        ((prec  ) dft_bin_3d.y * stp.y);
//      wy2_3d_st [y] = wy_3d_st [y] * wy_3d_st [y] ;
//    }
//
//  for(z=0;z<dft_bin_3d.z;z++)
//    {
//      wz_3d_st [z] = PI * (prec  ) z /
//        ((prec  ) dft_bin_3d.z * stp.z);
//      wz2_3d_st [z] = wz_3d_st [z] * wz_3d_st [z] ;
//    }
//
//}

// void copy_phi_from_fft (prec   *phi, struct POS p, int flg) {
//    if (flg)    *phi = phi_3d_st3[p.z][p.x][p.y];
//    else        *phi = phi_2d_st2[p.x][p.y];
//}

// void copy_theta_from_fft (prec   *theta, struct POS p, int flg) {
//    if (flg)    *theta = theta_3d_st3[p.z][p.x][p.y];
//    else        *theta = theta_2d_st2[p.x][p.y];
//}

// void copy_e_from_fft (struct FPOS *e, struct POS p, int flg) {
//    if (flg) {
//        e->x = ex_3d_st3[p.z][p.x][p.y];
//        e->y = ey_3d_st3[p.z][p.x][p.y];
//        e->z = ez_3d_st3[p.z][p.x][p.y];
//    } else {
//        e->x = ex_2d_st2[p.x][p.y];
//        e->y = ey_2d_st2[p.x][p.y];
//    }
//}

// void copy_den_to_fft (prec   den, struct POS p, int flg) {
//    if (flg)    den_3d_st3[p.z][p.x][p.y] = den;
//    else        den_2d_st2[p.x][p.y] = den;
//}

// void copy_powerDen_to_fft (prec   powerDen, struct POS p, int flg) {
//    if (flg)    powerDen_3d_st3[p.z][p.x][p.y] = powerDen;
//    else        powerDen_2d_st2[p.x][p.y] = powerDen;
//}

void charge_fft_delete(int flg) {
   charge_fft_delete_2d();
}

// void thermal_fft_delete (int flg) {
//    if (flg)    return thermal_fft_delete_3d ();
//    else        return thermal_fft_delete_2d ();
//}

/*
void charge_fft_delete_3d(void) {
  for(int i = 0; i < dft_bin_3d.x; i++) {
    for(int j = 0; j < dft_bin_3d.y; j++) {
      free(den_3d_st3[i][j]);
      free(phi_3d_st3[i][j]);
      free(ex_3d_st3[i][j]);
      free(ey_3d_st3[i][j]);
      free(ez_3d_st3[i][j]);
    }
    free(den_3d_st3[i]);
    free(phi_3d_st3[i]);
    free(ex_3d_st3[i]);
    free(ey_3d_st3[i]);
    free(ez_3d_st3[i]);
  }
  free(den_3d_st3);
  free(phi_3d_st3);
  free(ex_3d_st3);
  free(ey_3d_st3);
  free(ez_3d_st3);
  free(charge_ip_3d);
  free(w_3d);
  free(wx_3d_st);
  free(wy_3d_st);
  free(wz_3d_st);
  free(wx2_3d_st);
  free(wy2_3d_st);
  free(wz2_3d_st);
}
*/

// void thermal_fft_delete_3d (void) {
//    for (int i=0; i<dft_bin_3d.x; i++) {
//        for (int j=0; j<dft_bin_3d.y; j++) {
//            free (den_3d_st3[i][j]);
//            free (phi_3d_st3[i][j]);
//            free (ex_3d_st3 [i][j]);
//            free (ey_3d_st3 [i][j]);
//            free (ez_3d_st3 [i][j]);
//        }
//        free (den_3d_st3[i]);
//        free (phi_3d_st3[i]);
//        free (ex_3d_st3 [i]);
//        free (ey_3d_st3 [i]);
//        free (ez_3d_st3 [i]);
//    }
//    free (den_3d_st3);
//    free (phi_3d_st3);
//    free (ex_3d_st3);
//    free (ey_3d_st3);
//    free (ez_3d_st3);
//    free (charge_ip_3d);
//    free (w_3d);
//    free (wx_3d_st);
//    free (wy_3d_st);
//    free (wz_3d_st);
//    free (wx2_3d_st);
//    free (wy2_3d_st);
//    free (wz2_3d_st);
//}

void charge_fft_delete_2d(void) {
  // for (int i=0; i<dft_bin_2d.x; i++) {
  //    free (den_2d_st2[i]);
  //    free (phi_2d_st2[i]);
  //    free (ex_2d_st2 [i]);
  //    free (ey_2d_st2 [i]);
  //}
  free(den_2d_st2);
  free(phi_2d_st2);
  free(ex_2d_st2);
  free(ey_2d_st2);
  free(charge_ip_2d);
  free(w_2d);
  free(wx_2d_st);
  free(wy_2d_st);
  free(wx2_2d_st);
  free(wy2_2d_st);
}

// void thermal_fft_delete_2d (void) {
//    for (int i=0; i<dft_bin_2d.x; i++) {
//        free (powerDen_2d_st2[i]);
//        free (theta_2d_st2[i]);
//        free (heat_x_2d_st2 [i]);
//        free (heat_y_2d_st2 [i]);
//    }
//    free (powerDen_2d_st2);
//    free (theta_2d_st2);
//    free (heat_x_2d_st2);
//    free (heat_y_2d_st2);
//    free (thermal_ip_2d);
//    free (thermal_w_2d);
//    free (thermal_wx_2d_st);
//    free (thermal_wy_2d_st);
//    free (thermal_wx2_2d_st);
//    free (thermal_wy2_2d_st);
//}

//void fft_wxy_update_3d(struct FPOS stp) {
//  int x = 0, y = 0, z = 0;
//  for(x = 0; x < dft_bin_3d.x; x++) {
//    wx_3d_st[x] = PI * (prec)x / ((prec)dft_bin_3d.x * stp.x);
//    wx2_3d_st[x] = wx_3d_st[x] * wx_3d_st[x];
//  }
//  for(y = 0; y < dft_bin_3d.y; y++) {
//    wy_3d_st[y] = PI * (prec)y / ((prec)dft_bin_3d.y * stp.y);
//    wy2_3d_st[y] = wy_3d_st[y] * wy_3d_st[y];
//  }
//  for(z = 0; z < dft_bin_3d.z; z++) {
//    wz_3d_st[z] = PI * (prec)z / ((prec)dft_bin_3d.z * stp.z);
//    wz2_3d_st[z] = wz_3d_st[z] * wz_3d_st[z];
//  }
//}

// void thermal_fft_call (int flg) {
//    if (flg)    return thermal_fft_call_3d ();
//    else        return thermal_fft_call_2d ();
//}

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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bin.h"
#include "charge.h"
#include "fft.h"
#include "replace_private.h"
#include "macro.h"
#include "opt.h"
#include "plot.h"
#include "wlen.h"

using std::min;
using std::max;

/*
prec get_potn(struct FPOS *st, int N) {
    prec sum_cell_phi = 0;
    prec sum_phi = 0;
    prec sum_term_phi = 0;

    /// STD CELLs ///
    get_cell_potn(st, N, &sum_cell_phi);
    /// MACROs & IOs ///
    get_term_potn(&sum_term_phi);

    sum_phi = sum_cell_phi + sum_term_phi;
    return sum_phi;
}

void get_cell_potn(struct FPOS *st, int N, prec *phi0) {
#ifndef LS_DEN
    return get_cell_potn1(st, N, phi0);
#else
    return get_cell_potn2(st, N, phi0);
#endif
}

void get_cell_potn1(struct FPOS *st, int N, prec *phi0) {
    int bin_cnt = 0;
    prec cell_phi = 0;
    prec phi = 0;
    prec sum_phi = 0;
    prec area_share = 0;
    struct FPOS center = zeroFPoint;
    struct FPOS half_size = zeroFPoint;
    struct POS p = zeroPoint;
    struct BIN *bp = NULL;
    struct CELL *cell = NULL;

    for(int i = 0; i < N; i++) {
        center = st[i];
        cell_phi = 0;
        half_size = gcell_st[i].half_size;
        cell = &gcell_st[i];
        get_bins(center, cell, bin_st, bin_share_st, &bin_cnt);

        for(int j = 0; j < bin_cnt; j++) {
            p = bin_st[j];
            bp = get_bin_from_idx(p);
            area_share = bin_share_st[j];
            phi = bp->phi * area_share;
            cell_phi += phi;
        }
        sum_phi += cell_phi;
    }
    *phi0 = sum_phi;
}

void get_cell_potn2(struct FPOS *st, int N, prec *phi0) {
    struct BIN *bp = NULL;
    prec cell_phi = 0;
    prec sum_phi = 0;

    for(int i = 0; i < N; i++) {
        bp = get_bin_from_point(st[i]);
        cell_phi = bp->phi * gcell_st[i].area;
        sum_phi += cell_phi;
    }
    *phi0 = sum_phi;
}

void get_term_potn(prec *phi) {
    prec sum_phi = 0;
    prec term_phi = 0;
    prec virt_phi = 0;
    struct BIN *bp = NULL;

    for(int i = 0; i < tot_bin_cnt; i++) {
        bp = &bin_mat[i];
        term_phi = bp->term_area * bp->phi;
        virt_phi = bp->virt_area * bp->phi;
        sum_phi += term_phi + virt_phi;
    }
    *phi = sum_phi;
}
*/
// void potn_pre (int cell_idx, struct FPOS *charge_dpre) {
//    *charge_dpre = zeroFPoint;
//#ifdef NO_DEN
//    return;
//#endif
//    struct  CELL   *cell = &gcell_st[cell_idx];
//    switch (CHARGE_PRE) {
//        case    NoneDpre:
//            *charge_dpre = zeroFPoint;
//            break;
//        case    AreaDpre:
//            charge_dpre->x = charge_dpre->y = (prec  )(cell->area);
//            //if (flg_3dic) charge_dpre->z = (prec  )(cell->area);
//            break;
//	}
//}

/*
void potn_grad(int cell_idx, struct FPOS *pgrad) {
  int x = 0, y = 0, z = 0;
  int idx = 0;
  prec area_share = 0;
  prec min_x = 0;
  prec min_y = 0;
  prec min_z = 0;
  prec max_x = 0;
  prec max_y = 0;
  prec max_z = 0;
  BIN *bpx = NULL;
  BIN *bpy = NULL;
  BIN *bpz = NULL;
  POS b0;
  POS b1;
  CELL *cell = &gcell_st[cell_idx];

  *pgrad = zeroFPoint;

  b0.x = INT_CONVERT((cell->den_pmin.x - bin_org.x) * inv_bin_stp.x);
  b0.y = INT_CONVERT((cell->den_pmin.y - bin_org.y) * inv_bin_stp.y);
  b0.z = INT_CONVERT((cell->den_pmin.z - bin_org.z) * inv_bin_stp.z);
  b1.x = INT_CONVERT((cell->den_pmax.x - bin_org.x) * inv_bin_stp.x);
  b1.y = INT_CONVERT((cell->den_pmax.y - bin_org.y) * inv_bin_stp.y);
  b1.z = INT_CONVERT((cell->den_pmax.z - bin_org.z) * inv_bin_stp.z);

  if(b0.x < 0)
    b0.x = 0;
  if(b0.y < 0)
    b0.y = 0;
  if(b0.z < 0)
    b0.z = 0;
  if(b0.x >= max_bin.x)
    b0.x = max_bin.x - 1;
  if(b0.y >= max_bin.y)
    b0.y = max_bin.y - 1;
  if(b0.z >= max_bin.z)
    b0.z = max_bin.z - 1;
  if(b1.x < 0)
    b1.x = 0;
  if(b1.y < 0)
    b1.y = 0;
  if(b1.z < 0)
    b1.z = 0;
  if(b1.x >= max_bin.x)
    b1.x = max_bin.x - 1;
  if(b1.y >= max_bin.y)
    b1.y = max_bin.y - 1;
  if(b1.z >= max_bin.z)
    b1.z = max_bin.z - 1;

  idx = b0.x * msh_yz + b0.y * msh.z + b0.z;

  for(x = b0.x, bpx = &bin_mat[idx]; x <= b1.x; x++, bpx += msh_yz) {
    max_x = min(bpx->pmax.x, cell->den_pmax.x);
    min_x = max(bpx->pmin.x, cell->den_pmin.x);

    for(y = b0.y, bpy = bpx; y <= b1.y; y++, bpy += msh.z) {
      max_y = min(bpy->pmax.y, cell->den_pmax.y);
      min_y = max(bpy->pmin.y, cell->den_pmin.y);

      for(z = b0.z, bpz = bpy; z <= b1.z; z++, bpz++) {
        max_z = min(bpz->pmax.z, cell->den_pmax.z);
        min_z = max(bpz->pmin.z, cell->den_pmin.z);

        area_share = (max_x - min_x) * (max_y - min_y) * (max_z - min_z) *
                     cell->den_scal;

        pgrad->x += (area_share * bpz->e.x);
        pgrad->y += (area_share * bpz->e.y);
        pgrad->z += (area_share * bpz->e.z);
      }
    }
  }
}
*/

/*
// To calculate A_ij q_i \frac{\partial phi_j}{\partial cell_idx}
void potn_grad_local(int cell_idx, struct FPOS *grad, prec *cellLambda) {
  int x = 0, y = 0, z = 0;
  int idx = 0;
  prec area_share = 0;
  prec bpx_min_x = 0;
  prec bpx_min_y = 0;
  prec bpx_min_z = 0;
  prec bpx_max_x = 0;
  prec bpx_max_y = 0;
  prec bpx_max_z = 0;
  prec bpy_min_x = 0;
  prec bpy_min_y = 0;
  prec bpy_min_z = 0;
  prec bpy_max_x = 0;
  prec bpy_max_y = 0;
  prec bpy_max_z = 0;
  prec bpz_min_x = 0;
  prec bpz_min_y = 0;
  prec bpz_min_z = 0;
  prec bpz_max_x = 0;
  prec bpz_max_y = 0;
  prec bpz_max_z = 0;
  prec bpx_heightY;
  prec bpx_heightZ;
  prec bpx_delta_x_movement = 0;
  prec bpy_heightX;
  prec bpy_heightZ;
  prec bpy_delta_y_movement = 0;
  prec bpz_heightX;
  prec bpz_heightY;
  prec bpz_delta_z_movement = 0;
  prec common_val = 0;
  prec common_div = 0;
  prec common_mul = 0;
  prec exp_term = 0;
  struct POS b0;
  struct POS b1;
  struct CELL *cell = &gcell_st[cell_idx];
  struct BIN *bpx = NULL;
  struct BIN *bpy = NULL;
  struct BIN *bpz = NULL;
  *grad = zeroFPoint;

  // b0:  BoundingBox LL, indicating bin index
  // b1:  BoundingBox UR, indicating bin index
  b0.x = INT_CONVERT((cell->den_pmin.x - bin_org.x) * inv_bin_stp.x);
  b0.y = INT_CONVERT((cell->den_pmin.y - bin_org.y) * inv_bin_stp.y);

  b1.x = INT_CONVERT((cell->den_pmax.x - bin_org.x) * inv_bin_stp.x);
  b1.y = INT_CONVERT((cell->den_pmax.y - bin_org.y) * inv_bin_stp.y);

  if(b0.x < 0)
    b0.x = 0;
  if(b0.y < 0)
    b0.y = 0;

  if(b0.x >= max_bin.x)
    b0.x = max_bin.x - 1;
  if(b0.y >= max_bin.y)
    b0.y = max_bin.y - 1;

  if(b1.x < 0)
    b1.x = 0;
  if(b1.y < 0)
    b1.y = 0;

  if(b1.x >= max_bin.x)
    b1.x = max_bin.x - 1;
  if(b1.y >= max_bin.y)
    b1.y = max_bin.y - 1;

  idx = b0.x * msh_yz + b0.y * msh.z + b0.z;

  for(x = b0.x, bpx = &bin_mat[idx]; x <= b1.x; x++, bpx += msh_yz) {
    bpx_max_x = min(bpx->pmax.x, cell->den_pmax.x);
    bpx_min_x = max(bpx->pmin.x, cell->den_pmin.x);
    bpx_max_y = min(bpx->pmax.y, cell->pmax.y);
    bpx_min_y = max(bpx->pmin.y, cell->pmin.y);
    bpx_max_z = min(bpx->pmax.z, cell->pmax.z);
    bpx_min_z = max(bpx->pmin.z, cell->pmin.z);
    // heightx no need
    bpx_heightY = bpx_max_y - bpx_min_y;
    bpx_heightZ = bpx_max_z - bpx_min_z;
    bpx_delta_x_movement = fabs(bpx_heightY * bpx_heightZ);

    if(bpx_max_x == bpx->pmax.x && bpx_min_x == bpx->pmin.x) {
      bpx_delta_x_movement = 0;
    }
    else if(bpx_max_x == bpx->pmax.x) {
      bpx_delta_x_movement *= -1;
    }  // else just add, then plus

    for(y = b0.y, bpy = bpx; y <= b1.y; y++, bpy += msh.z) {
      bpy_max_x = min(bpy->pmax.x, cell->pmax.x);
      bpy_min_x = max(bpy->pmin.x, cell->pmin.x);
      bpy_max_y = min(bpy->pmax.y, cell->den_pmax.y);
      bpy_min_y = max(bpy->pmin.y, cell->den_pmin.y);
      bpy_max_z = min(bpy->pmax.z, cell->pmax.z);
      bpy_min_z = max(bpy->pmin.z, cell->pmin.z);
      // heighty no need
      bpy_heightX = bpy_max_x - bpy_min_x;
      bpy_heightZ = bpy_max_z - bpy_min_z;
      bpy_delta_y_movement = fabs(bpy_heightX * bpy_heightZ);

      if(bpy_max_y == bpy->pmax.y && bpy_min_y == bpy->pmin.y) {
        bpy_delta_y_movement = 0;
      }
      else if(bpy_max_y == bpy->pmax.y) {
        bpy_delta_y_movement *= -1;
      }  // else just add, then plus

      for(z = b0.z, bpz = bpy; z <= b1.z; z++, bpz++) {
        bpz_max_x = min(bpz->pmax.x, cell->pmax.x);
        bpz_min_x = max(bpz->pmin.x, cell->pmin.x);
        bpz_max_y = min(bpz->pmax.y, cell->pmax.y);
        bpz_min_y = max(bpz->pmin.y, cell->pmin.y);
        bpz_max_z = min(bpz->pmax.z, cell->den_pmax.z);
        bpz_min_z = max(bpz->pmin.z, cell->den_pmin.z);
        // heighty no need
        bpz_heightX = bpz_max_x - bpz_min_x;
        bpz_heightY = bpz_max_y - bpz_min_y;
        bpz_delta_z_movement = fabs(bpz_heightX * bpz_heightY);

        if(bpz_max_z == bpz->pmax.z && bpz_min_z == bpz->pmin.z) {
          bpz_delta_z_movement = 0;
        }
        else if(bpz_max_z == bpz->pmax.z) {
          bpz_delta_z_movement *= -1;
        }  // else just add, then plus

        common_div = ALPHA / bin_area;
        area_share = (bpx_max_x - bpx_min_x) * (bpy_max_y - bpy_min_y) *
                     (bpz_max_z - bpz_min_z) * cell->den_scal;

        exp_term = fastExp(common_div * (bpz->cell_area - bin_area));
        // exp_term   = exp (common_div * (bpz->cell_area - bin_area));

        if((bpz->cell_area - bin_area) > 0) {
          *cellLambda +=
              BETA * (bpy->cell_area - bin_area) * inv_total_modu_area;
        }

        common_val = common_div * bpz->phi * exp_term * bpz->cell_area;
        common_mul = exp_term * area_share;
        grad->x += common_val * bpx_delta_x_movement;
        grad->x += common_mul * bpz->e.x;
        grad->y += common_val * bpy_delta_y_movement;
        grad->y += common_mul * bpz->e.y;
        grad->z += common_val * bpz_delta_z_movement;
        grad->z += common_mul * bpz->e.z;
      }
    }
  }
}
*/

void potn_grad_2D(int cell_idx, struct FPOS *grad) {
  //    cout << "executed?" << endl;
  //    exit(1);

  assert(0 <= cell_idx && cell_idx < gcell_cnt);
  grad->SetZero();

  POS b0, b1;

  CELL *cell = &gcell_st[cell_idx];
  TIER *tier = &tier_st[cell->tier];
 
  b0.x =
      INT_CONVERT((cell->den_pmin.x - tier->bin_org.x) * tier->inv_bin_stp.x);
  b0.y =
      INT_CONVERT((cell->den_pmin.y - tier->bin_org.y) * tier->inv_bin_stp.y);
  b1.x =
      INT_CONVERT((cell->den_pmax.x - tier->bin_org.x) * tier->inv_bin_stp.x);
  b1.y =
      INT_CONVERT((cell->den_pmax.y - tier->bin_org.y) * tier->inv_bin_stp.y);


  if(b0.x < 0)
    b0.x = 0;
  if(b0.x > tier->dim_bin.x - 1)
    b0.x = tier->dim_bin.x - 1;
  if(b0.y < 0)
    b0.y = 0;
  if(b0.y > tier->dim_bin.y - 1)
    b0.y = tier->dim_bin.y - 1;

  if(b1.x < 0)
    b1.x = 0;
  if(b1.x > tier->dim_bin.x - 1)
    b1.x = tier->dim_bin.x - 1;
  if(b1.y < 0)
    b1.y = 0;
  if(b1.y > tier->dim_bin.y - 1)
    b1.y = tier->dim_bin.y - 1;

  BIN *bpx = NULL, *bpy = NULL;
  int x = 0, y = 0;
  int idx = b0.x * tier->dim_bin.y + b0.y;

  for(x = b0.x, bpx = &tier->bin_mat[idx]; x <= b1.x;
      x++, bpx += tier->dim_bin.y) {
    prec max_x = min(bpx->pmax.x, cell->den_pmax.x);
    prec min_x = max(bpx->pmin.x, cell->den_pmin.x);

    for(y = b0.y, bpy = bpx; y <= b1.y; y++, bpy++) {
      prec max_y = min(bpy->pmax.y, cell->den_pmax.y);
      prec min_y = max(bpy->pmin.y, cell->den_pmin.y);
      prec area_share = (max_x - min_x) * (max_y - min_y)
                        //* cell->size.z
                        * cell->den_scal;
      grad->x += area_share * bpy->e.x;
      grad->y += area_share * bpy->e.y;
    }
  }
}

void potn_grad_2D_local(int cell_idx, struct FPOS *grad, prec *cellLambda) {
  int x = 0, y = 0;
  int idx = 0;
  prec area_share = 0;
  prec bpx_min_x = 0;
  prec bpx_min_y = 0;
  prec bpx_max_x = 0;
  prec bpx_max_y = 0;
  prec bpy_min_x = 0;
  prec bpy_min_y = 0;
  prec bpy_max_x = 0;
  prec bpy_max_y = 0;
  prec bpx_heightY;
  prec bpx_delta_x_movement = 0;
  prec bpy_heightX;
  prec bpy_delta_y_movement = 0;
  prec exp_term = 0;
  prec common_val = 0;
  prec common_div = 0;
  prec common_mul = 0;
  struct BIN *bpx = NULL;
  struct BIN *bpy = NULL;
  struct POS b0, b1;
  struct CELL *cell = &gcell_st[cell_idx];
  struct TIER *tier = &tier_st[cell->tier];

  grad->x = grad->y = 0;

  b0.x =
      INT_CONVERT((cell->den_pmin.x - tier->bin_org.x) * tier->inv_bin_stp.x);
  b0.y =
      INT_CONVERT((cell->den_pmin.y - tier->bin_org.y) * tier->inv_bin_stp.y);
  b1.x =
      INT_CONVERT((cell->den_pmax.x - tier->bin_org.x) * tier->inv_bin_stp.x);
  b1.y =
      INT_CONVERT((cell->den_pmax.y - tier->bin_org.y) * tier->inv_bin_stp.y);

  if(b0.x < 0)
    b0.x = 0;
  if(b0.x > tier->dim_bin.x - 1)
    b0.x = tier->dim_bin.x - 1;
  if(b0.y < 0)
    b0.y = 0;
  if(b0.y > tier->dim_bin.y - 1)
    b0.y = tier->dim_bin.y - 1;
  if(b1.x < 0)
    b1.x = 0;
  if(b1.x > tier->dim_bin.x - 1)
    b1.x = tier->dim_bin.x - 1;
  if(b1.y < 0)
    b1.y = 0;
  if(b1.y > tier->dim_bin.y - 1)
    b1.y = tier->dim_bin.y - 1;

  idx = b0.x * tier->dim_bin.y + b0.y;

  for(x = b0.x, bpx = &tier->bin_mat[idx]; x <= b1.x;
      x++, bpx += tier->dim_bin.y) {
    bpx_max_x = min(bpx->pmax.x, cell->den_pmax.x);
    bpx_min_x = max(bpx->pmin.x, cell->den_pmin.x);
    bpx_max_y = min(bpx->pmax.y, cell->pmax.y);
    bpx_min_y = max(bpx->pmin.y, cell->pmin.y);
    // heightx no need
    bpx_heightY = bpx_max_y - bpx_min_y;
    bpx_delta_x_movement = fabs(bpx_heightY);

    if(bpx_max_x == bpx->pmax.x && bpx_min_x == bpx->pmin.x) {
      bpx_delta_x_movement = 0;
    }
    else if(bpx_max_x == bpx->pmax.x) {
      bpx_delta_x_movement *= -1;
    }  // else just add, then plus

    for(y = b0.y, bpy = bpx; y <= b1.y; y++, bpy++) {
      bpy_max_x = min(bpy->pmax.x, cell->pmax.x);
      bpy_min_x = max(bpy->pmin.x, cell->pmin.x);
      bpy_max_y = min(bpy->pmax.y, cell->den_pmax.y);
      bpy_min_y = max(bpy->pmin.y, cell->den_pmin.y);
      // heighty no need
      bpy_heightX = bpy_max_x - bpy_min_x;
      bpy_delta_y_movement = fabs(bpy_heightX);

      if(bpy_max_y == bpy->pmax.y && bpy_min_y == bpy->pmin.y) {
        bpy_delta_y_movement = 0;
      }
      else if(bpy_max_y == bpy->pmax.y) {
        bpy_delta_y_movement *= -1;
      }  // else just add, then plus

      common_div = ALPHA / tier->bin_area;
      area_share =
          (bpx_max_x - bpx_min_x) * (bpy_max_y - bpy_min_y) * cell->den_scal;

      exp_term = fastExp(common_div * (bpy->cell_area - tier->bin_area));
      // exp_term = exp (common_div * (bpy->cell_area - tier->bin_area));

      if((bpy->cell_area - tier->bin_area) > 0) {
        *cellLambda +=
            BETA * (bpy->cell_area - tier->bin_area) * inv_total_modu_area;
      }

      common_val = common_div * bpy->phi * exp_term * bpy->cell_area;
      common_mul = exp_term * area_share;

      grad->x += bpx_delta_x_movement * common_val;
      grad->x += common_mul * bpy->e.x;

      grad->y += bpy_delta_y_movement * common_val;
      grad->y += common_mul * bpy->e.y;

      ////igkang
      // if (cell->flg == FillerCell) continue;
      // else {
      //    bpy->e_local.x = grad->x / area_share;
      //    bpy->e_local.y = grad->y / area_share;
      //}
    }
  }
}

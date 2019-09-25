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

#ifndef __CHARGE__
#define __CHARGE__

#include "bin.h"
#include "replace_private.h"

// prec get_potn(struct FPOS *st, int N);
// void get_cell_potn(struct FPOS *st, int N, prec *phi0);
// void get_cell_potn1(struct FPOS *st, int N, prec *phi0);
// void get_cell_potn2(struct FPOS *st, int N, prec *phi0);
// void get_term_potn(prec *phi);

inline void potn_pre(int cell_idx, struct FPOS *charge_dpre) {
  charge_dpre->x = charge_dpre->y = 0;
#ifdef NO_DEN
  return;
#endif
  struct CELL *cell = &gcell_st[cell_idx];
  switch(CHARGE_PRE) {
    case NoneDpre:
      charge_dpre->x = charge_dpre->y = 0;
      break;
    case AreaDpre:
      charge_dpre->x = charge_dpre->y = (prec)(cell->area);
      break;
  }
}
//void potn_grad(int cell_idx, struct FPOS *grad);
//void potn_grad_local(int cell_idx, struct FPOS *grad, prec *cellLambda);
void potn_grad_2D(int cell_idx, struct FPOS *grad);
void potn_grad_2D_local(int cell_idx, struct FPOS *grad, prec *cellLambda);

#endif

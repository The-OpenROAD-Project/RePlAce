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

#ifndef __NS__
#define __NS__

#include "replace_private.h"
#include "opt.h"
#include "timing.h"

class myNesterov {
 private:
  struct FPOS *x_st;
  struct FPOS *y_st;
  struct FPOS *z_st;
  struct FPOS *y_dst;
  struct FPOS *z_dst;
  struct FPOS *y_wdst;
  struct FPOS *y_pdst;
  struct FPOS *z_wdst;
  struct FPOS *z_pdst;
  struct FPOS *y_pdstl;
  struct FPOS *z_pdstl;
  struct ITER *iter_st;
  struct ITER *it = nullptr;
  struct ITER it0;
  struct FPOS *y0_st;
  struct FPOS *y0_dst;
  struct FPOS *y0_wdst;
  struct FPOS *y0_pdst;
  struct FPOS *y0_pdstl;
  struct FPOS *x0_st;
  //    struct FPOS u;
  //    struct FPOS v;
  //    struct FPOS half_densize;
  struct FPOS wcof;
  struct FPOS wpre;
  struct FPOS charge_dpre;
  struct FPOS temp_dpre;
  struct FPOS pre;

  prec *cellLambdaArr;
  prec *pcofArr;
  prec *alphaArrCD;
  prec *deltaArrCD;
  int start_idx;
  int end_idx;
  int post_filler;
  prec sum_wgrad;
  prec sum_pgrad;
  prec sum_tgrad;
  int max_iter;
  int N;
  int N_org;
  int last_ra_iter;
  prec a;
  prec ab;
  prec alpha;
  prec cof;
  prec initialOVFL;
  prec alpha_pred;
  prec alpha_new;
  prec before100iter_cof;
  prec before100iter_alpha;
  prec before100iter_delta;

  int temp_iter;
  std::vector<pair<int, bool> > timingChkArr;
  bool isTimingIter(int ovlp);

  // myNesterov::functions
  void InitializationCommonVar(void);
  void InitializationCellStatus(void);
  void InitializationCoefficients(void);
  void InitializationPrecondition(void);
  void InitializationPrecondition_DEN_ONLY_PRECON(void);
  void InitializationIter(void);
  void InitializationCostFunctionGradient(prec *, prec *);
  int DoNesterovOptimization(Timing::Timing &TimingInst);
  void malloc_free(void);
  void SummarizeNesterovOpt(int last_index);
  void UpdateNesterovOptStatus(void);
  void UpdateNesterovIter(int iter, struct ITER *it, struct ITER *last_it);
  void ShiftPL_SA(struct FPOS *y_st, int N);
  void ShiftPL_SA_sub(struct FPOS *y_st, int N);
  void z_init(void);
  void UpdateAlpha(struct ITER *it);
  void UpdateBeta(struct ITER *it);

  void PrintNesterovOptStatus(int iter);

 public:
  void nesterov_opt(void);
};

#endif

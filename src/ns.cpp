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

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>

#include "bookShelfIO.h"
#include "bin.h"
#include "charge.h"
#include "global.h"
#include "mkl.h"
#include "ns.h"
#include "opt.h"
#include "plot.h"
#include "wlen.h"
#include "timing.h"
#include "routeOpt.h"

static int backtrack_cnt = 0;

void myNesterov::nesterov_opt() {
  int last_iter = 0;

  Timing::Timing TimingInst(moduleInstance, terminalInstance, netInstance,
                            netCNT, pinInstance, pinCNT, mPinName, tPinName,
                            clockPinName, timingClock);

  if(isTiming) {
    TimingInst.BuildSteiner(true);
    TimingInst.ExecuteStaFirst(verilogTopModule, verilogName, libStor, sdcName );
  }

  InitializationCommonVar();

  InitializationCellStatus();

  // if (stnCMD == true)     FLUTE_init();

  net_update(y_st);

  bin_update(N);

  // if (stnCMD == true)     buildRSMT_FLUTE(y_st);

  InitializationCostFunctionGradient(&sum_wgrad, &sum_pgrad);

  InitializationCoefficients();

  //    cout << it->ovfl << endl;
  if(DEN_ONLY_PRECON) {
    InitializationPrecondition_DEN_ONLY_PRECON();
  }
  else {
    InitializationPrecondition();
  }
  //    cout << it->ovfl << endl;

  InitializationIter();

  z_init();

  if(dynamicStepCMD && isTrial == false && isFirst_gp_opt == true) {
    UPPER_PCOF = 1.0001;
    potnPhaseDS = potnPhase1;
  }

  if(isTrial == true)
    UPPER_PCOF = 1.05;
  net_update(z_st);
  bin_update(N);  // igkang

  // if (stnCMD == true) {
  //    buildRSMT_FLUTE(z_st);
  //    it->tot_stnwl= total_stnwl.x + total_stnwl.y;
  //}
  it->tot_wwl = (1.0 - opt_w_cof) * it->tot_hpwl + opt_w_cof * it->tot_stnwl;

  getCostFuncGradient2(z_dst, z_wdst, z_pdst, z_pdstl, N, cellLambdaArr);

  a = 1.0;
  get_lc(y_st, y_dst, z_st, z_dst, &it0, N);

  it->alpha00 = it0.alpha00;

  if(isTrial) {
    initialOVFL = it->ovfl;

    prec tot_hpwl(it->tot_hpwl);
    trial_HPWLs.push_back(make_pair(tot_hpwl, 0.0));
    trial_POTNs.push_back(it->potn);
  }

  PrintNesterovOptStatus(0);

  // if (dynamicStepCMD) NUM_ITER_FILLER_PLACE = 20;
  // else                NUM_ITER_FILLER_PLACE = 20;

  last_iter = DoNesterovOptimization(TimingInst);

  SummarizeNesterovOpt(last_iter);

  // LW 06/01/17  Block the last evaluation.
  // if (isRoutability == true) {
  //    cell_update (x_st, N_org);
  //    modu_copy ();
  //    congEstimation (x_st);
  //    calcCong_print_detail();
  //}

  if(isTiming) {
    TimingInst.BuildSteiner(true);
    // TimingInst.PrintNetSteiner();

    string spefName = string(dir_bnd) + "/" + gbch + "_gp.spef";
    TimingInst.WriteSpef(spefName);
    TimingInst.ExecuteStaLater();
  }
  else {
    // for comparison
    //        TimingInst.ExecuteStaFirst(gbch, verilogCMD, libStor, sdcCMD);
  }
  mkl_malloc_free();
}

void myNesterov::InitializationCommonVar() {
  for(int i = 0; i < 200; i++) {
    timingCheck[i] = 1;
  }
  timingCheck[80] = timingCheck[79] = timingCheck[78] = 0;
  timingCheck[50] = timingCheck[49] = timingCheck[48] = 0;
  timingCheck[30] = timingCheck[29] = timingCheck[28] = 0;
  timingCheck[16] = timingCheck[15] = timingCheck[14] = 0;

  N = gcell_cnt;
  N_org = moduleCNT;
  start_idx = 0;
  end_idx = N;


  if(dynamicStepCMD)
    max_iter = 6000;
  else
    max_iter = 2500;

  // debug : mgwoo
//  max_iter = 3;

  last_ra_iter = 0;
  a = 0;
  ab = 0;
  alpha = 0;
  cof = 0;
  initialOVFL = 0;
  alpha_pred = 0;
  alpha_new = 0;
  post_filler = 0;
  sum_wgrad = 0;
  sum_pgrad = 0;
  sum_tgrad = 0;
  //    u.SetZero();
  //    v.SetZero();
  //    half_densize.SetZero();
  wcof.SetZero();
  wpre.SetZero();
  charge_dpre.SetZero();
  temp_dpre.SetZero();
  pre.SetZero();

  iter_st = (struct ITER *)mkl_malloc(sizeof(struct ITER) * (max_iter + 1), 64);

  x_st = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);

  y_st = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y_dst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y_wdst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y_pdst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y_pdstl = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);

  z_st = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  z_dst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  z_wdst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  z_pdst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  z_pdstl = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);

  x0_st = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);

  y0_st = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y0_dst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y0_wdst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y0_pdst = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);
  y0_pdstl = (struct FPOS *)mkl_malloc(sizeof(struct FPOS) * N, 64);

  memset(x_st, 0.0f, sizeof(prec) * N * 2);
  memset(y_st, 0.0f, sizeof(prec) * N * 2);
  memset(y_dst, 0.0f, sizeof(prec) * N * 2);
  memset(y_wdst, 0.0f, sizeof(prec) * N * 2);
  memset(y_pdst, 0.0f, sizeof(prec) * N * 2);
  memset(y_pdstl, 0.0f, sizeof(prec) * N * 2);

  memset(z_st, 0.0f, sizeof(prec) * N * 2);
  memset(z_dst, 0.0f, sizeof(prec) * N * 2);
  memset(z_wdst, 0.0f, sizeof(prec) * N * 2);
  memset(z_pdst, 0.0f, sizeof(prec) * N * 2);
  memset(z_pdstl, 0.0f, sizeof(prec) * N * 2);

  memset(x0_st, 0.0f, sizeof(prec) * N * 2);
  memset(y0_st, 0.0f, sizeof(prec) * N * 2);
  memset(y0_dst, 0.0f, sizeof(prec) * N * 2);
  memset(y0_wdst, 0.0f, sizeof(prec) * N * 2);
  memset(y0_pdst, 0.0f, sizeof(prec) * N * 2);
  memset(y0_pdstl, 0.0f, sizeof(prec) * N * 2);

  cellLambdaArr = (prec *)mkl_malloc(sizeof(prec) * N, 64);
  pcofArr = (prec *)mkl_malloc(sizeof(prec) * 100, 64);
  // alphaArrCD  =(prec*)mkl_malloc(sizeof(prec)*100, 64);
  // deltaArrCD  =(prec*)mkl_malloc(sizeof(prec)*100, 64);
  // IK 05/08/17
  // if (isRoutability) MIN_PRE = 1E-15;
  // else                MIN_PRE = 1;
  MIN_PRE = 1;
}

void myNesterov::InitializationCellStatus() {
  // OPT_INPUT == QWL_ISOL
  cg_input(x_st, N, OPT_INPUT);
  net_update(x_st);

  for(int i = 0; i < N; i++) {
    y_st[i] = y0_st[i] = x0_st[i] = x_st[i];
  }

  if(STAGE == mGP3D || STAGE == cGP3D)
    ShiftPL_SA(y_st, N);

  if(numLayer == 1) {
    if(placementMacroCNT == 0) {
      if(STAGE == cGP2D)
        ShiftPL_SA(y_st, N);
    }
    else {
      if(STAGE == mGP2D)
        ShiftPL_SA(y_st, N);
    }
  }
}

void myNesterov::InitializationCoefficients() {

  if(STAGE == mGP3D) {
    opt_phi_cof = sum_wgrad / sum_pgrad * INIT_LAMBDA_COF_GP;
    opt_w_cof = 0;
    ALPHA = ALPHAmGP;  // 1E-12;
    BETA = BETAmGP;    // 1E-16;
    wcof = get_wlen_cof(gsum_ovfl);
    wlen_cof = fp_mul(base_wcof, wcof);
    wlen_cof_inv = fp_inv(wlen_cof);
  }
  else if(STAGE == mGP2D) {
    opt_phi_cof = sum_wgrad / sum_pgrad * INIT_LAMBDA_COF_GP;
    ALPHA = ALPHAmGP;
    if(INPUT_FLG == MMS && (placementMacroCNT >= 3000 ||
                            (!dynamicStepCMD && (target_cell_den <= 0.60f ||
                                                 target_cell_den <= 0.60)))) {
      // Wants to have more inertia
      BETA = 1E-11;
      dampParam = 0.999900;
    }
    else {
      BETA = BETAmGP;
    }
    wcof = get_wlen_cof(gsum_ovfl);
    wlen_cof = fp_mul(base_wcof, wcof);
    wlen_cof_inv = fp_inv(wlen_cof);
  }
  else if(STAGE == cGP3D) {
    if(placementMacroCNT > 0) {
      cGP3D_buf_iter = mGP3D_tot_iter / 10;
      opt_phi_cof = mGP3D_opt_phi_cof / pow(UPPER_PCOF, (prec)cGP3D_buf_iter);
    }
    else if(placementMacroCNT == 0) {
      opt_phi_cof = sum_wgrad / sum_pgrad * INIT_LAMBDA_COF_GP;
    }
    ALPHA = ALPHAcGP;  // 1E-12;
    BETA = BETAcGP;    // 1E-16;
    wcof = get_wlen_cof(gsum_ovfl);
    wlen_cof = fp_mul(base_wcof, wcof);
    wlen_cof_inv = fp_inv(wlen_cof);
  }
  else if(STAGE == cGP2D) {
    if(placementMacroCNT > 0) {
      if(INPUT_FLG == MMS)
        cGP2D_buf_iter = mGP2D_tot_iter / 10;
      else
        cGP2D_buf_iter = mGP2D_tot_iter / 8;

      if(dynamicStepCMD && !constraintDrivenCMD) {
        UPPER_PCOF = 1.010;  // 1.010
        LOWER_PCOF = 0.60;   // 0.50
      }
      else if(dynamicStepCMD && constraintDrivenCMD) {
        UPPER_PCOF = 1.015;  // 1.015,1.012
        LOWER_PCOF = 0.90;   // 0.70, 0.60
      }

      opt_phi_cof = mGP2D_opt_phi_cof / pow(UPPER_PCOF, (prec)cGP2D_buf_iter);
    }
    else {
      opt_phi_cof = sum_wgrad / sum_pgrad * INIT_LAMBDA_COF_GP;
    }
    opt_w_cof = 0;
    if(placementMacroCNT == 0) {
      ALPHA = ALPHAcGP;  // 1E-15;
      BETA = BETAcGP;    // 1E-14;
    }
    else {
      ALPHA = ALPHAcGP;
      BETA = BETAcGP;
    }
    wcof = get_wlen_cof(gsum_ovfl);
    wlen_cof = fp_mul(base_wcof, wcof);
    wlen_cof_inv = fp_inv(wlen_cof);
  }

  if(lambda2CMD == true) {
    opt_phi_cof_local = opt_phi_cof;
    for(int i = 0; i < N; i++) {
      cellLambdaArr[i] = opt_phi_cof_local;
    }
  }
}

void myNesterov::InitializationPrecondition() {
  if(lambda2CMD == false) {
    for(int i = 0; i < N; i++) {
      y_dst[i].x = y_wdst[i].x + opt_phi_cof * y_pdst[i].x;
      y_dst[i].y = y_wdst[i].y + opt_phi_cof * y_pdst[i].y;
      wlen_pre(i, &wpre);
      potn_pre(i, &charge_dpre);

      pre.x = wpre.x + opt_phi_cof * charge_dpre.x;
      pre.y = wpre.y + opt_phi_cof * charge_dpre.y;

      if(pre.x < MIN_PRE)
        pre.x = MIN_PRE;
      if(pre.y < MIN_PRE)
        pre.y = MIN_PRE;

      y_dst[i].x /= pre.x;
      y_dst[i].y /= pre.y;
    }
  }
  else if(lambda2CMD == true) {
    for(int i = 0; i < N; i++) {
      y_dst[i].x = y_wdst[i].x + opt_phi_cof * y_pdst[i].x +
                   cellLambdaArr[i] * y_pdstl[i].x;
      y_dst[i].y = y_wdst[i].y + opt_phi_cof * y_pdst[i].y +
                   cellLambdaArr[i] * y_pdstl[i].y;

      wlen_pre(i, &wpre);
      potn_pre(i, &charge_dpre);

      pre.x = wpre.x + opt_phi_cof * charge_dpre.x;
      pre.y = wpre.y + opt_phi_cof * charge_dpre.y;

      if(pre.x < MIN_PRE)
        pre.x = MIN_PRE;
      if(pre.y < MIN_PRE)
        pre.y = MIN_PRE;

      y_dst[i].x /= pre.x;
      y_dst[i].y /= pre.y;
    }
  }
  else {
  }
}

void myNesterov::InitializationPrecondition_DEN_ONLY_PRECON() {
  if(lambda2CMD == false) {
    for(int i = 0; i < N; i++) {
      y_dst[i].x = y_wdst[i].x + opt_phi_cof * y_pdst[i].x;
      y_dst[i].y = y_wdst[i].y + opt_phi_cof * y_pdst[i].y;

      wlen_pre(i, &wpre);
      potn_pre(i, &charge_dpre);
      //#ifdef DEN_ONLY_PRECON
      // if (DEN_ONLY_PRECON) {
      pre = charge_dpre;
      //}
      //#else
      //                //else {
      //                pre.x = wpre.x + opt_phi_cof * charge_dpre.x;
      //                pre.y = wpre.y + opt_phi_cof * charge_dpre.y;
      //                //pre.z = wpre.z + opt_phi_cof * charge_dpre.z;
      //                //}
      //#endif
      if(pre.x < MIN_PRE)
        pre.x = MIN_PRE;
      if(pre.y < MIN_PRE)
        pre.y = MIN_PRE;
      y_dst[i].x /= pre.x;
      y_dst[i].y /= pre.y;
    }
  }
  else if(lambda2CMD == true) {
    for(int i = 0; i < N; i++) {
      y_dst[i].x = y_wdst[i].x + opt_phi_cof * y_pdst[i].x +
                   cellLambdaArr[i] * y_pdstl[i].x;
      y_dst[i].y = y_wdst[i].y + opt_phi_cof * y_pdst[i].y +
                   cellLambdaArr[i] * y_pdstl[i].y;
      wlen_pre(i, &wpre);
      potn_pre(i, &charge_dpre);
      //#ifdef DEN_ONLY_PRECON
      // if (DEN_ONLY_PRECON) {
      pre = charge_dpre;
      //}
      //#else
      //                //else {
      //                pre.x = wpre.x + opt_phi_cof * charge_dpre.x;
      //                pre.y = wpre.y + opt_phi_cof * charge_dpre.y;
      //                //}
      //#endif
      if(pre.x < MIN_PRE)
        pre.x = MIN_PRE;
      if(pre.y < MIN_PRE)
        pre.y = MIN_PRE;
      y_dst[i].x /= pre.x;
      y_dst[i].y /= pre.y;
    }
  }
  else {
  }
}

void myNesterov::InitializationIter() {
  init_iter(iter_st, 0);
  it = &iter_st[0];
  time_calc(0, &it->cpu_curr, &it->cpu_cost);
  it->cpu_cost = 0;
  it->tot_wlen = get_wlen();
  it->wlen = total_wlen;
  it->tot_hpwl = GetHpwl();
  it->hpwl = total_hpwl;
  it->potn = gsum_phi;
  it->ovfl = gsum_ovfl;
  global_ovfl = gsum_ovfl;
  it->grad = get_norm(y_dst, N, 2.0);
}

int myNesterov::DoNesterovOptimization(Timing::Timing &TimingInst) {
  int i;
  prec minPotn = PREC_MAX;
  temp_iter = 0;
  // int last_route_iter = -100;
  // int post_filler_route = 1;

  bool timeon = false;
  double time = 0.0f;

  for(i = 0; i < max_iter; i++) {
    if(timeon)
      time_start(&time);

    if(isTrial == false && isRoutability == true &&
       isRoutabilityInit == false) {
      routability_init();
    }

    // We can probably remove the following cell_update.
    if(constraintDrivenCMD)
      cell_update(y_st, N);

    it = &iter_st[i + 1];
    init_iter(it, i + 1);
    FILLER_PLACE = 0;

    // cout <<"postfiller = " <<post_filler <<endl;
    // cout <<"isFirst_gp_opt = " <<isFirst_gp_opt <<endl;

    if((isFirst_gp_opt == false) && post_filler == 0) {
      if(i < NUM_ITER_FILLER_PLACE) {
        FILLER_PLACE = 1;
        post_filler = 0;
        start_idx = moduleCNT;
        end_idx = N;
        //// NOTE3 LW 05/16/17
        // if (isFirst_gp_opt == false && STAGE == cGP2D) {
        //    DEN_ONLY_PRECON = true;
        //    overflowMin = 0.125f;
        //}
      }
      else {
        FILLER_PLACE = 0;
        post_filler = 1;
        start_idx = 0;
        end_idx = N;
        // igkang:  We comment out net_update and bin_update since we
        // will do it near line 507, 518.
        // lutong
        // if (stnCMD == true) {
        //    buildRSMT_FLUTE(y_st);
        //}
        //// NOTE3 LW 05/16/17
        // if (isFirst_gp_opt == false && STAGE == cGP2D) {
        //    DEN_ONLY_PRECON = false;
        //    //if (dynamicStepCMD) UPPER_PCOF = 1.05;
        //}

        getCostFuncGradient3(y_dst, y_wdst, y_pdst, y_pdstl, N, cellLambdaArr);

        get_lc(y_st, y_dst, z_st, z_dst, &it0, N);
      }
    }

    // if (isRoutability && post_filler_route == 0) {
    //    if (i < last_route_iter + NUM_ITER_FILLER_PLACE) {
    //        FILLER_PLACE = 1;
    //        post_filler_route = 0;
    //        start_idx = moduleCNT;
    //        end_idx = N;
    //    } else {
    //        FILLER_PLACE = 0;
    //        post_filler_route = 1;
    //        start_idx = 0;
    //        end_idx = N;
    //        getCostFuncGradient3 (y_dst, y_wdst, y_pdst, y_pdstl, N,
    //        cellLambdaArr);
    //        get_lc (y_st, y_dst, z_st, z_dst, &it0, N);
    //    }
    //}
    it->lc = it0.lc;
    alpha = it->alpha00 = it0.alpha00;
    it->alpha00 = it0.alpha00 = alpha;

    ab = a;
    a = (1.0 + sqrt(4.0 * a * a + 1.0)) * 0.5;
    cof = (ab - 1.0) / a;

    alpha_pred = it->alpha00;
    backtrack_cnt = 0;

    // cout <<"alpha_pred: " <<alpha_pred <<endl;
    // cout <<"cof: " <<cof <<endl;
    // int cnt = 0;
    if(timeon) {
      time_end(&time);
      cout << "prev: " << time << endl;
    }

    while(1) {
      backtrack_cnt++;

      if(timeon) {
        time_start(&time);
      };
      int j = 0;
#pragma omp parallel default(none) private(j) shared(gcell_st)
      {
        FPOS u, v;
        FPOS half_desize;
#pragma omp for
        for(j = start_idx; j < end_idx; j++) {
          FPOS half_densize = gcell_st[j].half_den_size;

          u.x = y_st[j].x + alpha_pred * y_dst[j].x;
          u.y = y_st[j].y + alpha_pred * y_dst[j].y;
          // cout <<"dst = (" <<y_dst[j].x <<", " <<y_dst[j].y <<")"
          // <<endl;

          v.x = u.x + cof * (u.x - x_st[j].x);
          v.y = u.y + cof * (u.y - x_st[j].y);

          x0_st[j] = valid_coor00(u, half_densize);
          // auto temp = y0_st[j];
          y0_st[j] = valid_coor00(v, half_densize);
          // if (temp.x == y0_st[j].x && temp.y == y0_st[j].y) ++cnt;
        }
      }
      if(timeon) {
        time_end(&time);
        cout << "inner for: " << time << endl;
        time_start(&time);
      }
      // cout <<"cnt: " <<cnt <<endl;

      net_update(y0_st);
      if(timeon) {
        time_end(&time);
        cout << "net update: " << time << endl;
        time_start(&time);
      }

      bin_update(N);  // igkang
      if(timeon) {
        time_end(&time);
        cout << "bin update: " << time << endl;
        time_start(&time);
      }

      // lutong
      // if (stnCMD == true) {
      //  //buildRMST(y0_st);
      //  buildRSMT_FLUTE(y0_st);
      //}

      getCostFuncGradient3(y0_dst, y0_wdst, y0_pdst, y0_pdstl, N,
                           cellLambdaArr);
      if(timeon) {
        time_end(&time);
        cout << "GetCost Grad: " << time << endl;
        time_start(&time);
      }

      get_lc(y_st, y_dst, y0_st, y0_dst, &it0, N);
      if(timeon) {
        time_end(&time);
        cout << "get_lc : " << time << endl;
        time_start(&time);
      }

      alpha_new = it0.alpha00;

      if(alpha_new > alpha_pred * 0.95 || backtrack_cnt >= MAX_BKTRK_CNT) {
        alpha_pred = alpha_new;
        it->alpha00 = alpha_new;
        break;
      }
      else {
        alpha_pred = alpha_new;
      }
    }

    UpdateNesterovOptStatus();
    UpdateNesterovIter(i + 1, it, &iter_st[i]);

    if(dynamicStepCMD && !isTrial) {
      if(isFirst_gp_opt)
        potnPhaseDS = definePOTNphase(it->potn);
      else
        potnPhaseDS = potnPhase8;

      stepSizeAdaptation_by2ndOrderEPs(it->tot_hpwl);
      // if (isFirst_gp_opt == false && STAGE == cGP2D) {
      //    // IK 05/15/17 :: NOTE2 DEN_ONLY_PRECON enabled.
      //    //UPPER_PCOF = 1.01;
      //    //overflowMin = 0.12;
      //    //DEN_ONLY_PRECON = true;
      //} else {
      //////////////////if (orderHPWL == thirdOrder)
      //////////////////    stepSizeAdaptation_by2ndOrderEPs
      ///(it->tot_hpwl);
      //////////////////else if (orderHPWL == secondOrder)
      //////////////////    stepSizeAdaptation_by1stOrderEPs
      ///(it->tot_hpwl);
      //////////////////else {}
      //}
      // NOTE2
      // if (isFirst_gp_opt == false && STAGE == cGP2D) DEN_ONLY_PRECON =
      // true;
    }

    if(isTrial) {
      if(it->ovfl > initialOVFL / 2.5) {
        prec tot_hpwl(it->tot_hpwl);
        trial_HPWLs.push_back(make_pair(tot_hpwl, 0.0));
        trial_POTNs.push_back(it->potn);
      }
      else {
        trial_iterCNT = trial_HPWLs.size();
        printf("\n");
        printf("INFO:    SUMMARY tGP\n");
        printf("INFO:    #iterations = %d\n", trial_iterCNT);
        return i;
      }
    }

    if(isRoutability == true && STAGE == cGP2D) {
      // LW mod 10/20/16 temp_con_orig = 0.12+
      // igkang
      // LW 05/30/17
      prec temp_con = 0.10 + bloating_max_count / 10.0 - bloatCNT / 10.0;
      if(DEN_ONLY_PRECON) {
        temp_con = 0.15 + bloating_max_count / 10.0 - bloatCNT / 10.0;
      }

      // for DEBUG
//      temp_con = 1.00;

      if((it->ovfl < temp_con) && (i - last_ra_iter > 10)) {
        // UPPER_PCOF = 1.01;
        if(bloatCNT < bloating_max_count) {
          last_ra_iter = i;
          cell_update(x_st, N_org);
          modu_copy();
          congEstimation(x_st);
          // if (inflation_cnt == 0) calcCong_print_detail();
          if(inflation_cnt % 2 == 0) {
            is_inflation_h = true;
          }
          else {
            is_inflation_h = false;
          }
          if(flg_noroute) {
            inflation_cnt = 100;
            if(inflation_cnt >= inflation_max_cnt) {
              bloatCNT++;
              inflation_cnt = 0;
            }
          }
          else {
            routability();
            inflation_cnt++;
            if(inflation_cnt >= inflation_max_cnt) {
              bloatCNT++;
              inflation_cnt = 0;
            }
            isBloatingBegin = true;
            if(inflation_cnt == 1) {
              before100iter_cof = pcofArr[(i + 2) % 100];
              // if (constraintDrivenCMD) {
              //    before100iter_alpha = alphaArrCD[(i+2)%100];
              //    before100iter_delta = deltaArrCD[(i+2)%100];
              //}
            }
            opt_phi_cof = before100iter_cof;
          }
          // post_filler_route = 0;
          // last_route_iter = i;
        }
      }
    }

    // Update ALPHA and BETA for Local Density Function
    if(constraintDrivenCMD == true)
      UpdateAlpha(it);
    if(lambda2CMD == true)
      UpdateBeta(it);
    // Elimination Condition
    if(it->tot_hpwl > 2000000000)
      exit(0);

    if(isTiming) {
      int checkIter = INT_CONVERT(it->ovfl * 100);
      if(timingCheck[checkIter] == 0) {
        // do something

        auto start = std::chrono::steady_clock::now();
        TimingInst.BuildSteiner(true);
        auto finish = std::chrono::steady_clock::now();

        double elapsed_seconds =
            std::chrono::duration_cast< std::chrono::duration< double > >(
                finish - start)
                .count();
        cout << "BuildSteiner: " << elapsed_seconds << endl;

        start = std::chrono::steady_clock::now();
        TimingInst.ExecuteStaLater();
        finish = std::chrono::steady_clock::now();

        elapsed_seconds =
            std::chrono::duration_cast< std::chrono::duration< double > >(
                finish - start)
                .count();
        cout << "ExecuteStaLater: " << elapsed_seconds << endl;
        
        for(int j=checkIter-2; j<=checkIter+2; j++) {
          timingCheck[j] = 1;
        }
      }
    }

    // Termination Condition 1
    if(it->ovfl <= overflowMin && i > 50) {
      return i;
    }

    // Termination Condition 2
    if(STAGE == cGP2D && i > 50) {
      if((it->ovfl <= 0.13f && dynamicStepCMD) || (it->ovfl <= 0.10f)) {
        if(minPotn * 1.01 <= it->potn && temp_iter == 0) {
          temp_iter++;
        }
        if(minPotn > it->potn) {
          minPotn = it->potn;
          temp_iter = 0;
        }
      }
      if(temp_iter == 10) {
        return i;
      }
      if(temp_iter) {
        temp_iter++;
      }
    }
  }
  return -1;
}

void myNesterov::mkl_malloc_free() {
  mkl_free(iter_st);
  mkl_free(x_st);
  mkl_free(y_st);
  mkl_free(y_dst);
  mkl_free(y_wdst);
  mkl_free(y_pdst);
  mkl_free(y_pdstl);
  mkl_free(z_st);
  mkl_free(z_dst);
  mkl_free(z_wdst);
  mkl_free(z_pdst);
  mkl_free(z_pdstl);
  mkl_free(x0_st);
  mkl_free(y0_st);
  mkl_free(y0_dst);
  mkl_free(y0_wdst);
  mkl_free(y0_pdst);
  mkl_free(y0_pdstl);
  mkl_free(cellLambdaArr);
  mkl_free(pcofArr);
  // mkl_free(alphaArrCD);
  // mkl_free(deltaArrCD);
}

void myNesterov::SummarizeNesterovOpt(int last_index) {
  prec tot_hpwl_y;
  prec tot_hpwl_x;

  if(STAGE == mGP3D) {
    mGP3D_iterCNT = last_index + 1;
    hpwl_mGP3D = it->tot_hpwl;
    printf("\n");
    printf("INFO:    SUMMARY mGP3D\n");
    printf("INFO:    #iterations = %d\n", mGP3D_iterCNT);
    printf("INFO:        mGP3D ITERATION HPWL %.4lf\n", it->tot_hpwl);
    printf("INFO:        mGP3D ITERATION OVFL %.4lf\n", it->ovfl);
    printf("INFO:        mGP3D      POTENTIAL %.4lf\n", it->potn);
    mGP3D_tot_iter = last_index;
    mGP3D_opt_phi_cof = opt_phi_cof;
  }
  else if(STAGE == mGP2D) {
    mGP2D_iterCNT = last_index + 1;
    hpwl_mGP2D = it->tot_hpwl;
    printf("\n");
    printf("INFO:    SUMMARY mGP2D\n");
    printf("INFO:    #iterations = %d\n", mGP2D_iterCNT);
    printf("INFO:        mGP2D ITERATION HPWL %.4lf\n", it->tot_hpwl);
    printf("INFO:        mGP2D ITERATION OVFL %.4lf\n", it->ovfl);
    printf("INFO:        mGP2D      POTENTIAL %.4lf\n", it->potn);
    mGP2D_tot_iter = last_index;
    mGP2D_opt_phi_cof = opt_phi_cof;
  }
  else if(STAGE == cGP3D) {
    cGP3D_iterCNT = last_index + 1;
    hpwl_cGP3D = it->tot_hpwl;
    printf("\n");
    printf("INFO:    SUMMARY cGP3D\n");
    printf("INFO:    #iterations = %d\n", cGP3D_iterCNT);
    printf("INFO:        cGP3D ITERATION HPWL %.4lf\n", it->tot_hpwl);
    printf("INFO:        cGP3D ITERATION OVFL %.4lf\n", it->ovfl);
    printf("INFO:        cGP3D      POTENTIAL %.4lf\n", it->potn);
    cGP3D_tot_iter = last_index;
    cGP3D_opt_phi_cof = opt_phi_cof;
  }
  else if(STAGE == cGP2D) {
    cGP2D_iterCNT = last_index + 1;
    hpwl_cGP2D = it->tot_hpwl;
    printf("\n");
    printf("INFO:    SUMMARY cGP2D\n");
    printf("INFO:    #iterations = %d\n", cGP2D_iterCNT);
    printf("INFO:        cGP2D ITERATION HPWL %.4lf\n", it->tot_hpwl);
    printf("INFO:        cGP2D ITERATION OVFL %.4lf\n", it->ovfl);
    printf("INFO:        cGP2D      POTENTIAL %.4lf\n", it->potn);
    cGP2D_tot_iter = last_index;
    cGP2D_opt_phi_cof = opt_phi_cof;
  }
  else {
  }

  cell_update(x_st, N_org);

  net_update(y_st);
  tot_hpwl_y = GetHpwl();

  net_update(x_st);
  tot_hpwl_x = GetHpwl();

  printf("INFO:    TOTAL HPWL (U_k, V_k) = %.6lf, %.6lf\n", tot_hpwl_x,
         tot_hpwl_y);
}

void getCostFuncGradient3(struct FPOS *dst, struct FPOS *wdst,
                          struct FPOS *pdst, struct FPOS *pdstl, int N,
                          prec *cellLambdaArr) {
  if(FILLER_PLACE && DEN_ONLY_PRECON) {
    getCostFuncGradient2_filler_DEN_ONLY_PRECON(dst, wdst, pdst, pdstl,
                                                moduleCNT, N, cellLambdaArr);
  }
  else if(FILLER_PLACE && !DEN_ONLY_PRECON) {
    getCostFuncGradient2_filler(dst, wdst, pdst, pdstl, moduleCNT, N,
                                cellLambdaArr);
  }
  else if(!FILLER_PLACE && DEN_ONLY_PRECON) {
    getCostFuncGradient2_DEN_ONLY_PRECON(dst, wdst, pdst, pdstl, N,
                                         cellLambdaArr);
  }
  else if(!FILLER_PLACE && !DEN_ONLY_PRECON) {
    getCostFuncGradient2(dst, wdst, pdst, pdstl, N, cellLambdaArr);
  }
}

void getCostFuncGradient2(struct FPOS *dst, struct FPOS *wdst,
                          struct FPOS *pdst, struct FPOS *pdstl, int N,
                          prec *cellLambdaArr) {
  //    bool timeon = true;
  //    double time = 0;

  //    if(timeon) { time_start(&time); }

  int i = 0;
#pragma omp parallel default(none) private(i)                              \
    shared(N, cellLambdaArr, gcell_st, dampParam, STAGE, pdstl, dst, wdst, \
           pdst, MIN_PRE, constraintDrivenCMD, opt_phi_cof, lambda2CMD)
  {
    CELL *cell = NULL;
    FPOS wgrad;
    FPOS pgrad;
    FPOS pgradl;
    FPOS wpre;
    FPOS charge_dpre;
    FPOS pre;

#pragma omp for
    for(i = 0; i < N; i++) {
      cell = &gcell_st[i];
      cellLambdaArr[i] *= dampParam;
      if(cell->flg == Macro && (STAGE == cGP3D || STAGE == cGP2D)) {
        wgrad.SetZero();
        pgrad.SetZero();
        pgradl.SetZero();
      }
      else {
        wlen_grad(i, &wgrad);
        if(STAGE == mGP2D) {
          if(constraintDrivenCMD == false) {
            potn_grad_2D(i, &pgrad);
          }
          else if(constraintDrivenCMD == true) {
            // if (lambda2CMD == false) {
            //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
            //} else if (lambda2CMD == true) {
            potn_grad_2D(i, &pgrad);
            potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
            //}
          }
        }
        else if(STAGE == cGP2D) {
          if(constraintDrivenCMD == false) {
            potn_grad_2D(i, &pgrad);
          }
          else if(constraintDrivenCMD == true) {
            // if (lambda2CMD == false) {
            //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
            //} else if (lambda2CMD == true) {
            potn_grad_2D(i, &pgrad);
            potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
            //}
          }
        }
      }

      wdst[i] = wgrad;
      pdst[i] = pgrad;

      dst[i].x = wgrad.x + opt_phi_cof * pgrad.x;
      dst[i].y = wgrad.y + opt_phi_cof * pgrad.y;

      if(lambda2CMD == true) {
        pdstl[i] = pgradl;
        dst[i].x += cellLambdaArr[i] * pgradl.x;
        dst[i].y += cellLambdaArr[i] * pgradl.y;
      }

      wlen_pre(i, &wpre);
      potn_pre(i, &charge_dpre);
      //#ifdef DEN_ONLY_PRECON
      //            //if (DEN_ONLY_PRECON) {
      //            pre = charge_dpre;
      //            //}
      //#else
      // else {
      pre.x = wpre.x + opt_phi_cof * charge_dpre.x;
      pre.y = wpre.y + opt_phi_cof * charge_dpre.y;
      //}
      //#endif
      if(pre.x < MIN_PRE)
        pre.x = MIN_PRE;
      if(pre.y < MIN_PRE)
        pre.y = MIN_PRE;

      dst[i].x /= pre.x;
      dst[i].y /= pre.y;
    }
  }
  //    if(timeon) { time_end(&time);cout << "!!: " << time << endl;}
  //    exit(1);
}
void getCostFuncGradient2_DEN_ONLY_PRECON(struct FPOS *dst, struct FPOS *wdst,
                                          struct FPOS *pdst, struct FPOS *pdstl,
                                          int N, prec *cellLambdaArr) {
  struct FPOS wgrad;
  struct FPOS pgrad;
  struct FPOS pgradl;
  struct FPOS wpre;
  struct FPOS charge_dpre;
  struct FPOS pre;
  struct CELL *cell = NULL;

  for(int i = 0; i < N; i++) {
    cell = &gcell_st[i];
    cellLambdaArr[i] *= dampParam;
    if(cell->flg == Macro && (STAGE == cGP3D || STAGE == cGP2D)) {
      wgrad.SetZero();
      pgrad.SetZero();
      pgradl.SetZero();
    }
    else {
      wlen_grad(i, &wgrad);
      if(STAGE == mGP2D) {
        if(constraintDrivenCMD == false)
          potn_grad_2D(i, &pgrad);
        else if(constraintDrivenCMD == true) {
          // if (lambda2CMD == false) {
          //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
          //} else if (lambda2CMD == true) {
          potn_grad_2D(i, &pgrad);
          potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
          //}
        }
      }
      else if(STAGE == cGP2D) {
        if(constraintDrivenCMD == false)
          potn_grad_2D(i, &pgrad);
        else if(constraintDrivenCMD == true) {
          // if (lambda2CMD == false) {
          //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
          //} else if (lambda2CMD == true) {
          potn_grad_2D(i, &pgrad);
          potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
          //}
        }
      }
    }

    wdst[i] = wgrad;
    pdst[i] = pgrad;

    dst[i].x = wgrad.x + opt_phi_cof * pgrad.x;
    dst[i].y = wgrad.y + opt_phi_cof * pgrad.y;

    if(lambda2CMD == true) {
      pdstl[i] = pgradl;
      dst[i].x += cellLambdaArr[i] * pgradl.x;
      dst[i].y += cellLambdaArr[i] * pgradl.y;
    }

    wlen_pre(i, &wpre);
    potn_pre(i, &charge_dpre);
    //#ifdef DEN_ONLY_PRECON
    // if (DEN_ONLY_PRECON) {
    pre = charge_dpre;
    //}
    //#else
    //            //else {
    //            pre.x = wpre.x + opt_phi_cof * charge_dpre.x;
    //            pre.y = wpre.y + opt_phi_cof * charge_dpre.y;
    //            //}
    //#endif
    if(pre.x < MIN_PRE)
      pre.x = MIN_PRE;
    if(pre.y < MIN_PRE)
      pre.y = MIN_PRE;

    dst[i].x /= pre.x;
    dst[i].y /= pre.y;
  }
}

void getCostFuncGradient2_filler(struct FPOS *dst, struct FPOS *wdst,
                                 struct FPOS *pdst, struct FPOS *pdstl,
                                 int start_idx, int end_idx,
                                 prec *cellLambdaArr) {
  struct FPOS pgrad;
  struct FPOS pgradl;
  struct FPOS charge_dpre;
  struct FPOS pre;
  struct CELL *cell = NULL;

  for(int i = start_idx; i < end_idx; i++) {
    cell = &gcell_st[i];
    if(cell->flg != FillerCell)
      continue;

    if(STAGE == mGP2D) {
      if(constraintDrivenCMD == false)
        potn_grad_2D(i, &pgrad);
      else if(constraintDrivenCMD == true) {
        // if (lambda2CMD == false) {
        //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
        //} else if (lambda2CMD == true) {
        potn_grad_2D(i, &pgrad);
        potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
        //}
      }
    }
    else if(STAGE == cGP2D) {
      if(constraintDrivenCMD == false)
        potn_grad_2D(i, &pgrad);
      else if(constraintDrivenCMD == true) {
        // if (lambda2CMD == false) {
        //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
        //} else if (lambda2CMD == true) {
        potn_grad_2D(i, &pgrad);
        potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
        //}
      }
    }

    wdst[i].SetZero();
    pdst[i] = pgrad;

    dst[i].x = opt_phi_cof * pgrad.x;
    dst[i].y = opt_phi_cof * pgrad.y;

    if(lambda2CMD == true) {
      pdstl[i] = pgradl;
      dst[i].x += cellLambdaArr[i] * pgradl.x;  // 53
      dst[i].y += cellLambdaArr[i] * pgradl.y;
    }

    potn_pre(i, &charge_dpre);
    //#ifdef DEN_ONLY_PRECON
    //            //if (DEN_ONLY_PRECON) {
    //            pre = charge_dpre;
    //            //}
    //#else
    // else {
    pre.x = opt_phi_cof * charge_dpre.x;
    pre.y = opt_phi_cof * charge_dpre.y;
    //}
    //#endif

    if(pre.x < MIN_PRE)
      pre.x = MIN_PRE;
    if(pre.y < MIN_PRE)
      pre.y = MIN_PRE;

    dst[i].x /= pre.x;
    dst[i].y /= pre.y;
  }
}

void getCostFuncGradient2_filler_DEN_ONLY_PRECON(
    struct FPOS *dst, struct FPOS *wdst, struct FPOS *pdst, struct FPOS *pdstl,
    int start_idx, int end_idx, prec *cellLambdaArr) {
  struct FPOS pgrad;
  struct FPOS pgradl;
  struct FPOS charge_dpre;
  struct FPOS pre;
  struct CELL *cell = NULL;

  for(int i = start_idx; i < end_idx; i++) {
    cell = &gcell_st[i];
    if(cell->flg != FillerCell)
      continue;
    // if (STAGE == mGP3D) {
    //    if (constraintDrivenCMD == false)
    //        potn_grad (i, &pgrad);
    //    else if (constraintDrivenCMD == true) {
    //        if (lambda2CMD == false) {
    //            potn_grad_local (i, &pgrad, &cellLambdaArr[i]);
    //        } else if (lambda2CMD == true) {
    //            potn_grad (i, &pgrad);
    //            potn_grad_local (i, &pgradl, &cellLambdaArr[i]);
    //        }
    //    }
    //}
    if(STAGE == mGP2D) {
      if(constraintDrivenCMD == false)
        potn_grad_2D(i, &pgrad);
      else if(constraintDrivenCMD == true) {
        // if (lambda2CMD == false) {
        //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
        //} else if (lambda2CMD == true) {
        potn_grad_2D(i, &pgrad);
        potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
        //}
      }
    }
    else if(STAGE == cGP2D) {
      if(constraintDrivenCMD == false)
        potn_grad_2D(i, &pgrad);
      else if(constraintDrivenCMD == true) {
        // if (lambda2CMD == false) {
        //    potn_grad_2D_local (i, &pgrad, &cellLambdaArr[i]);
        //} else if (lambda2CMD == true) {
        potn_grad_2D(i, &pgrad);
        potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
        //}
      }
    }

    wdst[i].SetZero();
    pdst[i] = pgrad;

    dst[i].x = opt_phi_cof * pgrad.x;
    dst[i].y = opt_phi_cof * pgrad.y;

    if(lambda2CMD == true) {
      pdstl[i] = pgradl;
      dst[i].x += cellLambdaArr[i] * pgradl.x;  // 53
      dst[i].y += cellLambdaArr[i] * pgradl.y;
    }

    potn_pre(i, &charge_dpre);
    //#ifdef DEN_ONLY_PRECON
    // if (DEN_ONLY_PRECON) {
    pre = charge_dpre;
    //}
    //#else
    //            //else {
    //            pre.x = opt_phi_cof * charge_dpre.x;
    //            pre.y = opt_phi_cof * charge_dpre.y;
    //            //}
    //#endif

    if(pre.x < MIN_PRE)
      pre.x = MIN_PRE;
    if(pre.y < MIN_PRE)
      pre.y = MIN_PRE;

    dst[i].x /= pre.x;
    dst[i].y /= pre.y;
  }
}

void myNesterov::z_init() {
  FPOS half_densize;
  prec zx = 0.0f, zy = 0.0f, zz = 0.0f;
  prec coeffi = z_ref_alpha * GP_SCAL;

  for(int j = start_idx; j < end_idx; j++) {
    half_densize = gcell_st[j].half_den_size;

    if(GP_DIM_ONE) {
      zx = y_st[j].x + place_backup.cnt.x * coeffi * y_dst[j].x;
      zy = y_st[j].y + place_backup.cnt.y * coeffi * y_dst[j].y;
      //            zz = y_st[j].z + place_backup.cnt.z * coeffi * y_dst[j].z;
    }
    else {
      zx = y_st[j].x + z_ref_alpha * y_dst[j].x;
      zy = y_st[j].y + z_ref_alpha * y_dst[j].y;
      //            zz = y_st[j].z + z_ref_alpha * y_dst[j].z;
//      y_st[j].Dump("y_st_test");
    }

    z_st[j].x = valid_coor2(zx, half_densize.x, 0);
    z_st[j].y = valid_coor2(zy, half_densize.y, 1);
    //        z_st[j].z = valid_coor2(zz, half_densize.z, 2);
  }
//  exit(1);
}

void myNesterov::UpdateNesterovOptStatus() {
  std::swap(z_st, y_st);
  std::swap(z_dst, y_dst);
  std::swap(z_wdst, y_wdst);
  std::swap(z_pdst, y_pdst);
  std::swap(z_pdstl, y_pdstl);
  std::swap(x_st, x0_st);
  std::swap(y_st, y0_st);
  std::swap(y_dst, y0_dst);
  std::swap(y_wdst, y0_wdst);
  std::swap(y_pdst, y0_pdst);
  std::swap(y_pdstl, y0_pdstl);

  //    for (int j=start_idx; j<end_idx; j++) {
  //        half_densize = gcell_st[j].half_den_size;
  //        z_st   [j] = y_st   [j];
  //        z_dst  [j] = y_dst  [j];
  //        z_wdst [j] = y_wdst [j];
  //        z_pdst [j] = y_pdst [j];
  //        z_pdstl[j] = y_pdstl[j];
  //        x_st   [j] = x0_st   [j];
  //        y_st   [j] = y0_st   [j];
  //        y_dst  [j] = y0_dst  [j];
  //        y_wdst [j] = y0_wdst [j];
  //        y_pdst [j] = y0_pdst [j];
  //        y_pdstl[j] = y0_pdstl[j];
  //    }
}

void myNesterov::UpdateNesterovIter(int iter, struct ITER *it,
                                    struct ITER *last_it) {
  it->grad = get_norm(y_dst, N, 2.0);
  it->potn = gsum_phi;
  it->ovfl = gsum_ovfl;
  it->dis00 = get_dis(z_st, y_st, N);
  it->wcof = get_wlen_cof(it->ovfl);
  wlen_cof = fp_mul(base_wcof, it->wcof);
  wlen_cof_inv = fp_inv(wlen_cof);
  pcofArr[iter % 100] = opt_phi_cof;
  // if (constraintDrivenCMD) {
  //    alphaArrCD[iter%100] = ALPHA;
  //    deltaArrCD[iter%100] = initialALPHA2D;
  //}

  if(!FILLER_PLACE) {
    it->tot_hpwl = GetHpwl();
    it->tot_stnwl = total_stnwl.x + total_stnwl.y;  // lutong
    // opt_w_cof = stn_weight ;   // lutong
    // opt_w_cof = (1.0 - it->ovfl) * stn_weight;   // lutong
    if(it->ovfl <= stn_weight) {
      opt_w_cof = 1;
    }
    else {
      opt_w_cof = (1.0 - it->ovfl) / (1.0 - stn_weight);  // lutong
    }

    it->tot_wwl =
        (1.0 - opt_w_cof) * it->tot_hpwl + opt_w_cof * it->tot_stnwl;  // lutong
    it->hpwl = total_hpwl;
    // IK 05/09/17
    // if (isRoutability) {
    //    if (it->ovfl <= 0.215) {
    //        UPPER_PCOF = 1.01;
    //    }
    //}
    // if (stnCMD == true) {
    //  it->pcof = get_phi_cof1 ((it->tot_wwl-last_it->tot_wwl)/ refDeltaWL );
    //} else {
    it->pcof = get_phi_cof1((it->tot_hpwl - last_it->tot_hpwl) / refDeltaWL );
    //}
    if(temp_iter == 1) {
      // opt_phi_cof /= 1.05;
    }
    else if(temp_iter > 1) {
      //;
    }
    else {
      opt_phi_cof *= it->pcof;  // igkang
    }
    if(lambda2CMD == true) {
      opt_phi_cof_local = 0;
      for(int i = 0; i < N; i++) {
        opt_phi_cof_local += (cellLambdaArr[i]);
      }
      opt_phi_cof_local /= (prec)N;
    }
    it->wlen = total_wlen;
  }
  else {
    it->hpwl = last_it->hpwl;
    it->tot_hpwl = last_it->tot_hpwl;
    it->tot_stnwl = last_it->tot_stnwl;  // lutong
    it->tot_wwl = last_it->tot_wwl;      // lutong
    it->wlen.SetZero();
    it->tot_wlen = 0;
  }

  if((iter % 10 == 0) && (isPlot || plotCellCMD)) {
    cell_update(x_st, N);

    // For circuit viewer
    //        SavePlot(string("Nesterov - Iter: " + std::to_string(iter)),
    //        true);

    string modeStr =
        (STAGE == cGP2D) ? "cGP2D" : (STAGE == mGP2D) ? "mGP2D" : "";
    // For JPEG Saving
    SaveCellPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(iter)), true,
                       string(dir_bnd) + string("/cell/" + modeStr + "_cell_") +
                           intoFourDigit(iter));
    SaveBinPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(iter)),
                      string(dir_bnd) + string("/bin/" + modeStr + "_bin_") +
                          intoFourDigit(iter));
    SaveArrowPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(iter)),
                        string(dir_bnd) +
                            string("/arrow/" + modeStr + "_arrow_") +
                            intoFourDigit(iter));
  }

  // if ((approxiPlotCMD && (iter%10 == 0 || iter==1)) || (approxiPlotCMD &&
  // isBloatingBegin && iter%5 == 0))
  // if (!approxiPlotCMD && (plotCellCMD || plotDensityCMD || plotFieldCMD))
  // PlotNesterov (iter);

  time_calc(last_it->cpu_curr, &it->cpu_curr, &it->cpu_cost);
  PrintNesterovOptStatus(iter);
  fflush(stdout);
}

void get_lc(struct FPOS *y_st, struct FPOS *y_dst, struct FPOS *z_st,
            struct FPOS *z_dst, struct ITER *iter, int N) {
  if(FILLER_PLACE)
    get_lc3_filler(y_st + moduleCNT, y_dst + moduleCNT, z_st + moduleCNT,
                   z_dst + moduleCNT, iter, gfiller_cnt);
  else
    get_lc3(y_st, y_dst, z_st, z_dst, iter, N);
}

void get_lc3(struct FPOS *y_st, struct FPOS *y_dst, struct FPOS *z_st,
             struct FPOS *z_dst, struct ITER *iter, int N) {
  prec yz_dis = 0;
  prec yz_dnm = 0;
  prec alpha;
  prec lc = 0;

  yz_dis = get_dis(y_st, z_st, N);
  yz_dnm = get_dis(y_dst, z_dst, N);

  lc = yz_dnm / yz_dis;
  // cout <<"N: " <<N <<endl;
  // cout <<"yz_dnm/yz_dis: " <<yz_dnm <<", " <<yz_dis <<endl;
  alpha = 1.0 / lc;
  iter->lc = lc;
  // if (alpha < 1) alpha = 1;
  iter->alpha00 = alpha;
}

void get_lc3_filler(struct FPOS *y_st, struct FPOS *y_dst, struct FPOS *z_st,
                    struct FPOS *z_dst, struct ITER *iter, int N) {
  prec yz_dis = 0;
  prec yz_dnm = 0;
  prec alpha;
  prec lc = 0;

  yz_dis = get_dis(y_st, z_st, N);
  yz_dnm = get_dis(y_dst, z_dst, N);

  lc = yz_dnm / yz_dis;
  alpha = 1.0 / lc;
  iter->lc = lc;
  iter->alpha00 = alpha;
}

void myNesterov::InitializationCostFunctionGradient(prec *sum_wgrad0,
                                                    prec *sum_pgrad0) {
  prec tmp_sum_wgrad = 0;
  prec tmp_sum_pgrad = 0;
  struct FPOS wgrad;
  struct FPOS pgrad;
  struct FPOS pgradl;
  struct CELL *cell = NULL;

  for(int i = 0; i < N; i++) {
    cell = &gcell_st[i];
    wlen_grad(i, &wgrad);
    // if (STAGE==mGP3D) {
    //    if (constraintDrivenCMD == false)
    //        potn_grad (i, &pgrad);
    //    else if (constraintDrivenCMD == true) {
    //        if (lambda2CMD == false)
    //            potn_grad_local (i, &pgrad, &cellLambdaArr[i]);
    //        else if (lambda2CMD == true) {
    //            potn_grad (i, &pgrad);
    //            potn_grad_local (i, &pgradl, &cellLambdaArr[i]);
    //        }
    //    }
    //}
    // else if (STAGE==cGP3D) {
    //    if (cell->flg==Macro) {
    //        wgrad = zeroFPoint;
    //        pgrad = zeroFPoint;
    //        pgradl= zeroFPoint;
    //    } else {
    //        if (constraintDrivenCMD == false)
    //            potn_grad (i, &pgrad);
    //        else if (constraintDrivenCMD == true) {
    //            if (lambda2CMD == false)
    //                potn_grad_local (i, &pgrad, &cellLambdaArr[i]);
    //            else if (lambda2CMD == true) {
    //                potn_grad (i, &pgrad);
    //                potn_grad_local (i, &pgradl, &cellLambdaArr[i]);
    //            }
    //        }
    //    }
    //}
    if(STAGE == cGP2D) {
      if(cell->flg == Macro) {
        wgrad.SetZero();
        pgrad.SetZero();
        pgradl.SetZero();
      }
      else {
        if(constraintDrivenCMD == false)
          potn_grad_2D(i, &pgrad);
        else if(constraintDrivenCMD == true) {
          if(lambda2CMD == false)
            potn_grad_2D_local(i, &pgrad, &cellLambdaArr[i]);
          else if(lambda2CMD == true) {
            potn_grad_2D(i, &pgrad);
            potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
          }
        }
        // wgrad.z = 0;
        // pgrad.z = 0;
        // pgradl.z= 0;
      }
    }
    else if(STAGE == mGP2D) {
      if(constraintDrivenCMD == false)
        potn_grad_2D(i, &pgrad);
      else if(constraintDrivenCMD == true) {
        if(lambda2CMD == false)
          potn_grad_2D_local(i, &pgrad, &cellLambdaArr[i]);
        else if(lambda2CMD == true) {
          potn_grad_2D(i, &pgrad);
          potn_grad_2D_local(i, &pgradl, &cellLambdaArr[i]);
        }
      }
      // wgrad.z = 0.0;
      // pgrad.z = 0.0;
      // pgradl.z= 0.0;
    }

    y_wdst[i] = wgrad;
    y_pdst[i] = pgrad;
    if(lambda2CMD == true)
      y_pdstl[i] = pgradl;

    tmp_sum_wgrad += fabs(wgrad.x) + fabs(wgrad.y);  // + fabs(wgrad.z);
    tmp_sum_pgrad += fabs(pgrad.x) + fabs(pgrad.y);  // + fabs(pgrad.z);
  }
  *sum_wgrad0 = tmp_sum_wgrad;
  *sum_pgrad0 = tmp_sum_pgrad;
}

void myNesterov::ShiftPL_SA(struct FPOS *y_st, int N) {
  if(FILLER_PLACE)
    ShiftPL_SA_sub(y_st + moduleCNT, gfiller_cnt);
  else
    ShiftPL_SA_sub(y_st, N);
}

void myNesterov::ShiftPL_SA_sub(struct FPOS *y_st, int N) {
  struct FPOS rnd;
  struct FPOS len;
  struct FPOS drnd;
  struct FPOS v;
  struct FPOS d;
  struct CELL *cell = NULL;
  struct FPOS half_densize;
  struct FPOS center;

  prec ratio_var_pl = 0.025;

  for(int i = 0; i < N; i++) {
    cell = &gcell_st[i];
    if(STAGE == cGP3D) {
      if(cell->flg == Macro)
        continue;
    }
    half_densize = cell->half_den_size;

    center = y_st[i];

    len.x = 2.0 * ratio_var_pl * half_densize.x;
    len.y = 2.0 * ratio_var_pl * half_densize.y;

    rnd.x = (prec)rand();
    rnd.y = (prec)rand();

    drnd.x = inv_RAND_MAX * rnd.x - 0.5;
    drnd.y = inv_RAND_MAX * rnd.y - 0.5;

    d.x = drnd.x * len.x;
    d.y = drnd.y * len.y;

    v.x = center.x + d.x;
    v.y = center.y + d.y;

    center = valid_coor00(v, half_densize);

    y_st[i] = center;
  }
}

void myNesterov::UpdateAlpha(struct ITER *it) {
  if(ALPHA < maxALPHA) {
    if(it->pcof > LOWER_PCOF && it->pcof < UPPER_PCOF) {
      ALPHA *= it->pcof;
    }
    else if(it->pcof <= LOWER_PCOF) {
      ALPHA *= LOWER_PCOF;
    }
    else if(it->pcof >= UPPER_PCOF) {
      ALPHA *= UPPER_PCOF;
    }
    else {
    }
  }
}

void myNesterov::UpdateBeta(struct ITER *it) {
  if(it->pcof > LOWER_PCOF && it->pcof < UPPER_PCOF) {
    BETA *= it->pcof;
  }
  else if(it->pcof <= LOWER_PCOF) {
    BETA *= LOWER_PCOF;
  }
  else if(it->pcof >= UPPER_PCOF) {
    BETA *= UPPER_PCOF;
  }
  else {
  }
}

void myNesterov::PrintNesterovOptStatus(int iter) {
  printf("\n");
  printf("ITER: %d\n", iter);
  printf("    HPWL=%.6f\n", it->tot_hpwl);
  // if (stnCMD)     printf ("    STNWL=%f\n", it->tot_stnwl);
  // printf ("    WEIGHTEDWL=%f\n", it->tot_wwl);
  printf("    OVFL=%.6f\n", it->ovfl);
  printf("    HPWL=(%.6f, %.6f)\n", it->hpwl.x, it->hpwl.y);
  printf("    POTN=%.6E\n", it->potn);
  printf("    PHIC=%.6E\n", opt_phi_cof);
  // if (lambda2CMD) {
  //    printf ("    LOCL=%.6E\n", opt_phi_cof_local);
  //    printf ("    ALPH=%.6E\n", ALPHA);
  //}
  // if (dynamicStepCMD) printf ("    UPCF=%.6E\n", UPPER_PCOF);
  printf("    GRAD=%.6E\n", it->grad);
  // printf ("    Alph=%.6E\n", it->alpha00);
  printf("    NuBT=%d\n", backtrack_cnt);
  printf("    CPU =%.6f\n", it->cpu_cost);
}

void myNesterov::PlotNesterov(int iter) {

  /*
  if(plotCellCMD) {
    cell_update(x_st, N);
    if(STAGE == mGP3D)
      plot("S1-cell-mGP3D-", iter, 1.0, 1);
    if(STAGE == mGP2D)
      plot("S2-cell-mGP2D-", iter, 1.0, 1);
    if(STAGE == cGP3D)
      plot("S4-cell-cGP3D-", iter, 1.0, 1);
    if(STAGE == cGP2D)
      plot("S5-cell-cGP2D-", iter, 1.0, 1);
  }
  if(plotDensityCMD) {
    if(STAGE == mGP3D)
      plot_bin("S1-den-mGP3D-", iter, 1.0, 0);
    if(STAGE == mGP2D)
      plot_bin("S2-den-mGP2D-", iter, 1.0, 0);
    if(STAGE == cGP3D)
      plot_bin("S4-den-cGP3D-", iter, 1.0, 0);
    if(STAGE == cGP2D)
      plot_bin("S5-den-cGP2D-", iter, 1.0, 0);
  }
  if(plotFieldCMD) {
    if(STAGE == mGP3D)
      plot_bin("S1-field-mGP3D-", iter, 1.0, 4);
    if(STAGE == mGP2D)
      plot_bin("S2-field-mGP2D-", iter, 1.0, 4);
    if(STAGE == cGP3D)
      plot_bin("S4-field-cGP3D-", iter, 1.0, 4);
    if(STAGE == cGP2D)
      plot_bin("S5-field-cGP2D-", iter, 1.0, 4);
  }
  */
}

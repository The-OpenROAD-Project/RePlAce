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
#include <iomanip>
#include <omp.h>
#include <unordered_map>
#include <fstream>
#include <sstream>

#include "replace_private.h"
#include "macro.h"
#include "opt.h"
#include "wlen.h"

#include "lefdefIO.h"

using std::min;
using std::max;
using std::make_pair;
using std::fstream;
using std::stringstream;

int wcof_flg;
int MAX_EXP;
int NEG_MAX_EXP;
prec hpwl_mGP3D;
prec hpwl_mGP2D;
prec hpwl_cGP3D;
prec hpwl_cGP2D;
prec TSV_WEIGHT;

FPOS wcof00;
FPOS wcof00_org;
FPOS total_hpwl;
FPOS total_stnwl;  // lutong
FPOS total_wlen;
FPOS gp_wlen_weight;
FPOS dp_wlen_weight;
FPOS base_wcof;
FPOS wlen_cof;
FPOS wlen_cof_inv;
EXP_ST *exp_st;

void SetMAX_EXP_wlen() {
  MAX_EXP = 300;
  NEG_MAX_EXP = -300;
}

FPOS get_wlen_cof(prec ovf) {
  switch(wcof_flg) {
    case 1:
      return get_wlen_cof1(ovf);
      break;
    case 2:
      return get_wlen_cof2(ovf);
      break;
    default:
      return get_wlen_cof3(ovf);
      break;
  }
}

FPOS get_wlen_cof3(prec ovf) {
  FPOS cof;
  prec tmp = 0;
  if(ovf > 1.0) {
    cof.x = 1.0 / 10.0;
    cof.y = 1.0 / 10.0;
  }
  else if(ovf < 0.1) {
    cof.x = 1.0 / 0.01;
    cof.y = 1.0 / 0.01;
  }
  else {
    tmp = 1.0 / pow(10.0, (ovf - 0.1) * 10.0 / 3.0 - 2.0);
    cof.x = cof.y =  tmp;
  }
  return cof;
}

FPOS get_wlen_cof1(prec ovf) {
  FPOS cof;

  if(ovf > 1.0)
    cof.x = cof.y = 1.0 / 10.0;
  else if(ovf < 0.1)
    cof.x = cof.y = 1.0 / 0.01;
  else
    cof.x = cof.y = 1.0 / pow(10.0, (ovf - 0.1) * 10.0 / 3.0 - 2.0);

  return cof;
}

// 
// Described in ePlace-MS paper !!!!
//
//
FPOS get_wlen_cof2(prec ovf) {
  FPOS cof;
  prec tmp = 0.0;
  if(ovf > 1.0) {
    cof.x = 0.1;
    cof.y = 0.1;
  }
  else if(ovf < 0.1) {
    cof.x = 10.0;
    cof.y = 10.0;
  }
  else {
    tmp = 1.0 / pow(10.0, (ovf - 0.1) * 20 / 9.0 - 1.0);
    cof.x = cof.y = tmp;
  }
//  cout << "ovfl: " << ovf << endl;
//  cof.Dump("current_wlen_cof:");
  return cof;
}

void wlen_init() {
//  int i = 0 /* ,cnt=exp_st_cnt */;
  /* prec interval = exp_interval ; */
//  exp_st = (EXP_ST *)malloc(sizeof(EXP_ST) * exp_st_cnt);
//  for(i = 0; i < exp_st_cnt; i++) {
//    exp_st[i].x = (prec)i * exp_interval - MAX_EXP;
//    exp_st[i].val = exp(exp_st[i].x);
//    if(i > 0) {
//      exp_st[i - 1].y_h = (exp_st[i].val - exp_st[i - 1].val) / exp_interval;
//    }
//  }

  gp_wlen_weight.x = gp_wlen_weight.y = 1.0;
  dp_wlen_weight.x = dp_wlen_weight.y = 1.0;
}

// 
void wcof_init(FPOS bstp) {
  // 0.5*(~) = binSize;
  //
  base_wcof.x = wcof00.x / (0.5 * (bstp.x + bstp.y));
  base_wcof.y = wcof00.y / (0.5 * (bstp.x + bstp.y));

  wlen_cof = fp_scal(0.1, base_wcof);
//  wlen_cof = base_wcof;
  wlen_cof_inv = fp_inv(wlen_cof);
}

prec get_wlen() {
  ////////////////////////////

  switch(WLEN_MODEL) {
    case WA:
      return get_wlen_wa();
      break;

    case LSE:
      return get_wlen_lse();
      break;
  }

  ////////////////////////////
  // return 0;
}

prec get_wlen_wa() {
  FPOS net_wlen;
  prec tot_wlen = 0;

  total_wlen.SetZero();

  for(int i = 0; i < netCNT; i++) {
    net_wlen = get_net_wlen_wa(&netInstance[i]);

    total_wlen.x += net_wlen.x;
    total_wlen.y += net_wlen.y;
  }

  tot_wlen = total_wlen.x + total_wlen.y;  // +

  return tot_wlen;
}

prec get_wlen_lse(void) {
  NET *net = NULL;
  FPOS net_wlen;
  prec tot_wlen = 0;

  total_wlen.SetZero();

  for(int i = 0; i < netCNT; i++) {
    net = &netInstance[i];
    if(net->pinCNTinObject <= 1)
      continue;
    net_wlen = get_net_wlen_lse(net);
    total_wlen.x += net_wlen.x;
    total_wlen.y += net_wlen.y;
  }

  tot_wlen = total_wlen.x + total_wlen.y;

  return tot_wlen;

  /* return  */
  /*   total_wlen.x * wlen_cof_inv.x * gp_wlen_weight.x + */
  /*   total_wlen.y * wlen_cof_inv.y * gp_wlen_weight.y +  */
}

FPOS get_net_wlen_wa(NET *net) {
  FPOS net_wlen;
  FPOS sum_num1 = net->sum_num1;
  FPOS sum_num2 = net->sum_num2;
  FPOS sum_denom1 = net->sum_denom1;
  FPOS sum_denom2 = net->sum_denom2;

  if(net->pinCNTinObject <= 1)
    return FPOS(0,0);

  net_wlen.x = sum_num1.x / sum_denom1.x - sum_num2.x / sum_denom2.x;
  net_wlen.y = sum_num1.y / sum_denom1.y - sum_num2.y / sum_denom2.y;

  return net_wlen;
}

FPOS get_net_wlen_lse(NET *net) {
  FPOS sum1, sum2;
  FPOS fp, wlen;
  PIN *pin = NULL;

  for(int i = 0; i < net->pinCNTinObject; i++) {
    pin = net->pin[i];
    fp = pin->fp;

    sum1.x += get_exp(wlen_cof.x * fp.x);
    sum1.y += get_exp(wlen_cof.y * fp.y);

    sum2.x += get_exp(-1.0 * wlen_cof.x * fp.x);
    sum2.y += get_exp(-1.0 * wlen_cof.y * fp.y);
  }

  wlen.x = log(sum1.x) + log(sum2.x);
  wlen.y = log(sum1.y) + log(sum2.y);

  return wlen;
}

//
// this only calculate HPWL based on the stored value in NET's ure
prec GetHpwl() {
  total_hpwl.SetZero();
  total_stnwl.SetZero();

  for(int i = 0; i < netCNT; i++) {
    NET *curNet = &netInstance[i];
    if(curNet->pinCNTinObject <= 1)
      continue;

    total_hpwl.x += curNet->max_x - curNet->min_x;
    total_hpwl.y += curNet->max_y - curNet->min_y;

    //// lutong
    // total_stnwl.x += curNet->stn_cof * (curNet->max_x - curNet->min_x) ;
    // total_stnwl.y += curNet->stn_cof * (curNet->max_y - curNet->min_y) ;

  }

  return total_hpwl.x + total_hpwl.y;
}

//
// this calculate current NET's informations (min_xyz/max_xyz) & calculate HPWL
// simultaneously
prec UpdateNetAndGetHpwl() {
  FPOS pof, fp;
  total_hpwl.SetZero();

  for(int i = 0; i < netCNT; i++) {
    NET *curNet = &netInstance[i];

    curNet->min_x = curNet->terminalMin.x;
    curNet->min_y = curNet->terminalMin.y;

    curNet->max_x = curNet->terminalMax.x;
    curNet->max_y = curNet->terminalMax.y;

    // update min_xyz, max_xyz in NET's info
    for(int j = 0; j < curNet->pinCNTinObject; j++) {
      PIN *pin = curNet->pin[j];

      // only for modules
      if(pin->term) {
        continue;
      }

      MODULE *curModule = &moduleInstance[pin->moduleID];
      pin->fp.SetAdd(curModule->center, curModule->pof[pin->pinIDinModule]);

      curNet->min_x = min(curNet->min_x, pin->fp.x);
      curNet->min_y = min(curNet->min_y, pin->fp.y);

      curNet->max_x = max(curNet->max_x, pin->fp.x);
      curNet->max_y = max(curNet->max_y, pin->fp.y);
    }

    if(curNet->pinCNTinObject <= 1) {
      continue;
    }

    // calculate HPWL
    total_hpwl.x += curNet->max_x - curNet->min_x;
    total_hpwl.y += curNet->max_y - curNet->min_y;
  }

  return total_hpwl.x + total_hpwl.y;
}

void wlen_grad2(int cell_idx, FPOS *grad2) {
  switch(WLEN_MODEL) {
    case LSE:
      wlen_grad2_lse(cell_idx, grad2);
      break;

    case WA:
      wlen_grad2_wa(grad2);
      break;
  }
  return;
}

void wlen_grad(int cell_idx, FPOS *grad) {
  grad->SetZero();
#ifdef NO_WLEN
  return;
#endif

  switch(WLEN_MODEL) {
    case LSE:
      wlen_grad_lse(cell_idx, grad);
      break;

    case WA:
      wlen_grad_wa(cell_idx, grad);
      break;
  }
  grad->x *= -1.0 * gp_wlen_weight.x;
  grad->y *= -1.0 * gp_wlen_weight.y;
}

void wlen_grad2_wa(FPOS *grad) {
  grad->x = 1.0;
  grad->y = 1.0;
  return;
}

void wlen_grad2_lse(int cell_idx, FPOS *grad2) {
  FPOS net_grad2;
  CELL *cell = &gcell_st[cell_idx];
  NET *net = NULL;
  PIN *pin = NULL;

  grad2->SetZero();

  for(int i = 0; i < cell->pinCNTinObject; i++) {
    pin = cell->pin[i];
    net = &netInstance[pin->netID];

    if(net->pinCNTinObject <= 1)
      continue;

    get_net_wlen_grad2_lse(net, pin, &net_grad2);

    grad2->x += net_grad2.x;
    grad2->y += net_grad2.y;
  }
}

void wlen_grad_lse(int cell_idx, FPOS *grad) {
  FPOS net_grad;
  CELL *cell = &gcell_st[cell_idx];
  NET *net = NULL;
  PIN *pin = NULL;

  grad->SetZero();

  for(int i = 0; i < cell->pinCNTinObject; i++) {
    pin = cell->pin[i];
    net = &netInstance[pin->netID];

    if(net->pinCNTinObject <= 1)
      continue;

    get_net_wlen_grad_lse(net, pin, &net_grad);

    grad->x += net_grad.x;
    grad->y += net_grad.y;
  }
}

void wlen_grad_wa(int cell_idx, FPOS *grad) {
  CELL *cell = &gcell_st[cell_idx];
  PIN *pin = NULL;
  NET *net = NULL;
  FPOS net_grad;

  grad->SetZero();

  for(int i = 0; i < cell->pinCNTinObject; i++) {
    pin = cell->pin[i];
    net = &netInstance[pin->netID];

    if(net->pinCNTinObject <= 1)
      continue;

    get_net_wlen_grad_wa(pin->fp, net, pin, &net_grad);

    float curTimingWeight = netInstance[pin->netID].timingWeight;
    // Timing Control Parts
    // curTimingWeight = netWeightBase + min(max(0.0f, netWeightBound - netWeightBase),
    // curTimingWeight / netWeightScale);
    //
    // cout << "calNetWeight: " << curTimingWeight << endl;
    if(hasUnitNetWeight) {
      net_grad.x *= netWeight;
      net_grad.y *= netWeight;
    }
    else if( hasCustomNetWeight ) {
      net_grad.x *= net->customWeight;
      net_grad.y *= net->customWeight;
    }
    else if(isTiming && netWeightApply && curTimingWeight > 0) {
      net_grad.x *= curTimingWeight;
      net_grad.y *= curTimingWeight;
    }

    grad->x += net_grad.x;
    grad->y += net_grad.y;
  }
}

// customWeight update functions:
void initCustomNetWeight(string netWeightFile) {

  PrintProcBegin("CustomNetWeightInit");
  // save netName / weight pair
  std::unordered_map<string,prec> tempMap;
  
  fstream fin(netWeightFile.c_str());
  string line;
  if (fin.is_open()){
    while (fin.good()){
      getline(fin, line);
      if (line.empty() || line[0] != '#'){
        char delimiter=' ';
        int pos = line.find(delimiter);
        string field = line.substr(0, pos);
        string value = line.substr(pos + 1);
        stringstream ss(value);
        if (line == "") continue;
        tempMap[field] = atof(value.c_str());
//        cout <<"Net " <<field <<" has weight " <<tempMap[field] <<endl;
      }
    }
    fin.close();
  }
  else {
    cout << "ERROR: Cannot open " << netWeightFile << endl;
    exit(1);
  }

  // fill in net->customWeight
  int customWeightCnt = 0;
  for (int i = 0; i < netCNT; i++) {
    if (tempMap.find(string(netInstance[i].Name())) != tempMap.end()) {
      netInstance[i].customWeight = tempMap[string(netInstance[i].Name())];
      customWeightCnt ++;
      //cout << netInstance[i].Name()
      // <<" weight " <<netInstance[i].customWeight 
      // <<" assigned" <<endl;
    }
  }
  PrintInfoInt("CustomNetWeightCount", customWeightCnt);
  PrintProcBegin("CustomNetWeightEnd");
}

void get_net_wlen_grad2_lse(NET *net, PIN *pin, FPOS *grad2) {
  POS flg1 = pin->flg1, flg2 = pin->flg2;
  FPOS e1 = pin->e1, e2 = pin->e2;
  FPOS grad2_1, grad2_2;
  FPOS sum_denom1 = net->sum_denom1;
  FPOS sum_denom2 = net->sum_denom2;

  if(flg1.x) {
    grad2_1.x = (e1.x) * (sum_denom1.x - e1.x) / (sum_denom1.x * sum_denom1.x);
  }

  if(flg2.x) {
    grad2_2.x = (e2.x) * (sum_denom2.x - e2.x) / (sum_denom2.x * sum_denom2.x);
  }

  grad2->x = grad2_1.x + grad2_2.x;

  if(flg1.y) {
    grad2_1.y = (e1.y) * (sum_denom1.y - e1.y) / (sum_denom1.y * sum_denom1.y);
  }

  if(flg2.y) {
    grad2_2.y = (e2.y) * (sum_denom2.y - e2.y) / (sum_denom2.y * sum_denom2.y);
  }

  grad2->y = grad2_1.y + grad2_2.y;

  return;
}

void get_net_wlen_grad_lse(NET *net, PIN *pin, FPOS *grad) {
  POS flg1 = pin->flg1, flg2 = pin->flg2;
  FPOS grad1, grad2;
  FPOS e1 = pin->e1, e2 = pin->e2;
  FPOS sum_denom1 = net->sum_denom1;
  FPOS sum_denom2 = net->sum_denom2;

  if(flg1.x) {
    grad1.x = e1.x / sum_denom1.x;
  }

  if(flg2.x) {
    grad2.x = e2.x / sum_denom2.x;
  }

  grad->x = grad1.x - grad2.x;

  if(flg1.y) {
    grad1.y = e1.y / sum_denom1.y;
  }

  if(flg2.y) {
    grad2.y = e2.y / sum_denom2.y;
  }

  grad->y = grad1.y - grad2.y;

  return;
}

// wlen_cof
// obj?
//
void get_net_wlen_grad_wa(FPOS obj, NET *net, PIN *pin, FPOS *grad) {
  FPOS grad_sum_num1, grad_sum_num2;
  FPOS grad_sum_denom1, grad_sum_denom2;
  FPOS grad1;
  FPOS grad2;
  FPOS e1 = pin->e1;
  FPOS e2 = pin->e2;
  POS flg1 = pin->flg1;
  POS flg2 = pin->flg2;
  FPOS sum_num1 = net->sum_num1;
  FPOS sum_num2 = net->sum_num2;
  FPOS sum_denom1 = net->sum_denom1;
  FPOS sum_denom2 = net->sum_denom2;

  if(flg1.x) {
    grad_sum_denom1.x = wlen_cof.x * e1.x;
    grad_sum_num1.x = e1.x + obj.x * grad_sum_denom1.x;
    grad1.x =
        (grad_sum_num1.x * sum_denom1.x - grad_sum_denom1.x * sum_num1.x) /
        (sum_denom1.x * sum_denom1.x);
  }

  if(flg1.y) {
    grad_sum_denom1.y = wlen_cof.y * e1.y;
    grad_sum_num1.y = e1.y + obj.y * grad_sum_denom1.y;
    grad1.y =
        (grad_sum_num1.y * sum_denom1.y - grad_sum_denom1.y * sum_num1.y) /
        (sum_denom1.y * sum_denom1.y);
  }

  if(flg2.x) {
    grad_sum_denom2.x = wlen_cof.x * e2.x;
    grad_sum_num2.x = e2.x - obj.x * grad_sum_denom2.x;
    grad2.x =
        (grad_sum_num2.x * sum_denom2.x + grad_sum_denom2.x * sum_num2.x) /
        (sum_denom2.x * sum_denom2.x);
  }

  if(flg2.y) {
    grad_sum_denom2.y = wlen_cof.y * e2.y;
    grad_sum_num2.y = e2.y - obj.y * grad_sum_denom2.y;
    grad2.y =
        (grad_sum_num2.y * sum_denom2.y + grad_sum_denom2.y * sum_num2.y) /
        (sum_denom2.y * sum_denom2.y);
  }

  grad->x = grad1.x - grad2.x;
  grad->y = grad1.y - grad2.y;
}

void net_update_init(void) {
  for(int i = 0; i < netCNT; i++) {
    NET *net = &netInstance[i];

    net->terminalMin.Set(place.end);
    net->terminalMax.Set(place.org);

    bool first_term = true;

    for(int j = 0; j < net->pinCNTinObject; j++) {
      PIN *pin = net->pin[j];
      if(pin->term) {
        if(first_term) {
          first_term = false;

          net->terminalMin.Set(pin->fp);
          net->terminalMax.Set(pin->fp);
        }
        else {
          net->terminalMin.Min(pin->fp);
          net->terminalMax.Max(pin->fp);
        }
      }
    }
  }
}

void net_update(FPOS *st) {
  switch(WLEN_MODEL) {
    case LSE:
      return net_update_lse(st);
      break;
    case WA:
      return net_update_wa(st);
      break;
  }
}

void net_update_lse(FPOS *st) {
  int i = 0, j = 0;
  CELL *cell = NULL;
  NET *net = NULL;
  PIN *pin = NULL;
  MODULE *curModule = NULL;
  FPOS fp, pof, center;
  FPOS sum_denom1, sum_denom2;
  prec exp_val = 0;
  prec min_x = 0, min_y = 0;
  prec max_x = 0, max_y = 0;
  prec exp_min_x = 0, exp_min_y = 0;
  prec exp_max_x = 0, exp_max_y = 0;

  for(i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];

    cell->center = st[i];

    cell->den_pmin.x = cell->center.x - cell->half_den_size.x;
    cell->den_pmin.y = cell->center.y - cell->half_den_size.y;
    // cell->den_pmin.z = cell->center.z - cell->half_den_size.z;

    cell->den_pmax.x = cell->center.x + cell->half_den_size.x;
    cell->den_pmax.y = cell->center.y + cell->half_den_size.y;
    // cell->den_pmax.z = cell->center.z + cell->half_den_size.z;
  }

  for(i = 0; i < netCNT; i++) {
    net = &netInstance[i];

    net->min_x = net->terminalMin.x;
    net->min_y = net->terminalMin.y;
    // net->min_z = net->terminalMin.z;

    net->max_x = net->terminalMax.x;
    net->max_y = net->terminalMax.y;
    // net->max_z = net->terminalMax.z;

    for(j = 0; j < net->pinCNTinObject; j++) {
      pin = net->pin[j];

      if(!pin->term) {
        curModule = &moduleInstance[pin->moduleID];
        pof = curModule->pof[pin->pinIDinModule];
        center = st[pin->moduleID];
        fp.x = center.x + pof.x;
        fp.y = center.y + pof.y;
        // fp.z = center.z + pof.z ;
        pin->fp = fp;

        net->min_x = min(net->min_x, fp.x);
        net->min_y = min(net->min_y, fp.y);
        // net->min_z = min ( net->min_z , fp.z ) ;

        net->max_x = max(net->max_x, fp.x);
        net->max_y = max(net->max_y, fp.y);
        // net->max_z = max ( net->max_z , fp.z ) ;
      }
      else {
        continue;
      }
    }

    min_x = net->min_x;
    min_y = net->min_y;
    // min_z = net->min_z ;

    max_x = net->max_x;
    max_y = net->max_y;
    // max_z = net->max_z;

    sum_denom1.x = sum_denom1.y = 0;
    sum_denom2.x = sum_denom2.y = 0;

    for(j = 0; j < net->pinCNTinObject; j++) {
      pin = net->pin[j];

      if(!pin->term) {
#ifdef CELL_CENTER_WLEN_GRAD
        fp = st[pin->moduleID];
#else
        fp = pin->fp;
#endif
      }
      else {
        fp = pin->fp;
      }

      exp_max_x = (fp.x - max_x) * wlen_cof.x;

      if(fabs(exp_max_x) < MAX_EXP) {
        exp_val = get_exp(exp_max_x);
        sum_denom1.x += exp_val;
        pin->flg1.x = 1;
        pin->e1.x = exp_val;
      }
      else {
        exp_val = 0;
        pin->flg1.x = 0;
      }

      exp_min_x = (min_x - fp.x) * wlen_cof.x;

      if(fabs(exp_min_x) < MAX_EXP) {
        exp_val = get_exp(exp_min_x);
        sum_denom2.x += exp_val;
        pin->flg2.x = 1;
        pin->e2.x = exp_val;
      }
      else {
        pin->flg2.x = 0;
      }

      exp_max_y = (fp.y - max_y) * wlen_cof.y;

      if(fabs(exp_max_y) < MAX_EXP) {
        exp_val = get_exp(exp_max_y);
        sum_denom1.y += exp_val;
        pin->flg1.y = 1;
        pin->e1.y = exp_val;
      }
      else {
        pin->flg1.y = 0;
      }

      exp_min_y = (min_y - fp.y) * wlen_cof.y;

      if(fabs(exp_min_y) < MAX_EXP) {
        exp_val = get_exp(exp_min_y);
        sum_denom2.y += exp_val;
        pin->flg2.y = 1;
        pin->e2.y = exp_val;
      }
      else {
        pin->flg2.y = 0;
      }
    }

    net->sum_denom1 = sum_denom1;
    net->sum_denom2 = sum_denom2;
  }
}

prec net_update_hpwl_mac(void) {
  int i = 0, j = 0;
  prec hpwl = 0;
  NET *net = NULL;
  PIN *pin = NULL;
  MODULE *curModule = NULL;
  FPOS fp, pof, p0;

  total_hpwl.x = total_hpwl.y = 0;

  for(i = 0; i < netCNT; i++) {
    net = &netInstance[i];

    net->min_x = net->terminalMin.x;
    net->min_y = net->terminalMin.y;

    net->max_x = net->terminalMax.x;
    net->max_y = net->terminalMax.y;

    for(j = 0; j < net->pinCNTinObject; j++) {
      pin = net->pin[j];
      if(!pin->term) {
        curModule = &moduleInstance[pin->moduleID];
        // cell = & gcell_st [pin->moduleID];
        p0 = curModule->center;
        pof = curModule->pof[pin->pinIDinModule];
        fp.x = p0.x + pof.x;
        fp.y = p0.y + pof.y;
        pin->fp = fp;

        net->min_x = min(net->min_x, fp.x);
        net->min_y = min(net->min_y, fp.y);

        net->max_x = max(net->max_x, fp.x);
        net->max_y = max(net->max_y, fp.y);
      }
    }

    if(net->pinCNTinObject <= 1)
      continue;

    total_hpwl.x += (net->max_x - net->min_x);
    total_hpwl.y += (net->max_y - net->min_y);
  }

  /* total_hpwl_xy = total_hpwl.x + total_hpwl.y; */
  //  total_hpwl_xyz = total_hpwl.x + total_hpwl.y + total_hpwl.z;

  hpwl = total_hpwl.x * dp_wlen_weight.x + total_hpwl.y * dp_wlen_weight.y;

  return hpwl;
}

// WA
//
void net_update_wa(FPOS *st) {
  int i = 0;

  bool timeon = false;
  double time = 0.0f;
  if(timeon)
    time_start(&time);

  omp_set_num_threads(numThread);
#pragma omp parallel default(none) shared(gcell_cnt, gcell_st, st) private(i)
  {
//        CELL* cell = NULL;
#pragma omp for
    for(i = 0; i < gcell_cnt; i++) {
      CELL *cell = &gcell_st[i];
      cell->center = st[i];
      cell->den_pmin.x = cell->center.x - cell->half_den_size.x;
      cell->den_pmin.y = cell->center.y - cell->half_den_size.y;
      cell->den_pmax.x = cell->center.x + cell->half_den_size.x;
      cell->den_pmax.y = cell->center.y + cell->half_den_size.y;
    }
  }
  if(timeon) {
    time_end(&time);
    cout << "parallelTime : " << time << endl;
  }
//  wlen_cof.Dump("current_wlen_cof"); 
//  cout << "NEG_MAX_EXP: " << NEG_MAX_EXP << endl;

#pragma omp parallel default(none) shared( \
    netInstance, moduleInstance, st, netCNT, NEG_MAX_EXP, wlen_cof) private(i)
  {
//        NET *net = NULL;
//        PIN *pin = NULL;
//        MODULE *curModule = NULL;
//        prec exp_min_x = 0, exp_min_y = 0;
//        prec exp_max_x = 0, exp_max_y = 0;
//        prec min_x = 0, min_y = 0;
//        prec max_x = 0, max_y = 0;

//        FPOS fp, pof, center;
//        FPOS sum_num1, sum_num2, sum_denom1, sum_denom2;

#pragma omp for
    for(i = 0; i < netCNT; i++) {
      NET *net = &netInstance[i];
      net->min_x = net->terminalMin.x;
      net->min_y = net->terminalMin.y;
      net->max_x = net->terminalMax.x;
      net->max_y = net->terminalMax.y;

      //        cout << "size: " << sizeof(PIN) << endl;
      //        cout << net->pinCNTinObject << endl;
      for(int j = 0; j < net->pinCNTinObject; j++) {
        PIN *pin = net->pin[j];

        //            cout << j << " " << pin << endl;

        if(!pin->term) {
          MODULE *curModule = &moduleInstance[pin->moduleID];
          FPOS pof = curModule->pof[pin->pinIDinModule];
          FPOS center = st[pin->moduleID];
          FPOS fp;
          fp.x = center.x + pof.x;
          fp.y = center.y + pof.y;
          pin->fp = fp;

          net->min_x = min(net->min_x, fp.x);
          net->min_y = min(net->min_y, fp.y);
          net->max_x = max(net->max_x, fp.x);
          net->max_y = max(net->max_y, fp.y);
        }
        else {
          continue;
        }
      }
      //        if( i >=2 )
      //        exit(1);

      prec min_x = net->min_x;
      prec min_y = net->min_y;
      prec max_x = net->max_x;
      prec max_y = net->max_y;

      FPOS sum_num1, sum_num2;
      FPOS sum_denom1, sum_denom2;

      // UPDATE
      // pin->e1 (MAX)
      // net->sum_num1 (MAX)
      // net->sum_denom1 (MAX)
      // pin->flg1 (MAX)
      //
      // pin->e2 (MIN)
      // net->sum_num2 (MIN)
      // net->sum_denom2 (MIN)
      // pin->flg2 (MIN)
      //
      //
      // Note that NEG_MAX_EXP is -300
      // The meaning NEG_MAX_EXP is. not to have weird out of range values 
      // in floating vars.
      //
      // we know that wlen_cof is 1/ gamma.
      // See main.cpp wcof00 and wlen.cpp: wcof_init. 
      //
      
      for(int j = 0; j < net->pinCNTinObject; j++) {
        PIN *pin = net->pin[j];
        FPOS fp = pin->fp;
        prec exp_max_x = (fp.x - max_x) * wlen_cof.x;
        prec exp_min_x = (min_x - fp.x) * wlen_cof.x;
        prec exp_max_y = (fp.y - max_y) * wlen_cof.y;
        prec exp_min_y = (min_y - fp.y) * wlen_cof.y;


        if(exp_max_x > NEG_MAX_EXP) {
          pin->e1.x = get_exp(exp_max_x);
          sum_num1.x += fp.x * pin->e1.x;
          sum_denom1.x += pin->e1.x;
          pin->flg1.x = 1;
        }
        else {
          pin->flg1.x = 0;
        }

        if(exp_min_x > NEG_MAX_EXP) {
          pin->e2.x = get_exp(exp_min_x);
          sum_num2.x += fp.x * pin->e2.x;
          sum_denom2.x += pin->e2.x;
          pin->flg2.x = 1;
        }
        else {
          pin->flg2.x = 0;
        }

        if(exp_max_y > NEG_MAX_EXP) {
          pin->e1.y = get_exp(exp_max_y);
          sum_num1.y += fp.y * pin->e1.y;
          sum_denom1.y += pin->e1.y;
          pin->flg1.y = 1;
        }
        else {
          pin->flg1.y = 0;
        }

        if(exp_min_y > NEG_MAX_EXP) {
          pin->e2.y = get_exp(exp_min_y);
          sum_num2.y += fp.y * pin->e2.y;
          sum_denom2.y += pin->e2.y;
          pin->flg2.y = 1;
        }
        else {
          pin->flg2.y = 0;
        }
      }

      net->sum_num1 = sum_num1;
      net->sum_num2 = sum_num2;
      net->sum_denom1 = sum_denom1;
      net->sum_denom2 = sum_denom2;
    }
  }
}

prec get_mac_hpwl(int idx) {
  MODULE *mac = macro_st[idx];
  PIN *pin = NULL;
  NET *net = NULL;
  int i = 0, j = 0;
  int moduleID = mac->idx;
  CELL *cell = &gcell_st[moduleID];
  FPOS fp;

  mac_hpwl.x = mac_hpwl.y = 0;
  mac_hpwl_xyz = 0;

  for(i = 0; i < cell->pinCNTinObject; i++) {
    pin = cell->pin[i];
    net = &netInstance[pin->netID];

    net->min_x = net->terminalMin.x;
    net->min_y = net->terminalMin.y;
    net->max_x = net->terminalMax.x;
    net->max_y = net->terminalMax.y;

    for(j = 0; j < net->pinCNTinObject; j++) {
      fp = net->pin[j]->fp;
      net->min_x = min(net->min_x, fp.x);
      net->min_y = min(net->min_y, fp.y);
      net->max_x = max(net->max_x, fp.x);
      net->max_y = max(net->max_y, fp.y);
    }

    if(net->pinCNTinObject <= 1)
      continue;

    mac_hpwl.x += (net->max_x - net->min_x);
    mac_hpwl.y += (net->max_y - net->min_y);
  }

  mac_hpwl_xyz = mac_hpwl.x * dp_wlen_weight.x + mac_hpwl.y * dp_wlen_weight.y;

  return mac_hpwl_xyz;
}

// update net->pin2->fp as updated version (modules' center + modules' offset)
void update_pin2(void) {
  //    cout << "called update_pin2" << endl;
  for(int i = 0; i < netCNT; i++) {
    NET *net = &netInstance[i];

    //        cout << net->pinCNTinObject2 << endl;
    for(int j = 0; j < net->pinCNTinObject2; j++) {
      PIN *pin = net->pin2[j];

      // if pin is terminal pin, then skip this procedure.
      if(pin->term) {
        continue;
      }

      MODULE *curModule = &moduleInstance[pin->moduleID];
      pin->fp.SetAdd(curModule->center, curModule->pof[pin->pinIDinModule]);
    }
  }
}

// Update HPWL based on net->pin2 definition.
void UpdateNetMinMaxPin2(void) {
  NET *net = NULL;
  PIN *pin = NULL;
  FPOS fp;

  for(int i = 0; i < netCNT; i++) {
    net = &netInstance[i];

    net->min_x = net->terminalMin.x;
    net->min_y = net->terminalMin.y;

    net->max_x = net->terminalMax.x;
    net->max_y = net->terminalMax.y;

    for(int j = 0; j < net->pinCNTinObject2; j++) {
      pin = net->pin2[j];

      if(pin->term) {
        continue;
      }
      fp = pin->fp;

      net->min_x = min(net->min_x, fp.x);
      net->min_y = min(net->min_y, fp.y);

      net->max_x = max(net->max_x, fp.x);
      net->max_y = max(net->max_y, fp.y);
    }

    if(net->pinCNTinObject2 <= 1)
      continue;
  }
}


// Get HPWL as micron units
pair<double, double> GetUnscaledHpwl() {
  double x = 0.0f, y = 0.0f;

  NET *curNet = NULL;
  for(int i = 0; i < netCNT; i++) {
    curNet = &netInstance[i];
    x += (curNet->max_x - curNet->min_x) * GetUnitX() / GetDefDbu();
    y += (curNet->max_y - curNet->min_y) * GetUnitY() / GetDefDbu();

    if(curNet->max_x - curNet->min_x < 0) {
      cout << "NEGATIVE HPWL ERROR! " << curNet->Name() << " " << curNet->max_x
           << " " << curNet->min_x << endl;
    }
    if(curNet->max_y - curNet->min_y < 0) {
      cout << "NEGATIVE HPWL ERROR! " << curNet->Name() << " " << curNet->max_y
           << " " << curNet->min_y << endl;
    }

    if(x < 0 || y < 0) {
      printf("NEGATIVE HPWL ERROR! \n");
      cout << curNet->Name() << x << " " << y << endl;
      exit(1);
    }
  }
  return make_pair(x, y);
}

void PrintUnscaledHpwl(string mode) {
  pair<double, double> hpwl = GetUnscaledHpwl();
  cout << "===HPWL(MICRON)====================================" << endl;
  cout << "  Mode  : " << mode << endl;
  cout << "  HPWL  : " << std::fixed << std::setprecision(4) 
       << hpwl.first + hpwl.second << endl
       << "          x= " << hpwl.first << " y= " << hpwl.second << endl;
  cout << "===================================================" << endl;
}

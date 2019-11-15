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
#include <ctime>
#include <iomanip>

#include "bin.h"
#include "fft.h"
#include "gcell.h"
#include "replace_private.h"
#include "ns.h"
#include "opt.h"
#include "plot.h"
#include "wlen.h"

using std::to_string;
using namespace std;


int FILLER_PLACE;
int NUM_ITER_FILLER_PLACE;  // fast filler pre-place, need tuning
prec opt_phi_cof;
prec opt_w_cof;  // lutong
prec opt_phi_cof_local;
prec opt_wlen_cof;
prec gsum_area;

prec avg80p_cell_area;
prec ref_dwl0;
prec LOWER_PCOF, UPPER_PCOF;
prec INIT_LAMBDA_COF_GP;
prec MIN_PRE;

FPOS avg80p_cell_dim;

// should be removed!!! 3D code!!!
//
//
void setup_before_opt(void) {
  //    mkdirPlot();
  whitespace_init();

  if(total_std_den > target_cell_den + Epsilon) {
    target_cell_den = total_std_den;
    global_macro_area_scale = total_std_den;
  }

  total_modu_area = total_std_area + total_macro_area * target_cell_den;
  inv_total_modu_area = 1.00 / total_modu_area;

  cell_init();
  cell_filler_init();  // So that we can get total_filler_area.

  total_cell_area = total_modu_area + total_filler_area;

  //
  wlen_init();
  msh_init();
  //    bin_init();

  charge_fft_init(msh, bin_stp, 1);
//  update_cell_den();
  wcof_init(bin_stp);
}

int setup_before_opt_mGP2D(void) {
  tier_init_2D(mGP2D);
  bin_init_2D(mGP2D);

  charge_fft_init(dim_bin_mGP2D, bin_stp_mGP2D, 0);
  wcof_init(bin_stp_mGP2D);
  wlen_init();
  cell_init_2D();
  UpdateGcellCoordiFromModule();
  net_update_init();
  return 1;
}

// only this!!!
int setup_before_opt_cGP2D(void) {
  tier_init_2D(cGP2D);
  bin_init_2D(cGP2D);

  // routability
//  if( isRoutability ) {
//    routeInst.Init();
//    WriteBookshelfForGR();

//    char routeLoc[BUF_SZ] = {0, };
//    sprintf(routeLoc, "%s/router_base/%s.route", dir_bnd, gbch);

//    if(isRoutability == true) {
//      read_routes_3D( routeLoc );
//      tile_init_cGP2D(); 
//    }
//  }

  charge_fft_init(dim_bin_cGP2D, bin_stp_cGP2D, 0);
  wcof_init(bin_stp_cGP2D);
  wlen_init();
  cell_init_2D();
  UpdateGcellCoordiFromModule();
  net_update_init();
  return 1;
}

int post_mGP2D_delete(void) {
  bin_delete_mGP2D();
  charge_fft_delete(0);
  // if (thermalAwarePlaceCMD == true) thermal_fft_delete (0);
  tier_delete_mGP2D();
  return 1;
}

void post_opt(void) {
  bin_delete();
  charge_fft_delete(1);
  // if (thermalAwarePlaceCMD == true) thermal_fft_delete (1);
  UpdateModuleCoordiFromGcell();
  cell_delete();
}

//
// st : center position of cells
// n : number of cells
//
// center : |----*----| -> *'s coordinate
//
void cell_update(FPOS *st, int n) {
  for(int i = 0; i < n; i++) {
    CELL *cell = &gcell_st[i];
    cell->center = st[i];

    FPOS tmp_scal = fp_scal(0.5, cell->size);
    cell->pmin = fp_subt(cell->center, tmp_scal);
    cell->pmax = fp_add(cell->center, tmp_scal);
  }
}

// void cell_update (struct FPOS *st, int n) {
//    struct  CELL   *cell = NULL;
//    for (int i=0; i<n; i++) {
//        cell = &gcell_st[i];
//        cell->center = st[i];
//        cell->pmin = fp_add (cell->center, fp_scal (-0.5, cell->size));
//        cell->pmax = fp_add (cell->center, fp_scal ( 0.5, cell->size));
//    }
//}

prec get_norm(struct FPOS *st, int n, prec num) {
  prec sum = 0;
  prec tmp = 0.0;
  for(int i = 0; i < n; i++) {
    sum += pow(fabs(st[i].x), num) + pow(fabs(st[i].y), num);
  }
  tmp = 1.0 / num;
  // In this function, pow produces infinite loop.
  return pow(sum, tmp) / pow(2.0 * n, tmp);
}

// below is to calculate all of density calculations....
void whitespace_init(void) {
  total_termPL_area = 0;

  TIER *tier = &tier_st[0];

  // for each Terminal Instance
  for(int i = 0; i < terminalCNT; i++) {
    TERM *curTerminal = &terminalInstance[i];
    curTerminal->PL_area = 0;

    // skip for IO pin..
    if(curTerminal->isTerminalNI) {
      continue;
    }

    // if there is no shapes..
    if(shapeMap.find(curTerminal->Name()) == shapeMap.end()) {
      for(int j = 0; j < place_st_cnt; j++) {
        PLACE *pl = &place_st[j];

        prec commonArea = pGetCommonAreaXY(curTerminal->pmin, curTerminal->pmax,
                                           pl->org, pl->end);
        curTerminal->PL_area += commonArea;
        total_termPL_area += commonArea;
        tier->term_area += commonArea;

        //                if( string(curTerminal->name) == "o859970" ) {
        //                    curTerminal->pmin.Dump("term->pmin");
        //                    curTerminal->pmax.Dump("term->pmax");
        //                    pl->org.Dump("pl->org");
        //                    pl->end.Dump("pl->end");
        //                }
      }
      //            cout << fixed <<setprecision(4) <<curTerminal->name << " : "
      //            << total_termPL_area << endl;
    }
    // there are shapes
    else {
      for(auto &curIdx : shapeMap[curTerminal->Name()]) {
        for(int j = 0; j < place_st_cnt; j++) {
          PLACE *pl = &place_st[j];

          prec llx = shapeStor[curIdx].llx, lly = shapeStor[curIdx].lly,
               width = shapeStor[curIdx].width,
               height = shapeStor[curIdx].height;

          FPOS tmpMin(llx, lly), tmpMax(llx + width, lly + height);

          prec commonArea = pGetCommonAreaXY(tmpMin, tmpMax, pl->org, pl->end);

          curTerminal->PL_area += commonArea;
          total_termPL_area += commonArea;
          tier->term_area += commonArea;
        }
      }
    }
    /*
     for(int j = 0; j < place_st_cnt; j++) {
         PLACE* pl = &place_st[j];
         FPOS len = get_int_rgn(term->pmin, term->pmax, pl->org, pl->end);
         term->PL_area += fp_product(len);
     }
     total_termPL_area += term->PL_area;*/
  }

  total_WS_area = total_PL_area - total_termPL_area;
  total_std_den = total_std_area / (total_WS_area - total_macro_area);
//  printf("INFO:  Chip Area: %lf x %lf = %lf \n", place.cnt.x, place.cnt.y, place.area);
//  fflush(stdout);

  PrintInfoPrec("TotalPlaceArea", total_PL_area);
  PrintInfoPrec("TotalFixedArea", total_termPL_area);
  PrintInfoPrec("TotalWhiteSpaceArea", total_WS_area);
  PrintInfoPrec("TotalPlaceMacrosArea", total_macro_area);
  PrintInfoPrec("TotalPlaceStdCellsArea", total_std_area); 
  PrintInfoPrec("Util(%)", (total_macro_area + total_std_area) / total_PL_area * 100);

  if( (total_macro_area + total_std_area)/total_PL_area > 1.00f ) {
    PrintError("Utilization Exceeds 100%. Please double-check your input DEF");
  }

//  printf("INFO:  Total_PL_Area = %lf, %lf of chip\n", total_PL_area,
//         total_PL_area / place.area * 100.0);
//  printf("INFO:  Total_TermPL_Area = %ld, %lf of PL\n", total_termPL_area,
//         1.0 * total_termPL_area / place.area * 100.0);
//  printf("INFO:  Total_WS_Area = %ld, %lf of PL\n", total_WS_area,
//         1.0 * total_WS_area / total_PL_area * 100.0);
//  printf("INFO:  Total_Macro_Area = %lf, %lf of WS\n", total_macro_area,
//         total_macro_area / total_WS_area * 100.0);
//  printf("INFO:  Total_StdCell_Area = %lf, %lf of WS\n", total_std_area,
//         total_std_area / total_WS_area * 100.0);
}

void FillerCellRandPlace() {
  struct FPOS pmin, pmax;
  struct FPOS rnd;
  struct TIER *tier = &tier_st[0];

  // filler_cell ranges
  for(int i = tier->modu_cnt; i < tier->cell_cnt; i++) {
    struct CELL* filler = tier->cell_st[i];

    pmin = fp_add(tier->pmin, fp_scal(0.5, filler->size));
    pmax = fp_subt(tier->pmax, fp_scal(0.5, filler->size));

    // rnd will give rand() vals.
    rnd = fp_rand();
    
    // 
    // pmin : possible minimum center point ranges inside layout.
    // pmax : possible maximum center point ranges inside layout.
    //
    // inv_RAND_MAX * rnd will give FPOS pairs, 
    // ranged on 0~1 float numbers.
    //
    // pmax - pmin : possible center point ranges' length.
    // 

    // Filler cells are randomly placed..!

    filler->center =
      fp_add(fp_mul(fp_scal(inv_RAND_MAX, rnd), fp_subt(pmax, pmin)), pmin);
    
    // updates pmin and pmax based on center
    filler->pmin = fp_subt(filler->center, fp_scal(0.5, filler->size));
    filler->pmax = fp_add(filler->center, fp_scal(0.5, filler->size));
  }
}

void cell_filler_init() {
  int i = 0;
  struct CELL *filler = NULL;
  prec k_f2c = 1.0;
  struct CELL *gcell_st_tmp = NULL;
  struct FPOS org, end, len, rnd;

  prec f_area = k_f2c * avg80p_cell_area;

  struct FPOS f_size;
  f_size.x = avg80p_cell_dim.x;
  f_size.y = avg80p_cell_dim.y;

//  f_size.Dump("f_size");
//  avg80p_cell_dim.Dump("avg80p");
//  bin_stp.Dump("bin_stp"); 

  f_area = f_size.x * f_size.y;

  total_move_available_area = total_WS_area * target_cell_den;
  total_filler_area = total_move_available_area - total_modu_area;

  if( total_filler_area < 0) {
    if( fabs(total_filler_area) < total_move_available_area * 0.0001 ) {
      total_filler_area = 0.0f;
    }
    else {
      cout << "ERROR: Negative filler area has been detected: " << total_filler_area << endl;
      exit(1);
    }
  }

  filler_area = f_area;
  filler_size = f_size;

  gfiller_cnt = (int)(total_filler_area / filler_area + 0.5);

  PrintInfoPrec("FillerInit: TotalFillerArea", total_filler_area);
  PrintInfoInt("FillerInit: NumFillerCells", gfiller_cnt);
  PrintInfoPrec("FillerInit: FillerCellArea", filler_area);
  PrintInfoPrecPair("FillerInit: FillerCellSize", filler_size.x, filler_size.y);

  gcell_cnt = moduleCNT + gfiller_cnt;

  // igkang:  replace realloc to mkl
  gcell_st_tmp =
      (struct CELL *)malloc(sizeof(struct CELL) * gcell_cnt);
  memcpy(gcell_st_tmp, gcell_st, moduleCNT * (sizeof(struct CELL)));
  free(gcell_st);

  gcell_st = (CELL *)malloc(sizeof(struct CELL) * gcell_cnt);
  memcpy(gcell_st, gcell_st_tmp, gcell_cnt * (sizeof(struct CELL)));
  free(gcell_st_tmp);

  PrintInfoInt("FillerInit: NumCells", gcell_cnt);
  PrintInfoInt("FillerInit: NumModules", moduleCNT);
  PrintInfoInt("FillerInit: NumFillers", gfiller_cnt);

  for(i = moduleCNT; i < gcell_cnt; i++) {
    filler = &gcell_st[i];
    filler->flg = FillerCell;
    filler->idx = i - moduleCNT;

//    sprintf(filler->Name, "f%d", filler->idx);
    cellNameStor.push_back( "f" + to_string(filler->idx) );

    filler->size = filler_size;
    filler->half_size.x = 0.5 * filler->size.x;
    filler->half_size.y = 0.5 * filler->size.y;

    filler->area = filler_area;
    filler->pinCNTinObject = 0;
    filler->netCNTinObject = 0;
    filler->pof = NULL;
    filler->pin = NULL;

    org = fp_add(place.org, fp_scal(0.5, filler->size));
    end = fp_add(place.end, fp_scal(-0.5, filler->size));

    len = fp_subt(end, org);

    rnd = fp_rand();

    filler->center = fp_add(fp_mul(fp_scal(inv_RAND_MAX, rnd), len), org);

    filler->pmin = fp_add(filler->center, fp_scal(-0.5, filler->size));
    filler->pmax = fp_add(filler->center, fp_scal(0.5, filler->size));
  }
}

void cell_init(void) {
  int i = 0, j = 0, k = 0;
  int min_idx = (int)(0.05 * (prec)moduleCNT);
  int max_idx = (int)(0.95 * (prec)moduleCNT);
  prec total_area = 0, avg_cell_area = 0;
  prec avg_cell_x = 0, avg_cell_y = 0;
  prec total_x = 0, total_y = 0;
  struct CELL *cell = NULL;
  struct MODULE *mdp = NULL;
  struct FPOS pof;
  struct PIN *pin = NULL;
  struct NET *net = NULL;
  prec *cell_area_st = (prec *)malloc(sizeof(prec) * moduleCNT);
  prec *cell_x_st = (prec *)malloc(sizeof(prec) * moduleCNT);
  prec *cell_y_st = (prec *)malloc(sizeof(prec) * moduleCNT);

  for(i = 0; i < moduleCNT; i++) {
    mdp = &moduleInstance[i];
    cell_area_st[i] = mdp->area;
    cell_x_st[i] = mdp->size.x;
    cell_y_st[i] = mdp->size.y;
  }

  //    printf("qsort has before problem?\n");
  //    fflush(stdout);
  qsort(cell_area_st, moduleCNT, sizeof(prec), area_sort);
  qsort(cell_x_st, moduleCNT, sizeof(prec), area_sort);
  qsort(cell_y_st, moduleCNT, sizeof(prec), area_sort);
  //    printf("qsort has after problem?\n");
  //    fflush(stdout);

  for(i = min_idx; i < max_idx; i++) {
    total_area += cell_area_st[i];
    total_x += cell_x_st[i];
    total_y += cell_y_st[i];
  }

  free(cell_area_st);
  free(cell_x_st);
  free(cell_y_st);

  avg_cell_area = total_area / ((prec)(max_idx - min_idx));
  avg_cell_x = total_x / ((prec)(max_idx - min_idx));
  avg_cell_y = total_y / ((prec)(max_idx - min_idx));

  avg80p_cell_area = avg_cell_area;
  avg80p_cell_dim.x = avg_cell_x;
  avg80p_cell_dim.y = avg_cell_y;

  PrintInfoPrec("80pCellArea", avg80p_cell_area);

  gcell_cnt = moduleCNT;
  gcell_st = (struct CELL *)malloc(sizeof(struct CELL) * gcell_cnt);

  // pin2 copy loop: pin2 is original pin info
  for(i = 0; i < netCNT; i++) {
    net = &netInstance[i];
    net->mod_idx = -1;
    net->pin2 = (struct PIN **)malloc(
        sizeof(struct PIN *) * net->pinCNTinObject);
    net->pinCNTinObject2 = net->pinCNTinObject;
    for(j = 0; j < net->pinCNTinObject2; j++) {
      net->pin2[j] = net->pin[j];
    }
  }

  for(i = 0; i < gcell_cnt; i++) {
    mdp = &moduleInstance[i];
    cell = &gcell_st[i];
    cell->flg = mdp->flg;
    cell->idx = i;
//    strcpy(cell->Name, mdp->name);
    cellNameStor.push_back( mdp->Name() );
    cell->size = mdp->size;
    cell->half_size = mdp->half_size;
    cell->area = mdp->area;
    cell->pof = (struct FPOS *)malloc(
        sizeof(struct FPOS) * mdp->pinCNTinObject);
    cell->pin = (struct PIN **)malloc(
        sizeof(struct PIN *) * mdp->pinCNTinObject);
    cell->pinCNTinObject = 0;

    // 
    // pin removal is executed 
    // when there are two duplicated pins 
    // (connected to a same net) in a single instance 
    //
    for(j = 0; j < mdp->pinCNTinObject; j++) {
      pin = mdp->pin[j];
      pof = mdp->pof[j];
      net = &netInstance[pin->netID];

      if(net->mod_idx == i) {
        for(k = pin->pinIDinNet; k < net->pinCNTinObject - 1; k++) {
          net->pin[k] = net->pin[k + 1];
          net->pin[k]->pinIDinNet = k;
        }
        net->pinCNTinObject--;
        continue;
      }
      else {
        net->mod_idx = i;
        cell->pin[cell->pinCNTinObject] = pin;
        cell->pof[cell->pinCNTinObject] = pof;
        cell->pinCNTinObject++;
      }
    }

    cell->netCNTinObject = cell->pinCNTinObject;
    cell->pin_tmp = (struct PIN **)malloc(
        sizeof(struct PIN *) * cell->pinCNTinObject);
    cell->pof_tmp = (struct FPOS *)malloc(
        sizeof(struct FPOS) * cell->pinCNTinObject);
    memcpy(cell->pin_tmp, cell->pin,
           cell->pinCNTinObject * (sizeof(struct PIN *)));
    memcpy(cell->pof_tmp, cell->pof,
           cell->pinCNTinObject * (sizeof(struct FPOS)));
    free(cell->pin);
    free(cell->pof);
    cell->pin = (struct PIN **)malloc(
        sizeof(struct PIN *) * cell->pinCNTinObject);
    cell->pof = (struct FPOS *)malloc(
        sizeof(struct FPOS) * cell->pinCNTinObject);
    memcpy(cell->pin, cell->pin_tmp,
           cell->pinCNTinObject * (sizeof(struct PIN *)));
    memcpy(cell->pof, cell->pof_tmp,
           cell->pinCNTinObject * (sizeof(struct FPOS)));
    free(cell->pin_tmp);
    free(cell->pof_tmp);

    cell->pmin = mdp->pmin;
    cell->pmax = mdp->pmax;
    cell->center = mdp->center;
  }
}

void cell_delete(void) {
  struct CELL *cell = NULL;

  for(int i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];
    free(cell->pin);
    free(cell->pof);
  }
  free(gcell_st);
}

void input_sol(struct FPOS *st, int N, char *fn) {
  int i = 0;
  FILE *fp = fopen(fn, "r");
  char *token = NULL;
  char buf[BUF_SZ];
  int buf_size = BUF_SZ;

  for(i = 0; i < N; i++) {
    do {
      fgets(buf, buf_size, fp);
      token = strtok(buf, " \t\n");
    } while(!token || token[0] == '#' || !strcmp(token, "UCLA"));

    token = strtok(NULL, " \t\n");
    st[i].x = atoi(token);

    token = strtok(NULL, " \t\n");
    st[i].y = atoi(token);
  }

  fclose(fp);
}

void UpdateModuleCoordiFromGcell(void) {
  int i = 0;
  struct CELL *cell = NULL;
  struct MODULE *mdp = NULL;

  for(i = 0; i < moduleCNT; i++) {
    cell = &gcell_st[i];
    mdp = &moduleInstance[i];

    mdp->center = cell->center;

    mdp->pmin.x = mdp->center.x - mdp->half_size.x;
    mdp->pmin.y = mdp->center.y - mdp->half_size.y;

    mdp->pmax.x = mdp->center.x + mdp->half_size.x;
    mdp->pmax.y = mdp->center.y + mdp->half_size.y;
  }

  return;
}

void UpdateGcellCoordiFromModule(void) {
  struct CELL *cell = NULL;
  struct MODULE *module = NULL;

  for(int i = 0; i < moduleCNT; i++) {
    cell = &gcell_st[i];
    module = &moduleInstance[i];

    cell->center = module->center;
    cell->pmin = module->pmin;
    cell->pmax = module->pmax;
  }
}

prec get_dis(struct FPOS *a, struct FPOS *b, int N) {
  prec sum_dis = 0;
  prec tmp = 0.5;

  for(int i = 0; i < N; i++) {
    // sum_dis += pow(fabs(a[i].x - b[i].x), num);
    // sum_dis += pow(fabs(a[i].y - b[i].y), num);
    // sum_dis += pow(fabs(a[i].x - b[i].x), 2);
    // sum_dis += pow(fabs(a[i].y - b[i].y), 2);
    sum_dis += (a[i].x - b[i].x) * (a[i].x - b[i].x);
    sum_dis += (a[i].y - b[i].y) * (a[i].y - b[i].y);
  }

  // cout <<"sum_dis: " <<sum_dis <<endl;
  // cout <<"flg_3dic: " <<flg_3dic <<endl;

  return (flg_3dic == 1) ? pow(sum_dis, tmp) / pow(3.0 * N, tmp)
                         : pow(sum_dis, tmp) / pow(2.0 * N, tmp);
}

prec get_phi_cof1(prec x) {
  prec retCoef = 0;
  prec bas = UPPER_PCOF;

  retCoef = (x < 0)? bas : bas * pow((bas), x * (-1.0));
  //retCoef = (x < 0) ? bas : bas * pow((bas * 1.2), x * (-1.0));
  retCoef = (retCoef < LOWER_PCOF) ? LOWER_PCOF : retCoef;

  return retCoef;
}

prec get_phi_cof(prec x) {
  prec cof = 0;

  if(x < 0)
    cof = 1.25;
  else if(x > 1.0)
    cof = 0.9;
  else
    cof = exp((-0.25) * x) * 1.25;
  return cof;
}

// cg input ?
//
void cg_input(struct FPOS *x_st, int N, int input) {
  char fn_x_isol[BUF_SZ];
  char fn_wl_sol[BUF_SZ];

  struct FPOS half_den_size;
  struct FPOS center;
  struct FPOS rnd;
  struct FPOS v;
  struct FPOS sqr_org;
  struct FPOS sqr_cnt;
  struct FPOS sqr_end;

  switch(input) {
    // 
    // ???
    //
    case QWL_ISOL:
      for(int i = 0; i < N; i++) {
        center = gcell_st[i].center;
        // gcell's half_den_size is initialized at cell_init_2D function!
        half_den_size = gcell_st[i].half_den_size;

        // 
        x_st[i] = GetCoordiLayoutInside(center, half_den_size);
      }
      break;

    case ISOL_INPUT:
      input_sol(x_st, N, fn_x_isol);
      break;

    case WL_SOL_INPUT:
      input_sol(x_st, N, fn_wl_sol);
      break;

    case RANDOM_INPUT:
      sqr_cnt = place.cnt;
      sqr_org = place.org;
      sqr_end = fp_add(sqr_org, sqr_cnt);
      for(int i = 0; i < N; i++) {
        half_den_size = gcell_st[i].half_den_size;
        rnd = fp_rand();
        v = fp_add(fp_mul(fp_scal(inv_RAND_MAX, rnd), sqr_cnt), sqr_org);
        x_st[i] = GetCoordiLayoutInside(v, half_den_size);
      }
      break;

    case IP_CEN_SQR:
      sqr_cnt = fp_scal(0.4, place.cnt);
      sqr_org = fp_add(place.org, fp_scal(0.3, place.cnt));
      sqr_end = fp_add(sqr_org, sqr_cnt);
      for(int i = 0; i < N; i++) {
        half_den_size = gcell_st[i].half_den_size;
        v = fp_add(fp_mul(fp_scal(inv_RAND_MAX, rnd), sqr_cnt), sqr_org);
        x_st[i] = GetCoordiLayoutInside(v, half_den_size);
      }
      break;
  }
}

int area_sort(const void *a, const void *b) {
  prec *aa = (prec *)a;
  prec *bb = (prec *)b;
  return *aa > *bb ? 1 : 0;
}

void init_iter(struct ITER *it, int idx) {
  it->idx = idx;
  it->wlen.x = it->wlen.y = 0;
  it->tot_wlen = 0;
  it->grad = 0;
  it->hpwl.x = it->hpwl.y = 0;
  it->tot_hpwl = 0;
  it->ovfl = 0;
  it->wcof.x = it->wcof.y = 0;
  it->pcof = 0;
  it->beta = 0;
  it->alpha00 = 0;
  it->alpha_dim.x = it->alpha_dim.y = 0;
  it->lc_dim.x = it->lc_dim.y = 0;
  it->lc_w = 0;
  it->lc_p = 0;
  it->cpu_curr = 0;
  it->cpu_cost = 0;
}

void gp_opt(void) {
  myNesterov ns_opt;

  // fillerCell random placement
  FillerCellRandPlace();

//  if( isRoutability ) {
//    routeInst.Init();
//    WriteBookshelf(); 
//  }

  printf("PROC:  Start NESTEROV's Optimization\n");
  if(constraintDrivenCMD == false) {
    printf("PROC:    Global Lagrangian Multiplier is Applied\n");
  }
  else if(constraintDrivenCMD == true) {
    printf(
        "PROC:    Both Global and Local Lagrangian Multipliers are "
        "Applied\n");
  }
    
  ns_opt.nesterov_opt();
  UpdateModuleCoordiFromGcell();
  fflush(stdout);
}

void routability() {
  if(inflation_cnt == 0) {
    restore_cells_info();
    target_cell_den = target_cell_den_orig;
    // mod LW 10/29/16
    global_macro_area_scale = target_cell_den;

    curr_WS_area = total_WS_area - total_cell_area;
    curr_filler_area = total_filler_area;
    cout << "curr_WS_area = " << curr_WS_area << endl;
    cout << "total_cell_area = " << total_cell_area << endl;
    cout << "total_filler_area = " << total_filler_area << endl;
    curr_cell_area = total_cell_area;
  }

  bool print = false;
  while(1) {  // adjust_inflation()
    bloat_prep();
    calc_Total_inflated_cell_area();
    gen_sort_InflationList();
    // cout <<"currTotalInflation = " <<currTotalInflation <<endl;
    // cout <<"currFillerArea = " <<curr_filler_area <<endl;
    // cout <<"inflation_area_over_whitespace = "
    // <<inflation_area_over_whitespace <<endl;
    // if (printed == false) {
    //    print_inflation_list();
    //    printed = true;
    //}

    if(print == false) {
      cout << "currTotalInflation = " << currTotalInflation << endl;
      cout << "max_infl_area = "
           << inflation_area_over_whitespace * (total_WS_area - total_cell_area)
           << endl;
      print = true;
    }
    // if (currTotalInflation > inflation_area_over_whitespace *
    // curr_WS_area) {
    if(currTotalInflation >
       inflation_area_over_whitespace *
           (total_WS_area - total_cell_area + total_filler_area)) {
      // cout <<"currTotalInflation = " <<currTotalInflation <<endl;
      // cout <<"entering here" <<endl;
      dynamicInflationAdjustment();
      // print_inflation_list();
    }
    else {
      // should be checked xxxxxxxxxxxxxxx
      print_inflation_list();
      break;
    }
  }
  cout << "currTotalInflation = " << currTotalInflation << endl;
  curr_cell_area += currTotalInflation;
  target_cell_den = curr_cell_area / total_WS_area;
  // mod LW 10/29/16
  global_macro_area_scale = target_cell_den;
  if(target_cell_den > routeMaxDensity) {
    prec area_to_shrink = curr_cell_area - routeMaxDensity * total_WS_area;
    target_cell_den = routeMaxDensity;
    // mod LW 10/29/16
    global_macro_area_scale = target_cell_den;
    shrink_filler_cells(area_to_shrink);
    curr_cell_area = routeMaxDensity * total_WS_area;
    curr_WS_area = curr_WS_area - (currTotalInflation - area_to_shrink);
  }
  cout << "DEN_NEW = " << target_cell_den << endl;
  cout << "curr_WS_area (pure WS)= " << curr_WS_area << endl;
  cout << "total_cell_area (cell+filler)= " << curr_cell_area << endl;
  cout << "total_filler_area = " << curr_filler_area << endl;
  bloating();
  calc_Total_inflate_ratio();

  cout << "\n\nroute opt" << endl;
  cout << "inflation_cnt = " << inflation_cnt << endl;
  cout << "bloatCNT = " << bloatCNT << endl;
}

void routability_init() {
  struct CELL *cell = NULL;

  // LW mod 11/17/16
  // inflation_area_over_whitespace = 0.10;
  inflation_area_over_whitespace = 1.0 / inflation_max_cnt;
  for(int i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];
    backup_org_CELL_info(cell);
  }
  isRoutabilityInit = true;
}

void restore_cells_info() {
  struct CELL *cell = NULL;

  for(int i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];
    restore_org_CELL_info(cell);
  }
}

void cell_macro_copy(void) {
  int i = 0;
  struct MODULE *mdp = NULL;
  struct CELL *cell = NULL;

  for(i = 0; i < moduleCNT; i++) {
    mdp = &moduleInstance[i];

    if(mdp->flg == StdCell)
      continue;

    cell = &gcell_st[i];

    cell->center = mdp->center;

    cell->pmin = fp_add(cell->center, fp_scal(-0.5, cell->size));
    cell->pmax = fp_add(cell->center, fp_scal(0.5, cell->size));
  }

  return;
}


// update gcell's half_den_size
// 
// tier-> bin_stp : binSize.
// tier-> half_bin_stp : binSize / 2
//
void cell_init_2D(void) {
  struct CELL *cell = NULL;
  struct TIER *tier = &tier_st[0];
  struct FPOS scal;

  cout << "cell Init 2D:" << endl;
  tier->bin_stp.Dump("tier->bin_stp");
  tier->half_bin_stp.Dump("tier->half_bin_stp");

  // normal cases
  // SQRT2 = smoothing parameter for density_size calculation
  for(int i = 0; i < tier->cell_cnt; i++) {
    cell = tier->cell_st[i];

    if(cell->size.x < tier->bin_stp.x * SQRT2) {
      scal.x = cell->size.x / (tier->bin_stp.x * SQRT2);
      cell->half_den_size.x = tier->half_bin_stp.x * SQRT2;
    }
    else {
      scal.x = 1.0;
      cell->half_den_size.x = cell->half_size.x;
    }

    if(cell->size.y < tier->bin_stp.y * SQRT2) {
      scal.y = cell->size.y / (tier->bin_stp.y * SQRT2);
      cell->half_den_size.y = tier->half_bin_stp.y * SQRT2;
    }
    else {
      scal.y = 1.0;
      cell->half_den_size.y = cell->half_size.y;
    }

    cell->den_scal = scal.x * scal.y;
  }
}

// why there is NO SQRT2?
// should be removed to avoid confusing. 
// void update_cell_den() {
//   struct CELL *cell = NULL;
//   struct FPOS scal;
// 
//   for(int i = 0; i < gcell_cnt; i++) {
//     cell = &gcell_st[i];
//     if(cell->size.x < bin_stp.x) {
//       scal.x = cell->size.x / bin_stp.x;
//       cell->half_den_size.x = half_bin_stp.x;
//     }
//     else {
//       scal.x = 1.0;
//       cell->half_den_size.x = cell->half_size.x;
//     }
// 
//     if(cell->size.y < bin_stp.y) {
//       scal.y = cell->size.y / bin_stp.y;
//       cell->half_den_size.y = half_bin_stp.y;
//     }
//     else {
//       scal.y = 1.0;
//       cell->half_den_size.y = cell->half_size.y;
//     }
// 
//     cell->den_scal = scal.x * scal.y;
//   }
// }

//
// update msh, msh_yz, d_msh
void msh_init() {
  msh = dim_bin;
  msh_yz = msh.y;
  int d_msh = msh.x * msh.y;

  printf("INFO:  D_MSH = %d \n", d_msh);
  printf("INFO:  MSH(X, Y) = (%d, %d)\n", msh.x, msh.y);
}

// void stepSizeAdaptation_by1stOrderEPs (prec   curr_hpwl) {
//    if (curr_hpwl < hpwlEPs_2ndOrder[1].first) {
//        if (curr_hpwl < hpwlEPs_1stOrder[0].first) {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[0].first,
//                            hpwlEPs_2ndOrder[0].first,
//                            trial1stPhase_1half);
//        } else {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[0].first,
//                            hpwlEPs_2ndOrder[1].first,
//                            trial2ndPhase_2half);
//        }
//    } else if (curr_hpwl >= hpwlEPs_2ndOrder[1].first
//            && curr_hpwl < hpwlEPs_2ndOrder[2].first) {
//        if (curr_hpwl < hpwlEPs_1stOrder[1].first) {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[1].first,
//                            hpwlEPs_2ndOrder[1].first,
//                            trial3rdPhase_1half);
//        } else {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[1].first,
//                            hpwlEPs_2ndOrder[2].first,
//                            trial3rdPhase_2half);
//        }
//    } else if (curr_hpwl > hpwlEPs_2ndOrder[2].first) {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[1].first,      // won't be used
//                            hpwlEPs_2ndOrder[3].first,      // won't be used
//                            trial4thPhase);
//    }
//
//}

void stepSizeAdaptation_by2ndOrderEPs(prec curr_hpwl) {
  if(potnPhaseDS == potnPhase1) {
    UPPER_PCOF = getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[0].first,
                                    hpwlEPs_2ndOrder[0].first, 1.0001, 0.0000);
  }
  else if(potnPhaseDS == potnPhase2) {
    UPPER_PCOF = getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[0].first,
                                    hpwlEPs_2ndOrder[1].first, 1.001, 0.000);
  }
  else if(potnPhaseDS == potnPhase3) {
    UPPER_PCOF = getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[1].first,
                                    hpwlEPs_2ndOrder[1].first, 1.001, 0.024);
  }
  else if(potnPhaseDS == potnPhase4) {
    UPPER_PCOF = getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[1].first,
                                    hpwlEPs_2ndOrder[2].first, 1.005, 0.020);
  }
  else if(potnPhaseDS == potnPhase5) {
    UPPER_PCOF = getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[2].first,
                                    hpwlEPs_2ndOrder[2].first, 1.005, 0.045);
  }
  else if(potnPhaseDS == potnPhase6) {
    UPPER_PCOF = getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[2].first,
                                    hpwlEPs_2ndOrder[3].first, 1.005, 0.045);
  }
  else if(potnPhaseDS == potnPhase7) {
    UPPER_PCOF = 1.005;
  }
  else if(potnPhaseDS == potnPhase8) {
    UPPER_PCOF = 1.01;
  }
}

// void stepSizeAdaptation_by2ndOrderEPs (prec curr_hpwl) {
//    if (curr_hpwl < hpwlEPs_2ndOrder[1].first) {
//        if (curr_hpwl < hpwlEPs_1stOrder[0].first) {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[0].first,
//                            hpwlEPs_2ndOrder[0].first,
//                            trial1stPhase_1half);
//        } else {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[0].first,
//                            hpwlEPs_2ndOrder[1].first,
//                            trial1stPhase_2half);
//        }
//    } else if (curr_hpwl >= hpwlEPs_2ndOrder[1].first
//            && curr_hpwl < hpwlEPs_2ndOrder[2].first) {
//        if (curr_hpwl < hpwlEPs_1stOrder[1].first) {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[1].first,
//                            hpwlEPs_2ndOrder[1].first,
//                            trial2ndPhase_1half);
//        } else {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[1].first,
//                            hpwlEPs_2ndOrder[2].first,
//                            trial2ndPhase_2half);
//        }
//    } else if (curr_hpwl >= hpwlEPs_2ndOrder[2].first
//            && curr_hpwl < hpwlEPs_2ndOrder[3].first) {
//        if (curr_hpwl < hpwlEPs_1stOrder[2].first) {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[2].first,
//                            hpwlEPs_2ndOrder[2].first,
//                            trial3rdPhase_1half);
//        } else {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[2].first,
//                            hpwlEPs_2ndOrder[3].first,
//                            trial3rdPhase_2half);
//        }
//    } else if (curr_hpwl > hpwlEPs_2ndOrder[3].first) {
//            UPPER_PCOF = getStepSizefromEPs (curr_hpwl,
//                            hpwlEPs_1stOrder[1].first,      // won't be used
//                            hpwlEPs_2ndOrder[3].first,      // won't be used
//                            trial4thPhase);
//    }
//}

bool isPOTNuphill() {
  switch(potnPhaseDS) {
    case potnPhase1:
      if(potnEPs[0] < potnEPs[1])
        return true;
      else
        return false;
      break;
    case potnPhase2:
      if(potnEPs[1] < potnEPs[2])
        return true;
      else
        return false;
      break;
    case potnPhase3:
      if(potnEPs[2] < potnEPs[3])
        return true;
      else
        return false;
      break;
    case potnPhase4:
      if(potnEPs[3] < potnEPs[4])
        return true;
      else
        return false;
      break;
    case potnPhase5:
      return false;
      break;
    case potnPhase6:
      return false;
      break;
    case potnPhase7:
      return false;
      break;
    case potnPhase8:
      return false;
      break;
    default:
      return false;
      break;
  }
}

int definePOTNphase(prec curr_potn) {
  if(isPOTNuphill()) {
    // cout << "Uphill" << endl;
    if(potnPhaseDS == potnPhase1 && curr_potn > potnEPs[1])
      potnPhaseDS = potnPhase2;
    else if(potnPhaseDS == potnPhase2 && curr_potn > potnEPs[2])
      potnPhaseDS = potnPhase3;
    else if(potnPhaseDS == potnPhase3 && curr_potn > potnEPs[3])
      potnPhaseDS = potnPhase4;
    else if(potnPhaseDS == potnPhase4 && curr_potn > potnEPs[4])
      potnPhaseDS = potnPhase5;
  }
  else {
    // cout << "Downhill" << endl;
    if(potnPhaseDS == potnPhase1 && curr_potn < potnEPs[1])
      potnPhaseDS = potnPhase2;
    else if(potnPhaseDS == potnPhase2 && curr_potn < potnEPs[2])
      potnPhaseDS = potnPhase3;
    else if(potnPhaseDS == potnPhase3 && curr_potn < potnEPs[3])
      potnPhaseDS = potnPhase4;
    else if(potnPhaseDS == potnPhase4 && curr_potn < potnEPs[4])
      potnPhaseDS = potnPhase5;
    else if(potnPhaseDS == potnPhase5 && curr_potn < potnEPs[5])
      potnPhaseDS = potnPhase6;
    else if(potnPhaseDS == potnPhase6 && curr_potn < potnEPs[6])
      potnPhaseDS = potnPhase7;
  }
  // cout << "Current potnPhase" << potnPhaseDS << endl;
  return potnPhaseDS;
}


// function is modified by mgwoo.
//
// this function will
// add pin's informations for moduleInstance & terminalInstance
// once it meets fixed or unfixed pins..
void AddPinInfoForModuleAndTerminal(PIN ***pin, FPOS **pof, int currentPinCount,
                                    FPOS offset, int curModuleIdx,
                                    int curNetIdx, int curPinIdxInNet,
                                    int curPinIdx, int curPinDirection,
                                    int isTerminal) {
  // pin : current Pin Object
  // pof : current Pin Offset
  //
  if(currentPinCount > 0) {
    currentPinCount++;
    *pin = (PIN **)realloc(*pin, sizeof(struct PIN *) * currentPinCount);
    *pof = (FPOS *)realloc(*pof, sizeof(struct FPOS) * currentPinCount);
    //*pin_tmp = (struct PIN**)malloc(sizeof(struct
    // PIN*)*currentPinCount, 64);
    //*pof_tmp = (struct FPOS*)malloc(sizeof(struct
    // FPOS)*currentPinCount, 64);
    // memcpy(*pin_tmp, *pin, currentPinCount*(sizeof(struct PIN*)));
    // memcpy(*pof_tmp, *pof, currentPinCount*(sizeof(struct FPOS)));
    // free(*pin);
    // free(*pof);
    //*pin = (struct PIN**)malloc(sizeof(struct PIN*)*currentPinCount,
    // 64);
    //*pof = (struct FPOS*)malloc(sizeof(struct FPOS)*currentPinCount,
    // 64);
    // memcpy(*pin, *pin_tmp, currentPinCount*(sizeof(struct PIN*)));
    // memcpy(*pof, *pof_tmp, currentPinCount*(sizeof(struct FPOS)));
    // free(*pin_tmp);
    // free(*pof_tmp);
  }
  else {
    currentPinCount++;
    *pin = (struct PIN **)malloc(sizeof(struct PIN *) * currentPinCount);
    *pof = (struct FPOS *)malloc(sizeof(struct FPOS) * currentPinCount);
  }

  (*pof)[currentPinCount - 1] = offset;

  (*pin)[currentPinCount - 1] = &pinInstance[curPinIdx];
  (*pin)[currentPinCount - 1]->term = isTerminal;
  (*pin)[currentPinCount - 1]->moduleID = curModuleIdx;
  (*pin)[currentPinCount - 1]->netID = curNetIdx;
  (*pin)[currentPinCount - 1]->pinIDinNet = curPinIdxInNet;
  (*pin)[currentPinCount - 1]->pinIDinModule = currentPinCount - 1;
  (*pin)[currentPinCount - 1]->gid = curPinIdx;
  (*pin)[currentPinCount - 1]->IO = curPinDirection;
}


static bool SortRowByCoordinate(const ROW& lhs, const ROW& rhs) {
  if(lhs.pmin.x < rhs.pmin.x) {
    return true;
  }
  if(lhs.pmin.x > rhs.pmin.x) {
    return false;
  }
  return (lhs.pmin.y < rhs.pmin.y);
}


static void GetSortedRowStor(ROW *origRowStor, int rowCnt) {
  // sort the Y-order and X-order
  vector< ROW > tmpRowStor;
  for(int i = 0; i < rowCnt; i++) {
    tmpRowStor.push_back(origRowStor[i]);
  }

  // sort function
  sort(tmpRowStor.begin(), tmpRowStor.end(), SortRowByCoordinate);

  // copy back
  for(auto &curRow : tmpRowStor) {
    origRowStor[&curRow - &tmpRowStor[0]] = curRow;
    //    cout << "SORTED!!" << endl;
    //    curRow.Dump(to_string(&curRow - &tmpRowStor[0]));
  }
}

//
// update tier_min, tier_max, and tier_row_cnt
//
void get_mms_3d_dim(FPOS *tier_min, FPOS *tier_max, int *tier_row_cnt) {
  prec xlen = grow_pmax.x - grow_pmin.x;
  prec ylen = grow_pmax.y - grow_pmin.y;
  prec aspect_ratio = (prec)ylen / (prec)xlen;

  PrintInfoPrec("AspectRatio", aspect_ratio);
  PrintInfoPrecPair("RowMinXY", grow_pmin.x, grow_pmin.y);
  PrintInfoPrecPair("RowMaxXY", grow_pmax.x, grow_pmax.y);
//  PrintInfoPrecPair("TerminalMinXY", terminal_pmin.x, terminal_pmin.y);
//  PrintInfoPrecPair("TerminalMaxXY", terminal_pmax.x, terminal_pmax.y);

  *tier_row_cnt = row_cnt;  

  tier_min->x = grow_pmin.x;
  tier_min->y = grow_pmin.y;
  
  tier_max->x = grow_pmax.x;
  tier_max->y = grow_pmax.y;
}

void transform_3d(FPOS *tier_min, FPOS *tier_max, int tier_row_cnt) {
  TERM *curTerminal = NULL;
  TIER *tier = NULL;
  MODULE *curModule = NULL;

  int tot_row_cnt = tier_row_cnt * numLayer;

  for(int i = 0; i < terminalCNT; i++) {
    curTerminal = &terminalInstance[i];

    // lutong
    // if (curTerminal->center.x > grow_pmin.x)
    // curTerminal->center.x
    // = grow_pmin.x + (curTerminal->center.x - grow_pmin.x) *
    // shrunk_ratio.x;
    // if (curTerminal->center.y > grow_pmin.y)
    // curTerminal->center.y
    // = grow_pmin.y + (curTerminal->center.y - grow_pmin.y) *
    // shrunk_ratio.y;

    curTerminal->pmin.x = curTerminal->center.x - 0.5 * curTerminal->size.x;
    curTerminal->pmin.y = curTerminal->center.y - 0.5 * curTerminal->size.y;
    curTerminal->pmax.x = curTerminal->center.x + 0.5 * curTerminal->size.x;
    curTerminal->pmax.y = curTerminal->center.y + 0.5 * curTerminal->size.y;
  }

  // mgwoo
  //
  assert(tot_row_cnt == row_cnt);

  //    row_st = (ROW *)realloc(row_st, sizeof(struct ROW) * tot_row_cnt);
  tier_st = (TIER *)malloc(sizeof(struct TIER) * numLayer);

  placementMacroCNT = 0;

  for(int i = 0; i < moduleCNT; i++) {
    curModule = &moduleInstance[i];

    if(curModule->size.y - rowHeight > Epsilon) {
      placementMacroCNT++;
      curModule->flg = Macro;
    }
    else if(curModule->size.y - rowHeight < -Epsilon) {
      printf("Cell %s   %d height ERROR: %f\n", curModule->Name(), i,
             curModule->size.y);
    }
    else {
      curModule->flg = StdCell;
    }
  }

  for(int i = 0; i < numLayer; i++) {
    tier = &tier_st[i];
    tier->row_st = &(row_st[tier_row_cnt * i]);
    tier->row_cnt = tier_row_cnt;
    tier->term_cnt = 0;

    if(terminalCNT > 0) {
      tier->term_st = (TERM **)malloc(sizeof(TERM *) * terminalCNT);
    }
    else {
      tier->term_st = NULL;
    }

    tier->modu_cnt = 0;
    tier->modu_st = NULL;
    tier->mac_cnt = 0;
    tier->mac_st = NULL;
    tier->term_area = 0;
    tier->virt_area = 0;
  }

  //    int curr_row_hei = tier_min->y;

  //    FPOS pmin;
  //    FPOS pmax;
  //    FPOS size;

  // site_wid= row_st[0].site_wid;
  // site_spa= row_st[0].site_spa;
  // ori     = row_st[0].ori;
  // sym     = row_st[0].sym;
  //    pmin = row_st[0].pmin;
  //    pmax = row_st[0].pmax;
  //    size = row_st[0].size;

  //    for(int i = 0; i < tier_row_cnt; i++) {
  // for (int j=0; j<numLayer; j++) {
  // row = &row_st[i + tier_row_cnt*j];
  // row->site_wid= site_wid; // lutong
  // row->site_spa= site_spa;
  // row->ori    = ori;
  // row->sym    = sym;
  // row->size.x = tier_max->x - tier_min->x;
  // row->size.y = rowHeight;
  // row->pmin.x = tier_min->x;
  // row->pmin.y = curr_row_hei;
  // row->pmax.x = row->pmin.x + row->size.x;
  // row->pmax.y = row->pmin.y + row->size.y;
  // row->x_cnt  = tier_max->x - tier_min->x;
  //}
  //        curr_row_hei += rowHeight;
  //    }

  // update global_variable
  row_cnt = tot_row_cnt;

  for(int i = 0; i < numLayer; i++) {
    tier = &tier_st[i];

    tier->pmin.x = tier_min->x;
    tier->pmin.y = tier_min->y;

    tier->pmax.x = tier_max->x;
    tier->pmax.y = tier_max->y;

    tier->size.x = tier->pmax.x - tier->pmin.x;
    tier->size.y = tier->pmax.y - tier->pmin.y;

    tier->center.x = (tier->pmin.x + tier->pmax.x) * 0.5;
    tier->center.y = (tier->pmin.y + tier->pmax.y) * 0.5;

    tier->area = tier->size.x * tier->size.y;
  }

  // TIER's term_area update!
  // mgwoo
  /*
  for(int i = 0; i < terminalCNT; i++) {
      curTerminal = &terminalInstance[i];
      tier = &tier_st[0];
      tier->term_st[tier->term_cnt++] = curTerminal;

      // skip for IO pin informations
//        if( curTerminal->isTerminalNI ) {
//            continue;
//        }

      // if there is no shape's information
      if( shapeMap.find( curTerminal->Name() ) == shapeMap.end() ) {
          cout << "not found: " << curTerminal->Name() << " " << place_st_cnt <<
endl;
          // tier->term_area += get_common_area(curTerminal->pmin,
          // curTerminal->pmax, tier->pmin, tier->pmax);
          for(int j=0; j<place_st_cnt; j++) {
              PLACE* curPlace = &place_st[j];
              tier->term_area += get_common_area(curTerminal->pmin,
                  curTerminal->pmax, curPlace->org, curPlace->end);
          }
      }
      // if there exist shape's information
      else {
          for(auto& curIdx : shapeMap[curTerminal->Name()]) {
              cout << "name: " << curTerminal->Name() << endl;
              prec llx = shapeStor[curIdx].llx,
                   lly = shapeStor[curIdx].lly,
                   width = shapeStor[curIdx].width,
                   height = shapeStor[curIdx].height;
              FPOS tmpMin(llx, lly, 0), tmpMax(llx + width, lly + height, 1);
              tmpMin.Dump("tmpMin");
              tmpMax.Dump("tmpMax");
              // prec commonArea = get_common_area(tmpMin, tmpMax,
              // tier->pmin, tier->pmax);
              for(int j=0; j<place_st_cnt; j++) {
                  PLACE* curPlace = &place_st[j];
                  tier->term_area += get_common_area(tmpMin, tmpMax,
                          curPlace->org, curPlace->end);
              }
          }
      }
  }
  */

  for(int i = 0; i < numLayer; i++) {
    tier = &tier_st[i];
    if(tier->term_cnt > 0) {
      tier->term_st = (TERM **)realloc(tier->term_st,
                                       sizeof(struct TERM *) * tier->term_cnt);
    }
    else if(tier->term_st) {
      free(tier->term_st);
    }
  }
}

//
// update: total_macro_area,
//          total_std_area,
//          total_PL_area,
//          gmov_macro_cnt,
//
//          placementStdcellCNT, placementMacroCNT
//
//          gmin, gmax // global minimum, global maximum
//          (plotting)
//
//  malloc : place_st, place_backup_st // place_st_cnt
//
void post_read_3d(void) {
  // int tmpMultiHeightCellCNT = 0;
  struct MODULE *curModule = NULL;
  struct TERM *curTerminal = NULL;

  placementStdcellCNT = 0;
  total_std_area = 0;
  placementMacroCNT = 0;
  total_macro_area = 0;

  for(int i = 0; i < moduleCNT; i++) {
    curModule = &moduleInstance[i];
    if(curModule->size.y - rowHeight > Epsilon) {
      // if (curModule->size.y - rowHeight > 24.0 + Epsilon) {
      // //lutong
      curModule->flg = Macro;
      placementMacroCNT++;
      total_macro_area += curModule->area;
    }
    else if(curModule->size.y - rowHeight < -Epsilon) {
      printf("*** ERROR:  Cell \"%d\" Height ERROR \n", i);
    }
    else {
      curModule->flg = StdCell;
      placementStdcellCNT++;
      total_std_area += curModule->area;
      continue;
    }
  }

  PrintInfoInt("NumPlaceStdCells", placementStdcellCNT);
  PrintInfoInt("NumPlaceMacros", placementMacroCNT);

  gmov_mac_cnt = placementMacroCNT;

  // initial malloc : row_cnt
  place_st_cnt = 0;
  place_st = (PLACE *)malloc(sizeof(struct PLACE) * row_cnt);

  ROW *last_row = NULL;
  PLACE *curPlace = NULL;

  GetSortedRowStor(row_st, row_cnt);

  //
  // Assume, height is equally distributed.
  //
  int place_hei = 0;
  for(int i = 0; i < row_cnt; i++) {
    ROW *row = &row_st[i];
    if(i == 0) {
      place_hei = row->size.y;
    }
    else if(row->pmin.x != last_row->pmin.x ||
            row->pmax.x != last_row->pmax.x ||

            row->pmin.y != last_row->pmax.y) {
      curPlace = &place_st[place_st_cnt++];

      curPlace->org.x = last_row->pmin.x;
      curPlace->org.y = last_row->pmax.y - place_hei;

      curPlace->end.x = last_row->pmax.x;
      curPlace->end.y = last_row->pmax.y;

      curPlace->stp.x = SITE_SPA;
      curPlace->stp.y = rowHeight;

      curPlace->cnt.x = curPlace->end.x - curPlace->org.x;
      curPlace->cnt.y = curPlace->end.y - curPlace->org.y;

      curPlace->area = curPlace->cnt.x * curPlace->cnt.y;

      curPlace->center.x = 0.5 * (curPlace->org.x + curPlace->end.x);
      curPlace->center.y = 0.5 * (curPlace->org.y + curPlace->end.y);

      place_hei = row->size.y;
    }
    else {
      place_hei += row->size.y;
    }
    last_row = row;
  }

  curPlace = &place_st[place_st_cnt++];

  curPlace->org.x = last_row->pmin.x;
  curPlace->org.y = last_row->pmax.y - place_hei;

  curPlace->end.x = last_row->pmax.x;
  curPlace->end.y = last_row->pmax.y;

  curPlace->stp.x = SITE_SPA;
  curPlace->stp.y = rowHeight;

  curPlace->cnt.x = curPlace->end.x - curPlace->org.x;
  curPlace->cnt.y = curPlace->end.y - curPlace->org.y;

  curPlace->area = curPlace->cnt.x * curPlace->cnt.y;

  curPlace->center.x = 0.5 * (curPlace->org.x + curPlace->end.x);
  curPlace->center.y = 0.5 * (curPlace->org.y + curPlace->end.y);

  //    for(int i=0; i<place_st_cnt; i++) {
  //        place_st[i].Dump(to_string(i));
  //    }

  // second re-malloc : upto place_st_cnt;
  place_st = (PLACE *)realloc(place_st, sizeof(struct PLACE) * place_st_cnt);
  place_backup_st =
      (PLACE *)realloc(place_backup_st, sizeof(struct PLACE) * place_st_cnt);

  for(int i = 0; i < place_st_cnt; i++) {
    place_backup_st[i] = place_st[i];
  }

  PrintInfoPrecPair( "RowSize", SITE_SPA, rowHeight );
  PrintInfoInt( "NumRows", row_cnt );

  // global variable 'place' update
  place.stp.x = SITE_SPA;
  place.stp.y = 1.0;  // rowHeight....................????

  place.org = place_st[0].org;
  place.end = place_st[0].end;

  total_PL_area = 0;
  for(int i = 0; i < place_st_cnt; i++) {
    curPlace = &place_st[i];

    place.org.x = min(place.org.x, curPlace->org.x);
    place.org.y = min(place.org.y, curPlace->org.y);

    place.end.x = max(place.end.x, curPlace->end.x);
    place.end.y = max(place.end.y, curPlace->end.y);

    total_PL_area += curPlace->area;
  }

  // global min & global max variable setting
  gmin = place.org;
  gmax = place.end;

  for(int i = 0; i < terminalCNT; i++) {
    curTerminal = &terminalInstance[i];

    gmin.x = min(gmin.x, curTerminal->pmin.x);
    gmin.y = min(gmin.y, curTerminal->pmin.y);

    gmax.x = max(gmax.x, curTerminal->pmax.x);
    gmax.y = max(gmax.y, curTerminal->pmax.y);
  }


  /*
  // setting additional margin
  // for plotting.. here It is not required

      gmin.Dump("gmin first");

      prec lx_mg = place.org.x - gmin.x,
           ly_mg = place.org.y - gmin.y,
           rx_mg = gmax.x - place.end.x,
           ry_mg = gmax.y - place.end.y;

  //    cout << "lx_mg: " << lx_mg << ", ly_mg: " << ly_mg << endl;
  //    cout << "rx_mg: " << rx_mg << ", ry_mg: " << ry_mg << endl;

      prec max_mg = PREC_MIN;
      max_mg = max(max_mg, lx_mg);
      max_mg = max(max_mg, ly_mg);
      max_mg = max(max_mg, rx_mg);
      max_mg = max(max_mg, ry_mg);

  //    cout << "max_mg: " << max_mg << endl;

      gmin.x = place.org.x - max_mg;
      gmin.y = place.org.y - max_mg;

      gmax.x = place.end.x + max_mg;
      gmax.y = place.end.y + max_mg;
  */

  PrintInfoPrecPair("GlobalAreaLxLy", gmin.x, gmin.y);
  PrintInfoPrecPair("GlobalAreaUxUy", gmax.x, gmax.y);

  PrintInfoPrecPair("PlaceAreaLxLy", place.org.x, place.org.y);
  PrintInfoPrecPair("PlaceAreaUxUy", place.end.x, place.end.y);

  place.cnt.x = place.end.x - place.org.x;
  place.cnt.y = place.end.y - place.org.y;

  place.center.x = 0.5 * (place.org.x + place.end.x);
  place.center.y = 0.5 * (place.org.y + place.end.y);

  place.area = place.cnt.x * place.cnt.y;
}

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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include <error.h>
#include <sys/time.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
// #include        <ncurses.h>
#include <iomanip>

#include "bookShelfIO.h"
#include "bin.h"
#include "fft.h"
#include "gcell.h"
#include "global.h"
#include "mkl.h"
#include "ns.h"
#include "opt.h"
#include "plot.h"
#include "wlen.h"

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

void setup_before_opt(void) {
//    mkdirPlot();
    whitespace_init();

    if(total_std_den > target_cell_den + Epsilon) {
        target_cell_den = total_std_den;
        global_macro_area_scale = total_std_den;
    }

    total_modu_area = total_std_area + total_macro_area * target_cell_den;
    inv_total_modu_area = 1.00 / total_modu_area;

    dp_margin_per_tier = 0.0f;

    cell_init();
    cell_filler_init();  // So that we can get total_filler_area.
    calc_average_module_width();

    total_cell_area = total_modu_area + total_filler_area;

    //
    wlen_init();
    msh_init();
//    bin_init();

    charge_fft_init(msh, bin_stp, 1);
    update_cell_den();
    wcof_init(bin_stp);
}

int setup_before_opt_mGP2D(void) {
    tier_init_2D(mGP2D);
    bin_init_2D(mGP2D);

    charge_fft_init(dim_bin_mGP2D, bin_stp_mGP2D, 0);
    wcof_init(bin_stp_mGP2D);
    wlen_init_mGP2D();
    cell_init_2D();
    cell_copy();
    net_update_init();
    return 1;
}

int setup_before_opt_cGP2D(void) {
    tier_init_2D(cGP2D);
    bin_init_2D(cGP2D);
    // routability
    if(routabilityCMD == true)
        tile_init_cGP2D();

    charge_fft_init(dim_bin_cGP2D, bin_stp_cGP2D, 0);
    wcof_init(bin_stp_cGP2D);
    wlen_init_cGP2D();
    cell_init_2D();
    cell_copy();
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
    modu_copy();
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
        CELLx* cell = &gcell_st[i];
        cell->center = st[i];

        FPOS tmp_scal = fp_scal(0.5, cell->size);
        cell->pmin = fp_subt(cell->center, tmp_scal);
        cell->pmax = fp_add(cell->center, tmp_scal);
    }
}

// void cell_update (struct FPOS *st, int n) {
//    struct  CELLx   *cell = NULL;
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

    TIER* tier = &tier_st[0];

    // for each Terminal Instance
    for(int i = 0; i < terminalCNT; i++) {
        TERM* curTerminal = &terminalInstance[i];
        curTerminal->PL_area = 0;

        // skip for IO pin..
        if( curTerminal->isTerminalNI ) {
            continue;
        }

        // if there is no shapes..
        if( shapeMap.find( curTerminal-> name ) == shapeMap.end() ) {
            for(int j = 0; j < place_st_cnt; j++) {
                PLACE* pl = &place_st[j];

//                long commonArea = lGetCommonAreaXY( curTerminal->pmin, curTerminal->pmax, 
//                                                   pl->org, pl->end ); 
                prec commonArea = pGetCommonAreaXY( curTerminal->pmin, curTerminal->pmax, 
                                                   pl->org, pl->end ); 
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
//            cout << fixed <<setprecision(4) <<curTerminal->name << " : " << total_termPL_area << endl;
        }
        // there are shapes
        else {
            for(auto& curIdx : shapeMap[curTerminal->name]) {
                for(int j = 0; j < place_st_cnt; j++) {
                    PLACE* pl = &place_st[j];

                    prec llx = shapeStor[curIdx].llx, 
                         lly = shapeStor[curIdx].lly,
                         width = shapeStor[curIdx].width,
                         height = shapeStor[curIdx].height;
                         
                    FPOS tmpMin(llx, lly, 0), tmpMax(llx + width, lly + height, 1);
                    
                    
//                    long commonArea = lGetCommonAreaXY( tmpMin, tmpMax, pl->org, pl->end ); 
                    prec commonArea = pGetCommonAreaXY( tmpMin, tmpMax, pl->org, pl->end ); 

                    curTerminal->PL_area += commonArea;
                    total_termPL_area += commonArea;
                    tier->term_area += commonArea;
                    
                }
//                cout << curTerminal->name << "/" << shapeStor[curIdx].name<<" : " << curTerminal->PL_area << endl;
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
    printf("INFO:  Chip Area: %.0lf x %.0lf x %.0lf = %.0lf \n", place.cnt.x,
           place.cnt.y, place.cnt.z, place.area);
    fflush(stdout);
    printf("INFO:  Total_PL_Area = %.0lf, %%%.2lf of chip\n", total_PL_area,
           total_PL_area / place.area * 100.0);
    fflush(stdout);
    printf("INFO:  Total_TermPL_Area = %ld, %%%.2lf of PL\n",
           total_termPL_area, 1.0 * total_termPL_area / place.area * 100.0);
    fflush(stdout);
    printf("INFO:  Total_WS_Area = %ld, %%%.2lf of PL\n", 
           total_WS_area, 1.0 * total_WS_area / total_PL_area * 100.0);
    fflush(stdout);
    printf("INFO:  Total_Macro_Area = %.0lf, %%%.2lf of WS\n", total_macro_area,
           total_macro_area / total_WS_area * 100.0);
    fflush(stdout);
    printf("INFO:  Total_StdCell_Area = %.0lf, %%%.2lf of WS\n", total_std_area,
           total_std_area / total_WS_area * 100.0);
    fflush(stdout);
}

void filler_adj(void) {
    if(STAGE == mGP2D)
        filler_adj_mGP2D();
    else if(STAGE == cGP3D)
        rand_filler_adj(moduleCNT);
    else if(STAGE == cGP2D)
        filler_adj_cGP2D();
}

void rand_filler_adj(int idx) {
    int i = 0;
    struct FPOS pmin = zeroFPoint, pmax = zeroFPoint;
    struct FPOS rnd = zeroFPoint;
    struct CELLx *filler = NULL;

    for(i = idx; i < gcell_cnt; i++) {
        filler = &gcell_st[i];
        pmin = fp_add(place.org, fp_scal(0.5, filler->size));
        pmax = fp_subt(place.end, fp_scal(0.5, filler->size));

        rnd = fp_rand();

        filler->center = fp_add(
            fp_mul(fp_scal(inv_RAND_MAX, rnd), fp_subt(pmax, pmin)), pmin);

        filler->pmin = fp_subt(filler->center, fp_scal(0.5, filler->size));
        filler->pmax = fp_add(filler->center, fp_scal(0.5, filler->size));
    }
}

void filler_adj_mGP2D(void) {
    int z = 0, i = 0;
    struct FPOS pmin = zeroFPoint, pmax = zeroFPoint;
    struct FPOS rnd = zeroFPoint;
    struct CELLx *filler = NULL;
    struct TIER *tier = NULL;

    for(z = 0; z < numLayer; z++) {
        tier = &tier_st[z];

        for(i = tier->modu_cnt; i < tier->cell_cnt; i++) {
            filler = tier->cell_st[i];

            pmin = fp_add(tier->pmin, fp_scal(0.5, filler->size));
            pmax = fp_subt(tier->pmax, fp_scal(0.5, filler->size));

            rnd = fp_rand();

            filler->center = fp_add(
                fp_mul(fp_scal(inv_RAND_MAX, rnd), fp_subt(pmax, pmin)), pmin);

            filler->center.z = tier->center.z;

            filler->pmin = fp_subt(filler->center, fp_scal(0.5, filler->size));
            filler->pmax = fp_add(filler->center, fp_scal(0.5, filler->size));
        }
    }
}

void filler_adj_cGP2D(void) {
    int z = 0, i = 0;
    struct FPOS pmin = zeroFPoint, pmax = zeroFPoint;
    struct FPOS rnd = zeroFPoint;
    struct CELLx *filler = NULL;
    struct TIER *tier = NULL;

    for(z = 0; z < numLayer; z++) {
        tier = &tier_st[z];

        for(i = tier->modu_cnt; i < tier->cell_cnt; i++) {
            filler = tier->cell_st[i];

            pmin = fp_add(tier->pmin, fp_scal(0.5, filler->size));
            pmax = fp_subt(tier->pmax, fp_scal(0.5, filler->size));

            rnd = fp_rand();

            filler->center = fp_add(
                fp_mul(fp_scal(inv_RAND_MAX, rnd), fp_subt(pmax, pmin)), pmin);

            filler->center.z = tier->center.z;

            filler->pmin = fp_subt(filler->center, fp_scal(0.5, filler->size));
            filler->pmax = fp_add(filler->center, fp_scal(0.5, filler->size));
        }

        /* min_idx = max_idx; */
    }
}

void cell_filler_init() {
    int i = 0;
    struct CELLx *filler = NULL;
    /* struct MODULE*mdp=NULL; */
    prec k_f2c = 1.0;
    prec f_area = 0;
    struct FPOS f_size = zeroFPoint;
    struct CELLx *gcell_st_tmp = NULL;
    prec a = 0;
    FILE *fp = NULL;
//    char filler_fn[BUF_SZ];
    struct FPOS org = zeroFPoint, end = zeroFPoint, len = zeroFPoint,
                rnd = zeroFPoint;

//    sprintf(filler_fn, "%s/%s_filler.txt", dir_bnd, gbch);
    a = k_f2c * avg80p_cell_area;
    f_area = a;

    if(flg_3dic) {
        f_size.x = avg80p_cell_dim.x;
        f_size.y = avg80p_cell_dim.y;
        f_size.z = avg80p_cell_dim.z;

        f_area = f_size.x * f_size.y * f_size.z;
    }
    else {
        f_size.x =
            avg80p_cell_dim.x < bin_stp.x ? avg80p_cell_dim.x : bin_stp.x;
        f_size.y =
            avg80p_cell_dim.y < bin_stp.y ? avg80p_cell_dim.y : bin_stp.y;
        f_size.z = TIER_DEP;

        f_area = f_size.x * f_size.y * f_size.z;
    }

//    fp = fopen(filler_fn, "w");

    total_move_available_area = total_WS_area * target_cell_den;
    total_filler_area = total_move_available_area - total_modu_area;

    if(total_filler_area < 0) {
        printf("ERROR -- negative filler area! \n");
        exit(1);
    }

    filler_area = f_area;
    filler_size = f_size;

    printf("INFO:  FillerCell's Area = %.6lf\n", filler_area);

    gfiller_cnt = (int)(total_filler_area / filler_area + 0.5);

//    fprintf(fp, "%d %.6lf\n", gfiller_cnt, filler_area);

    if(flg_3dic) {
        printf(
            "INFO:  FillerCell's X = %.6lf , FillerCell's Y = %.6lf, "
            "FillerCell's Z = %.6lf\n",
            filler_size.x, filler_size.y, filler_size.z);

//        fprintf(fp, "%.6lf %.6lf %.6lf\n", filler_size.x, filler_size.y,
//                filler_size.z);
    }
    else {
        printf("INFO:  FillerCell's X = %.6lf , FillerCell's Y = %.6lf\n",
               filler_size.x, filler_size.y);

//        fprintf(fp, "%.6lf %.6lf\n", filler_size.x, filler_size.y);
    }

    gcell_cnt = moduleCNT + gfiller_cnt;

    // igkang:  replace realloc to mkl
    gcell_st_tmp =
        (struct CELLx *)mkl_malloc(sizeof(struct CELLx) * gcell_cnt, 64);
    memcpy(gcell_st_tmp, gcell_st, moduleCNT * (sizeof(struct CELLx)));
    mkl_free(gcell_st);

    gcell_st = (CELLx *)mkl_malloc(sizeof(struct CELLx) * gcell_cnt, 64);
    memcpy(gcell_st, gcell_st_tmp, gcell_cnt * (sizeof(struct CELLx)));
    mkl_free(gcell_st_tmp);

    printf("INFO:  #CELL = %d =  %d (#MODULE) + %d (#FILLER)\n", gcell_cnt,
           moduleCNT, gfiller_cnt);

    for(i = moduleCNT; i < gcell_cnt; i++) {
        filler = &gcell_st[i];
        filler->flg = FillerCell;
        filler->idx = i - moduleCNT;

        sprintf(filler->name, "f%d", filler->idx);
        filler->size = filler_size;
        filler->half_size.x = 0.5 * filler->size.x;
        filler->half_size.y = 0.5 * filler->size.y;
        filler->half_size.z = 0.5 * filler->size.z;

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

//        if(flg_3dic)
//            fprintf(fp, "%.6lf %.6lf %.6lf\n", filler->center.x,
//                    filler->center.y, filler->center.z);
//        else
//            fprintf(fp, "%.6lf %.6lf\n", filler->center.x, filler->center.y);
    }
//    fclose(fp);
}

void cell_init(void) {
    int i = 0, j = 0, k = 0;
    int min_idx = (int)(0.05 * (prec)moduleCNT);
    int max_idx = (int)(0.95 * (prec)moduleCNT);
    prec total_area = 0, avg_cell_area = 0;
    prec avg_cell_x = 0, avg_cell_y = 0, avg_cell_z = 0;
    prec total_x = 0, total_y = 0, total_z = 0;
    struct CELLx *cell = NULL;
    struct MODULE *mdp = NULL;
    struct FPOS pof;
    struct PIN *pin = NULL;
    struct NET *net = NULL;
    prec *cell_area_st = (prec *)mkl_malloc(sizeof(prec) * moduleCNT, 64);
    prec *cell_x_st = (prec *)mkl_malloc(sizeof(prec) * moduleCNT, 64);
    prec *cell_y_st = (prec *)mkl_malloc(sizeof(prec) * moduleCNT, 64);
    prec *cell_z_st = (prec *)mkl_malloc(sizeof(prec) * moduleCNT, 64);

    for(i = 0; i < moduleCNT; i++) {
        mdp = &moduleInstance[i];
        cell_area_st[i] = mdp->area;
        cell_x_st[i] = mdp->size.x;
        cell_y_st[i] = mdp->size.y;
        cell_z_st[i] = mdp->size.z;
    }

//    printf("qsort has before problem?\n");
//    fflush(stdout);
    qsort(cell_area_st, moduleCNT, sizeof(prec), area_sort);
    qsort(cell_x_st, moduleCNT, sizeof(prec), area_sort);
    qsort(cell_y_st, moduleCNT, sizeof(prec), area_sort);
    qsort(cell_z_st, moduleCNT, sizeof(prec), area_sort);
//    printf("qsort has after problem?\n");
//    fflush(stdout);

    for(i = min_idx; i < max_idx; i++) {
        total_area += cell_area_st[i];
        total_x += cell_x_st[i];
        total_y += cell_y_st[i];
        total_z += cell_z_st[i];
    }

    mkl_free(cell_area_st);
    mkl_free(cell_x_st);
    mkl_free(cell_y_st);
    mkl_free(cell_z_st);

    avg_cell_area = total_area / ((prec)(max_idx - min_idx));
    avg_cell_x = total_x / ((prec)(max_idx - min_idx));
    avg_cell_y = total_y / ((prec)(max_idx - min_idx));
    avg_cell_z = total_z / ((prec)(max_idx - min_idx));

    avg80p_cell_area = avg_cell_area;
    avg80p_cell_dim.x = avg_cell_x;
    avg80p_cell_dim.y = avg_cell_y;
    avg80p_cell_dim.z = avg_cell_z;

    printf("INFO:  Average 80pct Cell Area = %.4lf\n", avg80p_cell_area);

    gcell_cnt = moduleCNT;
    gcell_st = (struct CELLx *)mkl_malloc(sizeof(struct CELLx) * gcell_cnt, 64);

    for(i = 0; i < netCNT; i++) {
        net = &netInstance[i];
        net->mod_idx = -1;
        net->pin2 = (struct PIN **)mkl_malloc(
            sizeof(struct PIN *) * net->pinCNTinObject, 64);
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
        strcpy(cell->name, mdp->name);
        cell->size = mdp->size;
        cell->half_size = mdp->half_size;
        cell->area = mdp->area;
        cell->pof = (struct FPOS *)mkl_malloc(
            sizeof(struct FPOS) * mdp->pinCNTinObject, 64);
        cell->pin = (struct PIN **)mkl_malloc(
            sizeof(struct PIN *) * mdp->pinCNTinObject, 64);
        cell->pinCNTinObject = 0;

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
        // cell->pin = (PIN**)realloc(cell->pin,sizeof(struct
        // PIN*)*cell->pinCNTinObject);
        // cell->pof = (FPOS*)realloc(cell->pof,sizeof(struct
        // FPOS)*cell->pinCNTinObject);
        cell->pin_tmp = (struct PIN **)mkl_malloc(
            sizeof(struct PIN *) * cell->pinCNTinObject, 64);
        cell->pof_tmp = (struct FPOS *)mkl_malloc(
            sizeof(struct FPOS) * cell->pinCNTinObject, 64);
        memcpy(cell->pin_tmp, cell->pin,
               cell->pinCNTinObject * (sizeof(struct PIN *)));
        memcpy(cell->pof_tmp, cell->pof,
               cell->pinCNTinObject * (sizeof(struct FPOS)));
        mkl_free(cell->pin);
        mkl_free(cell->pof);
        cell->pin = (struct PIN **)mkl_malloc(
            sizeof(struct PIN *) * cell->pinCNTinObject, 64);
        cell->pof = (struct FPOS *)mkl_malloc(
            sizeof(struct FPOS) * cell->pinCNTinObject, 64);
        memcpy(cell->pin, cell->pin_tmp,
               cell->pinCNTinObject * (sizeof(struct PIN *)));
        memcpy(cell->pof, cell->pof_tmp,
               cell->pinCNTinObject * (sizeof(struct FPOS)));
        mkl_free(cell->pin_tmp);
        mkl_free(cell->pof_tmp);

        cell->pmin = mdp->pmin;
        cell->pmax = mdp->pmax;
        cell->center = mdp->center;
    }
}

void cell_delete(void) {
    struct CELLx *cell = NULL;

    for(int i = 0; i < gcell_cnt; i++) {
        cell = &gcell_st[i];
        mkl_free(cell->pin);
        mkl_free(cell->pof);
    }
    mkl_free(gcell_st);
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

void modu_copy(void) {
    int i = 0;
    struct CELLx *cell = NULL;
    struct MODULE *mdp = NULL;

    for(i = 0; i < moduleCNT; i++) {
        cell = &gcell_st[i];
        mdp = &moduleInstance[i];

        mdp->center = cell->center;

        mdp->pmin.x = mdp->center.x - mdp->half_size.x;
        mdp->pmin.y = mdp->center.y - mdp->half_size.y;
        // mdp->pmin.z = mdp->center.z - mdp->half_size.z;

        mdp->pmax.x = mdp->center.x + mdp->half_size.x;
        mdp->pmax.y = mdp->center.y + mdp->half_size.y;
        // mdp->pmax.z = mdp->center.z + mdp->half_size.z;
    }

    return;
}

void cell_copy(void) {
    struct CELLx *cell = NULL;
    struct MODULE *module = NULL;

    for(int i = 0; i < moduleCNT; i++) {
        cell = &gcell_st[i];
        module = &moduleInstance[i];

        cell->center = module->center;
        cell->pmin = module->pmin;
        cell->pmax = module->pmax;
    }
}

int min_sort(const void *a, const void *b) {
    struct DST *aa = (struct DST *)a;
    struct DST *bb = (struct DST *)b;

    return aa->dxy > bb->dxy ? 1 : 0;
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
        // if (flg_3dic) sum_dis += pow(fabs(a[i].z-b[i].z),num);
        // if (flg_3dic) sum_dis += (a[i].z-b[i].z)*(a[i].z-b[i].z);
        sum_dis += (flg_3dic) ? (a[i].z - b[i].z) * (a[i].z - b[i].z) : 0;
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
    //retCoef = (x < 0)? bas : bas * pow((bas*1.2), x * (-1.0));
    retCoef = (retCoef < LOWER_PCOF)? LOWER_PCOF: retCoef;

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

void expand_gp_from_one(void) {
    int i = 0, j = 0;
    BIN *bp = NULL;
    MODULE *modu = NULL;
    CELLx *cell = NULL;
    TERM *term = NULL;
    PLACE *pl = NULL, *pl_backup = NULL;
    FPOS dim_scal = zeroFPoint;
    prec area_scal = 0;

    dim_scal.x = place_backup.cnt.x * GP_SCAL;
    dim_scal.y = place_backup.cnt.y * GP_SCAL;
    dim_scal.z = place_backup.cnt.z * GP_SCAL;

    area_scal = fp_product(dim_scal);

    total_modu_area *= area_scal;
    total_std_area *= area_scal;
    total_macro_area *= area_scal;
    total_cell_area *= area_scal;
    total_term_area *= area_scal;
    total_move_available_area *= area_scal;
    total_filler_area *= area_scal;
    total_PL_area *= area_scal;
    total_termPL_area *= area_scal;
    total_WS_area *= area_scal;
    filler_area *= area_scal;

    filler_size.x *= dim_scal.x;
    filler_size.y *= dim_scal.y;
    filler_size.z *= dim_scal.z;

    for(i = 0; i < moduleCNT; i++) {
        modu = &moduleInstance[i];
        modu->center.x = modu->center.x * (place_backup.cnt.x * GP_SCAL) +
                         place_backup.org.x;
        modu->center.y = modu->center.y * (place_backup.cnt.y * GP_SCAL) +
                         place_backup.org.y;
        // modu->center.z = modu->center.z * (place_backup.cnt.z * GP_SCAL) +
        // place_backup.org.z;
        modu->size.x *= place_backup.cnt.x * GP_SCAL;
        modu->size.y *= place_backup.cnt.y * GP_SCAL;
        // modu->size.z *= place_backup.cnt.z * GP_SCAL;
        modu->half_size.x = 0.5 * modu->size.x;
        modu->half_size.y = 0.5 * modu->size.y;
        // modu->half_size.z = 0.5 * modu->size.z;
        modu->pmin.x = modu->center.x - modu->half_size.x;
        modu->pmin.y = modu->center.y - modu->half_size.y;
        // modu->pmin.z = modu->center.z - modu->half_size.z;
        modu->pmax.x = modu->center.x + modu->half_size.x;
        modu->pmax.y = modu->center.y + modu->half_size.y;
        // modu->pmax.z = modu->center.z + modu->half_size.z;
        modu->area = modu->size.x * modu->size.y * modu->size.z;

        for(j = 0; j < modu->pinCNTinObject; j++) {
            modu->pof[j].x *= place_backup.cnt.x * GP_SCAL;
            modu->pof[j].y *= place_backup.cnt.y * GP_SCAL;
            // modu->pof[j].z *= place_backup.cnt.z * GP_SCAL;

            modu->pin[j]->fp.x = modu->center.x + modu->pof[j].x;
            modu->pin[j]->fp.y = modu->center.y + modu->pof[j].y;
            // modu->pin[j]->fp.z = modu->center.z + modu->pof[j].z;
        }
    }

    for(i = 0; i < gcell_cnt; i++) {
        cell = &gcell_st[i];
        cell->center.x = cell->center.x * (place_backup.cnt.x * GP_SCAL) +
                         place_backup.org.x;
        cell->center.y = cell->center.y * (place_backup.cnt.y * GP_SCAL) +
                         place_backup.org.y;
        // cell->center.z = cell->center.z * (place_backup.cnt.z * GP_SCAL) +
        // place_backup.org.z;
        cell->size.x *= place_backup.cnt.x * GP_SCAL;
        cell->size.y *= place_backup.cnt.y * GP_SCAL;
        // cell->size.z *= place_backup.cnt.z * GP_SCAL;
        cell->half_size.x = 0.5 * cell->size.x;
        cell->half_size.y = 0.5 * cell->size.y;
        // cell->half_size.z = 0.5 * cell->size.z;
        cell->pmin.x = cell->center.x - cell->half_size.x;
        cell->pmin.y = cell->center.y - cell->half_size.y;
        // cell->pmin.z = cell->center.z - cell->half_size.z;
        cell->pmax.x = cell->center.x + cell->half_size.x;
        cell->pmax.y = cell->center.y + cell->half_size.y;
        // cell->pmax.z = cell->center.z + cell->half_size.z;
        cell->half_den_size.x *= dim_scal.x;
        cell->half_den_size.y *= dim_scal.y;
        // cell->half_den_size.z *= dim_scal.z;
        cell->area = cell->size.x * cell->size.y;  // * cell->size.z;

        // total_cell_area += cell->area;

        for(j = 0; j < cell->pinCNTinObject; j++) {
            cell->pof[j].x *= place_backup.cnt.x * GP_SCAL;
            cell->pof[j].y *= place_backup.cnt.y * GP_SCAL;
            // cell->pof[j].z *= place_backup.cnt.z * GP_SCAL;

            cell->pin[j]->fp.x = cell->center.x + cell->pof[j].x;
            cell->pin[j]->fp.y = cell->center.y + cell->pof[j].y;
            // cell->pin[j]->fp.z = cell->center.z + cell->pof[j].z;
        }
    }

    for(i = 0; i < terminalCNT; i++) {
        term = &terminalInstance[i];
        term->center.x = term->center.x * (place_backup.cnt.x * GP_SCAL) +
                         place_backup.org.x;
        term->center.y = term->center.y * (place_backup.cnt.y * GP_SCAL) +
                         place_backup.org.y;
        // term->center.z = term->center.z * (place_backup.cnt.z * GP_SCAL) +
        // place_backup.org.z;
        term->size.x *= place_backup.cnt.x * GP_SCAL;
        term->size.y *= place_backup.cnt.y * GP_SCAL;
        // term->size.z *= place_backup.cnt.z * GP_SCAL;
        term->pmin.x = term->center.x - 0.5 * term->size.x;
        term->pmin.y = term->center.y - 0.5 * term->size.y;
        // term->pmin.z = term->center.z - 0.5 * term->size.z;
        term->pmax.x = term->center.x + 0.5 * term->size.x;
        term->pmax.y = term->center.y + 0.5 * term->size.y;
        // term->pmax.z = term->center.z + 0.5 * term->size.z;
        term->area = term->size.x * term->size.y;  // * term->size.z;
        term->PL_area *= area_scal;
        // total_term_area += term->area;

        for(j = 0; j < term->pinCNTinObject; j++) {
            term->pof[j].x *= place_backup.cnt.x * GP_SCAL;
            term->pof[j].y *= place_backup.cnt.y * GP_SCAL;
            // term->pof[j].z *= place_backup.cnt.z * GP_SCAL;

            term->pin[j]->fp.x = term->center.x + term->pof[j].x;
            term->pin[j]->fp.y = term->center.y + term->pof[j].y;
            // term->pin[j]->fp.z = term->center.z + term->pof[j].z;
        }
    }

    for(i = 0; i < place_st_cnt; i++) {
        pl = &place_st[i];
        pl_backup = &place_backup_st[i];
        pl->center = pl_backup->center;
        pl->stp = pl_backup->stp;
        pl->cnt = pl_backup->cnt;
        pl->org = pl_backup->org;
        pl->end = pl_backup->end;
        pl->area = pl->cnt.x * pl->cnt.y;  // * pl->cnt.z;
    }

    place.center = place_backup.center;
    place.stp = place_backup.stp;
    place.cnt = place_backup.cnt;
    place.org = place_backup.org;
    place.end = place_backup.end;
    place.area = place.cnt.x * place.cnt.y;  // * place.cnt.z;

    bin_stp.x = place.cnt.x / (prec)max_bin.x;
    bin_stp.y = place.cnt.y / (prec)max_bin.y;
    // bin_stp.z = place.cnt.z / (prec  ) max_bin.z;

    half_bin_stp.x = bin_stp.x * 0.5;
    half_bin_stp.y = bin_stp.y * 0.5;
    // half_bin_stp.z = bin_stp.z * 0.5;

    inv_bin_stp.x = 1.0 / (prec)bin_stp.x;
    inv_bin_stp.y = 1.0 / (prec)bin_stp.y;
    // inv_bin_stp.z = 1.0 / (prec  ) bin_stp.z;

    bin_org = place.org;

    bin_area = fp_product(bin_stp);

    inv_bin_area = 1.0 / bin_area;


    for(i = 0; i < tot_bin_cnt; i++) {
        bp = &bin_mat[i];
        bp->term_area *= area_scal;
//        bp->term_area2 *= area_scal;
        bp->virt_area *= area_scal;
//        bp->virt_area2 *= area_scal;
//        bp->pl_area *= area_scal;
        bp->pmin = fp_add(bin_org, fp_mul(bin_stp, p2fp(bp->p)));
        bp->pmax = fp_add(bp->pmin, bin_stp);
        bp->center = fp_add(bp->pmin, fp_scal(0.5, bin_stp));
        bp->flg = 0;
    }

    wcof_init(bin_stp);
    net_update_init();
    fft_wxy_update_3d(bin_stp);
}

void shrink_gp_to_one(void) {
    int i = 0, j = 0;
    struct BIN *bp = NULL;
    struct MODULE *modu = NULL;
    struct CELLx *cell = NULL;
    struct TERM *term = NULL;
    struct PLACE *pl = NULL;

    struct FPOS dim_scal = zeroFPoint;

    prec area_scal = 0;

    dim_scal.x = 1.0 / (place.cnt.x * GP_SCAL);
    dim_scal.y = 1.0 / (place.cnt.y * GP_SCAL);
    // dim_scal.z = 1.0 / (place.cnt.z * GP_SCAL);

    area_scal = fp_product(dim_scal);

    total_modu_area *= area_scal;
    total_std_area *= area_scal;
    total_macro_area *= area_scal;
    total_cell_area *= area_scal;
    total_term_area *= area_scal;
    total_move_available_area *= area_scal;
    total_filler_area *= area_scal;
    total_PL_area *= area_scal;
    total_termPL_area *= area_scal;
    total_WS_area *= area_scal;
    filler_area *= area_scal;

    filler_size.x *= dim_scal.x;
    filler_size.y *= dim_scal.y;
    // filler_size.z *= dim_scal.z;

    place_backup = place;

    for(i = 0; i < moduleCNT; i++) {
        modu = &moduleInstance[i];
        modu->center.x = (modu->center.x - place.org.x) * dim_scal.x;
        modu->center.y = (modu->center.y - place.org.y) * dim_scal.y;
        // modu->center.z = (modu->center.z - place.org.z) * dim_scal.z;
        modu->size.x *= dim_scal.x;
        modu->size.y *= dim_scal.y;
        // modu->size.z *= dim_scal.z;
        modu->half_size.x = 0.5 * modu->size.x;
        modu->half_size.y = 0.5 * modu->size.y;
        // modu->half_size.z = 0.5 * modu->size.z;
        modu->pmin.x = modu->center.x - modu->half_size.x;
        modu->pmin.y = modu->center.y - modu->half_size.y;
        // modu->pmin.z = modu->center.z - modu->half_size.z;
        modu->pmax.x = modu->center.x + modu->half_size.x;
        modu->pmax.y = modu->center.y + modu->half_size.y;
        // modu->pmax.z = modu->center.z + modu->half_size.z;
        modu->area = modu->size.x * modu->size.y;  // * modu->size.z;

        for(j = 0; j < modu->pinCNTinObject; j++) {
            modu->pof[j].x *= dim_scal.x;
            modu->pof[j].y *= dim_scal.y;
            // modu->pof[j].z *= dim_scal.z;

            modu->pin[j]->fp.x = modu->center.x + modu->pof[j].x;
            modu->pin[j]->fp.y = modu->center.y + modu->pof[j].y;
            // modu->pin[j]->fp.z = modu->center.z + modu->pof[j].z;
        }
    }

    for(i = 0; i < gcell_cnt; i++) {
        cell = &gcell_st[i];
        cell->center.x =
            (cell->center.x - place.org.x) / (place.cnt.x * GP_SCAL);
        cell->center.y =
            (cell->center.y - place.org.y) / (place.cnt.y * GP_SCAL);
        // cell->center.z = (cell->center.z - place.org.z) / (place.cnt.z *
        // GP_SCAL);
        cell->size.x /= place.cnt.x * GP_SCAL;
        cell->size.y /= place.cnt.y * GP_SCAL;
        // cell->size.z /= place.cnt.z * GP_SCAL;
        cell->half_size.x = 0.5 * cell->size.x;
        cell->half_size.y = 0.5 * cell->size.y;
        // cell->half_size.z = 0.5 * cell->size.z;
        cell->pmin.x = cell->center.x - cell->half_size.x;
        cell->pmin.y = cell->center.y - cell->half_size.y;
        // cell->pmin.z = cell->center.z - cell->half_size.z;
        cell->pmax.x = cell->center.x + cell->half_size.x;
        cell->pmax.y = cell->center.y + cell->half_size.y;
        // cell->pmax.z = cell->center.z + cell->half_size.z;
        cell->half_den_size.x *= dim_scal.x;
        cell->half_den_size.y *= dim_scal.y;
        // cell->half_den_size.z *= dim_scal.z;
        cell->area = cell->size.x * cell->size.y;  // * cell->size.z;

        // total_cell_area += cell->area;

        for(j = 0; j < cell->pinCNTinObject; j++) {
            cell->pof[j].x *= dim_scal.x;
            cell->pof[j].y *= dim_scal.y;
            // cell->pof[j].z *= dim_scal.z;

            cell->pin[j]->fp.x = cell->center.x + cell->pof[j].x;
            cell->pin[j]->fp.y = cell->center.y + cell->pof[j].y;
            // cell->pin[j]->fp.z = cell->center.z + cell->pof[j].z;
        }
    }

    for(i = 0; i < terminalCNT; i++) {
        term = &terminalInstance[i];
        term->center.x =
            (term->center.x - place.org.x) / (place.cnt.x * GP_SCAL);
        term->center.y =
            (term->center.y - place.org.y) / (place.cnt.y * GP_SCAL);
        // term->center.z = (term->center.z - place.org.z) / (place.cnt.z *
        // GP_SCAL);
        term->size.x /= place.cnt.x * GP_SCAL;
        term->size.y /= place.cnt.y * GP_SCAL;
        // term->size.z /= place.cnt.z * GP_SCAL;
        term->pmin.x = term->center.x - 0.5 * term->size.x;
        term->pmin.y = term->center.y - 0.5 * term->size.y;
        // term->pmin.z = term->center.z - 0.5 * term->size.z;
        term->pmax.x = term->center.x + 0.5 * term->size.x;
        term->pmax.y = term->center.y + 0.5 * term->size.y;
        // term->pmax.z = term->center.z + 0.5 * term->size.z;
        term->area = term->size.x * term->size.y;  // * term->size.z;
        term->PL_area *= area_scal;

        for(j = 0; j < term->pinCNTinObject; j++) {
            term->pof[j].x *= dim_scal.x;
            term->pof[j].y *= dim_scal.y;
            // term->pof[j].z *= dim_scal.z;

            term->pin[j]->fp.x = term->center.x + term->pof[j].x;
            term->pin[j]->fp.y = term->center.y + term->pof[j].y;
            // term->pin[j]->fp.z = term->center.z + term->pof[j].z;
        }
    }

    for(i = 0; i < place_st_cnt; i++) {
        place_backup_st[i] = place_st[i];

        pl = &place_st[i];

        pl->center.x = (pl->center.x - place.org.x) *
                       dim_scal.x;  // / (pl->cnt.x * GP_SCAL);
        pl->center.y = (pl->center.y - place.org.y) *
                       dim_scal.y;  // / (pl->cnt.y * GP_SCAL);
        // pl->center.z = (pl->center.z - place.org.z) * dim_scal.z; // /
        // (pl->cnt.z * GP_SCAL);

        pl->stp.x *= dim_scal.x;  // 1.0 / (pl->cnt.x * GP_SCAL);
        pl->stp.y *= dim_scal.y;  // 1.0 / (pl->cnt.y * GP_SCAL);
        // pl->stp.z *= dim_scal.z; // 1.0 / (pl->cnt.z * GP_SCAL);

        pl->cnt.x *= dim_scal.x;
        pl->cnt.y *= dim_scal.y;
        // pl->cnt.z *= dim_scal.z;

        pl->org.x = (pl->org.x - place.org.x) * dim_scal.x;
        pl->org.y = (pl->org.y - place.org.y) * dim_scal.y;
        // pl->org.z = (pl->org.z - place.org.z) * dim_scal.z;

        pl->end.x = pl->org.x + pl->cnt.x;
        pl->end.y = pl->org.y + pl->cnt.y;
        // pl->end.z = pl->org.z + pl->cnt.z;

        pl->area = pl->cnt.x * pl->cnt.y;  // * pl->cnt.z;
    }

    place.center.x = 0.5 / GP_SCAL;
    place.center.y = 0.5 / GP_SCAL;
    // place.center.z = 0.5 / GP_SCAL;

    place.stp.x = 1.0 / (place_backup.cnt.x * GP_SCAL);
    place.stp.y = 1.0 / (place_backup.cnt.y * GP_SCAL);
    // place.stp.z = 1.0 / (place_backup.cnt.z * GP_SCAL);

    place.cnt.x = 1.0 / GP_SCAL;
    place.cnt.y = 1.0 / GP_SCAL;
    // place.cnt.z = 1.0 / GP_SCAL;
    place.org = zeroFPoint;
    place.end.x = place.org.x + place.cnt.x;
    place.end.y = place.org.y + place.cnt.y;
    // place.end.z = place.org.z + place.cnt.z;
    place.area = place.cnt.x * place.cnt.y;  // * place.cnt.z;

    bin_stp.x = place.cnt.x / (prec)max_bin.x;
    bin_stp.y = place.cnt.y / (prec)max_bin.y;
    // bin_stp.z = place.cnt.z / (prec  ) max_bin.z;

    half_bin_stp.x = bin_stp.x * 0.5;
    half_bin_stp.y = bin_stp.y * 0.5;
    // half_bin_stp.z = bin_stp.z * 0.5;

    inv_bin_stp.x = 1.0 / (prec)bin_stp.x;
    inv_bin_stp.y = 1.0 / (prec)bin_stp.y;
    // inv_bin_stp.z = 1.0 / (prec  ) bin_stp.z;

    bin_org = place.org;

    bin_area = fp_product(bin_stp);

    inv_bin_area = 1.0 / bin_area;


    for(i = 0; i < tot_bin_cnt; i++) {
        bp = &bin_mat[i];
        bp->term_area *= area_scal;
//        bp->term_area2 *= area_scal;
        bp->virt_area *= area_scal;
//        bp->virt_area2 *= area_scal;
//        bp->pl_area *= area_scal;
        bp->pmin = fp_add(bin_org, fp_mul(bin_stp, p2fp(bp->p)));
        bp->pmax = fp_add(bp->pmin, bin_stp);
        bp->center = fp_add(bp->pmin, fp_scal(0.5, bin_stp));
        bp->flg = 0;
    }

    wcof_init(bin_stp);

    net_update_init();

    fft_wxy_update_3d(bin_stp);
}

void cg_input(struct FPOS *x_st, int N, int input) {
    char fn_x_isol[BUF_SZ];
    char fn_wl_sol[BUF_SZ];

    struct FPOS half_den_size = zeroFPoint;
    struct FPOS center = zeroFPoint;
    struct FPOS rnd = zeroFPoint;
    struct FPOS v = zeroFPoint;
    struct FPOS sqr_org = zeroFPoint;
    struct FPOS sqr_cnt = zeroFPoint;
    struct FPOS sqr_end = zeroFPoint;

    switch(input) {
        case QWL_ISOL:
            for(int i = 0; i < N; i++) {
                center = gcell_st[i].center;
                // printf("%.16f %.16f\n",center.x, center.y);
                half_den_size = gcell_st[i].half_den_size;
                // printf("%.16f %.16f\n",half_den_size.x, half_den_size.y);
                x_st[i] = valid_coor00(center, half_den_size);
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
                v = fp_add(fp_mul(fp_scal(inv_RAND_MAX, rnd), sqr_cnt),
                           sqr_org);
                x_st[i] = valid_coor00(v, half_den_size);
            }
            break;

        case IP_CEN_SQR:
            sqr_cnt = fp_scal(0.4, place.cnt);
            sqr_org = fp_add(place.org, fp_scal(0.3, place.cnt));
            sqr_end = fp_add(sqr_org, sqr_cnt);
            for(int i = 0; i < N; i++) {
                half_den_size = gcell_st[i].half_den_size;
                v = fp_add(fp_mul(fp_scal(inv_RAND_MAX, rnd), sqr_cnt),
                           sqr_org);
                x_st[i] = valid_coor00(v, half_den_size);
            }
            break;
    }
}

int area_sort(const void *a, const void *b) {
    prec *aa = (prec *)a;
    prec *bb = (prec *)b;
    return *aa > *bb ? 1 : 0;
}

int abc_sort(const void *a, const void *b) {
    struct ABC *aa = (struct ABC *)a;
    struct ABC *bb = (struct ABC *)b;

    return aa->val < bb->val ? 1 : 0;
}

void init_iter(struct ITER *it, int idx) {
    it->idx = idx;
    it->wlen = zeroFPoint;
    it->tot_wlen = 0;
    it->grad = 0;
    it->hpwl = zeroFPoint;
    it->tot_hpwl = 0;
    it->ovfl = 0;
    it->wcof = zeroFPoint;
    it->pcof = 0;
    it->beta = 0;
    it->alpha00 = 0;
    it->alpha_dim = zeroFPoint;
    it->lc_dim = zeroFPoint;
    it->lc_w = 0;
    it->lc_p = 0;
    it->cpu_curr = 0;
    it->cpu_cost = 0;
}

void gp_opt(void) {
    myNesterov ns_opt;

    if(GP_DIM_ONE) {
        shrink_gp_to_one();
    }

#ifdef FILLER_ADJ
    filler_adj();
#endif

    printf("PROC:  Start NESTEROV's Optimization\n");
    if(constraintDrivenCMD == false) {
        printf("PROC:    Global Lagrangian Multiplier is Applied\n");
        ns_opt.nesterov_opt();
    }
    else if(constraintDrivenCMD == true) {
        printf(
            "PROC:    Both Global and Local Lagrangian Multipliers are "
            "Applied\n");
        ns_opt.nesterov_opt();
    }

    modu_copy();

    if(GP_DIM_ONE)
        expand_gp_from_one();

    if(STAGE == mGP2D)
        output_mGP2D_pl(gmGP2D_pl);
    else if(STAGE == cGP2D)
        output_cGP2D_pl(gGP3_pl);

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
                 << inflation_area_over_whitespace *
                        (total_WS_area - total_cell_area)
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
    if(target_cell_den > 0.99) {
        prec area_to_shrink = curr_cell_area - 0.99 * total_WS_area;
        target_cell_den = 0.99;
        // mod LW 10/29/16
        global_macro_area_scale = target_cell_den;
        shrink_filler_cells(area_to_shrink);
        curr_cell_area = 0.99 * total_WS_area;
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
    struct CELLx *cell = NULL;

    // LW mod 11/17/16
    // inflation_area_over_whitespace = 0.10;
    inflation_area_over_whitespace = 1.0 / inflation_max_cnt;
    for(int i = 0; i < gcell_cnt; i++) {
        cell = &gcell_st[i];
        backup_org_CELLx_info(cell);
    }
    isRoutabilityInit = true;
}

void restore_cells_info() {
    struct CELLx *cell = NULL;

    for(int i = 0; i < gcell_cnt; i++) {
        cell = &gcell_st[i];
        restore_org_CELLx_info(cell);
    }
}

void cell_macro_copy(void) {
    int i = 0;
    struct MODULE *mdp = NULL;
    struct CELLx *cell = NULL;

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

void cell_init_2D(void) {
    int z = 0, i = 0;
    struct CELLx *cell = NULL;
    struct TIER *tier = NULL;
    struct FPOS scal;

    // if ((INPUT_FLG == MMS && (target_cell_den == 1.00f ||
    //                          target_cell_den == 0.80f ||
    //                          target_cell_den == 1.00 ||
    //                          target_cell_den == 0.80)) ||
    //    (routabilityCMD == true && tier_st[0].modu_den < 0.32)) {
    if((INPUT_FLG == MMS &&
        (target_cell_den == 1.00f || target_cell_den == 0.80f ||
         target_cell_den == 1.00 || target_cell_den == 0.80))) {
        for(z = 0; z < numLayer; z++) {
            tier = &tier_st[z];

            for(i = 0; i < tier->cell_cnt; i++) {
                cell = tier->cell_st[i];

                // LW 05/05/17
                if(cell->size.x < tier->bin_stp.x) {
                    scal.x = cell->size.x / tier->bin_stp.x;
                    cell->half_den_size.x = tier->half_bin_stp.x;
                }
                else {
                    scal.x = 1.0;
                    cell->half_den_size.x = cell->half_size.x;
                }

                if(cell->size.y < tier->bin_stp.y) {
                    scal.y = cell->size.y / tier->bin_stp.y;
                    cell->half_den_size.y = tier->half_bin_stp.y;
                }
                else {
                    scal.y = 1.0;
                    cell->half_den_size.y = cell->half_size.y;
                }

                if(cell->size.z < tier->bin_stp.z) {
                    scal.z = cell->size.z / tier->bin_stp.z;
                    cell->half_den_size.z = tier->half_bin_stp.z;
                }
                else {
                    scal.z = 1.0;
                    cell->half_den_size.z = cell->half_size.z;
                }

                cell->den_scal = scal.x * scal.y * scal.z;
            }
        }
    }
    else {
        for(z = 0; z < numLayer; z++) {
            tier = &tier_st[z];

            for(i = 0; i < tier->cell_cnt; i++) {
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

                if(cell->size.z < tier->bin_stp.z) {
                    scal.z = cell->size.z / tier->bin_stp.z;
                    cell->half_den_size.z = tier->half_bin_stp.z;
                }
                else {
                    scal.z = 1.0;
                    cell->half_den_size.z = cell->half_size.z;
                }

                cell->den_scal = scal.x * scal.y * scal.z;
            }
        }
    }
}

void update_cell_den() {
    struct CELLx *cell = NULL;
    struct FPOS scal = zeroFPoint;

    for(int i = 0; i < gcell_cnt; i++) {
        cell = &gcell_st[i];
        if(cell->size.x < bin_stp.x) {
            scal.x = cell->size.x / bin_stp.x;
            cell->half_den_size.x = half_bin_stp.x;
        }
        else {
            scal.x = 1.0;
            cell->half_den_size.x = cell->half_size.x;
        }

        if(cell->size.y < bin_stp.y) {
            scal.y = cell->size.y / bin_stp.y;
            cell->half_den_size.y = half_bin_stp.y;
        }
        else {
            scal.y = 1.0;
            cell->half_den_size.y = cell->half_size.y;
        }

        if(cell->size.z < bin_stp.z) {
            scal.z = cell->size.z / bin_stp.z;
            cell->half_den_size.z = half_bin_stp.z;
        }
        else {
            scal.z = 1.0;
            cell->half_den_size.z = cell->half_size.z;
        }
        cell->den_scal = scal.x * scal.y * scal.z;
    }
}

void calc_average_module_width() {
    prec tot_mod_wid = 0;
    prec avg_mod_wid = 0;

    for(int i = 0; i < moduleCNT; i++) {
        tot_mod_wid += moduleInstance[i].size.x;
    }
    avg_mod_wid = tot_mod_wid / (prec)moduleCNT;

    printf("INFO:  Average Module Width = %.6lf, Row Height = %f\n",
           avg_mod_wid, rowHeight);
}

//
// update msh, msh_yz, d_msh
void msh_init() {
    msh = dim_bin;
    msh_yz = msh.y * msh.z;
    int d_msh = msh.x * msh.y * msh.z;

    printf("INFO:  D_MSH = %d \n", d_msh);
    printf("INFO:  MSH(X, Y, Z) = (%d, %d, %d)\n", msh.x, msh.y, msh.z);
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
        UPPER_PCOF =
            getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[0].first,
                               hpwlEPs_2ndOrder[0].first, 1.0001, 0.0000);
    }
    else if(potnPhaseDS == potnPhase2) {
        UPPER_PCOF =
            getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[0].first,
                               hpwlEPs_2ndOrder[1].first, 1.001, 0.000);
    }
    else if(potnPhaseDS == potnPhase3) {
        UPPER_PCOF =
            getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[1].first,
                               hpwlEPs_2ndOrder[1].first, 1.001, 0.024);
    }
    else if(potnPhaseDS == potnPhase4) {
        UPPER_PCOF =
            getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[1].first,
                               hpwlEPs_2ndOrder[2].first, 1.005, 0.020);
    }
    else if(potnPhaseDS == potnPhase5) {
        UPPER_PCOF =
            getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[2].first,
                               hpwlEPs_2ndOrder[2].first, 1.005, 0.045);
    }
    else if(potnPhaseDS == potnPhase6) {
        UPPER_PCOF =
            getStepSizefromEPs(curr_hpwl, hpwlEPs_1stOrder[2].first,
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

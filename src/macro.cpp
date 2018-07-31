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

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "bookShelfIO.h"
#include "bin.h"
#include "global.h"
#include "macro.h"
#include "mkl.h"
#include "opt.h"
#include "plot.h"
#include "wlen.h"

int macro_cnt;
int *ovlp_y;
int org_mac_ovlp;
int tot_mac_ovlp;
int new_mac_ovlp;
int ovlp_free_flg;
int sa_max_iter;
int sa_max_iter2;
int sa_max_iter0;
int sa_iter_cof;
int tot_mac_area, tot_std_area;
int real_mac_area;
int tot_accept_cnt;
int tot_reject_cnt;
int cur_acc_cnt;
int cur_rej_cnt;
int sa_cnt;
int sa_x_gtz_cnt;
int sa_x_eqz_cnt;
int sa_x_ltz_cnt;
int sa_y_gtz_cnt;
int sa_y_eqz_cnt;
int sa_y_ltz_cnt;
int sa_z_gtz_cnt;
int sa_z_eqz_cnt;
int sa_z_ltz_cnt;
int sa_n_disp;
int ovlp_mac_cnt;
prec sa_hpwl_wgt;
prec sa_hpwl_cof;
prec sa_den_wgt;
prec sa_den_cof;
prec sa_ovlp_wgt;
prec sa_ovlp_cof;
prec org_mac_hpwl;
prec org_mac_den;
prec tot_mac_hpwl;
prec tot_mac_den;
prec new_mac_hpwl;
prec new_mac_den;
prec mac_hpwl_xyz;
prec sa_init_neg_rate;
prec sa_last_neg_rate;
prec sa_t;
prec sa_init_t;
prec sa_t_cof;
prec MIN_SA_R_Z;
prec MAX_SA_R_Z;

MODULE **macro_st;
FPOS sa_r;
FPOS max_sa_r;
FPOS min_sa_r;
FPOS sa_r_stp;
FPOS sa_ncof;
FPOS sa_n;
FPOS mac_hpwl;
FPOS sum_mac_size, avg_mac_size;
FPOS avg_mac_cnt_per_dim;
POS sa_u;
NODE *ovlp_node;
seg_tree_node *ovlp_a;

void sa_macro_lg(void) {
    sa_init_top();

    if(macro_cnt == 0)
        return;
    if(!sa_mac_leg_top())
        post_mac_leg();

    sa_post();
    sa_delete();
    fflush(stdout);
}

int sa_mac_leg_top(void) {
    int iter = 0;
    struct MODULE *mac = NULL;

    for(iter = 0; iter < sa_max_iter0 && ovlp_free_flg == 0; iter++) {
        printf(
            "\
ITER mLG: %d\n\
    HPWL=%f\n\
    DEN =%.4f\n\
    OVLP=%d\n\
    #ACC=%d\n\
    #REJ=%d\n",
            iter + 1, total_hpwl.x + total_hpwl.y + total_hpwl.z, tot_mac_den,
            tot_mac_ovlp, tot_accept_cnt, tot_reject_cnt);

        sa_param_init(iter);
        sa_mac_leg(iter);
        sa_param_update();

        tot_mac_ovlp = get_mac_ovlp_3d_top();
        fflush(stdout);
    }

    cout << "INFO:  FINAL MACRO LEGALIZATION RESULT" << endl;
    printf(
        "\
ITER mLG: %d\n\
    HPWL=%f\n\
    DEN =%.4f\n\
    OVLP=%d\n\
    #ACC=%d\n\
    #REJ=%d\n",
        iter + 1, total_hpwl.x + total_hpwl.y + total_hpwl.z, tot_mac_den,
        tot_mac_ovlp, tot_accept_cnt, tot_reject_cnt);

    if(plotMacroCMD) {
        for(int j = 0; j < macro_cnt; j++) {
            mac = macro_st[j];
            mac->pmin.x = mac->center.x - 0.5 * mac->size.x;
            mac->pmin.y = mac->center.y - 0.5 * mac->size.y;
            mac->pmin.z = mac->center.z - 0.5 * mac->size.z;
            mac->pmax.x = mac->center.x + 0.5 * mac->size.x;
            mac->pmax.y = mac->center.y + 0.5 * mac->size.y;
            mac->pmax.z = mac->center.z + 0.5 * mac->size.z;
        }
        cell_copy();
        plot("S3-LG3-macro", 9999, 1.0, 1);
    }
    return ovlp_free_flg;
}

void sa_mac_leg(int iter) {
    int i = 0, j = 0;
    double t0 = 0;
    struct MODULE *mac = NULL;

    printf(
        "  -- ITER, TEMP, Rx, Ry, Rz, HPWL-(X, Y, Z) , DEN, OVLP, # ACC, # "
        "REJ\n");
    printf(
        "  -- %d, %.2e, %.2e, %.2e, %.2e, (%.8e, %.8e, %.8e), %.2e, "
        "\033[36m%d\033[0m, %d, %d \n",
        0, sa_t, sa_r.x, sa_r.y, sa_r.z,
        // tot_mac_hpwl ,
        total_hpwl.x, total_hpwl.y, total_hpwl.z, tot_mac_den, tot_mac_ovlp,
        tot_accept_cnt, tot_reject_cnt);

    if(plotMacroCMD) {
        for(j = 0; j < macro_cnt; j++) {
            mac = macro_st[j];
            mac->pmin.x = mac->center.x - 0.5 * mac->size.x;
            mac->pmin.y = mac->center.y - 0.5 * mac->size.y;
            mac->pmin.z = mac->center.z - 0.5 * mac->size.z;
            mac->pmax.x = mac->center.x + 0.5 * mac->size.x;
            mac->pmax.y = mac->center.y + 0.5 * mac->size.y;
            mac->pmax.z = mac->center.z + 0.5 * mac->size.z;
        }

        cell_copy();

        plot("S3-LG2-macro", 0, 1.0, 1);
    }
    cur_acc_cnt = 0;
    cur_rej_cnt = 0;

    time_start(&t0);

    for(i = 0; i < sa_max_iter && ovlp_free_flg == 0; i++) {
        for(j = 0; j < sa_max_iter2 && ovlp_free_flg == 0; j++) {
            sa_mac_leg_sub();
        }

        sa_param_update_sub();

        if(i % (sa_max_iter / sa_n_disp) == (sa_max_iter / sa_n_disp - 1)) {
            time_end(&t0);

            //////////////////////////////////
            // net_update_hpwl_mac ();
            //////////////////////////////////

            printf(
                "  -- %d, %.2e, %.2e, %.2e, %.2e, (%.8e, %.8e, %.8e), %.2e, "
                "\033[36m%d\033[0m, %d, %d, %.2lf\n",
                i / (sa_max_iter / sa_n_disp) + 1, sa_t, sa_r.x, sa_r.y, sa_r.z,
                // tot_mac_hpwl ,
                total_hpwl.x, total_hpwl.y, total_hpwl.z, tot_mac_den,
                tot_mac_ovlp, cur_acc_cnt, cur_rej_cnt, t0);
            fflush(stdout);

            time_start(&t0);

            if(plotMacroCMD) {
                for(j = 0; j < macro_cnt; j++) {
                    mac = macro_st[j];
                    mac->pmin.x = mac->center.x - 0.5 * mac->size.x;
                    mac->pmin.y = mac->center.y - 0.5 * mac->size.y;
                    mac->pmin.z = mac->center.z - 0.5 * mac->size.z;
                    mac->pmax.x = mac->center.x + 0.5 * mac->size.x;
                    mac->pmax.y = mac->center.y + 0.5 * mac->size.y;
                    mac->pmax.z = mac->center.z + 0.5 * mac->size.z;
                }

                cell_copy();

                mac_ovlp_mark();

                plot("S3-LG2-macro",
                     iter * sa_n_disp + i / (sa_max_iter / sa_n_disp) + 1, 1.0,
                     1);
            }
            cur_acc_cnt = 0;
            cur_rej_cnt = 0;
        }
    }
}

void sa_mac_leg_sub() {
    int flg = 0;
    int mac_idx = 0;
    prec mac_c0 = 0, mac_c1 = 0;
    struct POS mac_mov = zeroPoint;

    sa_cnt++;

    mac_idx = get_mac_idx();
    mac_mov = get_mac_mov(sa_r, sa_u);

    mac_c0 = get_mac_cost(mac_idx, &org_mac_hpwl, &org_mac_den, &org_mac_ovlp);

    struct FPOS mac_hpwl_00 = mac_hpwl;

    do_mac_mov(mac_idx, &mac_mov);

    mac_c1 = get_mac_cost(mac_idx, &new_mac_hpwl, &new_mac_den, &new_mac_ovlp);

    flg = mov_accept(mac_c0, mac_c1);

    if(flg) {
        tot_accept_cnt++;
        cur_acc_cnt++;
        tot_mac_hpwl += new_mac_hpwl - org_mac_hpwl;
        tot_mac_den += new_mac_den - org_mac_den;
        tot_mac_ovlp += new_mac_ovlp - org_mac_ovlp;

        ///////////////////////
        total_hpwl.x += mac_hpwl.x - mac_hpwl_00.x;
        total_hpwl.y += mac_hpwl.y - mac_hpwl_00.y;
        total_hpwl.z += mac_hpwl.z - mac_hpwl_00.z;
        ///////////////////////

        if(tot_mac_ovlp <= 0 && new_mac_ovlp <= 0 && org_mac_ovlp > 0) {
            tot_mac_ovlp = get_mac_ovlp_3d_top();
            if(tot_mac_ovlp <= 0)
                ovlp_free_flg = 1;
        }
    }
    else {
        tot_reject_cnt++;
        cur_rej_cnt++;
        mac_mov.x *= -1;
        mac_mov.y *= -1;
        mac_mov.z *= -1;
        do_mac_mov(mac_idx, &mac_mov);
    }

#ifdef MACRO_OVLP_DEBUG
    int ovlp1 = tot_mac_ovlp;  // get_mac_ovlp_top();
    int ovlp2 = get_tot_mac_ovlp();
    cnt2 = ovlp_mac_cnt;
    if(ovlp1 != ovlp2 || ovlp1 != tot_mac_ovlp || ovlp2 != tot_mac_ovlp) {
        g_rrr++;
        printf(" == ERROR tot_mac_ovlp: %d != %d != %d\n", tot_mac_ovlp, ovlp1,
               ovlp2);
        for(i = 0; i < macro_cnt; i++) {
            mac = macro_st[i];
            printf(
                "%d. pminlg=(%d,%d,%d), pmaxlg=(%d,%d,%d), size=(%d,%d,%d)\n",
                i + 1, mac->pmin_lg.x, mac->pmin_lg.y, mac->pmin_lg.z,
                mac->pmax_lg.x, mac->pmax_lg.y, mac->pmax_lg.z,
                prec2int(mac->size.x), prec2int(mac->size.y),
                prec2int(mac->size.z));
            if(mac->pmin_lg.x + prec2int(mac->size.x) != mac->pmax_lg.x) {
                printf("ERROR: x\n");
            }
            if(mac->pmin_lg.y + prec2int(mac->size.y) != mac->pmax_lg.y) {
                printf("ERROR: y\n");
            }
            if(mac->pmin_lg.z + prec2int(mac->size.z) != mac->pmax_lg.z) {
                printf("ERROR: z\n");
            }
        }
        if(plotMacroCMD) {
            for(i = 0; i < macro_cnt; i++) {
                mac = macro_st[i];
                mac->pmin.x = (prec)mac->pmin_lg.x;
                mac->pmin.y = (prec)mac->pmin_lg.y;
                mac->pmin.z = (prec)mac->pmin_lg.z;
                mac->pmax.x = (prec)mac->pmax_lg.x;
                mac->pmax.y = (prec)mac->pmax_lg.y;
                mac->pmax.z = (prec)mac->pmax_lg.z;
            }

            cell_copy();

            plot("S3-macro-sa", sa_cnt, 1.0, 1);
        }
    }

#endif
}

void sa_init_top(void) {
    int i = 0;
    int n = 0;
    struct MODULE *mdp = NULL, *mac = NULL;

    macro_st = (struct MODULE **)mkl_malloc(
        sizeof(struct MODULE *) * gmov_mac_cnt, 64);

    macro_cnt = 0;

    tot_std_area = 0;
    tot_mac_area = 0;

    sum_mac_size = zeroFPoint;

    for(i = 0; i < moduleCNT; i++) {
        mdp = &moduleInstance[i];

        if(mdp->flg == StdCell) {
            tot_std_area += prec2int(mdp->area);
            continue;
        }
        else {
            tot_mac_area += prec2int(mdp->area);
            mac = macro_st[macro_cnt] = mdp;
            mac->mac_idx = macro_cnt;
            macro_cnt++;

            sum_mac_size.x += mac->size.x;
            sum_mac_size.y += mac->size.y;
            sum_mac_size.z += mac->size.z;
        }
    }

    if(macro_cnt == 0)
        return;

#ifdef SORT_MAC_BY_SZ
    qsort(macro_st, macro_cnt, sizeof(struct MODULE *), macro_area_cmp);
    for(i = 0; i < macro_cnt; i++) {
        macro_st[i]->mac_idx = i;
    }
#endif

    avg_mac_size = fp_scal(1.0 / ((prec)macro_cnt), sum_mac_size);

    avg_mac_cnt_per_dim = fp_div(place.cnt, avg_mac_size);

    printf("std_area = %d, mac_area = %d\n", tot_std_area, tot_mac_area);

    n = macro_cnt;

    if(plotMacroCMD) {
        for(i = 0; i < macro_cnt; i++) {
            mac = macro_st[i];
            mac->pmin.x = mac->center.x - 0.5 * mac->size.x;
            mac->pmin.y = mac->center.y - 0.5 * mac->size.y;
            mac->pmin.z = mac->center.z - 0.5 * mac->size.z;
            mac->pmax.x = mac->center.x + 0.5 * mac->size.x;
            mac->pmax.y = mac->center.y + 0.5 * mac->size.y;
            mac->pmax.z = mac->center.z + 0.5 * mac->size.z;
        }

        cell_copy();

        plot("S3-LG1-macro-gp", 0, 1.0, 1);
    }

    sa_mac_leg_init();

    tot_mac_hpwl = net_update_hpwl_mac();

    den_update();
    tot_mac_den = get_all_macro_den();

    mac_ovlp_init(n);

    tot_mac_ovlp =
        get_mac_ovlp_3d_top();  // get_tot_mac_ovlp ();// get_mac_ovlp_top();

    printf(
        "initial cost: HPWL = (%.2lf, %.2lf, %.2lf), den = %.2lf , ovlp = %d "
        "\n",
        total_hpwl.x, total_hpwl.y, total_hpwl.z, tot_mac_den, tot_mac_ovlp);

    if(tot_mac_ovlp <= 0)
        ovlp_free_flg = 1;
    else
        ovlp_free_flg = 0;

    sa_param_init_top();

    MIN_SA_R_Z = 1.5 * TIER_DEP;
    MAX_SA_R_Z = (prec)numLayer * TIER_DEP;
}

void sa_mac_leg_init(void) {
    sa_mac_leg_init_with_margin();
}

void sa_mac_leg_init_with_margin(void) {
    int i = 0, j = 0, z = 0;
    struct MODULE *mac = NULL;
    struct TIER *tier = NULL;
    struct FPOS pmin = zeroFPoint, center = zeroFPoint, center_lg = zeroFPoint;
    struct POS pmin_lg = zeroPoint;
    int *z_st = (int *)mkl_malloc(sizeof(int) * numLayer, 64);
    struct T0 *t0_st =
        (struct T0 *)mkl_malloc(sizeof(struct T0) * numLayer, 64);

    for(i = 0; i < macro_cnt; i++) {
        mac = macro_st[i];
        pmin = mac->pmin;
        center = mac->center;

        pmin_lg.x = (int)(pmin.x + 0.5);
        pmin_lg.y =
            ((int)((pmin.y + 0.5 * rowHeight - place.org.y /* 1.0 * ROW_Y0 */) /
                   (prec)rowHeight)) *
                rowHeight +
            (int)(place.org.y + 0.5) /* ROW_Y0 */;

        find_close_tier(center.z, t0_st, z_st);

        for(j = 0; j < numLayer; j++) {
            z = z_st[j];

            tier = &tier_st[z];

            if((tier->term_area + tier->virt_area + tier->temp_mac_area +
                mac->area) /
                       tier->area <=
                   1.0 - MAC_MARGIN_3DIC &&
               !(tier->max_mac &&
                 tier->max_mac->size.x + mac->size.x > tier->size.x &&
                 tier->max_mac->size.y + mac->size.y > tier->size.y)) {
                center_lg.z = tier->center.z;

                tier->temp_mac_area += mac->area;

                if(!tier->max_mac)
                    tier->max_mac = mac;
                else if(tier->max_mac->area < mac->area)
                    tier->max_mac = mac;

                mac->tier = z;

                break;
            }
            else {
                if(j == numLayer - 1) {
                    printf("ERROR: no more tier to assign macro %s!\n",
                           mac->name);
                    g_rrr++;
                    exit(1);
                }
                else
                    continue;
            }
        }

        center_lg.x = (prec)pmin_lg.x + 0.5 * mac->size.x;
        center_lg.y = (prec)pmin_lg.y + 0.5 * mac->size.y;

        center_lg = valid_coor4(center_lg, mac->size);

        mac->center = center_lg;

        mac->pmin.x = mac->center.x - 0.5 * mac->size.x;
        mac->pmin.y = mac->center.y - 0.5 * mac->size.y;
        mac->pmin.z = mac->center.z - 0.5 * mac->size.z;

        mac->pmax.x = mac->center.x + 0.5 * mac->size.x;
        mac->pmax.y = mac->center.y + 0.5 * mac->size.y;
        mac->pmax.z = mac->center.z + 0.5 * mac->size.z;

        mac->pmin_lg.x = (int)(mac->pmin.x + 0.5);
        mac->pmin_lg.y = (int)(mac->pmin.y + 0.5);
        mac->pmin_lg.z = (int)(mac->pmin.z + 0.5);

        mac->pmax_lg.x = (int)(mac->pmax.x + 0.5);
        mac->pmax_lg.y = (int)(mac->pmax.y + 0.5);
        mac->pmax_lg.z = (int)(mac->pmax.z + 0.5);
    }

    mkl_free(t0_st);
    mkl_free(z_st);
}

void sa_param_init_top(void) {
    sa_hpwl_wgt = 1.0;
    sa_den_wgt = tot_mac_hpwl / tot_mac_den;
    sa_ovlp_wgt = (tot_mac_hpwl * sa_hpwl_wgt + tot_mac_den * sa_den_wgt) /
                  (prec)tot_mac_ovlp;
    sa_hpwl_cof = 1.0;
    sa_den_cof = 1.0;
    sa_ovlp_cof = 1.5;    /// need tuning
    sa_max_iter0 = 1000;  /// need tuning
}

void sa_param_init(int iter) {
    prec sa_coef = 0.03;
    sa_iter_cof = 1;

    ////////////PROBLEM//////////////

    sa_max_iter = 1000;
    sa_max_iter2 = macro_cnt * sa_iter_cof /* 10 */;

    ///////////-PROBLEM-/////////////

    sa_n_disp = 10;

    sa_init_neg_rate =
        sa_coef * pow(1.5, (prec)iter);
    // 3% cost increase will be
    // accepted in 50% probability

    sa_last_neg_rate = 0.0001 * pow(1.5, (prec)iter);
    // 0.01% cost increase will be
    // accepted in 50% probability

    sa_init_t = sa_init_neg_rate / log(2.0);  // based on the equation that
                                              // exp(-1.0*sa_init_neg_rate/
                                              // sa_init_t) = 0.5

    sa_t = sa_init_t;
    // 0.1 ; // 400 ; // from Howard's ICCCAS 13 paper

    sa_t_cof = pow((sa_last_neg_rate / sa_init_neg_rate),
                   1.0 / (prec)sa_max_iter);  // make sure sa_t(last) equals
                                              // its expected value

    prec mac_cnt_sum = fp_sum(avg_mac_cnt_per_dim);

    sa_n.x = sa_n.y = sqrt((prec)macro_cnt / (prec)numLayer);

    sa_n.z = (prec)macro_cnt * avg_mac_cnt_per_dim.z / mac_cnt_sum;

    sa_ncof.x = 0.05 * pow(1.5, (prec)iter);
    sa_ncof.y = 0.05 * pow(1.5, (prec)iter);
    sa_ncof.z = 0.05 * pow(1.5, (prec)iter);

    max_sa_r.x = place.cnt.x / sa_n.x * sa_ncof.x;
    max_sa_r.y = place.cnt.y / sa_n.y * sa_ncof.y;
    max_sa_r.z = place.cnt.z / sa_n.z * sa_ncof.z * MAX_SA_R_COF;

    if(max_sa_r.z < MIN_SA_R_Z)
        max_sa_r.z = MIN_SA_R_Z;
    if(max_sa_r.z > MAX_SA_R_Z)
        max_sa_r.z = MAX_SA_R_Z;

    min_sa_r.x = 1.0;
    min_sa_r.y = 1.0;  // rowHeight ;
    min_sa_r.z = 1.0;  // TIER_DEP ;

    sa_r.x = max_sa_r.x;
    sa_r.y = max_sa_r.y;
    sa_r.z = max_sa_r.z;

    sa_r_stp.x = (max_sa_r.x - min_sa_r.x) / (prec)sa_max_iter;
    sa_r_stp.y = (max_sa_r.y - min_sa_r.y) / (prec)sa_max_iter;
    sa_r_stp.z = (max_sa_r.z - min_sa_r.z) / (prec)sa_max_iter;

    sa_u.x = 1;
    sa_u.y = 1;
    sa_u.z = 1;
}

////////////// OVERLAP COUNT //////////////

int get_mac_ovlp(int idx) {
    int i = 0;
    int ovlp = 0, tot_ovlp = 0;
    struct MODULE *mac = macro_st[idx], *mac2 = NULL;
    ovlp_mac_cnt = 0;

    for(i = 0; i < macro_cnt; i++) {
        if(i == idx)
            continue;

        mac2 = macro_st[i];
        ovlp = get_ovlp_area(mac, mac2);
        if(ovlp > 0) {
            ovlp_mac_cnt++;
        }
        tot_ovlp += ovlp;
    }

    return tot_ovlp;
}

int get_ovlp_area(MODULE *mac1, MODULE *mac2) {
    return iGetCommonAreaXY (mac1->pmin_lg, mac1->pmax_lg, mac2->pmin_lg,
                            mac2->pmax_lg);
}

int get_all_macro_ovlp(void) {
    int i = 0, t = 0;
    int sum_ovlp = 0;
    int x1 = 0, x2 = 0;
    int y1 = 0, y2 = 0;
    struct MODULE *mac = NULL;
    int n = macro_cnt;

    t = 1;

    for(i = 0; i < n; i++) {
        mac = macro_st[i];

        x1 = mac->pmin_lg.x;
        y1 = mac->pmin_lg.y;

        x2 = mac->pmax_lg.x;
        y2 = mac->pmax_lg.y;

        ovlp_node[t].x = x1;
        ovlp_node[t].y1 = y1;
        ovlp_node[t].y2 = y2;
        ovlp_node[t].flg = 1;
        ovlp_y[t++] = y1;

        ovlp_node[t].x = x2;
        ovlp_node[t].y1 = y1;
        ovlp_node[t].y2 = y2;
        ovlp_node[t].flg = -1;
        ovlp_y[t++] = y2;
    }

    qsort(ovlp_node + 1, 2 * n, sizeof(struct NODE), ovlp_node_cmp);
    qsort(ovlp_y + 1, 2 * n, sizeof(int), int_cmp);

    build_seg_tree(1, 1, t - 1);

    updata(1, ovlp_node[1]);

    for(i = 2; i < t; i++) {
        sum_ovlp += ovlp_a[1].len * (ovlp_node[i].x - ovlp_node[i - 1].x);
        updata(1, ovlp_node[i]);
    }

    return sum_ovlp;
}

int ovlp_node_cmp(const void *a, const void *b) {
    struct NODE *aa = (struct NODE *)a;
    struct NODE *bb = (struct NODE *)b;

    return aa->x > bb->x ? 1 : 0;
}

int int_cmp(const void *a, const void *b) {
    int *aa = (int *)a;
    int *bb = (int *)b;

    return *aa > *bb ? 1 : 0;
}

void updata(int i, struct NODE b) {
    if(ovlp_a[i].ml == b.y1 && ovlp_a[i].mr == b.y2) {
        ovlp_a[i].s += b.flg;
        callen(i);
        return;
    }

    if(b.y2 <= ovlp_a[i * 2].mr)
        updata(i * 2, b);
    else if(b.y1 >= ovlp_a[i * 2 + 1].ml)
        updata(i * 2 + 1, b);
    else {
        struct NODE temp = b;
        temp.y2 = ovlp_a[i * 2].mr;
        updata(i * 2, temp);
        temp = b;
        temp.y1 = ovlp_a[i * 2 + 1].ml;
        updata(i * 2 + 1, temp);
    }
    callen(i);
    return;
}

void callen(int i) {
    if(ovlp_a[i].s > 0) {
        ovlp_a[i].len = ovlp_a[i].mr - ovlp_a[i].ml;
    }
    else if(ovlp_a[i].r - ovlp_a[i].l == 1) {
        ovlp_a[i].len = 0;
    }
    else {
        ovlp_a[i].len = ovlp_a[i * 2].len + ovlp_a[i * 2 + 1].len;
    }
}

void build_seg_tree(int i, int left, int right) {
    ovlp_a[i].l = left;
    ovlp_a[i].r = right;
    ovlp_a[i].ml = ovlp_y[left];
    ovlp_a[i].mr = ovlp_y[right];
    ovlp_a[i].s = 0;
    ovlp_a[i].len = 0;

    if(ovlp_a[i].l + 1 == ovlp_a[i].r) {
        return;
    }
    else {
        int moduleID = (left + right) >> 1;
        build_seg_tree(i * 2, left, moduleID);
        build_seg_tree(i * 2 + 1, moduleID, right);
    }
}

//////////////-OVERLAP COUNT-//////////////

struct POS get_mac_mov(struct FPOS r, struct POS u) {
    struct POS rnd = zeroPoint;
    struct FPOS drnd = zeroFPoint;
    struct POS mov = zeroPoint;
    rnd.x = rand();
    rnd.y = rand();
    rnd.z = rand();
    drnd.x = (prec)rnd.x * inv_RAND_MAX - 0.5;
    drnd.y = (prec)rnd.y * inv_RAND_MAX - 0.5;
    drnd.z = (prec)rnd.z * inv_RAND_MAX - 0.5;
    mov.x = prec2int(drnd.x * r.x) * u.x;
    mov.y = prec2int(drnd.y * r.y / rowHeight) * rowHeight * u.y;
    mov.z = prec2int(drnd.y * r.z / TIER_DEP) * TIER_DEP * u.z;
    if(mov.x > 0)
        sa_x_gtz_cnt++;
    if(mov.x == 0)
        sa_x_eqz_cnt++;
    if(mov.x < 0)
        sa_x_ltz_cnt++;
    if(mov.y > 0)
        sa_y_gtz_cnt++;
    if(mov.y == 0)
        sa_y_eqz_cnt++;
    if(mov.y < 0)
        sa_y_ltz_cnt++;
    if(mov.z > 0)
        sa_z_gtz_cnt++;
    if(mov.z == 0)
        sa_z_eqz_cnt++;
    if(mov.z < 0)
        sa_z_ltz_cnt++;
    return mov;
}

void do_mac_mov(int idx, struct POS *mov) {
    int i = 0, mx = mov->x, my = mov->y, mz = mov->z;
    struct PIN *pin = NULL;
    struct FPOS pof = zeroFPoint;
    struct MODULE *mac = macro_st[idx];
    int moduleID = mac->idx;
    struct CELLx *cell = &gcell_st[moduleID];
    struct FPOS p0 = mac->center;

    mac->center.x += (prec)mx;
    mac->center.y += (prec)my;
    mac->center.z += (prec)mz;

    mac->center = valid_coor4(mac->center, mac->size);

    mov->x = mac->center.x - p0.x;
    mov->y = mac->center.y - p0.y;
    mov->z = mac->center.z - p0.z;

    mac->pmin_lg.x = (int)(mac->center.x - 0.5 * mac->size.x + 0.5);
    mac->pmin_lg.y = (int)(mac->center.y - 0.5 * mac->size.y + 0.5);
    mac->pmin_lg.z = (int)(mac->center.z - 0.5 * mac->size.z + 0.5);

    mac->pmax_lg.x = (int)(mac->center.x + 0.5 * mac->size.x + 0.5);
    mac->pmax_lg.y = (int)(mac->center.y + 0.5 * mac->size.y + 0.5);
    mac->pmax_lg.z = (int)(mac->center.z + 0.5 * mac->size.z + 0.5);

    for(i = 0; i < cell->pinCNTinObject; i++) {
        pin = cell->pin[i];
        pof = cell->pof[i];
        pin->fp.x = mac->center.x + pof.x;
        pin->fp.y = mac->center.y + pof.y;
        pin->fp.z = mac->center.z + pof.z;
    }

    mac->tier = (int)((prec)(mac->pmin_lg.z) / TIER_DEP + 0.5);
}

int mov_accept(prec c0, prec c1) {
    int rnd = 0;
    int flg = 0;
    prec dc = (c1 - c0) / c0;
    prec drnd = 0;
    prec exp_val = 0;

    rnd = rand();
    drnd = (prec)rnd / RAND_MAX;
    exp_val = exp(-1.0 * dc / sa_t);

    if(drnd < exp_val)
        flg = 1;
    else
        flg = 0;

    return flg;
}

void sa_param_update_sub() {
    sa_t *= sa_t_cof;  // 0.999999 ;
    sa_r.x -= sa_r_stp.x;
    sa_r.y -= sa_r_stp.y;
    sa_r.z -= sa_r_stp.z;
}

void sa_param_update() {
    sa_hpwl_wgt *= sa_hpwl_cof;  // 1.0
    sa_den_wgt *= sa_den_cof;    // 1.0
    sa_ovlp_wgt *= sa_ovlp_cof;  // 1.0015 ;
}

void sa_delete(void) {
    mkl_free(ovlp_a);
    mkl_free(ovlp_node);
    mkl_free(ovlp_y);
}

prec get_mac_cost(int idx, prec *hpwl_cost, prec *den_cost, int *ovlp_cost) {
    *hpwl_cost = get_mac_hpwl(idx);
    *den_cost = get_mac_den(idx);
    *ovlp_cost = get_mac_ovlp(idx);

    prec tot_cost = (*hpwl_cost) * sa_hpwl_wgt + (*den_cost) * sa_den_wgt +
                    (prec)(*ovlp_cost) * sa_ovlp_wgt;

    return tot_cost;
}

int get_mac_idx(void) {
    int rnd_idx = 0;
    double drnd_idx = 0;
    int mac_idx = 0;

    rnd_idx = rand();
    drnd_idx = (double)rnd_idx / RAND_MAX;
    mac_idx = (int)(drnd_idx * (double)macro_cnt);

    return mac_idx;
}

void post_mac_leg(void) {
    //////////// NEED IMPLEMENTATION //////////
    return;
}

void sa_post(void) {
    int i = 0, j = 0, z = 0;
    struct MODULE *mac = NULL;
    struct PIN *pin = NULL;

    for(i = 0; i < macro_cnt; i++) {
        mac = macro_st[i];
        mac->pmin.x = (prec)mac->pmin_lg.x;
        mac->pmin.y = (prec)mac->pmin_lg.y;
        mac->pmin.z = (prec)mac->pmin_lg.z;
        mac->pmax.x = (prec)mac->pmax_lg.x;
        mac->pmax.y = (prec)mac->pmax_lg.y;
        mac->pmax.z = (prec)mac->pmax_lg.z;

        z = (int)((prec)mac->pmin_lg.z / TIER_DEP + 0.5);

        mac->tier = z;

        for(j = 0; j < mac->pinCNTinObject; j++) {
            pin = mac->pin[j];
            pin->tier = z;
            pin->fp.z = mac->center.z;  // tier->center.z;
        }
    }

    total_modu_area -= total_macro_area * target_cell_den;
    total_cell_area -= total_macro_area * target_cell_den;
    total_move_available_area -= total_macro_area * target_cell_den;

    total_term_area += total_macro_area;
    total_PL_area -= total_macro_area;
    total_termPL_area += total_macro_area;
    total_WS_area -= total_macro_area;
}

int get_mac_ovlp_3d_top(void) {
    int mac_ovlp_3d = 0;

    real_mac_area = get_all_macro_ovlp_3d();
    mac_ovlp_3d = (tot_mac_area - real_mac_area) * TIER_DEP;

    return mac_ovlp_3d;
}

int get_mac_ovlp_top(void) {
    int mac_ovlp = 0;
    real_mac_area = get_all_macro_ovlp();
    mac_ovlp = tot_mac_area - real_mac_area;

    return mac_ovlp;
}

int get_tot_mac_ovlp(void) {
    int ovlp = 0, tot_ovlp = 0;
    struct MODULE *mac = NULL, *mac2 = NULL;
    ovlp_mac_cnt = 0;

    for(int i = 0; i < macro_cnt; i++) {
        mac = macro_st[i];
        for(int j = i + 1; j < macro_cnt; j++) {
            mac2 = macro_st[j];
            ovlp = get_ovlp_area(mac, mac2);
            tot_ovlp += ovlp;
            if((i == 35 || j == 35) && ovlp > 0) {
                ovlp_mac_cnt++;
            }
        }
    }
    return tot_ovlp;
}

void mac_ovlp_init(int n) {
    ovlp_a = (struct seg_tree_node *)mkl_malloc(
        sizeof(struct seg_tree_node) * (12 * n + 3), 64);
    ovlp_node =
        (struct NODE *)mkl_malloc(sizeof(struct NODE) * (2 * n + 1), 64);
    ovlp_y = (int *)mkl_malloc(sizeof(int) * (2 * n + 1), 64);
}

int macro_area_cmp(const void *a, const void *b) {
    struct MODULE **aa = (struct MODULE **)a;
    struct MODULE **bb = (struct MODULE **)b;

    return (*aa)->area < (*bb)->area ? 1 : 0;
}

int get_all_macro_ovlp_3d(void) {
    int i = 0, t = 0, z = 0;
    int tier_ovlp = 0, sum_ovlp = 0;
    int x1 = 0, x2 = 0;
    int y1 = 0, y2 = 0;
    struct MODULE *mac = NULL;
    int n = macro_cnt;

    for(z = 0; z < numLayer; z++) {
        t = 1;
        tier_ovlp = 0;

        for(i = 0; i < n; i++) {
            mac = macro_st[i];

            if(mac->tier != z)
                continue;

            x1 = mac->pmin_lg.x;
            y1 = mac->pmin_lg.y;

            x2 = mac->pmax_lg.x;
            y2 = mac->pmax_lg.y;

            ovlp_node[t].x = x1;
            ovlp_node[t].y1 = y1;
            ovlp_node[t].y2 = y2;
            ovlp_node[t].flg = 1;
            ovlp_y[t++] = y1;

            ovlp_node[t].x = x2;
            ovlp_node[t].y1 = y1;
            ovlp_node[t].y2 = y2;
            ovlp_node[t].flg = -1;
            ovlp_y[t++] = y2;
        }

        if(t <= 1)
            continue;

        qsort(ovlp_node + 1, t - 1, sizeof(struct NODE), ovlp_node_cmp);
        qsort(ovlp_y + 1, t - 1, sizeof(int), int_cmp);

        build_seg_tree(1, 1, t - 1);

        updata(1, ovlp_node[1]);

        for(i = 2; i < t; i++) {
            tier_ovlp += ovlp_a[1].len * (ovlp_node[i].x - ovlp_node[i - 1].x);
            updata(1, ovlp_node[i]);
        }

        sum_ovlp += tier_ovlp;
    }
    return sum_ovlp;
}

void mac_ovlp_mark(void) {
    int ovlp = 0;
    struct MODULE *mac = NULL;

    for(int i = 0; i < macro_cnt; i++) {
        mac = macro_st[i];
        mac->ovlp_flg = 0;
        ovlp = get_mac_ovlp(i);
        if(ovlp > 0)
            mac->ovlp_flg = 1;
    }
}

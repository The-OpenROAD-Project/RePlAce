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
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "bin.h"
#include "global.h"
#include "opt.h"

int prec_eqv(prec x, prec y) {
    return get_abs(x - y) < Epsilon;
}

int prec_le(prec x, prec y) {
    return prec_lt(x, y) | prec_eqv(x, y);
}

int prec_ge(prec x, prec y) {
    return prec_gt(x, y) | prec_eqv(x, y);
}

int prec_lt(prec x, prec y) {
    return x - y < -1.0 * Epsilon;
}

int prec_gt(prec x, prec y) {
    return x - y > 1.0 * Epsilon;
}

prec get_abs(prec a) {
    return a < 0.0 ? -1.0 * a : a;
}

int prec2int(prec a) {
    int af = floor(a);
    int ac = ceil(a);

    return a - (prec)af < (prec)ac - a ? af : ac;
}

unsigned prec2unsigned(prec a) {
    int af = floor(a);
    int ac = ceil(a);

    return a - (prec)af < (prec)ac - a ? af : ac;
}

int find_non_zero(prec *a, int cnt) {
    int i = 0;
    for(i = 0; i < cnt; i++) {
        if(!prec_eqv(a[i], 0.0))
            return i;
    }
    return -1;
}

void itoa(int n, char k[]) {
    char s[BUF_SZ];
    int i = 0, j = 0, sign = 0;
    if((sign = n) < 0)
        n = -n;
    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while((n /= 10) > 0);

    if(sign < 0)
        s[i++] = '-';
    s[i] = '\0';

    for(j = i - 1; j >= 0; j--) {
        /* printf("%c",s[j]); */
        k[j] = s[i - 1 - j];
    }
    k[i] = '\0';
    return;
}

void time_start(double *time_cost) {
    struct timeval time_val;
    time_t time_secs;
    suseconds_t time_micro;
    gettimeofday(&time_val, NULL);
    time_micro = time_val.tv_usec;
    time_secs = time_val.tv_sec;
    *time_cost = (double)time_micro / 1000000 + time_secs;
    return;
}

void time_end(double *time_cost) {
    struct timeval time_val;
    time_t time_secs;
    suseconds_t time_micro;
    gettimeofday(&time_val, NULL);
    time_micro = time_val.tv_usec;
    time_secs = time_val.tv_sec;
    *time_cost = (double)time_micro / 1000000 + time_secs - *time_cost;
    return;
}

void time_calc(double last_time, double *curr_time, double *time_cost) {
    struct timeval time_val;
    time_t time_secs;
    suseconds_t time_micro;
    gettimeofday(&time_val, NULL);
    time_micro = time_val.tv_usec;
    time_secs = time_val.tv_sec;
    *curr_time = (double)time_micro / 1000000 + time_secs;
    *time_cost = *curr_time - last_time;
}

string getexepath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return string(result, (count > 0) ? count : 0);
}

struct FPOS fp_mul(struct FPOS a, struct FPOS b) {
    struct FPOS c = zeroFPoint;
    c.x = a.x * b.x;
    c.y = a.y * b.y;
    if(flg_3dic)
        c.z = a.z * b.z;
    return c;
}

// struct FPOS fp_add (struct FPOS a, struct FPOS b)
//{
//  struct FPOS c=zeroFPoint;
//  c.x = a.x + b.x;
//  c.y = a.y + b.y;
//  if(flg_3dic)    c.z = a.z + b.z;
//  return c;
//}

struct FPOS fp_add_abs(struct FPOS a, struct FPOS b) {
    struct FPOS c = zeroFPoint;
    c.x = fabs(a.x) + fabs(b.x);
    c.y = fabs(a.y) + fabs(b.y);
    if(flg_3dic)
        c.z = fabs(a.z) + fabs(b.z);
    return c;
}

// struct FPOS fp_scal (prec s, struct FPOS a) {
//  struct FPOS c = a;
//  c.x *= s;
//  c.y *= s;
//  if (flg_3dic) c.z *= s;
//  return c;
//}

struct FPOS fp_subt(struct FPOS a, struct FPOS b) {
    struct FPOS c = zeroFPoint;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    if(flg_3dic)
        c.z = a.z - b.z;
    return c;
}

struct FPOS fp_subt_const(struct FPOS a, prec b) {
    struct FPOS c = zeroFPoint;
    c.x = a.x - b;
    c.y = a.y - b;
    if(flg_3dic)
        c.z = a.z - b;
    return c;
}

prec fp_sum(struct FPOS a) {
    prec sum = 0.0;
    sum = a.x + a.y;
    if(flg_3dic)
        sum += a.z;
    return sum;
}

prec fp_product(struct FPOS a) {
    prec prod = 0;
    prod = a.x * a.y;
    if(flg_3dic)
        prod *= a.z;
    return prod;
}

int p_product(struct POS a) {
    int product = a.x * a.y;
    if(flg_3dic)
        product *= a.z;
    return product;
}

int p_max(struct POS a) {
    int m = 0;
    m = max(a.x, a.y);
    if(flg_3dic)
        m = max(m, a.z);
    return m;
}

struct FPOS fp_exp(struct FPOS a) {
    struct FPOS b = zeroFPoint;
    b.x = exp(a.x);
    b.y = exp(a.y);
    if(flg_3dic)
        b.z = exp(a.z);
    return b;
}

struct FPOS fp_inv(struct FPOS a) {
    struct FPOS b = zeroFPoint;
    b.x = 1.0 / a.x;
    b.y = 1.0 / a.y;
    if(flg_3dic)
        b.z = 1.0 / a.z;
    return b;
}

struct FPOS fp_rand(void) {
    struct FPOS r = zeroFPoint;
    r.x = rand();
    r.y = rand();
    if(flg_3dic)
        r.z = rand();
    return r;
}

struct FPOS fp_min2(struct FPOS a, struct FPOS b) {
    struct FPOS c = zeroFPoint;
    c.x = min(a.x, b.x);
    c.y = min(a.y, b.y);
    if(flg_3dic)
        c.z = min(a.z, b.z);
    return c;
}

struct FPOS fp_max2(struct FPOS a, struct FPOS b) {
    struct FPOS c = zeroFPoint;
    c.x = max(a.x, b.x);
    c.y = max(a.y, b.y);
    if(flg_3dic)
        c.z = max(a.z, b.z);
    return c;
}

struct FPOS fp_div(struct FPOS a, struct FPOS b) {
    struct FPOS c = zeroFPoint;
    c.x = a.x / b.x;
    c.y = a.y / b.y;
    if(flg_3dic)
        c.z = a.z / b.z;
    return c;
}

struct FPOS p2fp(struct POS a) {
    struct FPOS b = zeroFPoint;
    b.x = (prec)a.x;
    b.y = (prec)a.y;
    if(flg_3dic)
        b.z = (prec)a.z;
    return b;
}

struct POS fp2p_floor(struct FPOS a) {
    struct POS b = zeroPoint;
    b.x = (int)(a.x);
    b.y = (int)(a.y);
    if(flg_3dic)
        b.z = (int)(a.z);
    return b;
}

struct POS fp2p_ceil(struct FPOS a) {
    struct POS b = zeroPoint;
    b.x = (int)(a.x) + 1;
    b.y = (int)(a.y) + 1;
    if(flg_3dic)
        b.z = (int)(a.z) + 1;
    return b;
}

// int dge (prec a, prec b) {
//    return  (a>b || a==b) ? 1 : 0;
//}

// int dle (prec a, prec b) {
//    return  (a<b || a==b) ? 1 : 0;
//}

int prec_cmp(const void *a, const void *b) {
    struct T0 *aa = (struct T0 *)a;
    struct T0 *bb = (struct T0 *)b;

    return aa->dis > bb->dis ? 1 : 0;
}

int max_pinCNTinObject_cmp(const void *a, const void *b) {
    struct MODULE **aa = (struct MODULE **)a;
    struct MODULE **bb = (struct MODULE **)b;

    return (*aa)->pinCNTinObject < (*bb)->pinCNTinObject ? 1 : 0;
}

int min_tier_cmp(const void *a, const void *b) {
    struct MODULE **aa = (struct MODULE **)a;
    struct MODULE **bb = (struct MODULE **)b;

    return (*aa)->center.z > (*bb)->center.z ? 1 : 0;
}

int max_area_dis_div_cmp(const void *a, const void *b) {
    struct MODULE **aa = (struct MODULE **)a;
    struct MODULE **bb = (struct MODULE **)b;

    prec dis_a = fabs((*aa)->center.z - (prec)((int)((*aa)->center.z + 0.5)));

    prec cost_a = (*aa)->area / dis_a;

    prec dis_b = fabs((*bb)->center.z - (prec)((int)((*bb)->center.z + 0.5)));

    prec cost_b = (*bb)->area / dis_b;

    return cost_a < cost_b ? 1 : 0;
}

bool TwoPinNets_comp(TwoPinNets x, TwoPinNets y) {
    return x.rect_dist < y.rect_dist;
}

prec dmax(prec a, prec b) {
    return a > b ? a : b;
}

prec dmin(prec a, prec b) {
    return a < b ? a : b;
}



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
#include "replace_private.h"
#include "opt.h"
#include "util.h"


using std::string;
using std::min;
using std::max;
using std::cout;
using std::endl;

prec get_abs(prec a) {
  return a < 0.0 ? -1.0 * a : a;
}

unsigned prec2unsigned(prec a) {
  int af = floor(a);
  int ac = ceil(a);

  return a - (prec)af < (prec)ac - a ? af : ac;
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
  struct FPOS c;
  c.x = a.x * b.x;
  c.y = a.y * b.y;
  return c;
}

// struct FPOS fp_add (struct FPOS a, struct FPOS b)
//{
//  struct FPOS c;
//  c.x = a.x + b.x;
//  c.y = a.y + b.y;
//  if(flg_3dic)    c.z = a.z + b.z;
//  return c;
//}

struct FPOS fp_add_abs(struct FPOS a, struct FPOS b) {
  struct FPOS c;
  c.x = fabs(a.x) + fabs(b.x);
  c.y = fabs(a.y) + fabs(b.y);
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
  struct FPOS c;
  c.x = a.x - b.x;
  c.y = a.y - b.y;
  return c;
}

struct FPOS fp_subt_const(struct FPOS a, prec b) {
  struct FPOS c;
  c.x = a.x - b;
  c.y = a.y - b;
  return c;
}

prec fp_sum(struct FPOS a) {
  prec sum = 0.0;
  sum = a.x + a.y;
  return sum;
}

prec fp_product(struct FPOS a) {
  prec prod = 0;
  prod = a.x * a.y;
  return prod;
}

int p_product(struct POS a) {
  int product = a.x * a.y;
  return product;
}

int p_max(struct POS a) {
  int m = 0;
  m = max(a.x, a.y);
  return m;
}

struct FPOS fp_exp(struct FPOS a) {
  struct FPOS b;
  b.x = exp(a.x);
  b.y = exp(a.y);
  return b;
}

struct FPOS fp_inv(struct FPOS a) {
  struct FPOS b;
  b.x = 1.0 / a.x;
  b.y = 1.0 / a.y;
  return b;
}

struct FPOS fp_rand(void) {
  struct FPOS r;
  r.x = rand();
  r.y = rand();
  return r;
}

struct FPOS fp_div(struct FPOS a, struct FPOS b) {
  struct FPOS c;
  c.x = a.x / b.x;
  c.y = a.y / b.y;
  return c;
}

struct FPOS p2fp(struct POS a) {
  struct FPOS b;
  b.x = (prec)a.x;
  b.y = (prec)a.y;
  return b;
}


// return Common Area
// between Rectangle A and Rectangle B.
// type : casted long from prec
int iGetCommonAreaXY(POS aLL, POS aUR, POS bLL, POS bUR) {
  int xLL = max(aLL.x, bLL.x), yLL = max(aLL.y, bLL.y),
       xUR = min(aUR.x, bUR.x), yUR = min(aUR.y, bUR.y);

  if(xLL >= xUR || yLL >= yUR) {
    return 0;
  }
  else {
    return (xUR - xLL) * (yUR - yLL);
  }
}

// return Common Area
// between Rectangle A and Rectangle B.
// type : prec
prec pGetCommonAreaXY(FPOS aLL, FPOS aUR, FPOS bLL, FPOS bUR) {
  prec xLL = max(aLL.x, bLL.x), yLL = max(aLL.y, bLL.y),
       xUR = min(aUR.x, bUR.x), yUR = min(aUR.y, bUR.y);

  if(xLL >= xUR || yLL >= yUR) {
    return 0;
  }
  else {
    return (xUR - xLL) * (yUR - yLL);
  }
}

// for string escape
bool ReplaceStringInPlace(std::string &subject,
                                 const std::string &search,
                                 const std::string &replace) {
  size_t pos = 0;
  bool isFound = false;
  while((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
    isFound = true;
  }
  return isFound;
}

void SetEscapedStr(std::string &inp) {
  if(ReplaceStringInPlace(inp, "/", "\\/")) {
    ReplaceStringInPlace(inp, "[", "\\[");
    ReplaceStringInPlace(inp, "]", "\\]");
  }
}

char *GetEscapedStr(const char *name, bool isEscape) {
  std::string tmp(name);
  if( isEscape ) {
    SetEscapedStr(tmp);
  }
  return strdup(tmp.c_str());
}

string GetRealPath(string path ) {
  char tmp[PATH_MAX] = {0, };
  realpath(path.c_str(), tmp);
  return string(tmp);
}

#define VERBOSE_CHECK() if( gVerbose < verbose ) { return; };

// Procedure message
void PrintProc(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] " << input << endl;
}
void PrintProcBegin(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] Begin " << input << " ..." << endl;
}
void PrintProcEnd(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] End " << input << endl;
}

// Error message
void PrintError(string input, int verbose) {
  cout << "[ERROR] " << input << endl;
  exit(1);
}

// Info message
void PrintInfoInt(string input, int val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}
void PrintInfoPrec(string input, prec val, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = %.6f\n", input.c_str(), val);
  fflush(stdout);
}
// SI format due to WNS/TNS
void PrintInfoPrecSignificant(string input, prec val, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = %g\n", input.c_str(), val);
  fflush(stdout);
}

void PrintInfoPrecPair(string input, prec val1, prec val2, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = (%.6f, %.6f)\n", input.c_str(), val1, val2);
  fflush(stdout);
}
void PrintInfoString(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << endl;
}
void PrintInfoString(string input, string val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}
void PrintInfoRuntime(string input, double runtime, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %sRuntime = %.4f\n", input.c_str(), runtime);
  fflush(stdout);
}


// lef 2 def unit convert
static prec l2d = 0.0f;

//
// To support book-shelf based tools, scale down all values..
//
// Set unitX as siteWidth
// Set unitY as Vertical routing pitch (in lowest metal)
//
static prec unitX = 0.0f;
static prec unitY = 0.0f;

//
// To prevent mis-matching in ROW offset.
//
static prec offsetX = PREC_MAX;
static prec offsetY = PREC_MAX;


void SetUnitX(float _unitX) {
  unitX = _unitX;
}

void SetUnitY(float _unitY) {
  unitY = _unitY;
}

void SetUnitY(double _unitY) {
  unitY = _unitY;
}

void SetOffsetX(float _offsetX) { 
  offsetX = _offsetX;
}

void SetOffsetY(float _offsetY) {
  offsetY = _offsetY;
}

void SetDefDbu(float _dbu) {
  l2d = _dbu;
}

prec GetUnitX() { return unitX; }
prec GetUnitY() { return unitY; }
prec GetOffsetX() { return offsetX; }
prec GetOffsetY() { return offsetY; }
prec GetDefDbu() { return l2d; }

int GetScaleUpSize(prec input) {
  return INT_CONVERT( input * GetUnitX() );
}

int GetScaleUpPointX(prec input) {
  return INT_CONVERT( input * GetUnitX() - GetOffsetX() );
}
int GetScaleUpPointY(prec input) {
  return INT_CONVERT( input * GetUnitY() - GetOffsetY() );
}

float GetScaleUpPointFloatX(float input) {
  return input * GetUnitX() - GetOffsetX();
}
float GetScaleUpPointFloatY(float input) {
  return input * GetUnitY() - GetOffsetY();
}

prec GetScaleDownSize(prec input) {
  return input / GetUnitX();
}
prec GetScaleDownPoint( prec input) {
  return (input + GetOffsetX()) / GetUnitX();
}


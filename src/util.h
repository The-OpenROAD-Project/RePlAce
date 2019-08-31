#ifndef __REPLACE_UTIL__
#define __REPLACE_UTIL__ 0

#include <iostream>
#include "replace_private.h"

int prec_eqv(prec x, prec y);
//int prec_le(prec x, prec y);
//int prec_ge(prec x, prec y);
//int prec_lt(prec x, prec y);
//int prec_gt(prec x, prec y);
unsigned prec2unsigned(prec a);
int find_non_zero(prec *a, int cnt);
void itoa(int n, char k[]);


void time_start(double *time_cost);
void time_end(double *time_cost);
void time_calc(double last_time, double *curr_time, double *time_cost);

string getexepath();

FPOS fp_mul(struct FPOS a, struct FPOS b);
inline FPOS fp_add(struct FPOS a, struct FPOS b) {
  struct FPOS c;
  c.x = a.x + b.x;
  c.y = a.y + b.y;
  return c;
}
FPOS fp_add_abs(struct FPOS a, struct FPOS b);
inline FPOS fp_scal(prec s, struct FPOS a) {
  struct FPOS c = a;
  c.x *= s;
  c.y *= s;
  return c;
}

prec fp_sum(struct FPOS a);
FPOS fp_subt(struct FPOS a, struct FPOS b);
FPOS fp_subt_const(struct FPOS a, prec b);
FPOS fp_exp(struct FPOS a);
prec fp_product(struct FPOS a);

int p_product(struct POS a);
int p_max(struct POS a);

FPOS fp_div(struct FPOS a, struct FPOS b);
FPOS fp_rand(void);
FPOS fp_inv(struct FPOS a);
FPOS p2fp(struct POS a);

prec get_abs(prec a);


// Rect common area functions
int iGetCommonAreaXY(POS aLL, POS aUR, POS bLL, POS bUR);
prec pGetCommonAreaXY(FPOS aLL, FPOS aUR, FPOS bLL, FPOS bUR); 

// String replace functions
bool ReplaceStringInPlace(std::string &subject,
                                 const std::string &search,
                                 const std::string &replace);
void SetEscapedStr(std::string &inp);
char *GetEscapedStr(const char *name, bool isEscape = true);
string GetRealPath(string path );


// Print functions
void PrintProc(string input, int verbose = 0);
void PrintProcBegin(string input, int verbose = 0);
void PrintProcEnd(string input, int verbose = 0);
void PrintError(string input, int verbose = 0); 
void PrintInfoInt(string input, int val, int verbose = 0);
void PrintInfoPrec(string input, prec val, int verbose = 0);
void PrintInfoPrecPair(string input, prec val1, prec val2, int verbose = 0);
void PrintInfoString(string input, int verbose = 0);
void PrintInfoString(string input, string val, int verbose = 0);
void PrintInfoRuntime(string input, double runtime, int verbose = 0);




#endif

#ifndef __REPLACE_UTIL__
#define __REPLACE_UTIL__ 0

#include <iostream>
#include "replace_private.h"


unsigned prec2unsigned(prec a);
void itoa(int n, char k[]);

void time_start(double *time_cost);
void time_end(double *time_cost);
void time_calc(double last_time, double *curr_time, double *time_cost);

std::string getexepath();

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
std::string GetRealPath(std::string path );


// Print functions
void PrintProc(std::string input, int verbose = 0);
void PrintProcBegin(std::string input, int verbose = 0);
void PrintProcEnd(std::string input, int verbose = 0);
void PrintError(std::string input, int verbose = 0); 
void PrintInfoInt(std::string input, int val, int verbose = 0);
void PrintInfoPrec(std::string input, prec val, int verbose = 0);
void PrintInfoPrecSignificant(std::string input, prec val, int verbose = 0);
void PrintInfoPrecPair(std::string input, prec val1, prec val2, int verbose = 0);
void PrintInfoString(std::string input, int verbose = 0);
void PrintInfoString(std::string input, std::string val, int verbose = 0);
void PrintInfoRuntime(std::string input, double runtime, int verbose = 0);


// scaleDown vars / functions
// custom scale down parameter setting during the stage
void SetDefDbu(float _dbu);
void SetUnitX(float _unitX);
void SetUnitY(float _unitY);
void SetOffsetX(float _offsetX);
void SetOffsetY(float _offsetY);
void SetUnitY(double _unitY);

prec GetUnitX();
prec GetUnitY();
prec GetOffsetX();
prec GetOffsetY();
prec GetDefDbu(); 

int GetScaleUpSize(float input);
int GetScaleUpPointX(float input);
int GetScaleUpPointY(float input);
float GetScaleUpPointFloatX( float input);
float GetScaleUpPointFloatY( float input);

prec GetScaleDownSize(float input);
prec GetScaleDownPoint(float input);




#endif

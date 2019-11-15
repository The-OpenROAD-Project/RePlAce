#ifndef __REPLACE_TIMING_STA__
#define __REPLACE_TIMING_STA__ 0

#include "db_sta/dbSta.hh"

#ifdef YY_INPUT
#undef YY_INPUT
#endif

// to import Swig
extern "C" {
extern int Sta_Init(Tcl_Interp *interp);
}

// to import TCL functions
namespace sta {
extern const char *tcl_inits[];
}

#endif

#include <tcl.h>
#include "StaMain.hh"
#include "openroad/OpenRoad.hh"
#include "MakeReplace.h"

namespace sta {
// Tcl files encoded into strings.
extern const char *replace_tcl_inits[];
}

extern "C" {
extern int Replace_Init(Tcl_Interp *interp);
}

namespace ord {

void *
makeReplaceu()
{
  return nullptr;
}

void
deleteReplace(void *)
{
}

void
initReplace(OpenRoad *openroad)
{
  Tcl_Interp *tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Replace_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(tcl_interp, sta::replace_tcl_inits);
}

}

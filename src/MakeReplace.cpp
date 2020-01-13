#include "MakeReplace.h"
#include "Replace.h"

namespace ord {

replace::Replace* 
makeReplace() {
  return new Replace::replace();
}

void
initReplace(OpenRoad* openroad) {
}

void
deleteReplace(replace::Replace *replace) {
  delete replace;
}

}

#include "replace/MakeReplace.h"
#include "replace/Replace.h"

namespace ord {

replace::Replace* 
makeReplace() {
  return new replace::Replace();
}

void
initReplace(OpenRoad* openroad) {
}

void
deleteReplace(replace::Replace *replace) {
  delete replace;
}

}

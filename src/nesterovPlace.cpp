#include "nesterovPlace.h"

namespace replace {

NesterovPlace::NesterovPlace() : pb_(nullptr), nb_(nullptr) {}
NesterovPlace::NesterovPlace(PlacerBase* placerBase, NesterovBase* nesterovBase) 
: pb_(placerBase), nb_(nesterovBase) {}

NesterovPlace::~NesterovPlace() {
  pb_ = nullptr;
  nb_ = nullptr;
}

void
NesterovPlace::doNesterovPlace() {

}

}

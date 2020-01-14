#include "nesterovPlace.h"

namespace replace {

NesterovPlace::NesterovPlace() : pb_(nullptr) {}
NesterovPlace::NesterovPlace(PlacerBase* placerBase) 
: pb_(placerBase) {}

NesterovPlace::~NesterovPlace() {
  pb_ = nullptr;
}

void
NesterovPlace::doNesterovPlace() {

}

}

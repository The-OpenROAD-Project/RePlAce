#include "nesterovPlace.h"

namespace replace {

NesterovPlace::NesterovPlace() 
  : pb_(nullptr), nb_(nullptr) {}

NesterovPlace::NesterovPlace(
    std::shared_ptr<PlacerBase> pb, 
    std::shared_ptr<NesterovBase> nb) 
: pb_(pb), nb_(nb) {}

NesterovPlace::~NesterovPlace() {}

void
NesterovPlace::doNesterovPlace() {

}

}

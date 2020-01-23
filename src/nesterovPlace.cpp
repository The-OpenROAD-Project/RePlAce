#include "nesterovPlace.h"
#include "nesterovBase.h"
#include <iostream>
using namespace std;

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
  cout << "nesterovPlace: " << endl;
  float initCoefX = 0.1, initCoefY = 0.1;
  nb_->updateWireLengthForceWA(initCoefX, initCoefY);

}

}

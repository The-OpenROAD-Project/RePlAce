#include "nesterovPlace.h"
#include "nesterovBase.h"
#include <iostream>
using namespace std;

namespace replace {

NesterovPlaceVars::NesterovPlaceVars()
  : initDensityPanelty(0.00001),
  maxBackTrack(10) {}

NesterovPlace::NesterovPlace() 
  : npVars_(), pb_(nullptr), nb_(nullptr) {}

NesterovPlace::NesterovPlace(
    NesterovPlaceVars npVars,
    std::shared_ptr<PlacerBase> pb, 
    std::shared_ptr<NesterovBase> nb) 
: pb_(pb), nb_(nb),
  wireLengthGradSum_(0),
  densityGradSum_(0),
  backTrackStepLength_(0),
  curDensityPanelty_(0) {
  npVars_ = npVars;
  init();
}

NesterovPlace::~NesterovPlace() {
  reset();
}

void NesterovPlace::init() {
  const int gCellSize = nb_->gCells().size();
  curCoordi_.reserve(gCellSize);
  curWireLengthGrads_.reserve(gCellSize);
  curDensityGrads_.reserve(gCellSize);
  curSumGrads_.reserve(gCellSize);

  newCoordi_.reserve(gCellSize);
  newWireLengthGrads_.reserve(gCellSize);
  newDensityGrads_.reserve(gCellSize);
  newSumGrads_.reserve(gCellSize);
  
  prevCoordi_.reserve(gCellSize);
  prevWireLengthGrads_.reserve(gCellSize);
  prevDensityGrads_.reserve(gCellSize);
  prevSumGrads_.reserve(gCellSize);

  for(auto& gCell : nb_->gCells()) {
    curCoordi_.push_back(FloatCoordi(gCell->cx(),gCell->cy())); 
    prevCoordi_.push_back(FloatCoordi(gCell->cx(), gCell->cy()));
  }
}

// clear reset
void NesterovPlace::reset() {
  vector<FloatCoordi>().swap(curCoordi_);
  vector<FloatCoordi>().swap(curWireLengthGrads_);
  vector<FloatCoordi>().swap(curDensityGrads_);
  vector<FloatCoordi>().swap(curSumGrads_);

  vector<FloatCoordi>().swap(newCoordi_);
  vector<FloatCoordi>().swap(newWireLengthGrads_);
  vector<FloatCoordi>().swap(newDensityGrads_);
  vector<FloatCoordi>().swap(newSumGrads_);

  vector<FloatCoordi>().swap(prevCoordi_);
  vector<FloatCoordi>().swap(prevWireLengthGrads_);
  vector<FloatCoordi>().swap(prevDensityGrads_);
  vector<FloatCoordi>().swap(prevSumGrads_);
}

void
NesterovPlace::doNesterovPlace() {

  cout << "nesterovPlace: " << endl;
  float initCoefX = 0.1, initCoefY = 0.1;
  nb_->updateWireLengthForceWA(initCoefX, initCoefY);
  cout << "WL force Done" << endl;
}

}

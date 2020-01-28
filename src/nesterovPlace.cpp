#include "placerBase.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "opendb/db.h"
#include <iostream>
using namespace std;

namespace replace {

NesterovPlaceVars::NesterovPlaceVars()
  : maxNesterovIter(2500), 
  maxBackTrack(10),
  initDensityPanelty(0.0001),
  initWireLengthCoeff(1.0/8.0),
  targetOverflow(0.1),
  minBoundMuK(0.95),
  maxBoundMuK(1.05) {}

NesterovPlace::NesterovPlace() 
  : pb_(nullptr), nb_(nullptr), npVars_(), 
  wireLengthGradSum_(0), 
  densityGradSum_(0),
  backTrackStepLength_(0),
  densityPanelty_(0),
  wireLengthCoeffX_(0), 
  wireLengthCoeffY_(0) {}

NesterovPlace::NesterovPlace(
    NesterovPlaceVars npVars,
    std::shared_ptr<PlacerBase> pb, 
    std::shared_ptr<NesterovBase> nb) 
: NesterovPlace() {
  npVars_ = npVars;
  pb_ = pb;
  nb_ = nb;
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
    nb_->updateDensityCoordiLayoutInside( gCell );
    curCoordi_.push_back(
        FloatCoordi(gCell->dCx(), gCell->dCy())); 
    prevCoordi_.push_back(
        FloatCoordi(gCell->dCx(), gCell->dCy()));
  }

  // bin update
  nb_->updateGCellDensityCenterLocation(curCoordi_);

  float baseWireLengthCoeff 
    = npVars_.initWireLengthCoeff 
    / static_cast<float>(
        (nb_->binSizeX() + nb_->binSizeY())) 
    * 0.5;
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(pb_->placeInstsArea());

  cout << "InitSumOverflow: " << sumOverflow_ << endl;

}

// clear reset
void NesterovPlace::reset() {
  vector<FloatCoordi> ().swap(curCoordi_);
  vector<FloatCoordi> ().swap(curWireLengthGrads_);
  vector<FloatCoordi> ().swap(curDensityGrads_);
  vector<FloatCoordi> ().swap(curSumGrads_);

  vector<FloatCoordi> ().swap(newCoordi_);
  vector<FloatCoordi> ().swap(newWireLengthGrads_);
  vector<FloatCoordi> ().swap(newDensityGrads_);
  vector<FloatCoordi> ().swap(newSumGrads_);

  vector<FloatCoordi> ().swap(prevCoordi_);
  vector<FloatCoordi> ().swap(prevWireLengthGrads_);
  vector<FloatCoordi> ().swap(prevDensityGrads_);
  vector<FloatCoordi> ().swap(prevSumGrads_);
}

void
NesterovPlace::updateGradients(
    std::vector<FloatCoordi>& sumGrads,
    std::vector<FloatCoordi>& wireLengthGrads,
    std::vector<FloatCoordi>& densityGrads) {

  for(int i=0; i<sumGrads.size(); i++) {
    GCell* gCell = nb_->gCells().at(i);


  }
}

void
NesterovPlace::doNesterovPlace() {
  cout << "nesterovPlace: " << endl;

  updateGradients(
      curSumGrads_, curWireLengthGrads_,
      curDensityGrads_);


  float initCoefX = 0.1, initCoefY = 0.1;
  nb_->updateWireLengthForceWA(initCoefX, initCoefY);
  cout << "WL force Done" << endl;
}

}

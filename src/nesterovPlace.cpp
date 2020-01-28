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
  baseWireLengthCoeff_(0), 
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
  curCoordi_.resize(gCellSize, FloatCoordi());
  curWireLengthGrads_.resize(gCellSize, FloatCoordi());
  curDensityGrads_.resize(gCellSize, FloatCoordi());
  curSumGrads_.resize(gCellSize, FloatCoordi());

  newCoordi_.resize(gCellSize, FloatCoordi());
  newWireLengthGrads_.resize(gCellSize, FloatCoordi());
  newDensityGrads_.resize(gCellSize, FloatCoordi());
  newSumGrads_.resize(gCellSize, FloatCoordi());
  
  prevCoordi_.resize(gCellSize, FloatCoordi());
  prevWireLengthGrads_.resize(gCellSize, FloatCoordi());
  prevDensityGrads_.resize(gCellSize, FloatCoordi());
  prevSumGrads_.resize(gCellSize, FloatCoordi());

  for(auto& gCell : nb_->gCells()) {
    nb_->updateDensityCoordiLayoutInside( gCell );
    int idx = &gCell - &nb_->gCells()[0];
    curCoordi_[idx] = 
      FloatCoordi(gCell->dCx(), gCell->dCy()); 
    prevCoordi_[idx] = 
      FloatCoordi(gCell->dCx(), gCell->dCy());
  }

  // bin update
  nb_->updateGCellDensityCenterLocation(curCoordi_);

  // FFT update
  nb_->updateDensityForceBin();

  baseWireLengthCoeff_ 
    = 500 * npVars_.initWireLengthCoeff 
    / static_cast<float>(
        (nb_->binSizeX() + nb_->binSizeY())) 
    * 0.5;
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(pb_->placeInstsArea());

  cout << "InitSumOverflow: " << sumOverflow_ << endl;
  updateWireLengthCoef(sumOverflow_);
  cout << "wireLengthCoeff: " << wireLengthCoeffX_ << endl;

  // WL update
  nb_->updateWireLengthForceWA(wireLengthCoeffX_, wireLengthCoeffY_);
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

// to execute following function,
// 
// nb_->updateGCellDensityCenterLocation(coordi); // bin update
// nb_->updateDensityForceBin(); // bin Force update
//  
// nb_->updateWireLengthForceWA(wireLengthCoeffX_, wireLengthCoeffY_); // WL update
//
void
NesterovPlace::updateGradients(
    std::vector<FloatCoordi>& sumGrads,
    std::vector<FloatCoordi>& wireLengthGrads,
    std::vector<FloatCoordi>& densityGrads) {

  for(int i=0; i<nb_->gCells().size(); i++) {
    GCell* gCell = nb_->gCells().at(i);
    wireLengthGrads[i] = nb_->getWireLengthGradientWA(
        gCell, wireLengthCoeffX_, wireLengthCoeffY_);
    densityGrads[i] = nb_->getDensityGradient(gCell); 

    sumGrads[i].x = wireLengthGrads[i].x + densityPanelty_ * densityGrads[i].x;
    sumGrads[i].y = wireLengthGrads[i].y + densityPanelty_ * densityGrads[i].y;
    // cout << "w: " << wireLengthGrads[i].x << " d: " << densityGrads[i].x << endl;
  }
}

void
NesterovPlace::doNesterovPlace() {
  cout << "nesterovPlace: " << endl;
  
  updateGradients(
      curSumGrads_, curWireLengthGrads_,
      curDensityGrads_);

  cout << "WL force Done" << endl;
}

void
NesterovPlace::updateWireLengthCoef(float overflow) {
  if( overflow > 1.0 ) {
    wireLengthCoeffX_ = wireLengthCoeffY_ = 0.1;
  }
  else if( overflow < 0.1 ) {
    wireLengthCoeffX_ = wireLengthCoeffY_ = 10.0;
  }
  else {
    wireLengthCoeffX_ = wireLengthCoeffY_ 
      = 1.0 / pow(10.0, (overflow-0.1)*20 / 9.0 - 1.0);
  }

  wireLengthCoeffX_ *= baseWireLengthCoeff_;
  wireLengthCoeffY_ *= baseWireLengthCoeff_;
}

}

#include "placerBase.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "opendb/db.h"
#include <iostream>
using namespace std;

namespace replace {

static float
getDistance(vector<FloatCoordi>& a, vector<FloatCoordi>& b);

NesterovPlaceVars::NesterovPlaceVars()
  : maxNesterovIter(2500), 
  maxBackTrack(10),
  initDensityPanelty(0.0001),
  initWireLengthCoeff(1.0/8.0),
  targetOverflow(0.1),
  minBoundMuK(0.95),
  maxBoundMuK(1.05),
  initialPrevCoordiUpdateCoeff(0.01) {}

NesterovPlace::NesterovPlace() 
  : pb_(nullptr), nb_(nullptr), npVars_(), 
  wireLengthGradSum_(0), 
  densityGradSum_(0),
  stepLength_(0),
  densityPanelty_(npVars_.initDensityPanelty),
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
    = 250 * npVars_.initWireLengthCoeff 
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
 
  // fill in curSumGrads_, curWireLengthGrads_, curDensityGrads_ 
  cout << "current: " << endl;
  updateGradients(
      curSumGrads_, curWireLengthGrads_,
      curDensityGrads_);

  // approximately fill in prevCoordi_ to calculate lc vars
  updateInitialPrevCoordi();

  
  // bin, FFT, wlen update with prevCoordi.
  nb_->updateGCellDensityCenterLocation(prevCoordi_);
  nb_->updateDensityForceBin();
  nb_->updateWireLengthForceWA(wireLengthCoeffX_, wireLengthCoeffY_);

  // update previSumGrads_, prevWireLengthGrads_, prevDensityGrads_
  cout << "prev: " << endl;
  updateGradients(
      prevSumGrads_, prevWireLengthGrads_,
      prevDensityGrads_);
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(pb_->placeInstsArea());
  
  cout << "PrevSumOverflow: " << sumOverflow_ << endl;
  
  stepLength_  
    = getStepLength (prevCoordi_, prevSumGrads_, curCoordi_, curSumGrads_);

  cout << "initialStepLength: " << stepLength_ << endl;
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
//    cout << "wx: " << wireLengthGrads[i].x << " dx: " << densityGrads[i].x;
//    cout << " tx: " << sumGrads[i].x << endl;
//    cout << "wy: " << wireLengthGrads[i].y << " dy: " << densityGrads[i].y;
//    cout << " ty: " << sumGrads[i].y << endl << endl;
  }
}

void
NesterovPlace::doNesterovPlace() {
  cout << "nesterovPlace: " << endl;
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

void
NesterovPlace::updateInitialPrevCoordi() {
  for(int i=0; i<nb_->gCells().size(); i++) {
    GCell* curGCell = nb_->gCells()[i];

    float prevCoordiX 
      = curCoordi_[i].x + npVars_.initialPrevCoordiUpdateCoeff 
      * curSumGrads_[i].x;
  
    float prevCoordiY
      = curCoordi_[i].y + npVars_.initialPrevCoordiUpdateCoeff
      * curSumGrads_[i].y;
    
    FloatCoordi newCoordi( 
      nb_->getDensityCoordiLayoutInsideX( curGCell, prevCoordiX),
      nb_->getDensityCoordiLayoutInsideY( curGCell, prevCoordiY) );

    prevCoordi_[i] = newCoordi;
  } 
}

float
NesterovPlace::getStepLength(
    std::vector<FloatCoordi>& prevCoordi_,
    std::vector<FloatCoordi>& prevSumGrads_,
    std::vector<FloatCoordi>& curCoordi_,
    std::vector<FloatCoordi>& curSumGrads_ ) {
  float coordiDistance 
    = getDistance(prevCoordi_, curCoordi_);
  float gradDistance 
    = getDistance(prevSumGrads_, curSumGrads_);

//  cout << "cDist: " << coordiDistance << endl;
//  cout << "gDist: " << gradDistance << endl;

  return 1.0 / gradDistance / coordiDistance;
}


static float
getDistance(vector<FloatCoordi>& a, vector<FloatCoordi>& b) {
  float sumDistance = 0.0f;
  for(int i=0; i<a.size(); i++) {
    sumDistance += (a[i].x - b[i].x) * (a[i].x - b[i].x);
    sumDistance += (a[i].y - b[i].y) * (a[i].y - b[i].y);
  }

  return sqrt( sumDistance / (2.0 * a.size()) );
}

}

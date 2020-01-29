#include "placerBase.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "opendb/db.h"
#include <iostream>
using namespace std;

namespace replace {

static float
getDistance(vector<FloatCoordi>& a, vector<FloatCoordi>& b);

static float
getSecondNorm(vector<FloatCoordi>& a);

NesterovPlaceVars::NesterovPlaceVars()
  : maxNesterovIter(20), 
  maxBackTrack(10),
  initDensityPanelty(0.0001),
  initWireLengthCoeff(1.0/8.0),
  targetOverflow(0.1),
  minBoundMuK(0.95),
  maxBoundMuK(1.05),
  initialPrevCoordiUpdateCoeff(700),
  minPreconditioner(1.0),
  referenceHpwl(3460000) {}

NesterovPlace::NesterovPlace() 
  : pb_(nullptr), nb_(nullptr), npVars_(), 
  wireLengthGradSum_(0), 
  densityGradSum_(0),
  stepLength_(0),
  densityPanelty_(0),
  baseWireLengthCoeff_(0), 
  wireLengthCoeffX_(0), 
  wireLengthCoeffY_(0),
  prevHpwl_(0) {}

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
  curSLPCoordi_.resize(gCellSize, FloatCoordi());
  curSLPWireLengthGrads_.resize(gCellSize, FloatCoordi());
  curSLPDensityGrads_.resize(gCellSize, FloatCoordi());
  curSLPSumGrads_.resize(gCellSize, FloatCoordi());

  nextSLPCoordi_.resize(gCellSize, FloatCoordi());
  nextSLPWireLengthGrads_.resize(gCellSize, FloatCoordi());
  nextSLPDensityGrads_.resize(gCellSize, FloatCoordi());
  nextSLPSumGrads_.resize(gCellSize, FloatCoordi());
  
  prevSLPCoordi_.resize(gCellSize, FloatCoordi());
  prevSLPWireLengthGrads_.resize(gCellSize, FloatCoordi());
  prevSLPDensityGrads_.resize(gCellSize, FloatCoordi());
  prevSLPSumGrads_.resize(gCellSize, FloatCoordi());

  curCoordi_.resize(gCellSize, FloatCoordi());
  nextCoordi_.resize(gCellSize, FloatCoordi());

  for(auto& gCell : nb_->gCells()) {
    nb_->updateDensityCoordiLayoutInside( gCell );
    int idx = &gCell - &nb_->gCells()[0];
    curSLPCoordi_[idx] 
      = prevSLPCoordi_[idx] 
      = curCoordi_[idx] 
      = FloatCoordi(gCell->dCx(), gCell->dCy()); 
  }

  // bin update
  nb_->updateGCellDensityCenterLocation(curSLPCoordi_);
  
  prevHpwl_ 
    = nb_->getHpwl();
  cout << "prev Hpwl : " << prevHpwl_ << endl;

  // FFT update
  nb_->updateDensityForceBin();

  baseWireLengthCoeff_ 
    = 100 * npVars_.initWireLengthCoeff 
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
 
  // fill in curSLPSumGrads_, curSLPWireLengthGrads_, curSLPDensityGrads_ 
  updateGradients(
      curSLPSumGrads_, curSLPWireLengthGrads_,
      curSLPDensityGrads_);

  // approximately fill in 
  // prevSLPCoordi_ to calculate lc vars
  updateInitialPrevSLPCoordi();

  // bin, FFT, wlen update with prevSLPCoordi.
  nb_->updateGCellDensityCenterLocation(prevSLPCoordi_);
  nb_->updateDensityForceBin();
  nb_->updateWireLengthForceWA(wireLengthCoeffX_, wireLengthCoeffY_);
  
  int32_t tmpHpwl 
    = nb_->getHpwl();
  cout << "tmp Hpwl : " << tmpHpwl << endl;
  // update previSumGrads_, prevSLPWireLengthGrads_, prevSLPDensityGrads_
  updateGradients(
      prevSLPSumGrads_, prevSLPWireLengthGrads_,
      prevSLPDensityGrads_);

  cout << "wireLengthGradSum_ : " << wireLengthGradSum_ << endl;
  cout << "densityGradSum_ : " << densityGradSum_ << endl;
  densityPanelty_ 
    = wireLengthGradSum_ / densityGradSum_ 
    * npVars_.initDensityPanelty; 
  
  cout << "initDensityPanelty_ : " << densityPanelty_ << endl;
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(pb_->placeInstsArea());
  
  cout << "PrevSumOverflow: " << sumOverflow_ << endl;
  
  stepLength_  
    = getStepLength (prevSLPCoordi_, prevSLPSumGrads_, curSLPCoordi_, curSLPSumGrads_);


  cout << "initialStepLength: " << stepLength_ << endl;
}

// clear reset
void NesterovPlace::reset() {
  vector<FloatCoordi> ().swap(curSLPCoordi_);
  vector<FloatCoordi> ().swap(curSLPWireLengthGrads_);
  vector<FloatCoordi> ().swap(curSLPDensityGrads_);
  vector<FloatCoordi> ().swap(curSLPSumGrads_);

  vector<FloatCoordi> ().swap(nextSLPCoordi_);
  vector<FloatCoordi> ().swap(nextSLPWireLengthGrads_);
  vector<FloatCoordi> ().swap(nextSLPDensityGrads_);
  vector<FloatCoordi> ().swap(nextSLPSumGrads_);

  vector<FloatCoordi> ().swap(prevSLPCoordi_);
  vector<FloatCoordi> ().swap(prevSLPWireLengthGrads_);
  vector<FloatCoordi> ().swap(prevSLPDensityGrads_);
  vector<FloatCoordi> ().swap(prevSLPSumGrads_);

  vector<FloatCoordi> ().swap(curCoordi_);
  vector<FloatCoordi> ().swap(nextCoordi_);
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

  wireLengthGradSum_ = 0;
  densityGradSum_ = 0;
  cout << "densityPanelty_: " << densityPanelty_ << endl;
  for(int i=0; i<nb_->gCells().size(); i++) {
    GCell* gCell = nb_->gCells().at(i);
    wireLengthGrads[i] = nb_->getWireLengthGradientWA(
        gCell, wireLengthCoeffX_, wireLengthCoeffY_);
    densityGrads[i] = nb_->getDensityGradient(gCell); 

    wireLengthGradSum_ += fabs(wireLengthGrads[i].x) + fabs(wireLengthGrads[i].y);
    densityGradSum_ += fabs(densityGrads[i].x) + fabs(densityGrads[i].y);

    sumGrads[i].x = wireLengthGrads[i].x + densityPanelty_ * densityGrads[i].x;
    sumGrads[i].y = wireLengthGrads[i].y + densityPanelty_ * densityGrads[i].y;

    FloatCoordi wireLengthPreCondi 
      = nb_->getWireLengthPreconditioner(gCell);
    FloatCoordi densityPrecondi
      = nb_->getDensityPreconditioner(gCell);

    FloatCoordi sumPrecondi(
        wireLengthPreCondi.x + densityPanelty_ * densityPrecondi.x,
        wireLengthPreCondi.y + densityPanelty_ * densityPrecondi.y);

    if( sumPrecondi.x <= npVars_.minPreconditioner ) {
      sumPrecondi.x = npVars_.minPreconditioner;
    }

    if( sumPrecondi.y <= npVars_.minPreconditioner ) {
      sumPrecondi.y = npVars_.minPreconditioner; 
    }
    
//    cout << "wx: " << wireLengthGrads[i].x << " dx: " << densityGrads[i].x;
//    cout << " tx: " << sumGrads[i].x << endl;
//    cout << "wy: " << wireLengthGrads[i].y << " dy: " << densityGrads[i].y;
//    cout << " ty: " << sumGrads[i].y << endl ;

    sumGrads[i].x /= sumPrecondi.x;
    sumGrads[i].y /= sumPrecondi.y; 
    cout << "sumPreCondi: " << sumPrecondi.x << " " << sumPrecondi.y << endl ;
    cout << "atx: " << sumGrads[i].x << " aty: " << sumGrads[i].y << endl << endl;
  }
}

void
NesterovPlace::doNesterovPlace() {
  cout << "nesterovPlace: " << endl;
  
  // backTracking variable.
  float curA = 1.0;

  // Core Nesterov Loop
  for(int i=0; i<npVars_.maxNesterovIter; i++) {
    
//    updateGradients(curSLPSumGrads_, curSLPWireLengthGrads_, curSLPDensityGrads_);
//    stepLength_  
//      = getStepLength (prevSLPCoordi_, prevSLPSumGrads_, curSLPCoordi_, curSLPSumGrads_);

    float prevA = curA;

    // here, prevA is a_(k), curA is a_(k+1)
    // See, the papers' Algorithm 4 section
    //
    curA = (1.0 + sqrt(4.0 * prevA * prevA + 1.0)) * 0.5;

    // coeff is (a_k -1) / ( a_(k+1)) in paper.
    float coeff = (prevA - 1.0)/curA;

    cout << "stepLength_: " << stepLength_ << endl; 
    // Back-Tracking loop
    int numBackTrak = 0;
    for(numBackTrak = 0; numBackTrak < npVars_.maxBackTrack; numBackTrak++) {
      
      // fill in nextCoordinates with given stepLength_
      for(int k=0; k<nb_->gCells().size(); k++) {
        FloatCoordi nextCoordi(
          curSLPCoordi_[k].x + stepLength_ * curSLPSumGrads_[k].x,
          curSLPCoordi_[k].y + stepLength_ * curSLPSumGrads_[k].y );

        FloatCoordi nextSLPCoordi(
          nextCoordi.x + coeff * (nextCoordi.x - curCoordi_[k].x),
          nextCoordi.y + coeff * (nextCoordi.y - curCoordi_[k].y));

        GCell* curGCell = nb_->gCells()[k];

        nextCoordi_[k] 
          = FloatCoordi( 
              nb_->getDensityCoordiLayoutInsideX( 
                curGCell, nextCoordi.x),
              nb_->getDensityCoordiLayoutInsideY(
                curGCell, nextCoordi.y));
        
        nextSLPCoordi_[k]
          = FloatCoordi(
              nb_->getDensityCoordiLayoutInsideX(
                curGCell, nextSLPCoordi.x),
              nb_->getDensityCoordiLayoutInsideY(
                curGCell, nextSLPCoordi.y));
      }
 

      nb_->updateGCellDensityCenterLocation(nextSLPCoordi_);
      nb_->updateDensityForceBin();
      nb_->updateWireLengthForceWA(wireLengthCoeffX_, wireLengthCoeffY_);

      updateGradients(nextSLPSumGrads_, nextSLPWireLengthGrads_, nextSLPDensityGrads_ );
  
      float newStepLength  
        = getStepLength (curSLPCoordi_, curSLPSumGrads_, nextSLPCoordi_, nextSLPSumGrads_);
     
      cout << "newStepLength: " << newStepLength << endl; 
      if( newStepLength > stepLength_ * 0.95) {
        stepLength_ = newStepLength;
        break;
      }
      else {
        stepLength_ = newStepLength;
      } 
    }

    cout << "numBackTrak: " << numBackTrak << endl;   
    updateNextIter(); 
  }

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
NesterovPlace::updateInitialPrevSLPCoordi() {
  for(int i=0; i<nb_->gCells().size(); i++) {
    GCell* curGCell = nb_->gCells()[i];


    float prevCoordiX 
      = curSLPCoordi_[i].x + npVars_.initialPrevCoordiUpdateCoeff 
      * curSLPSumGrads_[i].x;
  
    float prevCoordiY
      = curSLPCoordi_[i].y + npVars_.initialPrevCoordiUpdateCoeff
      * curSLPSumGrads_[i].y;
    
    FloatCoordi newCoordi( 
      nb_->getDensityCoordiLayoutInsideX( curGCell, prevCoordiX),
      nb_->getDensityCoordiLayoutInsideY( curGCell, prevCoordiY) );

    prevSLPCoordi_[i] = newCoordi;
    
    cout << "SLP: " << curSLPCoordi_[i].x << " " << curSLPSumGrads_[i].x ;
    cout << " new: " << newCoordi.x << endl;
    cout << "SLP: " << curSLPCoordi_[i].y << " " << curSLPSumGrads_[i].y ;
    cout << " new: " << newCoordi.y << endl;
  } 
}

void
NesterovPlace::updateNextIter() {
  // swap vector pointers
  std::swap(prevSLPCoordi_, curSLPCoordi_);
  std::swap(prevSLPWireLengthGrads_, curSLPWireLengthGrads_);
  std::swap(prevSLPDensityGrads_, curSLPDensityGrads_);
  std::swap(prevSLPSumGrads_, curSLPSumGrads_);
  
  std::swap(curSLPCoordi_, nextSLPCoordi_);
  std::swap(curSLPWireLengthGrads_, nextSLPWireLengthGrads_);
  std::swap(curSLPDensityGrads_, nextSLPDensityGrads_);
  std::swap(curSLPSumGrads_, nextSLPSumGrads_);

  std::swap(curCoordi_, nextCoordi_);
    
  sumOverflow_ = 
      static_cast<float>(nb_->overflowArea()) 
          / static_cast<float>(pb_->placeInstsArea());

  cout << "gradient: " << getSecondNorm(curSLPSumGrads_) << endl;
  cout << "phi     : " << nb_->sumPhi() << endl;
  cout << "overflow: " << sumOverflow_ << endl;
  updateWireLengthCoef(sumOverflow_);
  int32_t hpwl = nb_->getHpwl();
  cout << "prevHpwl: " << prevHpwl_ << endl; 
  cout << "hpwl    : " << hpwl << endl; 

  float phiCoeff = getPhiCoeff( 
      static_cast<float>(prevHpwl_ - hpwl) 
      / npVars_.referenceHpwl );
  densityPanelty_ *= phiCoeff;
  cout << "phiCoeff: " << phiCoeff << endl;
}

float
NesterovPlace::getStepLength(
    std::vector<FloatCoordi>& prevSLPCoordi_,
    std::vector<FloatCoordi>& prevSLPSumGrads_,
    std::vector<FloatCoordi>& curSLPCoordi_,
    std::vector<FloatCoordi>& curSLPSumGrads_ ) {
  float coordiDistance 
    = getDistance(prevSLPCoordi_, curSLPCoordi_);
  float gradDistance 
    = getDistance(prevSLPSumGrads_, curSLPSumGrads_);

  cout << "cDist: " << coordiDistance << endl;
  cout << "gDist: " << gradDistance << endl;
  cout << "calVal: " << 1.0 / gradDistance / coordiDistance << endl;

  return 1.0 / gradDistance / coordiDistance;
}

float
NesterovPlace::getPhiCoeff(float scaledDiffHpwl) {
  float retCoef 
    = (scaledDiffHpwl < 0)? 
    npVars_.maxBoundMuK : 
    npVars_.maxBoundMuK * pow( npVars_.maxBoundMuK, scaledDiffHpwl * -1.0 );
  retCoef = std::max(npVars_.minBoundMuK, retCoef);
  return retCoef;
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

static float
getSecondNorm(vector<FloatCoordi>& a) {
  float norm = 0;
  for(auto& coordi : a) {
    norm += coordi.x * coordi.x + coordi.y + coordi.y;
  }
  return sqrt( norm / (2.0*a.size()) ); 
}


}

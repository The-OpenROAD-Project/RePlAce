#include "placerBase.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "opendb/db.h"
#include <iostream>
using namespace std;

#include "plot.h"

namespace replace {

static float
getDistance(vector<FloatCoordi>& a, vector<FloatCoordi>& b);

static float
getSecondNorm(vector<FloatCoordi>& a);

NesterovPlaceVars::NesterovPlaceVars()
  : maxNesterovIter(2000), 
  maxBackTrack(10),
  verboseLevel(1),
  initDensityPenalty(0.0001),
  initWireLengthCoef(1.0),
  targetOverflow(0.1),
  minPhiCoef(0.95),
  maxPhiCoef(1.05),
  minPreconditioner(1.0),
  initialPrevCoordiUpdateCoef(100),
  referenceHpwl(446000000) {}

NesterovPlace::NesterovPlace() 
  : pb_(nullptr), nb_(nullptr), log_(nullptr), npVars_(), 
  wireLengthGradSum_(0), 
  densityGradSum_(0),
  stepLength_(0),
  densityPenalty_(0),
  baseWireLengthCoef_(0), 
  wireLengthCoefX_(0), 
  wireLengthCoefY_(0),
  prevHpwl_(0),
  isDiverged_(false) {}

NesterovPlace::NesterovPlace(
    NesterovPlaceVars npVars,
    std::shared_ptr<PlacerBase> pb, 
    std::shared_ptr<NesterovBase> nb,
    std::shared_ptr<Logger> log) 
: NesterovPlace() {
  npVars_ = npVars;
  pb_ = pb;
  nb_ = nb;
  log_ = log;
  init();
}

NesterovPlace::~NesterovPlace() {
  reset();
}

#ifdef ENABLE_CIMG_LIB
static PlotEnv pe;
#endif

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

  if( npVars_.verboseLevel > 3 ) {
    cout << "InitialHPWL: " << prevHpwl_ << endl;
  }

  // FFT update
  nb_->updateDensityForceBin();

  baseWireLengthCoef_ 
    = npVars_.initWireLengthCoef 
    / static_cast<float>(nb_->binSizeX() + nb_->binSizeY()) * 0.5;

  if( npVars_.verboseLevel > 3 ) {
    cout << "BaseWireLengthCoef: " << baseWireLengthCoef_ << endl;
  }
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(pb_->placeInstsArea());

  if( npVars_.verboseLevel > 3 ) {
    cout << "InitSumOverflow: " << sumOverflow_ << endl;
  }
  updateWireLengthCoef(sumOverflow_);

  if( npVars_.verboseLevel > 3 ) {
    cout << "WireLengthCoef: " << wireLengthCoefX_ << endl;
  }

  // WL update
  nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);
 
  // fill in curSLPSumGrads_, curSLPWireLengthGrads_, curSLPDensityGrads_ 
  updateGradients(
      curSLPSumGrads_, curSLPWireLengthGrads_,
      curSLPDensityGrads_);

  if( isDiverged_ ) {
    return;
  }

  // approximately fill in 
  // prevSLPCoordi_ to calculate lc vars
  updateInitialPrevSLPCoordi();

  // bin, FFT, wlen update with prevSLPCoordi.
  nb_->updateGCellDensityCenterLocation(prevSLPCoordi_);
  nb_->updateDensityForceBin();
  nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);
  
  // update previSumGrads_, prevSLPWireLengthGrads_, prevSLPDensityGrads_
  updateGradients(
      prevSLPSumGrads_, prevSLPWireLengthGrads_,
      prevSLPDensityGrads_);
  
  if( isDiverged_ ) {
    return;
  }
  
  if( npVars_.verboseLevel > 3 ) {
    cout << "WireLengthGradSum: " << wireLengthGradSum_ << endl;
    cout << "DensityGradSum: " << densityGradSum_ << endl;
  }

  densityPenalty_ 
    = (wireLengthGradSum_ / densityGradSum_ )
    * npVars_.initDensityPenalty; 
  
  if( npVars_.verboseLevel > 3 ) {
    cout << "InitDensityPenalty: " << densityPenalty_ << endl;
  }
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(pb_->placeInstsArea());
  
  if( npVars_.verboseLevel > 3 ) {
    cout << "PrevSumOverflow: " << sumOverflow_ << endl;
  }
  
  stepLength_  
    = getStepLength (prevSLPCoordi_, prevSLPSumGrads_, curSLPCoordi_, curSLPSumGrads_);


  if( npVars_.verboseLevel > 3 ) {
    cout << "InitialStepLength: " << stepLength_ << endl;
  }
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
// nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_); // WL update
//
void
NesterovPlace::updateGradients(
    std::vector<FloatCoordi>& sumGrads,
    std::vector<FloatCoordi>& wireLengthGrads,
    std::vector<FloatCoordi>& densityGrads) {

  wireLengthGradSum_ = 0;
  densityGradSum_ = 0;

  float gradSum = 0;

  if( npVars_.verboseLevel > 3 ) {
    cout << "  DensityPenalty: " << densityPenalty_ << endl;
  }

  for(size_t i=0; i<nb_->gCells().size(); i++) {
    GCell* gCell = nb_->gCells().at(i);
    wireLengthGrads[i] = nb_->getWireLengthGradientWA(
        gCell, wireLengthCoefX_, wireLengthCoefY_);
    densityGrads[i] = nb_->getDensityGradient(gCell); 

    // Different compiler has different results on the following formula.
    // e.g. wireLengthGradSum_ += fabs(~~.x) + fabs(~~.y);
    //
    // To prevent instability problem,
    // I partitioned the fabs(~~.x) + fabs(~~.y) as two terms.
    //
    wireLengthGradSum_ += fabs(wireLengthGrads[i].x);
    wireLengthGradSum_ += fabs(wireLengthGrads[i].y);
      
    densityGradSum_ += fabs(densityGrads[i].x);
    densityGradSum_ += fabs(densityGrads[i].y);

    sumGrads[i].x = wireLengthGrads[i].x + densityPenalty_ * densityGrads[i].x;
    sumGrads[i].y = wireLengthGrads[i].y + densityPenalty_ * densityGrads[i].y;

    FloatCoordi wireLengthPreCondi 
      = nb_->getWireLengthPreconditioner(gCell);
    FloatCoordi densityPrecondi
      = nb_->getDensityPreconditioner(gCell);

    FloatCoordi sumPrecondi(
        wireLengthPreCondi.x + densityPenalty_ * densityPrecondi.x,
        wireLengthPreCondi.y + densityPenalty_ * densityPrecondi.y);

    if( sumPrecondi.x <= npVars_.minPreconditioner ) {
      sumPrecondi.x = npVars_.minPreconditioner;
    }

    if( sumPrecondi.y <= npVars_.minPreconditioner ) {
      sumPrecondi.y = npVars_.minPreconditioner; 
    }
    
    sumGrads[i].x /= sumPrecondi.x;
    sumGrads[i].y /= sumPrecondi.y; 

    gradSum += fabs(sumGrads[i].x) + fabs(sumGrads[i].y);
  }
  
  if( npVars_.verboseLevel > 3 ) {
    cout << "  WireLengthGradSum: " << wireLengthGradSum_ << endl;
    cout << "  DensityGradSum: " << densityGradSum_ << endl;
    cout << "  GradSum: " << gradSum << endl;
  }

  // divergence detection on 
  // Wirelength / density gradient calculation
  if( isnan(wireLengthGradSum_) || isinf(wireLengthGradSum_) ||
      isnan(densityGradSum_) || isinf(densityGradSum_) ) {
    cout << "INFO: RePlAce divergence detected. " << endl;
    cout << "      Please decrease init_wirelength_coeff value" << endl;
    isDiverged_ = true;
  }
}

void
NesterovPlace::doNesterovPlace() {

  // if replace diverged in init() function, 
  // replace must be skipped.
  if( isDiverged_ ) {
    cout << "INFO: RePlAce diverged. Please tune the parameters again" << endl;
    return;
  }

#ifdef ENABLE_CIMG_LIB  
  pe.setPlacerBase(pb_);
  pe.setNesterovBase(nb_);
  pe.Init();
#endif


  // backTracking variable.
  float curA = 1.0;

  // divergence detection
  float minSumOverflow = 1e30;
  float hpwlWithMinSumOverflow = 1e30;

  // dynamic adjustment of max_phi_coef
  bool isMaxPhiCoefChanged = false;

  // Core Nesterov Loop
  for(int i=0; i<npVars_.maxNesterovIter; i++) {
    if( npVars_.verboseLevel > 3 ) {
      cout << "Iter: " << i+1 << endl;
    }
    
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
    
    if( npVars_.verboseLevel > 3 ) {
      cout << "  PreviousA: " << prevA << endl;
      cout << "  CurrentA: " << curA << endl;
      cout << "  Coefficient: " << coeff << endl;
      cout << "  StepLength: " << stepLength_ << endl; 
    }
    // Back-Tracking loop
    int numBackTrak = 0;
    for(numBackTrak = 0; numBackTrak < npVars_.maxBackTrack; numBackTrak++) {
      
      // fill in nextCoordinates with given stepLength_
      for(size_t k=0; k<nb_->gCells().size(); k++) {
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
      nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

      updateGradients(nextSLPSumGrads_, nextSLPWireLengthGrads_, nextSLPDensityGrads_ );

      // NaN or inf is detected in WireLength/Density Coef 
      if( isDiverged_ ) {
        break;
      }
  
      float newStepLength  
        = getStepLength (curSLPCoordi_, curSLPSumGrads_, nextSLPCoordi_, nextSLPSumGrads_);
     
      if( npVars_.verboseLevel > 3 ) {
        cout << "  NewStepLength: " << newStepLength << endl; 
      }

      if( newStepLength > stepLength_ * 0.95) {
        stepLength_ = newStepLength;
        break;
      }
      else {
        stepLength_ = newStepLength;
      } 
    }

    if( npVars_.verboseLevel > 3 ) {
      cout << "  NumBackTrak: " << numBackTrak << endl;   
    }

    // dynamic adjustment for
    // better convergence with
    // large designs 
    if( !isMaxPhiCoefChanged && sumOverflow_ 
        < 0.35f ) {
      isMaxPhiCoefChanged = true;
      npVars_.maxPhiCoef *= 0.99;
    }

    // usually, maxBackTrack should be 1~3
    // 10 is the case when
    // all of cells are not moved at all.
    if( npVars_.maxBackTrack == numBackTrak ) {
      cout << "INFO: RePlAce divergence detected" << endl;
      cout << "      Please decrease init_density_penalty" << endl;
      isDiverged_ = true;
    } 

    if( isDiverged_ ) {
      break;
    }

    updateNextIter(); 


    // For JPEG Saving
    // debug

    if( i == 0 || (i+1) % 10 == 0 ) {
      cout << "[NesterovSolve] Iter: " << i+1 
        << " overflow: " << sumOverflow_ << " HPWL: " << prevHpwl_ << endl; 
#ifdef ENABLE_CIMG_LIB
      pe.SaveCellPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)), true,
          string("./plot/cell/cell_") +
          std::to_string (i+1));
      pe.SaveBinPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)),
          string("./plot/bin/bin_") +
          std::to_string(i+1));
      pe.SaveArrowPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)),
          string("./plot/arrow/arrow_") +
          std::to_string(i+1));
#endif
    }

    if( minSumOverflow > sumOverflow_ ) {
      minSumOverflow = sumOverflow_;
      hpwlWithMinSumOverflow = prevHpwl_; 
    }

    // diverge detection on
    // large max_phi_cof value + large design 
    //
    // 1) happen overflow < 20%
    // 2) Hpwl is growing
    //
    if( sumOverflow_ < 0.3f 
        && sumOverflow_ - minSumOverflow >= 0.02f
        && hpwlWithMinSumOverflow * 1.2f < prevHpwl_ ) {
      cout << "INFO: RePlAce divergence detected" << endl;
      cout << "      Please decrease max_phi_cof" << endl;
      isDiverged_ = true;
      break;
    }

    // minimum iteration is 50
    if( i > 50 && sumOverflow_ <= npVars_.targetOverflow) {
      cout << "[NesterovSolve] Finished with Overflow: " << sumOverflow_ << endl;
      break;
    }
  }

  if( isDiverged_ ) { 
    cout << "INFO: RePlAce diverged. Please tune the parameters again" << endl;
  }

  updateDb();
}

void
NesterovPlace::updateWireLengthCoef(float overflow) {
  if( overflow > 1.0 ) {
    wireLengthCoefX_ = wireLengthCoefY_ = 0.1;
  }
  else if( overflow < 0.1 ) {
    wireLengthCoefX_ = wireLengthCoefY_ = 10.0;
  }
  else {
    wireLengthCoefX_ = wireLengthCoefY_ 
      = 1.0 / pow(10.0, (overflow-0.1)*20 / 9.0 - 1.0);
  }

  wireLengthCoefX_ *= baseWireLengthCoef_;
  wireLengthCoefY_ *= baseWireLengthCoef_;
  if( npVars_.verboseLevel > 3 ) {
    cout << "  NewWireLengthCoef: " << wireLengthCoefX_ << endl;
  }
}

void
NesterovPlace::updateInitialPrevSLPCoordi() {
  for(size_t i=0; i<nb_->gCells().size(); i++) {
    GCell* curGCell = nb_->gCells()[i];


    float prevCoordiX 
      = curSLPCoordi_[i].x + npVars_.initialPrevCoordiUpdateCoef 
      * curSLPSumGrads_[i].x;
  
    float prevCoordiY
      = curSLPCoordi_[i].y + npVars_.initialPrevCoordiUpdateCoef
      * curSLPSumGrads_[i].y;
    
    FloatCoordi newCoordi( 
      nb_->getDensityCoordiLayoutInsideX( curGCell, prevCoordiX),
      nb_->getDensityCoordiLayoutInsideY( curGCell, prevCoordiY) );

    prevSLPCoordi_[i] = newCoordi;
    
//    cout << "SLP: " << curSLPCoordi_[i].x << " " << curSLPSumGrads_[i].x ;
//    cout << " new: " << newCoordi.x << endl;
//    cout << "SLP: " << curSLPCoordi_[i].y << " " << curSLPSumGrads_[i].y ;
//    cout << " new: " << newCoordi.y << endl;
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

  if( npVars_.verboseLevel > 3 ) {
    cout << "  Gradient: " << getSecondNorm(curSLPSumGrads_) << endl;
    cout << "  Phi: " << nb_->sumPhi() << endl;
    cout << "  Overflow: " << sumOverflow_ << endl;
  }

  updateWireLengthCoef(sumOverflow_);
  int64_t hpwl = nb_->getHpwl();
  
  if( npVars_.verboseLevel > 3 ) {
    cout << "  PreviousHpwl: " << prevHpwl_ << endl; 
    cout << "  NewHpwl: " << hpwl << endl; 
  }
  

  float phiCoef = getPhiCoef( 
      static_cast<float>(hpwl - prevHpwl_) 
      / npVars_.referenceHpwl );
  
  prevHpwl_ = hpwl;
  densityPenalty_ *= phiCoef;
  
  if( npVars_.verboseLevel > 3 ) {
    cout << "  PhiCoef: " << phiCoef << endl;
    cout << endl << endl;
  }
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

//  coordiDistance *= 0.001;

  if( npVars_.verboseLevel > 3) {
    cout << "  CoordinateDistance: " << coordiDistance << endl;
    cout << "  GradientDistance: " << gradDistance << endl;
  }

  return coordiDistance / gradDistance;
}

float
NesterovPlace::getPhiCoef(float scaledDiffHpwl) {
  if( npVars_.verboseLevel > 3) {
    cout << "  InputScaleDiffHPWL: " << scaledDiffHpwl << endl;
  }

  float retCoef 
    = (scaledDiffHpwl < 0)? 
    npVars_.maxPhiCoef: 
    npVars_.maxPhiCoef * pow( npVars_.maxPhiCoef, scaledDiffHpwl * -1.0 );
  retCoef = std::max(npVars_.minPhiCoef, retCoef);
  return retCoef;
}

void
NesterovPlace::updateDb() {
  for(auto& gCell : nb_->gCells()) {
    if( gCell->isInstance() ) {
      odb::dbInst* inst = gCell->instance()->dbInst();
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED); 
      inst->setLocation( gCell->dCx()-gCell->dDx()/2,
           gCell->dCy()-gCell->dDy()/2 ); 
    }
  }
}


static float
getDistance(vector<FloatCoordi>& a, vector<FloatCoordi>& b) {
  float sumDistance = 0.0f;
  for(size_t i=0; i<a.size(); i++) {
    sumDistance += (a[i].x - b[i].x) * (a[i].x - b[i].x);
    sumDistance += (a[i].y - b[i].y) * (a[i].y - b[i].y);
  }

  return sqrt( sumDistance / (2.0 * a.size()) );
}

static float
getSecondNorm(vector<FloatCoordi>& a) {
  float norm = 0;
  for(auto& coordi : a) {
    norm += coordi.x * coordi.x + coordi.y * coordi.y;
  }
  return sqrt( norm / (2.0*a.size()) ); 
}


}

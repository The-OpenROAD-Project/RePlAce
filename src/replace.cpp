#include "replace/Replace.h"
#include "initialPlace.h"
#include "nesterovPlace.h"
#include "placerBase.h"
#include "nesterovBase.h"
#include "logger.h"
#include <iostream>

namespace replace {

using namespace std;

Replace::Replace()
  : db_(nullptr), 
  sta_(nullptr), 
  pb_(nullptr), nb_(nullptr), 
  ip_(nullptr), np_(nullptr),
  log_(nullptr),
  initialPlaceMaxIter_(20), 
  initialPlaceMinDiffLength_(1500),
  initialPlaceMaxSolverIter_(100),
  initialPlaceMaxFanout_(200),
  initialPlaceNetWeightScale_(800),
  nesterovPlaceMaxIter_(2000),
  binGridCntX_(0), binGridCntY_(0), 
  overflow_(0.1), density_(1.0),
  initDensityPenalityFactor_(0.00008), 
  initWireLengthCoef_(0.25),
  minPhiCoef_(0.95), maxPhiCoef_(1.05),
  referenceHpwl_(446000000),
  incrementalPlaceMode_(false),
  verbose_(0) {
};

Replace::~Replace() {
  reset();
}

void Replace::init() {
}

void Replace::reset() {
  // two pointers should not be freed.
  db_ = nullptr;
  sta_ = nullptr;

  initialPlaceMaxIter_ = 20;
  initialPlaceMinDiffLength_ = 1500;
  initialPlaceMaxSolverIter_ = 100;
  initialPlaceMaxFanout_ = 200;
  initialPlaceNetWeightScale_ = 800;

  nesterovPlaceMaxIter_ = 2000;
  binGridCntX_ = binGridCntY_ = 0;
  overflow_ = 0;
  density_ = 0;
  initDensityPenalityFactor_ = 0.0001;
  initWireLengthCoef_ = 1.0;
  minPhiCoef_ = 0.95;
  maxPhiCoef_ = 1.05;
  referenceHpwl_= 446000000;

  incrementalPlaceMode_ = false;
  verbose_ = 0;
}

void Replace::setDb(odb::dbDatabase* db) {
  db_ = db;
}
void Replace::setSta(sta::dbSta* sta) {
  sta_ = sta;
}
void Replace::doInitialPlace() {
  log_ = std::make_shared<Logger>("REPL", verbose_);
  pb_ = std::make_shared<PlacerBase>(db_, log_);

  InitialPlaceVars ipVars;
  ipVars.maxIter = initialPlaceMaxIter_;
  ipVars.minDiffLength = initialPlaceMinDiffLength_;
  ipVars.maxSolverIter = initialPlaceMaxSolverIter_;
  ipVars.maxFanout = initialPlaceMaxFanout_;
  ipVars.netWeightScale = initialPlaceNetWeightScale_;
  ipVars.incrementalPlaceMode = incrementalPlaceMode_;
  
  std::unique_ptr<InitialPlace> ip(new InitialPlace(ipVars, pb_, log_));
  ip_ = std::move(ip);
  ip_->doBicgstabPlace();
}

void Replace::doNesterovPlace() {
  if( !log_ ) {
    log_ = std::make_shared<Logger>("REPL", verbose_);
  }

  if( !pb_ ) {
    pb_ = std::make_shared<PlacerBase>(db_, log_);
  }

  NesterovBaseVars nbVars;
  nbVars.targetDensity = density_;
  
  if( binGridCntX_ != 0 ) {
    nbVars.isSetBinCntX = 1;
    nbVars.binCntX = binGridCntX_;
  }

  if( binGridCntY_ != 0 ) {
    nbVars.isSetBinCntY = 1;
    nbVars.binCntY = binGridCntY_;
  }

  nb_ = std::make_shared<NesterovBase>(nbVars, pb_, log_);

  NesterovPlaceVars npVars;

  npVars.minPhiCoef = minPhiCoef_;
  npVars.maxPhiCoef = maxPhiCoef_;
  npVars.referenceHpwl = referenceHpwl_;
  npVars.initDensityPenalty = initDensityPenalityFactor_;
  npVars.initWireLengthCoef = initWireLengthCoef_;
  npVars.targetOverflow = overflow_;
  npVars.maxNesterovIter = nesterovPlaceMaxIter_; 

  std::unique_ptr<NesterovPlace> np(new NesterovPlace(npVars, pb_, nb_, log_));
  np_ = std::move(np);

  np_->doNesterovPlace();
}


void
Replace::setInitialPlaceMaxIter(int iter) {
  initialPlaceMaxIter_ = iter; 
}

void
Replace::setInitialPlaceMinDiffLength(int length) {
  initialPlaceMinDiffLength_ = length; 
}

void
Replace::setInitialPlaceMaxSolverIter(int iter) {
  initialPlaceMaxSolverIter_ = iter;
}

void
Replace::setInitialPlaceMaxFanout(int fanout) {
  initialPlaceMaxFanout_ = fanout;
}

void
Replace::setInitialPlaceNetWeightScale(float scale) {
  initialPlaceNetWeightScale_ = scale;
}

void
Replace::setNesterovPlaceMaxIter(int iter) {
  nesterovPlaceMaxIter_ = iter;
}

void 
Replace::setBinGridCntX(int binGridCntX) {
  binGridCntX_ = binGridCntX;
}

void 
Replace::setBinGridCntY(int binGridCntY) {
  binGridCntY_ = binGridCntY;
}

void 
Replace::setTargetOverflow(float overflow) {
  overflow_ = overflow;
}

void
Replace::setTargetDensity(float density) {
  density_ = density;
}

void
Replace::setInitDensityPenalityFactor(float penaltyFactor) {
  initDensityPenalityFactor_ = penaltyFactor;
}

void
Replace::setInitWireLengthCoef(float coef) {
  initWireLengthCoef_ = coef;
}

void
Replace::setMinPhiCoef(float minPhiCoef) {
  minPhiCoef_ = minPhiCoef;
}

void
Replace::setMaxPhiCoef(float maxPhiCoef) {
  maxPhiCoef_ = maxPhiCoef;
}

void
Replace::setReferenceHpwl(float refHpwl) {
  referenceHpwl_ = refHpwl;
}

void
Replace::setIncrementalPlaceMode(bool mode) {
  incrementalPlaceMode_ = mode;
}

void
Replace::setVerboseLevel(int verbose) {
  verbose_ = verbose;
}

}


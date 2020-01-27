#include "replace/Replace.h"
#include "initialPlace.h"
#include "nesterovPlace.h"
#include "placerBase.h"
#include "nesterovBase.h"
#include <iostream>

namespace replace {

using namespace std;

Replace::Replace()
  : db_(nullptr), 
  sta_(nullptr), 
  pb_(nullptr), nb_(nullptr), 
  ip_(nullptr), np_(nullptr),
  initialPlaceMaxIter_(20), 
  initialPlaceMinDiffLength_(1500),
  initialPlaceMaxSolverIter_(100),
  initialPlaceNetWeightScale_(800),
  nesterovPlaceMaxIter_(2000),
  binGridCntX_(0), binGridCntY_(0), 
  overflow_(0.1), density_(1.0),
  initPenalityFactor_(0.00001), 
  minPCoef_(0.95), maxPCoef_(1.05),
  deltaHpwl_(346000),
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
  initialPlaceNetWeightScale_ = 800;

  nesterovPlaceMaxIter_ = 2000;
  binGridCntX_ = binGridCntY_ = 0;
  overflow_ = 0;
  density_ = 0;
  initPenalityFactor_ = 0;
  minPCoef_ = 0;
  maxPCoef_ = 0;
  deltaHpwl_ = 0;
  verbose_ = 0;
}

void Replace::setDb(odb::dbDatabase* db) {
  db_ = db;
}
void Replace::setSta(sta::dbSta* sta) {
  sta_ = sta;
}
void Replace::doInitialPlace() {
  pb_ = std::make_shared<PlacerBase>(db_);

  InitialPlaceVars ipVars;
  ipVars.maxIter = initialPlaceMaxIter_;
  ipVars.minDiffLength = initialPlaceMinDiffLength_;
  ipVars.maxSolverIter = initialPlaceMaxSolverIter_;
  ipVars.netWeightScale = initialPlaceNetWeightScale_;
  ipVars.verbose = verbose_;
  
  std::unique_ptr<InitialPlace> ip(new InitialPlace(ipVars, pb_));
  ip_ = std::move(ip);
  ip_->doBicgstabPlace();
}

void Replace::doNesterovPlace() {
  if( !pb_ ) {
    pb_ = std::make_shared<PlacerBase>(db_);
  }

  NesterovBaseVars nbVars;
  nbVars.targetDensity = density_;

  nb_ = std::make_shared<NesterovBase>(nbVars, pb_);

  NesterovPlaceVars npVars;
  std::unique_ptr<NesterovPlace> np(new NesterovPlace(npVars, pb_, nb_));
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
Replace::setInitPenalityFactor(float penaltyFactor) {
  initPenalityFactor_ = penaltyFactor;
}

void
Replace::setMinPCoef(float minPCoef) {
  minPCoef_ = minPCoef;
}

void
Replace::setMaxPCoef(float maxPCoef) {
  maxPCoef_ = maxPCoef;
}

void
Replace::setDeltaHpwl(float deltaHpwl) {
  deltaHpwl_ = deltaHpwl;
}

void
Replace::setVerboseLevel(int verbose) {
  verbose_ = verbose;
}

}


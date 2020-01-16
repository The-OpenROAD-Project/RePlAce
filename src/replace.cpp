#include "replace/Replace.h"
#include "initialPlace.h"
#include "nesterovPlace.h"
#include "placerBase.h"
#include <iostream>

namespace replace {

using namespace std;

Replace::Replace()
  : db_(nullptr), 
  sta_(nullptr), ip_(nullptr), 
  np_(nullptr), pb_(nullptr),
  maxInitialPlaceIter_(20), 
  maxNesterovPlaceIter_(2000),
  binGridCntX_(0), binGridCntY_(0), 
  overflow_(0.1), density_(1.0),
  lambda_(0.00001), 
  minPCoef_(0.95), maxPCoef_(1.05),
  deltaHpwl_(346000),
  verbose_(0) {
};

Replace::~Replace() {
  clear();
}

void Replace::init() {
  pb_ = new PlacerBase(db_);
  ip_ = new InitialPlace(pb_);
  np_ = new NesterovPlace(pb_);
}

void Replace::clear() {
  // two pointers should not be freed.
  db_ = nullptr;
  sta_ = nullptr;

  // below objects were from replace
  if( pb_ ) {
    delete pb_;
  }
  pb_ = nullptr;

  if( ip_ ) {
    delete ip_;
  }
  ip_ = nullptr;

  if( np_ ) {
    delete np_;
  }
  np_ = nullptr;

  maxInitialPlaceIter_ = 0;
  maxNesterovPlaceIter_ = 0;
  binGridCntX_ = binGridCntY_ = 0;
  overflow_ = 0;
  density_ = 0;
  lambda_ = 0;
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
  if( !pb_ || !ip_ || !np_ ) {
    init();
  }

  InitialPlaceVars ipVars;
  ipVars.maxInitialPlaceIter = maxInitialPlaceIter_;
  ipVars.verbose = verbose_;

  ip_->setInitialPlaceVars(ipVars);
  ip_->doBicgstabPlace();
}

void Replace::doNesterovPlace() {
  if( !pb_ || !ip_ || !np_ ) {
    init();
  }
  np_->doNesterovPlace();

}


void
Replace::setMaxInitialPlaceIter(int iter) {
  maxInitialPlaceIter_ = iter; 
}
void
Replace::setMaxNesvPlaceIter(int iter) {
  maxNesterovPlaceIter_ = iter;
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
Replace::setInitLambda(float lambda) {
  lambda_ = lambda;
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


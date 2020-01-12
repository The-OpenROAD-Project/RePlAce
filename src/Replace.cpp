#include "replace/Replace.h"

namespace replace {

Replace::Replace()
  : odb_(nullptr), sta_(nullptr), init_place_(nullptr), 
  nesterov_place_(nullptr), maxInitPlaceIter_(20), 
  binCntX_(0), binCntY_(0), 
  overflow_(0.1), density_(1.0),
  lambda_(0.00001), minPCoef_(0.95), maxPCoef_(1.05),
  deltaHpwl_(346000) {};

Replace::~Replace() {
  clear();
}


void Replace::init() {
  odb_ = nullptr;
  sta_ = nullptr;
  init_place_ = new InitPlace();
  nesterov_place_ = new NesterovPlace();
  
}

void Replace::clear() {
  odb_ = nullptr;
  sta_ = nullptr;
  init_place_ = nullptr;
  nesterov_place_ = nullptr;

  maxInitPlaceIter_ = 0;
  maxNesterovPlaceIter_ = 0;
  binCntX_ = binCntY_ = 0;
  overflow_ = 0;
  density_ = 0;
  lambda_ = 0;
  minPCoef = 0;
  maxPCoef = 0;
  deltaHpwl_ = 0;
}

void Replace::setDb(odb::dbDatabase* odb) {
  odb_ = odb;
}
void Replace::setSta(sta::dbSta* sta) {
  sta_ = sta;
}
void Replace::doInitPlace() {

}
void Replace::doNesterovPlace() {

}

}


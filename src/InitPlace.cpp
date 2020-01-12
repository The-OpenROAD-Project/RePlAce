#include "InitPlace.h"

namespace replace {

InitPlaceVars::InitPlaceVars() 
  : maxInitPlaceIter(0) {}

void InitPlaceVars::clear() {
  maxInitPlaceIter = 0;
}

InitPlace::InitPlace()
: db_(nullptr), initPlaceVars_(), placeInsts_(nullptr) {};

InitPlace::~InitPlace() {
  clear();
}

void InitPlace::clear()
  db_ = nullptr;
  initPlaceVars_.clear();
  placeInsts_ = nullptr;
}

void InitPlace::setDb(odb::dbDatabase* db) {
  db_ = db;
}

void InitPlace::setInitPlaceVars(InitPlaceVars initPlaceVars) {
  initPlaceVars_ = initPlaceVars;
}

void InitPlace::setPlaceInsts(odb::dbSet<odb::dbInst>* placeInsts) {
  placeInsts_ = placeInsts;
}

void InitPlace::doInitPlace() {
  for(int i=1; i<=initPlaceVars_.maxInitPlaceIter; i++) {
    
  }
}

// solve matX_ * xcg_x_ = xcg_b_ and matY_ * ycg_x_ = ycg_b_ eq.
void InitPlace::createSparseMatrix() {
  const int placeCnt = placeInsts->size();
  xcgX_.resize( placeCnt );
  xcgB_.resize( placeCnt );
  ycgX_.resize( placeCnt );
  ycgB_.resize( placeCnt );

  matX_.resize( placeCnt, placeCnt );
  matY_.resize( placeCnt, placeCnt );


}

}

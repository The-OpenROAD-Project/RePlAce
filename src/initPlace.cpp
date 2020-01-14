#include "initPlace.h"
#include "placerBase.h"

namespace replace {

InitPlaceVars::InitPlaceVars() 
  : maxInitPlaceIter(0), verbose(0) {}

void InitPlaceVars::clear() {
  maxInitPlaceIter = 0;
  verbose = 0;
}

InitPlace::InitPlace()
: pb_(nullptr), initPlaceVars_() {};

InitPlace::InitPlace(PlacerBase* placerBase)
: pb_(placerBase), initPlaceVars_() {};

InitPlace::~InitPlace() {
  clear();
}

void InitPlace::clear() {
  pb_ = nullptr;
  initPlaceVars_.clear();
}

void InitPlace::setInitPlaceVars(InitPlaceVars initPlaceVars) {
  initPlaceVars_ = initPlaceVars;
}

void InitPlace::doInitPlace() {
  for(int i=1; i<=initPlaceVars_.maxInitPlaceIter; i++) {
    updatePinInfo();
    createSparseMatrix();
  }
}

void InitPlace::updatePinInfo() {
  // clear all MinMax attributes
  for(auto& pin : pb_->pins()) {
    pin.unsetMinPinX();
    pin.unsetMinPinY();
    pin.unsetMaxPinX();
    pin.unsetMaxPinY();
  }

  for(auto& net : pb_->nets()) {
    Pin* pinMinX = nullptr, *pinMinY = nullptr;
    Pin* pinMaxX = nullptr, *pinMaxY = nullptr;  
    int lx = INT_MAX, ly = INT_MAX;
    int ux = INT_MIN, uy = INT_MIN;

    // Mark B2B info on Pin structures
    for(auto& pin : net.pins()){
      if( pinMinX ) {
        if( lx > pin->cx() ) {
          lx = pinMinX->cx();
          pinMinX->unsetMinPinX();
          pinMinX = pin; 
          pinMinX->setMinPinX();
        }
      } 
      else {
        lx = pin->cx();
        pinMinX = pin;
        pinMinX->setMinPinX();
      }
      
      if( pinMaxX ) {
        if( ux < pin->cx() ) {
          ux = pinMinX->cx();
          pinMaxX->unsetMaxPinX();
          pinMaxX = pin; 
          pinMaxX->setMaxPinX();
        }
      } 
      else {
        ux = pin->cx();
        pinMaxX = pin;
        pinMaxX->setMaxPinX();
      }

      if( pinMinY ) {
        if( ly > pin->cy() ) {
          ly = pinMinY->cy();
          pinMinY->unsetMinPinY();
          pinMinY = pin; 
          pinMinY->setMinPinY();
        }
      } 
      else {
        ly = pin->cy();
        pinMinY = pin;
        pinMinY->setMinPinY();
      }
      
      if( pinMaxY ) {
        if( uy < pin->cy() ) {
          uy = pinMinY->cy();
          pinMaxY->unsetMaxPinY();
          pinMaxY = pin; 
          pinMaxY->setMaxPinY();
        }
      } 
      else {
        uy = pin->cy();
        pinMaxY = pin;
        pinMaxY->setMaxPinY();
      }
    }
  } 
}

// solve matX_ * xcg_x_ = xcg_b_ and matY_ * ycg_x_ = ycg_b_ eq.
void InitPlace::createSparseMatrix() {
  const int placeCnt = pb_->placeInsts().size();
  xcgX_.resize( placeCnt );
  xcgB_.resize( placeCnt );
  ycgX_.resize( placeCnt );
  ycgB_.resize( placeCnt );

  matX_.resize( placeCnt, placeCnt );
  matY_.resize( placeCnt, placeCnt );


}

}

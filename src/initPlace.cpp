#include "initPlace.h"
#include "placerBase.h"
#include <iostream>

namespace replace {
using namespace std;

typedef Eigen::Triplet< int > T;

InitPlaceVars::InitPlaceVars() 
  : maxInitPlaceIter(0), 
  minDiffLength(25), 
  verbose(0) {}

void InitPlaceVars::clear() {
  maxInitPlaceIter = 0;
  minDiffLength = 0;
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
  if( initPlaceVars_.verbose > 0 ) {
    cout << "Begin InitPlace ..." << endl;
  }
  
  placeInstsCenter();
  // set ExtId for idx reference // easy recovery
  setPlaceInstExtId();
  for(int i=1; i<=initPlaceVars_.maxInitPlaceIter; i++) {
    updatePinInfo();
    createSparseMatrix();
  }
}

// starting point of initial place is center.
void InitPlace::placeInstsCenter() {
  for(auto& inst: pb_->insts()) {
     
  }
}

void InitPlace::setPlaceInstExtId() {
  // clear ExtId for all instances
  for(auto& inst : pb_->insts()) {
    inst.setExtId(INT_MAX);
  }
  // set index only with place-able instances
  for(auto& inst : pb_->placeInsts()) {
    inst->setExtId(&inst - &(pb_->placeInsts()[0]));
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
    for(auto& pin : net.pins()) {
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


  vector< T > listX, listY;
  listX.reserve(1000000);
  listY.reserve(1000000);

  // for each net
  for(auto& net : pb_->nets()) {

    // skip for small nets.
    if( net.pins().size() <= 1 ) {
      continue;
    }

    float netWeight = 1.0 / (net.pins().size() - 1);

    // foreach two pins in single nets.
    for(auto& pin1 : net.pins()) {
      int pinIdx1 = &pin1 - &(net.pins()[0]);
      for(auto& pin2 : net.pins()) {
        int pinIdx2 = &pin2 - &(net.pins()[0]);

        // 
        // will compare two pins "only once."
        //
        if( pinIdx1 >= pinIdx2 ) {
          continue;
        }

        // no need to fill in when instance is same
        if( pin1->instance() == pin2->instance() ) {
          continue;
        }

        if( pin1->isMinPinX() || pin1->isMaxPinX() ||
            pin2->isMinPinX() || pin2->isMaxPinX() ) {
          int diffX = abs(pin1->cx() - pin2->cx());
          float weightX = 0;
          if( diffX > initPlaceVars_.minDiffLength ) {
            weightX = netWeight / diffX;
          }
          else {
            weightX = netWeight / initPlaceVars_.minDiffLength;
          }

          // both pin cames from instance
          if( pin1->instance() && pin2->instance() ) {
            // pin1->extId()
          }
          // pin1 from IO port / pin2 from Instance
          else if( !pin1->instance() && pin2->instance() ) {

          }
          // pin1 from Instance / pin2 from IO port
          else if( pin1->instance() && !pin2->instance() ) {

          }


          




        }

      }
    }

  } 


}

}

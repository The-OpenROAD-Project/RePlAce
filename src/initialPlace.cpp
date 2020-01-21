#include "initialPlace.h"
#include "placerBase.h"
#include <iostream>

#include <Eigen/IterativeLinearSolvers>

namespace replace {
using namespace std;

using Eigen::BiCGSTAB;
using Eigen::IdentityPreconditioner;

typedef Eigen::Triplet< float > T;

InitialPlaceVars::InitialPlaceVars() 
  : maxIter(20), 
  minDiffLength(1500), 
  maxSolverIter(100),
  netWeightScale(800.0),
  verbose(0) {}

void InitialPlaceVars::reset() {
  maxIter = 0;
  minDiffLength = 0;
  verbose = 0;
}

InitialPlace::InitialPlace()
: pb_(nullptr), initialPlaceVars_() {};

InitialPlace::InitialPlace(std::shared_ptr<PlacerBase> pb)
: pb_(pb), initialPlaceVars_() {};

InitialPlace::~InitialPlace() {
  reset();
}

void InitialPlace::reset() {
  pb_ = nullptr;
  initialPlaceVars_.reset();
}

void InitialPlace::setInitialPlaceVars(InitialPlaceVars initialPlaceVars) {
  initialPlaceVars_ = initialPlaceVars;
}

void InitialPlace::doBicgstabPlace() {
  if( initialPlaceVars_.verbose > 0 ) {
    cout << "Begin InitialPlace ..." << endl;
  }

  float errorX = 0.0f, errorY = 0.0f;
  
  placeInstsCenter();
  // set ExtId for idx reference // easy recovery
  setPlaceInstExtId();
  for(int i=1; i<=initialPlaceVars_.maxIter; i++) {
    updatePinInfo();
    createSparseMatrix();

    // BiCGSTAB solver for initial place
    BiCGSTAB< SMatrix, IdentityPreconditioner > solver;
    solver.setMaxIterations(initialPlaceVars_.maxSolverIter);
    solver.compute(matX_);
    xcgX_ = solver.solveWithGuess(xcgB_, xcgX_);
    errorX = solver.error();

    solver.compute(matY_);
    ycgX_ = solver.solveWithGuess(ycgB_, ycgX_);
    errorY = solver.error();

    cout << "[InitialPlace]  Iter: " << i 
      << " CG Error: " << max(errorX, errorY)
      << " HPWL: " << pb_->hpwl() << endl; 
    updateCoordi();
  }

  if( initialPlaceVars_.verbose > 0 ) {
    cout << "End InitialPlace" << endl;
  }
}

// starting point of initial place is center.
void InitialPlace::placeInstsCenter() {
  const int centerX = pb_->die().coreCx();
  const int centerY = pb_->die().coreCy();

  for(auto& inst: pb_->insts()) {
    inst->setCenterLocation(centerX, centerY);
  }
}

void InitialPlace::setPlaceInstExtId() {
  // reset ExtId for all instances
  for(auto& inst : pb_->insts()) {
    inst->setExtId(INT_MAX);
  }
  // set index only with place-able instances
  for(auto& inst : pb_->placeInsts()) {
    inst->setExtId(&inst - &(pb_->placeInsts()[0]));
  }
}

void InitialPlace::updatePinInfo() {
  // reset all MinMax attributes
  for(auto& pin : pb_->pins()) {
    pin->unsetMinPinX();
    pin->unsetMinPinY();
    pin->unsetMaxPinX();
    pin->unsetMaxPinY();
  }

  for(auto& net : pb_->nets()) {
    Pin* pinMinX = nullptr, *pinMinY = nullptr;
    Pin* pinMaxX = nullptr, *pinMaxY = nullptr;  
    int lx = INT_MAX, ly = INT_MAX;
    int ux = INT_MIN, uy = INT_MIN;

    // Mark B2B info on Pin structures
    for(auto& pin : net->pins()) {
      if( lx > pin->cx() ) {
        if( pinMinX ) {
          pinMinX->unsetMinPinX();
        }
        lx = pin->cx();
        pinMinX = pin; 
        pinMinX->setMinPinX();
      } 
      
      if( ux < pin->cx() ) {
        if( pinMaxX ) {
          pinMaxX->unsetMaxPinX();
        }
        ux = pin->cx();
        pinMaxX = pin; 
        pinMaxX->setMaxPinX();
      } 

      if( ly > pin->cy() ) {
        if( pinMinY ) {
          pinMinY->unsetMinPinY();
        }
        ly = pin->cy();
        pinMinY = pin; 
        pinMinY->setMinPinY();
      } 
      
      if( uy < pin->cy() ) {
        if( pinMaxY ) {
          pinMaxY->unsetMaxPinY();
        }
        uy = pin->cy();
        pinMaxY = pin; 
        pinMaxY->setMaxPinY();
      } 
    }
//    cout << "pinAddr: " << pinMinX << " " 
//      << pinMaxX << " " 
//      << pinMinY << " " 
//      << pinMaxY << endl;
//    cout << pinMinX->isMinPinX() << endl;

//      cout << "final bbox: " 
//        << pinMinX->cx() << " " 
//        << pinMinY->cy() << " " 
//        << pinMaxX->cx() << " " 
//        << pinMaxY->cy() << endl; 
  } 
}

// solve matX_ * xcg_x_ = xcg_b_ and matY_ * ycg_x_ = ycg_b_ eq.
void InitialPlace::createSparseMatrix() {
  const int placeCnt = pb_->placeInsts().size();
  xcgX_.resize( placeCnt );
  xcgB_.resize( placeCnt );
  ycgX_.resize( placeCnt );
  ycgB_.resize( placeCnt );

  matX_.resize( placeCnt, placeCnt );
  matY_.resize( placeCnt, placeCnt );


  // 
  // listX and listY is a temporary vector that have tuples, (idx1, idx2, val)
  //
  // listX finally becomes matX_
  // listY finally becomes matY_
  //
  // The triplet vector is recommended usages 
  // to fill in SparseMatrix from Eigen docs.
  //

  vector< T > listX, listY;
  listX.reserve(1000000);
  listY.reserve(1000000);

  // initialize vector
  for(auto& inst : pb_->placeInsts()) {
    int idx = inst->extId(); 
    
    xcgX_(idx) = inst->cx();
    ycgX_(idx) = inst->cy();

    xcgB_(idx) = ycgB_(idx) = 0;
  }

  // for each net
  for(auto& net : pb_->nets()) {

    // skip for small nets.
    if( net->pins().size() <= 1 ) {
      continue;
    }

    float netWeight = initialPlaceVars_.netWeightScale 
      / (net->pins().size() - 1);
    //cout << "net: " << net.net()->getConstName() << endl;

    // foreach two pins in single nets.
    for(auto& pin1 : net->pins()) {
      int pinIdx1 = &pin1 - &(net->pins()[0]);
      for(auto& pin2 : net->pins()) {
        int pinIdx2 = &pin2 - &(net->pins()[0]);

        // 
        // will compare two pins "only once."
        //
        if( pinIdx1 < pinIdx2 ) {
          break;
        }

        // no need to fill in when instance is same
        if( pin1->instance() == pin2->instance() ) {
          continue;
        }

        // B2B modeling on min/maxX pins.
        if( pin1->isMinPinX() || pin1->isMaxPinX() ||
            pin2->isMinPinX() || pin2->isMaxPinX() ) {
          int diffX = abs(pin1->cx() - pin2->cx());
          float weightX = 0;
          if( diffX > initialPlaceVars_.minDiffLength ) {
            weightX = netWeight / diffX;
          }
          else {
            weightX = netWeight 
              / initialPlaceVars_.minDiffLength;
          }
          //cout << weightX << endl;

          // both pin cames from instance
          if( pin1->instance() && pin2->instance() ) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();
            //cout << "inst: " << inst1 << " " << inst2 << endl;

            listX.push_back( T(inst1, inst1, weightX) );
            listX.push_back( T(inst2, inst2, weightX) );

            listX.push_back( T(inst1, inst2, -weightX) );
            listX.push_back( T(inst2, inst1, -weightX) );

            //cout << pin1->cx() << " " 
            //  << pin1->instance()->cx() << endl;
            xcgB_(inst1) += 
              -weightX * (
              (pin1->cx() - pin1->instance()->cx()) - 
              (pin2->cx() - pin2->instance()->cx()));

            xcgB_(inst2) +=
              -weightX * (
              (pin2->cx() - pin2->instance()->cx()) -
              (pin1->cx() - pin1->instance()->cx())); 
          }
          // pin1 from IO port / pin2 from Instance
          else if( !pin1->instance() && pin2->instance() ) {
            const int inst2 = pin2->instance()->extId();
            //cout << "inst2: " << inst2 << endl;
            listX.push_back( T(inst2, inst2, weightX) );
            xcgB_(inst2) += weightX * 
              ( pin1->cx() - 
                ( pin2->cx() - pin2->instance()->cx()) );
          }
          // pin1 from Instance / pin2 from IO port
          else if( pin1->instance() && !pin2->instance() ) {
            const int inst1 = pin1->instance()->extId();
            //cout << "inst1: " << inst1 << endl;
            listX.push_back( T(inst1, inst1, weightX) );
            xcgB_(inst1) += weightX *
              ( pin2->cx() -
                ( pin1->cx() - pin1->instance()->cx()) );
          }
        }
        
        // B2B modeling on min/maxY pins.
        if( pin1->isMinPinY() || pin1->isMaxPinY() ||
            pin2->isMinPinY() || pin2->isMaxPinY() ) {
          
          int diffY = abs(pin1->cy() - pin2->cy());
          float weightY = 0;
          if( diffY > initialPlaceVars_.minDiffLength ) {
            weightY = netWeight / diffY;
          }
          else {
            weightY = netWeight 
              / initialPlaceVars_.minDiffLength;
          }

          // both pin cames from instance
          if( pin1->instance() && pin2->instance() ) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();

            listY.push_back( T(inst1, inst1, weightY) );
            listY.push_back( T(inst2, inst2, weightY) );

            listY.push_back( T(inst1, inst2, -weightY) );
            listY.push_back( T(inst2, inst1, -weightY) );

            ycgB_(inst1) += 
              -weightY * (
              (pin1->cy() - pin1->instance()->cy()) - 
              (pin2->cy() - pin2->instance()->cy()));

            ycgB_(inst2) +=
              -weightY * (
              (pin2->cy() - pin2->instance()->cy()) -
              (pin1->cy() - pin1->instance()->cy())); 
          }
          // pin1 from IO port / pin2 from Instance
          else if( !pin1->instance() && pin2->instance() ) {
            const int inst2 = pin2->instance()->extId();
            listY.push_back( T(inst2, inst2, weightY) );
            ycgB_(inst2) += weightY * 
              ( pin1->cy() - 
                ( pin2->cy() - pin2->instance()->cy()) );
          }
          // pin1 from Instance / pin2 from IO port
          else if( pin1->instance() && !pin2->instance() ) {
            const int inst1 = pin1->instance()->extId();
            listY.push_back( T(inst1, inst1, weightY) );
            ycgB_(inst1) += weightY *
              ( pin2->cy() -
                ( pin1->cy() - pin1->instance()->cy()) );
          }
        }
      }
    }
  } 

//  for(auto& t : listX) {
//    cout << t.row() << " " << t.col() << " " << t.value() << endl; 
//  }

  
  matX_.setFromTriplets(listX.begin(), listX.end());
  matY_.setFromTriplets(listY.begin(), listY.end());
}

void InitialPlace::updateCoordi() {
  const int placeCnt = pb_->placeInsts().size();
  for(auto& inst : pb_->placeInsts()) {
    int idx = inst->extId();
    //cout << "extId: " << idx << endl;
    inst->dbSetCenterLocation( xcgX_(idx), ycgX_(idx) );
    //cout << "xcgX_(" << idx << "): " << xcgX_(idx) 
    //  << " ycgX_(" << idx << "): " << ycgX_(idx) << endl;
  }
}

}

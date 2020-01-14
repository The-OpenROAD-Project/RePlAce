#include "placerBase.h"
#include <opendb/db.h>
#include <iostream>

namespace replace {

using namespace odb;
using namespace std;

////////////////////////////////////////////////////////
// Instance 

Instance::Instance() : inst_(nullptr), lx_(0), ly_(0) {}
Instance::Instance(odb::dbInst* inst) : Instance() {
  inst_ = inst;
  int lx = 0, ly = 0;
  inst_->getLocation(lx, ly);
  lx_ = lx; 
  ly_ = ly;
}

Instance::~Instance() { 
  inst_ = nullptr;
  lx_ = ly_ = 0;
  pins_.clear();
}

bool 
Instance::isFixed() {
  switch( inst_->getPlacementStatus() ) {
    case dbPlacementStatus::NONE:
    case dbPlacementStatus::UNPLACED:
    case dbPlacementStatus::SUGGESTED:
    case dbPlacementStatus::PLACED:
      return true;
      break;
    case dbPlacementStatus::LOCKED:
    case dbPlacementStatus::FIRM:
    case dbPlacementStatus::COVER:
      return false;
      break;
  }
  return false;
}

void
Instance::setLocation(int x, int y) {
  lx_ = x; 
  ly_ = y; 
}

void
Instance::setCenterLocation(int x, int y) {
  lx_ = x - inst_->getBBox()->getDX()/2;
  ly_ = y - inst_->getBBox()->getDY()/2;
}

void
Instance::dbSetPlaced() {
  inst_->setPlacementStatus(dbPlacementStatus::PLACED);
}

void
Instance::dbSetPlacementStatus(dbPlacementStatus ps) {
  inst_->setPlacementStatus(ps);
}

void
Instance::dbSetLocation() {
  inst_->setLocation(lx_, ly_);
}

void
Instance::dbSetLocation(int x, int y) {
  lx_ = x;
  ly_ = y;
  dbSetLocation();
}

void
Instance::dbSetCenterLocation(int x, int y) {
  lx_ = x - inst_->getBBox()->getDX()/2;
  ly_ = y - inst_->getBBox()->getDY()/2;
  dbSetLocation();
}


int
Instance::lx() {
  return lx_;
}

int
Instance::ly() {
  return ly_; 
}

int
Instance::ux() {
  return lx_ + inst_->getBBox()->getDX();
}

int
Instance::uy() {
  return ly_ + inst_->getBBox()->getDY();
}

int
Instance::cx() {
  return lx_ + inst_->getBBox()->getDX()/2;
}

int
Instance::cy() {
  return ly_ + inst_->getBBox()->getDY()/2;
}

void
Instance::addPin(Pin* pin) {
  pins_.push_back(pin);
}

////////////////////////////////////////////////////////
// Pin 

Pin::Pin()
  : term_(nullptr), inst_(nullptr), net_(nullptr), 
    attribute_(0), offsetLx_(0), offsetLy_(0),
    offsetUx_(0), offsetUy_(0),
    lx_(0), ly_(0) {}

Pin::Pin(dbITerm* iTerm): Pin() {
  setITerm();
  term_ = (void*)iTerm;
  updateOffset(iTerm);
  updateLocation(iTerm);
}

Pin::Pin(dbBTerm* bTerm): Pin() {
  setBTerm();
  term_ = (void*)bTerm;
  updateOffset(bTerm);
  updateLocation(bTerm);
}

void Pin::setITerm() {
  attribute_ |= (1 << 0);
}

void Pin::setBTerm() {
  attribute_ |= (1 << 1);
}

void Pin::setMinPinX() {
  attribute_ |= (1 << 2);
}

void Pin::setMinPinY() {
  attribute_ |= (1 << 3);
}

void Pin::setMaxPinX() {
  attribute_ |= (1 << 4);
}

void Pin::setMaxPinY() {
  attribute_ |= (1 << 5);
}

void Pin::unsetMinPinX() {
  attribute_ &= 11111011;
}

void Pin::unsetMinPinY() {
  attribute_ &= 11110111;
}

void Pin::unsetMaxPinX() {
  attribute_ &= 11101111;
}

void Pin::unsetMaxPinY() {
  attribute_ &= 11011111;
}

bool Pin::isITerm() {
  return (attribute_ & (1 << 0) == 1);
}

bool Pin::isBTerm() {
  return (attribute_ & (1 << 1) == 1);
}

bool Pin::isMinPinX() {
  return (attribute_ & (1 << 2) == 1);
}

bool Pin::isMinPinY() {
  return (attribute_ & (1 << 3) == 1);
}

bool Pin::isMaxPinX() {
  return (attribute_ & (1 << 4) == 1);
}

bool Pin::isMaxPinY() {
  return (attribute_ & (1 << 5) == 1);
}

int Pin::lx() {
  return lx_;
}

int Pin::ly() {
  return ly_;
}

int Pin::ux() {
  return lx_ + offsetUx_;
}

int Pin::uy() {
  return ly_ + offsetUy_;
}

int Pin::cx() {
  return lx_ + (offsetLx_ + offsetUx_)/2; 
}

int Pin::cy() {
  return ly_ + (offsetLy_ + offsetUy_)/2;
}

odb::dbITerm* Pin::iTerm() {
  return (isITerm())? (odb::dbITerm*) term_ : nullptr;
}
odb::dbBTerm* Pin::bTerm() {
  return (isBTerm())? (odb::dbBTerm*) term_ : nullptr;
}

void Pin::updateOffset(odb::dbITerm* iTerm) {
  offsetLx_ = INT_MAX;
  offsetLy_ = INT_MAX;
  offsetUx_ = INT_MIN;
  offsetUy_ = INT_MIN;

  for(dbMPin* mPin : iTerm->getMTerm()->getMPins()) {
    for(dbBox* box : mPin->getGeometry()) {
      offsetLx_ = (offsetLx_ > box->xMin())? box->xMin() : offsetLx_;
      offsetLy_ = (offsetLy_ > box->yMin())? box->yMin() : offsetLy_;
      offsetUx_ = (offsetUx_ < box->xMax())? box->xMax() : offsetUx_;
      offsetUy_ = (offsetUy_ < box->yMax())? box->yMax() : offsetUy_;
    } 
  }

  // NOT FOUND
  if( offsetLx_ == INT_MAX || offsetLy_ == INT_MAX || 
      offsetUx_ == INT_MIN || offsetUy_ == INT_MIN ) {
    offsetLx_ = iTerm->getInst()->getBBox()->xMin();
    offsetLy_ = iTerm->getInst()->getBBox()->yMin();
    offsetUx_ = iTerm->getInst()->getBBox()->xMax();
    offsetUy_ = iTerm->getInst()->getBBox()->yMax();
  }
}

// 
// for BTerm, offset* will hold bbox info.
//
void Pin::updateOffset(odb::dbBTerm* bTerm) {
  offsetLx_ = INT_MAX;
  offsetLy_ = INT_MAX;
  offsetUx_ = INT_MIN;
  offsetUy_ = INT_MIN;

  for(dbBPin* bPin : bTerm->getBPins()) {
    offsetLx_ = (offsetLx_ > bPin->getBox()->xMin())? 
      bPin->getBox()->xMin() : offsetLx_;
    offsetLy_ = (offsetLy_ > bPin->getBox()->yMin())? 
      bPin->getBox()->yMin() : offsetLy_;
    offsetUx_ = (offsetUx_ < bPin->getBox()->xMax())? 
      bPin->getBox()->xMax() : offsetUx_;
    offsetUy_ = (offsetUy_ < bPin->getBox()->yMax())? 
      bPin->getBox()->yMax() : offsetUy_;
  }
  offsetUx_ = offsetUx_ - offsetLx_;
  offsetUy_ = offsetUy_ - offsetLy_;
  offsetLx_ = 0;
  offsetLy_ = 0;
}

void Pin::updateLocation() {
  // only ITerm can be updated. 
  // BTerm is fixed on placer.
  if( isITerm() ) {
    updateLocation((odb::dbITerm*) term_);
  }
}
void Pin::updateLocation(odb::dbITerm* iTerm) {
  iTerm->getInst()->getLocation(lx_, ly_);
  lx_ += offsetLx_;
  ly_ += offsetLy_;
}

void Pin::updateLocation(odb::dbBTerm* bTerm) {
  lx_ = INT_MAX;
  ly_ = INT_MAX;  

  for(dbBPin* bPin : bTerm->getBPins()) {
    lx_ = (lx_ > bPin->getBox()->xMin())? 
      bPin->getBox()->xMin() : lx_;
    ly_ = (ly_> bPin->getBox()->yMin())? 
      bPin->getBox()->yMin() : ly_;
  }
}

void 
Pin::setInstance(Instance* inst) {
  inst_ = inst;
}

void
Pin::setNet(Net* net) {
  net_ = net;
}

Pin::~Pin() {
  term_ = nullptr;
  attribute_ = 0; 
}

////////////////////////////////////////////////////////
// Net 

Net::Net() : net_(nullptr), lx_(0), ly_(0), ux_(0), uy_(0) {}
Net::Net(odb::dbNet* net) : Net() {
  net_ = net;
  updateBox();
}

Net::~Net() {
  net_ = nullptr;
  lx_ = ly_ = ux_ = uy_ = 0;
}

int Net::lx() {
  return lx_;
}

int Net::ly() {
  return ly_;
}

int Net::ux() {
  return ux_;
}

int Net::uy() {
  return uy_;
}

int Net::cx() {
  return (lx_ + ux_)/2;
}

int Net::cy() {
  return (ly_ + uy_)/2;
}

int Net::hpwl() {
  return (ux_-lx_) + (uy_-ly_);
}

void Net::updateBox() {
  lx_ = INT_MAX;
  ly_ = INT_MAX;
  ux_ = INT_MIN;
  uy_ = INT_MIN;
  for(dbITerm* iTerm : net_->getITerms()) {
    dbBox* box = iTerm->getInst()->getBBox();
    lx_ = (lx_ > box->xMin())? box->xMin() : lx_;
    ly_ = (ly_ > box->yMin())? box->yMin() : ly_;
    ux_ = (ux_ > box->xMax())? box->xMax() : ux_;
    uy_ = (uy_ > box->yMax())? box->yMax() : uy_;
  }
  for(dbBTerm* bTerm : net_->getBTerms()) {
    for(dbBPin* bPin : bTerm->getBPins()) {
      lx_ = (lx_ > bPin->getBox()->xMin())? 
        bPin->getBox()->xMin() : lx_;
      ly_ = (ly_> bPin->getBox()->yMin())? 
        bPin->getBox()->yMin() : ly_;
    }
  }
}

void Net::addPin(Pin* pin) {
  pins_.push_back(pin);
}

////////////////////////////////////////////////////////
// PlacerBase

PlacerBase::PlacerBase() : db_(nullptr) {}
PlacerBase::PlacerBase(odb::dbDatabase* db) : db_(db) {
  init();
}

PlacerBase::~PlacerBase() {
  clear();
}

void PlacerBase::init() {
  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbInst> insts = block->getInsts();
 
  // insts fill 
  insts_.reserve(insts.size());
  for(dbInst* inst : insts) {
    Instance myInst(inst);
    insts_.push_back( myInst );
    instMap_[inst] = &insts_[insts_.size()-1];  
  }

  for(auto& inst : insts_) {
    if(inst.isFixed()) {
      fixedInsts_.push_back(&inst); 
    }
    else {
      placeInsts_.push_back(&inst);
    }
  }

  // pins fill 
  dbSet<dbBTerm> bTerms = block->getBTerms();
  dbSet<dbITerm> iTerms = block->getITerms();
  pins_.reserve(bTerms.size() + iTerms.size());
  for(dbBTerm* bTerm : bTerms) {
    Pin myPin(bTerm);
    pins_.push_back( myPin );
    pinMap_[(void*)bTerm] = &pins_[pins_.size()-1];
  }
  for(dbITerm* iTerm : iTerms) {
    Pin myPin(iTerm);
    pins_.push_back( myPin );
    pinMap_[(void*)iTerm] = &pins_[pins_.size()-1];
  }

  // nets fill
  dbSet<dbNet> nets = block->getNets();
  nets_.reserve(nets.size());
  for(dbNet* net : nets) {
    Net myNet(net);
    nets_.push_back( myNet );
    netMap_[net] = &nets_[nets_.size()-1];
  }

  // insts_' pins_ fill
  for(auto& inst : insts_) {
    for(dbITerm* iTerm : inst.inst()->getITerms()) {
      inst.addPin( dbToPlace(iTerm) );
    }
  }

  // pins' net and instance fill 
  for(auto& pin : pins_) {
    if( pin.isITerm() ) {
      pin.setInstance( dbToPlace( pin.iTerm()->getInst() ) );
      pin.setNet( dbToPlace( pin.iTerm()->getNet() ) );
    }
    else if( pin.isBTerm() ) {
      pin.setNet( dbToPlace( pin.bTerm()->getNet() ) );
    }
  }
 
  //nets' pin update
  for(auto& net : nets_) {
    for(dbITerm* iTerm : net.net()->getITerms()) {
      net.addPin( dbToPlace( iTerm ) );
    }
    for(dbBTerm* bTerm : net.net()->getBTerms()) {
      net.addPin( dbToPlace( bTerm ) );
    }
  }

}

void PlacerBase::clear() {
  db_ = nullptr;
  insts_.clear();
  pins_.clear();
  nets_.clear();
  instMap_.clear();
  pinMap_.clear();
  netMap_.clear();
  
  placeInsts_.clear();
  fixedInsts_.clear();
}

int PlacerBase::hpwl() {
  int hpwl = 0;
  for(auto& net : nets_) {
    net.updateBox();
    hpwl += net.hpwl();
  }
  return hpwl;
}

Instance* PlacerBase::dbToPlace(odb::dbInst* inst) {
  auto instPtr = instMap_.find(inst);
  return (instPtr == instMap_.end())? nullptr : instPtr->second;
}

Pin* PlacerBase::dbToPlace(odb::dbITerm* term) {
  auto pinPtr = pinMap_.find((void*)term);
  return (pinPtr == pinMap_.end())? nullptr : pinPtr->second;
}

Pin* PlacerBase::dbToPlace(odb::dbBTerm* term) {
  auto pinPtr = pinMap_.find((void*)term);
  return (pinPtr == pinMap_.end())? nullptr : pinPtr->second;
}

Net* PlacerBase::dbToPlace(odb::dbNet* net) {
  auto netPtr = netMap_.find(net);
  return (netPtr == netMap_.end())? nullptr : netPtr->second;
}

}


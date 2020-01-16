#include "placerBase.h"
#include <opendb/db.h>
#include <iostream>

namespace replace {

using namespace odb;
using namespace std;

static odb::adsRect 
getCoreRectFromDb(dbSet<odb::dbRow> &rows);

////////////////////////////////////////////////////////
// Instance 

Instance::Instance() : inst_(nullptr), 
  lx_(0), ly_(0), extId_(INT_MIN) {}
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
      return false;
      break;
    case dbPlacementStatus::LOCKED:
    case dbPlacementStatus::FIRM:
    case dbPlacementStatus::COVER:
      return true;
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

void
Instance::setExtId(int extId) {
  extId_ = extId;
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
  attribute_ |= (1 << iTermField);
}

void Pin::setBTerm() {
  attribute_ |= (1 << bTermField);
}

void Pin::setMinPinX() {
  attribute_ |= (1 << minPinXField);
}

void Pin::setMinPinY() {
  attribute_ |= (1 << minPinYField);
}

void Pin::setMaxPinX() {
  attribute_ |= (1 << maxPinXField);
}

void Pin::setMaxPinY() {
  attribute_ |= (1 << maxPinYField);
}

void Pin::unsetMinPinX() {
  attribute_ &= ~(1 << minPinXField);
}

void Pin::unsetMinPinY() {
  attribute_ &= ~(1 << minPinYField);
}

void Pin::unsetMaxPinX() {
  attribute_ &= ~(1 << maxPinXField);
}

void Pin::unsetMaxPinY() {
  attribute_ &= ~(1 << maxPinYField);
}

bool Pin::isITerm() {
  return ((attribute_ >> iTermField) & 1 == 1);
}

bool Pin::isBTerm() {
  return ((attribute_ >> bTermField) & 1 == 1);
}

bool Pin::isMinPinX() {
  return ((attribute_ >> minPinXField) & 1 == 1);
}

bool Pin::isMinPinY() {
  return ((attribute_ >> minPinYField) & 1 == 1);
}

bool Pin::isMaxPinX() {
  return ((attribute_ >> maxPinXField) & 1 == 1);
}

bool Pin::isMaxPinY() {
  return ((attribute_ >> maxPinYField) & 1 == 1);
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
Pin::updateLocation(Instance* inst) {
  lx_ = inst->lx() + offsetLx_;
  ly_ = inst->ly() + offsetLy_; 
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
    ux_ = (ux_ < box->xMax())? box->xMax() : ux_;
    uy_ = (uy_ < box->yMax())? box->yMax() : uy_;
  }
  for(dbBTerm* bTerm : net_->getBTerms()) {
    for(dbBPin* bPin : bTerm->getBPins()) {
      lx_ = (lx_ > bPin->getBox()->xMin())? 
        bPin->getBox()->xMin() : lx_;
      ly_ = (ly_ > bPin->getBox()->yMin())? 
        bPin->getBox()->yMin() : ly_;
      ux_ = (ux_ < bPin->getBox()->xMax())? 
        bPin->getBox()->xMax() : ux_;
      uy_ = (uy_ < bPin->getBox()->yMax())? 
        bPin->getBox()->yMax() : uy_;
    }
  }
}

void Net::addPin(Pin* pin) {
  pins_.push_back(pin);
}

////////////////////////////////////////////////////////
// Die 

Die::Die() : 
  dieLx_(0), dieLy_(0), dieUx_(0), dieUy_(0),
  coreLx_(0), coreLy_(0), coreUx_(0), coreUy_(0) {}

Die::Die(odb::dbBox* dieBox, 
    odb::adsRect* coreRect) : Die() {
  setDieBox(dieBox);
  setCoreBox(coreRect);
}

Die::~Die() {
  dieLx_ = dieLy_ = dieUx_ = dieUy_ = 0;
  coreLx_ = coreLy_ = coreUx_ = coreUy_ = 0;
}

void
Die::setDieBox(odb::dbBox* dieBox) {
  dieLx_ = dieBox->xMin();
  dieLy_ = dieBox->yMin();
  dieUx_ = dieBox->xMax();
  dieUy_ = dieBox->yMax();
}

void
Die::setCoreBox(odb::adsRect* coreRect) {
  coreLx_ = coreRect->xMin();
  coreLy_ = coreRect->yMin();
  coreUx_ = coreRect->xMax();
  coreUy_ = coreRect->yMax();
}

int
Die::dieCx() { 
  return (dieLx_ + dieUx_)/2;
}

int
Die::dieCy() { 
  return (dieLy_ + dieUy_)/2;
}

int
Die::coreCx() {
  return (coreLx_ + coreUx_)/2;
}

int
Die::coreCy() {
  return (coreLy_ + coreUy_)/2;
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

void 
PlacerBase::init() {
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

  dbSet<dbRow> rows = block->getRows();

  odb::adsRect coreRect = getCoreRectFromDb(rows);
  die_ = Die(block->getBBox(), &coreRect);

  printInfo();
}

void
PlacerBase::clear() {
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

int 
PlacerBase::hpwl() {
  int hpwl = 0;
  for(auto& net : nets_) {
    net.updateBox();
    hpwl += net.hpwl();
  }
  return hpwl;
}

Instance* 
PlacerBase::dbToPlace(odb::dbInst* inst) {
  auto instPtr = instMap_.find(inst);
  return (instPtr == instMap_.end())? nullptr : instPtr->second;
}

Pin* 
PlacerBase::dbToPlace(odb::dbITerm* term) {
  auto pinPtr = pinMap_.find((void*)term);
  return (pinPtr == pinMap_.end())? nullptr : pinPtr->second;
}

Pin* 
PlacerBase::dbToPlace(odb::dbBTerm* term) {
  auto pinPtr = pinMap_.find((void*)term);
  return (pinPtr == pinMap_.end())? nullptr : pinPtr->second;
}

Net* 
PlacerBase::dbToPlace(odb::dbNet* net) {
  auto netPtr = netMap_.find(net);
  return (netPtr == netMap_.end())? nullptr : netPtr->second;
}

void 
PlacerBase::printInfo() { 
  cout << "Design Info" << endl;
  cout << "Instances      : " << insts_.size() << endl;
  cout << "PlaceInstances : " << placeInsts_.size() << endl;
  cout << "FixedInstances : " << fixedInsts_.size() << endl;
  cout << "Nets           : " << nets_.size() << endl;
  cout << "Pins           : " << pins_.size() << endl;

  int maxFanout = INT_MIN;
  int sumFanout = 0;
  dbNet* maxFanoutNet = nullptr;
  for(auto& net : nets_) {
    if( maxFanout < (int)net.pins().size() ) {
      maxFanout = (int)net.pins().size();
      maxFanoutNet = net.net();
    }
    sumFanout += (int)net.pins().size();
  }
  cout << "MaxFanout      : " << maxFanout << endl;
  cout << "MaxFanoutNet   : " 
    << maxFanoutNet->getConstName() << endl;
  cout << "AvgFanout      : " 
    << static_cast<float>(sumFanout) / nets_.size() << endl; 
  cout << endl;

  cout << "DieBox         : ( " 
    << die_.dieLx() << " " << die_.dieLy() 
    << " ) - ( " 
    << die_.dieUx() << " " << die_.dieUy() 
    << " ) " << endl;
  cout << "CoreBox        : ( " 
    << die_.coreLx() << " " << die_.coreLy() 
    << " ) - ( " 
    << die_.coreUx() << " " << die_.coreUy() 
    << " ) " << endl;
}


static odb::adsRect 
getCoreRectFromDb(dbSet<odb::dbRow> &rows) {
  int minX = INT_MAX, minY = INT_MAX;
  int maxX = INT_MIN, maxY = INT_MIN;

  for(dbRow* row : rows) {
    adsRect rowRect;
    row->getBBox( rowRect );

    minX = (minX > rowRect.xMin()) ? rowRect.xMin(): minX;
    minY = (minY > rowRect.yMin()) ? rowRect.yMin(): minY;
    maxX = (maxX < rowRect.xMax()) ? rowRect.xMax(): maxX;
    maxY = (maxY < rowRect.yMax()) ? rowRect.yMax(): maxY;
  }
  return odb::adsRect(minX, minY, maxX, maxY);
}



}


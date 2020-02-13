#include "placerBase.h"
#include "nesterovBase.h"
#include <opendb/db.h>
#include <iostream>

namespace replace {

using namespace odb;
using namespace std;

static odb::adsRect 
getCoreRectFromDb(dbSet<odb::dbRow> &rows);

static int 
fastModulo(const int input, const int ceil);

static std::pair<int, int>
getMinMaxIdx(int ll, int uu, int coreLL, 
    int siteSize);


////////////////////////////////////////////////////////
// Instance 

Instance::Instance() : inst_(nullptr), 
  lx_(0), ly_(0), ux_(0), uy_(0), extId_(INT_MIN) {}

// for movable real instances
Instance::Instance(odb::dbInst* inst) : Instance() {
  inst_ = inst;
  int lx = 0, ly = 0;
  inst_->getLocation(lx, ly);
  lx_ = lx; 
  ly_ = ly;
  ux_ = lx + inst_->getBBox()->getDX();
  uy_ = ly + inst_->getBBox()->getDY();

  // 
  // TODO
  // need additional adjustment 
  // if instance (macro) is fixed and
  // its coordi is not multiple of rows' integer. 
}

// for dummy instances
Instance::Instance(int lx, int ly, int ux, int uy) 
  : Instance() {
  inst_ = nullptr;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux; 
  uy_ = uy;
}


Instance::~Instance() { 
  inst_ = nullptr;
  lx_ = ly_ = 0;
  ux_ = uy_ = 0;
  pins_.clear();
}

bool 
Instance::isFixed() const {
  // dummy instance is always fixed
  if( isDummy() ) {
    return true;
  }

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

bool
Instance::isInstance() const {
  return (inst_ != nullptr);
}

bool
Instance::isDummy() const {
  return (inst_ == nullptr);
}

void
Instance::setLocation(int x, int y) {
  ux_ = x + (ux_ - lx_);
  uy_ = y + (uy_ - ly_);

  lx_ = x; 
  ly_ = y; 

  // pins update
  for(auto& pin : pins_) {
    pin->updateLocation(this);
  }
}

void
Instance::setCenterLocation(int x, int y) {
  const int halfX = (ux_ - lx_)/2;
  const int halfY = (uy_ - ly_)/2;
  lx_ = x - halfX; 
  ly_ = y - halfY;
  ux_ = x + halfX;
  uy_ = y + halfY;

  // pins update
  for(auto& pin : pins_) {
    pin->updateLocation(this);
  }
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
  setLocation(x, y);
  dbSetLocation();
}

void
Instance::dbSetCenterLocation(int x, int y) {
  setCenterLocation(x, y);
  dbSetLocation();
}


int
Instance::lx() const {
  return lx_;
}

int
Instance::ly() const {
  return ly_; 
}

int
Instance::ux() const {
  return ux_; 
}

int
Instance::uy() const {
  return uy_; 
}

int
Instance::cx() const {
  return (lx_ + ux_)/2; 
}

int
Instance::cy() const {
  return (ly_ + uy_)/2; 
}

int
Instance::dx() const {
  return (ux_ - lx_);
}

int
Instance::dy() const {
  return (uy_ - ly_);
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
    cx_(0), cy_(0),
    offsetCx_(0), offsetCy_(0),
    iTermField_(0), bTermField_(0),
    minPinXField_(0), minPinYField_(0),
    maxPinXField_(0), maxPinYField_(0) {}

Pin::Pin(odb::dbITerm* iTerm): Pin() {
  setITerm();
  term_ = (void*)iTerm;
  updateCoordi(iTerm);
}

Pin::Pin(odb::dbBTerm* bTerm): Pin() {
  setBTerm();
  term_ = (void*)bTerm;
  updateCoordi(bTerm);
}

void Pin::setITerm() {
  iTermField_ = 1;
}

void Pin::setBTerm() {
  bTermField_ = 1;
}

void Pin::setMinPinX() {
  minPinXField_ = 1;
}

void Pin::setMinPinY() {
  minPinYField_ = 1;
}

void Pin::setMaxPinX() {
  maxPinXField_ = 1;
}

void Pin::setMaxPinY() {
  maxPinYField_ = 1;
}

void Pin::unsetMinPinX() {
  minPinXField_ = 0;
}

void Pin::unsetMinPinY() {
  minPinYField_ = 0;
}

void Pin::unsetMaxPinX() {
  maxPinXField_ = 0;
}

void Pin::unsetMaxPinY() {
  maxPinYField_ = 0; 
}

bool Pin::isITerm() const {
  return (iTermField_ == 1);
}

bool Pin::isBTerm() const {
  return (bTermField_ == 1);
}

bool Pin::isMinPinX() const {
  return (minPinXField_ == 1);
}

bool Pin::isMinPinY() const {
  return (minPinYField_ == 1);
}

bool Pin::isMaxPinX() const {
  return (maxPinXField_ == 1);
}

bool Pin::isMaxPinY() const {
  return (maxPinYField_ == 1);
}

int Pin::cx() const {
  return cx_; 
}

int Pin::cy() const {
  return cy_;
}

int Pin::offsetCx() const { 
  return offsetCx_; 
}

int Pin::offsetCy() const { 
  return offsetCy_;
}

odb::dbITerm* Pin::dbITerm() const {
  return (isITerm())? (odb::dbITerm*) term_ : nullptr;
}
odb::dbBTerm* Pin::dbBTerm() const {
  return (isBTerm())? (odb::dbBTerm*) term_ : nullptr;
}

void Pin::updateCoordi(odb::dbITerm* iTerm) {
  int offsetLx = INT_MAX;
  int offsetLy = INT_MAX;
  int offsetUx = INT_MIN;
  int offsetUy = INT_MIN;

  for(dbMPin* mPin : iTerm->getMTerm()->getMPins()) {
    for(dbBox* box : mPin->getGeometry()) {
      offsetLx = std::min(box->xMin(), offsetLx);
      offsetLy = std::min(box->yMin(), offsetLy);
      offsetUx = std::max(box->xMax(), offsetUx);
      offsetUy = std::max(box->yMax(), offsetUy);
    } 
  }

  int lx = iTerm->getInst()->getBBox()->xMin();
  int ly = iTerm->getInst()->getBBox()->yMin();
    
  int instCenterX = iTerm->getInst()->getBBox()->getDX()/2;
  int instCenterY = iTerm->getInst()->getBBox()->getDY()/2;

  // Pin SHAPE is NOT FOUND; 
  // (may happen on OpenDB bug case)
  if( offsetLx == INT_MAX || offsetLy == INT_MAX || 
      offsetUx == INT_MIN || offsetUy == INT_MIN ) {
    
    // offset is center of instances
    offsetCx_ = offsetCy_ = 0;
  }
  // usual case
  else {


    // offset is Pin BBoxs' center, so
    // subtract the Origin coordinates (e.g. instCenterX, instCenterY)
    //
    // Transform coordinates 
    // from (origin: 0,0) 
    // to (origin: instCenterX, instCenterY)
    //
    offsetCx_ = (offsetLx + offsetUx)/2 - instCenterX;
    offsetCy_ = (offsetLy + offsetUy)/2 - instCenterY;
  }

  cx_ = lx + instCenterX + offsetCx_;
  cy_ = ly + instCenterY + offsetCy_;
}

// 
// for BTerm, offset* will hold bbox info.
//
void Pin::updateCoordi(odb::dbBTerm* bTerm) {
  int lx = INT_MAX;
  int ly = INT_MAX;
  int ux = INT_MIN;
  int uy = INT_MIN;

  for(dbBPin* bPin : bTerm->getBPins()) {
    lx = std::min(bPin->getBox()->xMin(), lx);
    ly = std::min(bPin->getBox()->yMin(), ly);
    ux = std::max(bPin->getBox()->xMax(), ux);
    uy = std::max(bPin->getBox()->yMax(), uy);
  }

  if( lx == INT_MAX || ly == INT_MAX ||
      ux == INT_MIN || uy == INT_MIN ) {
    cout << "Error: " << bTerm->getConstName() 
      << "I/O port is not placed!" << endl;
    cout << "       Please Run ioPlacer to place I/O ports" << endl;
    exit(1);
  }

  // Just center 
  offsetCx_ = offsetCy_ = 0;

  cx_ = (lx + ux)/2;
  cy_ = (ly + uy)/2;
}

void
Pin::updateLocation(const Instance* inst) {
  cx_ = inst->cx() + offsetCx_;
  cy_ = inst->cy() + offsetCy_;
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
  inst_ = nullptr;
  net_ = nullptr;
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

int Net::lx() const {
  return lx_;
}

int Net::ly() const {
  return ly_;
}

int Net::ux() const {
  return ux_;
}

int Net::uy() const {
  return uy_;
}

int Net::cx() const {
  return (lx_ + ux_)/2;
}

int Net::cy() const {
  return (ly_ + uy_)/2;
}

int64_t Net::hpwl() const {
  return static_cast<int64_t>((ux_-lx_) + (uy_-ly_));
}

void Net::updateBox() {
  lx_ = INT_MAX;
  ly_ = INT_MAX;
  ux_ = INT_MIN;
  uy_ = INT_MIN;
  for(dbITerm* iTerm : net_->getITerms()) {
    dbBox* box = iTerm->getInst()->getBBox();
    lx_ = std::min(box->xMin(), lx_);
    ly_ = std::min(box->yMin(), ly_);
    ux_ = std::max(box->xMax(), ux_);
    uy_ = std::max(box->yMax(), uy_);
  }
  for(dbBTerm* bTerm : net_->getBTerms()) {
    for(dbBPin* bPin : bTerm->getBPins()) {
      lx_ = std::min(bPin->getBox()->xMin(), lx_);
      ly_ = std::min(bPin->getBox()->yMin(), ly_);
      ux_ = std::max(bPin->getBox()->xMax(), ux_);
      uy_ = std::max(bPin->getBox()->yMax(), uy_);
    }
  }
}

void Net::addPin(Pin* pin) {
  pins_.push_back(pin);
}

odb::dbSigType Net::getSigType() const { 
  return net_->getSigType();
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
Die::dieCx() const { 
  return (dieLx_ + dieUx_)/2;
}

int
Die::dieCy() const { 
  return (dieLy_ + dieUy_)/2;
}

int
Die::dieDx() const { 
  return dieUx_ - dieLx_;
}

int 
Die::dieDy() const {
  return dieUy_ - dieLy_;
}

int
Die::coreCx() const {
  return (coreLx_ + coreUx_)/2;
}

int
Die::coreCy() const {
  return (coreLy_ + coreUy_)/2;
}

int
Die::coreDx() const {
  return coreUx_ - coreLx_;
}

int
Die::coreDy() const {
  return coreUy_ - coreLy_;
}

////////////////////////////////////////////////////////
// PlacerBase

PlacerBase::PlacerBase() 
  : db_(nullptr), log_(nullptr), siteSizeX_(0), siteSizeY_(0),
  placeInstsArea_(0), nonPlaceInstsArea_(0) {}

PlacerBase::PlacerBase(odb::dbDatabase* db,
    std::shared_ptr<Logger> log)
  : PlacerBase() {
  db_ = db;
  log_ = log;
  init();
}

PlacerBase::~PlacerBase() {
  reset();
}


void 
PlacerBase::init() {
  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbInst> insts = block->getInsts();
  
  // die-core area update
  dbSet<dbRow> rows = block->getRows();
  odb::adsRect coreRect = getCoreRectFromDb(rows);
  die_ = Die(block->getBBox(), &coreRect);
 
  // siteSize update 
  dbRow* firstRow = *(rows.begin());
  siteSizeX_ = firstRow->getSite()->getWidth();
  siteSizeY_ = firstRow->getSite()->getHeight();
  
  // insts fill with real instances
  instStor_.reserve(insts.size());
  for(dbInst* inst : insts) {
    Instance myInst(inst);
    instStor_.push_back( myInst );
  }

  // insts fill with fake instances (fragmented row)
  initInstsForFragmentedRow();


  // init inst ptrs and areas
  insts_.reserve(instStor_.size());
  for(auto& inst : instStor_) {
    if(inst.isInstance()) {
      if(inst.isFixed()) {
        fixedInsts_.push_back(&inst); 
        nonPlaceInsts_.push_back(&inst);
        nonPlaceInstsArea_ += inst.dx() * inst.dy();
      }
      else {
        placeInsts_.push_back(&inst);
        placeInstsArea_ += inst.dx() * inst.dy();
      }
      instMap_[inst.dbInst()] = &inst;
    }
    else if(inst.isDummy()) {
      dummyInsts_.push_back(&inst);
      nonPlaceInsts_.push_back(&inst);
      nonPlaceInstsArea_ += inst.dx() * inst.dy();
    }
    insts_.push_back(&inst);
  }

  // pins fill 
  dbSet<dbBTerm> bTerms = block->getBTerms();
  dbSet<dbITerm> iTerms = block->getITerms();
  pinStor_.reserve(bTerms.size() + iTerms.size());
  for(dbBTerm* bTerm : bTerms) {
    Pin myPin(bTerm);
    pinStor_.push_back( myPin );
    pinMap_[(void*)bTerm] = &pinStor_[pinStor_.size()-1];
  }
  for(dbITerm* iTerm : iTerms) {
    Pin myPin(iTerm);
    pinStor_.push_back( myPin );
    pinMap_[(void*)iTerm] = &pinStor_[pinStor_.size()-1];
  }

  // nets fill
  dbSet<dbNet> nets = block->getNets();
  netStor_.reserve(nets.size());
  for(dbNet* net : nets) {
    Net myNet(net);
    netStor_.push_back( myNet );
    netMap_[net] = &netStor_[netStor_.size()-1];
  }

  // instStor_'s pins_ fill
  for(auto& inst : instStor_ ) {
    if( !inst.isInstance() ) {
      continue;
    }
    for(dbITerm* iTerm : inst.dbInst()->getITerms()) {
      inst.addPin( dbToPlace(iTerm) );
    }
  }

  // pins' net and instance fill 
  pins_.reserve(pinStor_.size());
  for(auto& pin : pinStor_) {
    if( pin.isITerm() ) {
      pin.setInstance( dbToPlace( pin.dbITerm()->getInst() ) );
      pin.setNet( dbToPlace( pin.dbITerm()->getNet() ) );
    }
    else if( pin.isBTerm() ) {
      pin.setNet( dbToPlace( pin.dbBTerm()->getNet() ) );
    }
    pins_.push_back(&pin);
  }
 
  //nets' pin update
  nets_.reserve(netStor_.size());
  for(auto& net : netStor_) {
    for(dbITerm* iTerm : net.dbNet()->getITerms()) {
      net.addPin( dbToPlace( iTerm ) );
    }
    for(dbBTerm* bTerm : net.dbNet()->getBTerms()) {
      net.addPin( dbToPlace( bTerm ) );
    }
    nets_.push_back(&net);
  }

  printInfo();
}

void
PlacerBase::initInstsForFragmentedRow() {
  dbSet<dbRow> rows = db_->getChip()->getBlock()->getRows();
  
  // dummy cell update to understand fragmented-row
  //

  int siteCountX = (die_.coreUx()-die_.coreLx())/siteSizeX_;
  int siteCountY = (die_.coreUy()-die_.coreLy())/siteSizeY_;
  
  enum PlaceInfo {
    Empty, Row, FixedInst
  };
 
  // 
  // Initialize siteGrid as empty
  //
  std::vector<PlaceInfo> 
    siteGrid (
        siteCountX * siteCountY, 
        PlaceInfo::Empty);
  

  // fill in rows' bbox
  for(dbRow* row : rows) {
    adsRect rect;
    row->getBBox(rect);
    
    std::pair<int, int> pairX 
      = getMinMaxIdx(rect.xMin(), rect.xMax(), 
          die_.coreLx(), siteSizeX_);

    std::pair<int, int> pairY
      = getMinMaxIdx(rect.yMin(), rect.yMax(),
          die_.coreLy(), siteSizeY_);

    for(int i=pairX.first; i<pairX.second; i++) {
      for(int j=pairY.first; j<pairY.second; j++) {
        siteGrid[ j * siteCountX + i ] = Row; 
      }
    }
  }

  // fill fixed instances' bbox
  for(auto& fixedInst : fixedInsts_) {
    std::pair<int, int> pairX 
      = getMinMaxIdx(fixedInst->lx(), fixedInst->ux(),
          die_.coreLx(), siteSizeX_);
    std::pair<int, int> pairY 
      = getMinMaxIdx(fixedInst->ly(), fixedInst->uy(),
          die_.coreLy(), siteSizeY_);

    for(int i=pairX.first; i<pairX.second; i++) {
      for(int j=pairY.first; j<pairY.second; j++) {
        siteGrid[ j * siteCountX + i ] = FixedInst; 
      }
    }
  }

  // 
  // Search the "Empty" coordinates on site-grid
  // --> These sites need to be dummyInstance
  //
  for(int j=0; j<siteCountY; j++) {
    for(int i=0; i<siteCountX; i++) {
      // if empty spot found
      if( siteGrid[j * siteCountX + i] == Empty ) {
        int startX = i;
        // find end points
        while(i < siteCountX &&
            siteGrid[j*siteCountX + i] == Empty) {
          i++;
        }
        int endX = i;
        Instance myInst(
            die_.coreLx() + siteSizeX_ * startX,
            die_.coreLy() + siteSizeY_ * j, 
            die_.coreLx() + siteSizeX_ * endX,
            die_.coreLy() + siteSizeY_ * (j+1));
        instStor_.push_back( myInst );
      }
    }
  }
}

void
PlacerBase::reset() {
  db_ = nullptr;
  instStor_.clear();
  pinStor_.clear();
  netStor_.clear();

  pins_.clear();
  nets_.clear();
  insts_.clear();

  instMap_.clear();
  pinMap_.clear();
  netMap_.clear();
  
  placeInsts_.clear();
  fixedInsts_.clear();
  nonPlaceInsts_.clear();
}

int64_t 
PlacerBase::hpwl() const {
  int64_t hpwl = 0;
  for(auto& net : nets_) {
    net->updateBox();
    hpwl += net->hpwl();
  }
  return hpwl;
}

Instance* 
PlacerBase::dbToPlace(odb::dbInst* inst) const {
  auto instPtr = instMap_.find(inst);
  return (instPtr == instMap_.end())? nullptr : instPtr->second;
}

Pin* 
PlacerBase::dbToPlace(odb::dbITerm* term) const {
  auto pinPtr = pinMap_.find((void*)term);
  return (pinPtr == pinMap_.end())? nullptr : pinPtr->second;
}

Pin* 
PlacerBase::dbToPlace(odb::dbBTerm* term) const {
  auto pinPtr = pinMap_.find((void*)term);
  return (pinPtr == pinMap_.end())? nullptr : pinPtr->second;
}

Net* 
PlacerBase::dbToPlace(odb::dbNet* net) const {
  auto netPtr = netMap_.find(net);
  return (netPtr == netMap_.end())? nullptr : netPtr->second;
}

void 
PlacerBase::printInfo() const { 
  cout << "Design Info" << endl;
  cout << "Instances      : " << instStor_.size() << endl;
  cout << "PlaceInstances : " << placeInsts_.size() << endl;
  cout << "FixedInstances : " << fixedInsts_.size() << endl;
  cout << "DummyInstances : " << dummyInsts_.size() << endl;
  cout << "Nets           : " << nets_.size() << endl;
  cout << "Pins           : " << pins_.size() << endl;

  int maxFanout = INT_MIN;
  int sumFanout = 0;
  dbNet* maxFanoutNet = nullptr;
  for(auto& net : nets_) {
    if( maxFanout < (int)net->pins().size() ) {
      maxFanout = (int)net->pins().size();
      maxFanoutNet = net->dbNet();
    }
    sumFanout += (int)net->pins().size();
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

  int64_t coreArea = 
    static_cast<int64_t>(die_.coreUx() - die_.coreLx()) * 
    static_cast<int64_t>(die_.coreUy() - die_.coreLy());
  float util = 
    static_cast<float>(placeInstsArea_) 
    / (coreArea - nonPlaceInstsArea_) * 100;

  cout << "coreArea       : " << coreArea << endl;
  cout << "placeInstsArea : " << placeInstsArea_ << endl;
  cout << "nonPlaceInstsArea : " << nonPlaceInstsArea_ << endl;
  cout << "utilization    : " << util << endl; 
  cout << endl;

  if( util >= 100.1 ) {
    cout << "Error: Util exceeds 100%." << endl;
    cout << "       Please double-check your die/row size" << endl;
    exit(1);
  }

}


static odb::adsRect 
getCoreRectFromDb(dbSet<odb::dbRow> &rows) {
  int minX = INT_MAX, minY = INT_MAX;
  int maxX = INT_MIN, maxY = INT_MIN;

  for(dbRow* row : rows) {
    adsRect rowRect;
    row->getBBox( rowRect );

    minX = std::min(rowRect.xMin(), minX);
    minY = std::min(rowRect.yMin(), minY);
    maxX = std::max(rowRect.xMax(), maxX);
    maxY = std::max(rowRect.yMax(), maxY);
  }
  return odb::adsRect(minX, minY, maxX, maxY);
}

// https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op
static int 
fastModulo(const int input, const int ceil) {
  return input >= ceil? input % ceil : input;
}

static std::pair<int, int>
getMinMaxIdx(int ll, int uu, int coreLL, int siteSize) {
  int lowerIdx = (ll - coreLL)/siteSize;
  int upperIdx =
   ( fastModulo((uu - coreLL), siteSize) == 0)? 
   (uu - coreLL) / siteSize : (uu - coreLL)/siteSize + 1;
  return std::make_pair(lowerIdx, upperIdx);
}


}


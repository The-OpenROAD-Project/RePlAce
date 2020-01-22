#include "opendb/db.h"

#include "nesterovBase.h"
#include "placerBase.h"

#include <algorithm>
#include <iostream>
#include <random>

namespace replace {

using namespace std;
using namespace odb;

static int 
fastModulo(const int input, const int ceil);

static uint32_t 
getOverlapArea(Bin* bin, GCell* cell);

static uint32_t
getOverlapArea(Bin* bin, Instance* inst);

static uint32_t
getDensityOverlapArea(Bin* bin, GCell* cell);


////////////////////////////////////////////////
// GCell 

GCell::GCell() 
  : lx_(0), ly_(0), ux_(0), uy_(0),
  dLx_(0), dLy_(0), dUx_(0), dUy_(0),
  densityScale_(0), 
  gradientX_(0), gradientY_(0) {}


GCell::GCell(Instance* inst) 
  : GCell() {
  setInstance(inst);
}

GCell::GCell(std::vector<Instance*>& insts) 
  : GCell() {
  setClusteredInstance(insts);
}

GCell::GCell(int cx, int cy, int dx, int dy) 
  : GCell() {
  lx_ = cx - dx/2;
  ly_ = cy - dy/2;
  ux_ = cx + dx/2;
  uy_ = cy + dy/2; 
  setFiller();
}

GCell::~GCell() {
  vector<Instance*> ().swap(insts_);
}
void
GCell::setInstance(Instance* inst) {
  insts_.push_back(inst);
}

// do nothing
void
GCell::setFiller() {
}

void
GCell::setClusteredInstance(std::vector<Instance*>& insts) {
  insts_ = insts;
}

int
GCell::lx() const {
  return lx_;
}
int
GCell::ly() const {
  return ly_; 
}

int
GCell::ux() const { 
  return ux_; 
}

int
GCell::uy() const {
  return uy_; 
}

int
GCell::cx() const {
  return (lx_ + ux_)/2;
}

int
GCell::cy() const {
  return (ly_ + uy_)/2;
}

int 
GCell::dx() const {
  return ux_ - lx_; 
}

int
GCell::dy() const {
  return uy_ - ly_;
}

int
GCell::dLx() const {
  return dLx_;
}

int
GCell::dLy() const {
  return dLy_;
}

int
GCell::dUx() const {
  return dUx_;
}

int
GCell::dUy() const {
  return dUy_;
}

int
GCell::dCx() const {
  return (dUx_ - dLx_)/2;
}

int
GCell::dCy() const {
  return (dUy_ - dLy_)/2;
}

int
GCell::dDx() const {
  return dUx_ - dLx_; 
}

int
GCell::dDy() const {
  return dUy_ - dLy_;
}

void
GCell::setLocation(int lx, int ly) {
  ux_ = lx + (ux_ - lx_);
  uy_ = ly + (uy_ - ly_);
  lx = lx_;
  ly = ly_;
}

void
GCell::setCenterLocation(int cx, int cy) {
  const int halfDx = dx()/2;
  const int halfDy = dy()/2;

  lx_ = cx - halfDx;
  ly_ = cy - halfDy;
  ux_ = cx + halfDx;
  uy_ = cy + halfDy;
}

// changing size and preserve center coordinates
void
GCell::setSize(int dx, int dy) {
  const int centerX = cx();
  const int centerY = cy();

  lx_ = centerX - dx/2;
  ly_ = centerY - dy/2;
  ux_ = centerX + dx/2;
  uy_ = centerY + dy/2;
}

void
GCell::setDensityLocation(int dLx, int dLy) {
  dUx_ = dLx + (dUx_ - dLx_);
  dUy_ = dLy + (dUy_ - dLy_);
  dLx_ = dLx;
  dLy_ = dLy;
}

void
GCell::setDensityCenterLocation(int dCx, int dCy) {
  const int halfDDx = dDx()/2;
  const int halfDDy = dDy()/2;

  dLx_ = dCx - halfDDx;
  dLy_ = dCy - halfDDy;
  dUx_ = dCx + halfDDx;
  dUy_ = dCy + halfDDy;
}

// changing size and preserve center coordinates
void
GCell::setDensitySize(int dDx, int dDy) {
  const int dCenterX = dCx();
  const int dCenterY = dCy();

  dLx_ = dCenterX - dDx/2;
  dLy_ = dCenterY - dDy/2;
  dUx_ = dCenterX + dDx/2;
  dUy_ = dCenterY + dDy/2;
}

void
GCell::setDensityScale(float densityScale) {
  densityScale_ = densityScale;
}

void
GCell::setGradientX(float gradientX) {
  gradientX_ = gradientX;
}

void
GCell::setGradientY(float gradientY) {
  gradientY_ = gradientY;
}

bool 
GCell::isInstance() const {
  return (insts_.size() == 1);
}

bool
GCell::isClusteredInstance() const {
  return (insts_.size() > 0);
}

bool
GCell::isFiller() const {
  return (insts_.size() == 0);
}

////////////////////////////////////////////////
// GNet

GNet::GNet()
  : lx_(0), ly_(0), ux_(0), uy_(0),
  customWeight_(1), weight_(1),
  waExpMinSumStorX_(0), waXExpMinSumStorX_(0),
  waExpMaxSumStorX_(0), waXExpMaxSumStorX_(0),
  waExpMinSumStorY_(0), waYExpMinSumStorY_(0),
  waExpMaxSumStorY_(0), waYExpMaxSumStorY_(0) {}

GNet::GNet(Net* net) : GNet() {
  nets_.push_back(net);
}

GNet::GNet(std::vector<Net*>& nets) : GNet() {
  nets_ = nets;
}

GNet::~GNet() {
  gPins_.clear();
  nets_.clear();
}

Net* 
GNet::net() const { 
  return *nets_.begin();
}

void
GNet::setCustomWeight(float customWeight) {
  customWeight_ = customWeight; 
}

void
GNet::addGPin(GPin* gPin) {
  gPins_.push_back(gPin);
}

////////////////////////////////////////////////
// GPin 

GPin::GPin()
  : gCell_(nullptr), gNet_(nullptr),
  offsetCx_(0), offsetCy_(0),
  cx_(0), cy_(0),
  posExpSum_(0), negExpSum_(0),
  hasPosExpSum_(0), hasNegExpSum_(0) {}

GPin::GPin(Pin* pin)
  : GPin() {
  pins_.push_back(pin);
  cx_ = pin->cx();
  cy_ = pin->cy();
  offsetCx_ = pin->offsetCx();
  offsetCy_ = pin->offsetCy();
}

GPin::GPin(std::vector<Pin*> & pins) {
  pins_ = pins;
}

GPin::~GPin() {
  gCell_ = nullptr;
  gNet_ = nullptr;
  pins_.clear();
}

Pin* 
GPin::pin() const {
  return *pins_.begin();
}

void
GPin::setCenterLocation(int cx, int cy) {
  cx_ = cx;
  cy_ = cy;
}

void
GPin::updateLocation(const GCell* gCell) {
  cx_ = gCell->lx() + offsetCx_;
  cy_ = gCell->ly() + offsetCy_;
}

////////////////////////////////////////////////////////
// Bin

Bin::Bin() 
  : lx_(0), ly_(0),
  ux_(0), uy_(0), 
  nonPlaceArea_(0), placedArea_(0),
  fillerArea_(0),
  phi_(0), density_(0) {}

Bin::Bin(int lx, int ly, int ux, int uy) 
  : Bin() {
  lx_ = lx; 
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

Bin::~Bin() {
  lx_ = ly_ = ux_ = uy_ = 0;
  nonPlaceArea_ = placedArea_ = fillerArea_ = 0;
  phi_ = density_ = electroForce_ = 0;
}

int 
Bin::lx() const {
  return lx_;
}
int
Bin::ly() const { 
  return ly_;
}

int
Bin::ux() const { 
  return ux_;
}

int
Bin::uy() const { 
  return uy_;
}

int
Bin::cx() const { 
  return (ux_ - lx_)/2;
}

int
Bin::cy() const { 
  return (uy_ - ly_)/2;
}

int
Bin::dx() const { 
  return (ux_ - lx_);
} 
int
Bin::dy() const { 
  return (uy_ - ly_);
}


uint32_t& 
Bin::nonPlaceArea() {
  return nonPlaceArea_; 
}
uint32_t&
Bin::placedArea() {
  return placedArea_; 
}

uint32_t&
Bin::fillerArea() {
  return fillerArea_;
}

float
Bin::phi() const {
  return phi_;
}

float
Bin::density() const {
  return density_;
}

float
Bin::electroForce() const {
  return electroForce_;
}

void
Bin::setPhi(float phi) {
  phi_ = phi;
}

void
Bin::setDensity(float density) {
  density_ = density;
}

void
Bin::setElectroForce(float electroForce) {
  electroForce_ = electroForce;
}

////////////////////////////////////////////////
// BinGrid

BinGrid::BinGrid()
  : lx_(0), ly_(0), ux_(0), uy_(0),
  binCntX_(0), binCntY_(0),
  binSizeX_(0), binSizeY_(0),
  targetDensity_(0), 
  isSetBinCntX_(0), isSetBinCntY_(0) {}

BinGrid::BinGrid(Die* die) : BinGrid() {
  setCoordi(die);
}

BinGrid::~BinGrid() {
  std::vector<Bin> ().swap(binStor_);
  std::vector<Bin*> ().swap(bins_);
  lx_ = ly_ = ux_ = uy_ = 0;
  binCntX_ = binCntY_ = 0;
  binSizeX_ = binSizeY_ = 0;
  isSetBinCntX_ = isSetBinCntY_ = 0;
}

void
BinGrid::setCoordi(Die* die) {
  lx_ = die->coreLx();
  ly_ = die->coreLy();
  ux_ = die->coreUx();
  uy_ = die->coreUy();
}

void
BinGrid::setPlacerBase(std::shared_ptr<PlacerBase> pb) {
  pb_ = pb;
}

void
BinGrid::setTargetDensity(float density) {
  targetDensity_ = density;
}

void
BinGrid::setBinCnt(int binCntX, int binCntY) {
  setBinCntX(binCntX);
  setBinCntY(binCntY);
}

void
BinGrid::setBinCntX(int binCntX) {
  isSetBinCntX_ = 1;
  binCntX_ = binCntX;
}

void
BinGrid::setBinCntY(int binCntY) {
  isSetBinCntY_ = 1;
  binCntY_ = binCntY;
}


int
BinGrid::lx() const {
  return lx_;
}
int
BinGrid::ly() const { 
  return ly_;
}

int
BinGrid::ux() const { 
  return ux_;
}

int
BinGrid::uy() const { 
  return uy_;
}

int
BinGrid::cx() const { 
  return (ux_ - lx_)/2;
}

int
BinGrid::cy() const { 
  return (uy_ - ly_)/2;
}

int
BinGrid::dx() const { 
  return (ux_ - lx_);
} 
int
BinGrid::dy() const { 
  return (uy_ - ly_);
}

void
BinGrid::initBins() {

  int32_t totalBinArea 
    = static_cast<int32_t>(ux_ - lx_) 
    * static_cast<int32_t>(uy_ - ly_);

  int32_t averagePlaceInstArea 
    = totalBinArea / pb_->placeInsts().size();

  int32_t idealBinArea = averagePlaceInstArea / targetDensity_;
  int idealBinCnt = totalBinArea / idealBinArea; 
  
  cout << "idealBinArea   : " << idealBinArea << endl;
  cout << "idealBinCnt    : " << idealBinCnt << endl;

  int foundBinCnt = 2;
  // find binCnt: 2, 4, 8, 16, 32, 64, ...
  // s.t. binCnt^2 <= idealBinCnt <= (binCnt*2)^2.
  for(foundBinCnt = 2; foundBinCnt <= 1024; foundBinCnt *= 2) {
    if( foundBinCnt * foundBinCnt <= idealBinCnt 
        && 4 * foundBinCnt * foundBinCnt > idealBinCnt ) {
      break;
    }
  }

  // setBinCntX_;
  if( !isSetBinCntX_ ) {
    binCntX_ = foundBinCnt;
  }

  // setBinCntY_;
  if( !isSetBinCntY_ ) {
    binCntY_ = foundBinCnt;
  }

  cout << "binCnt         : ( " << binCntX_ 
    << ", " << binCntY_ << " )" << endl;
  
  binSizeX_ = ceil(
      static_cast<float>((ux_ - lx_))/binCntX_);
  binSizeY_ = ceil(
      static_cast<float>((uy_ - ly_))/binCntY_);
  
  cout << "binSize        : ( " << binSizeX_
    << ", " << binSizeY_ << " )" << endl;

  // initialize binStor_, bins_ vector
  binStor_.resize(binCntX_ * binCntY_);
  int x = lx_, y = ly_;
  for(auto& bin : binStor_) {

    int sizeX = (x + binSizeX_ > ux_)? 
      ux_ - x : binSizeX_;
    int sizeY = (y + binSizeY_ > uy_)? 
      uy_ - y : binSizeY_;

    //cout << x << " " << y 
    //  << " " << x+sizeX << " " << y+sizeY << endl;
    bin = Bin(x, y, x+sizeX, y+sizeY);
    
    // move x, y coordinates.
    x += binSizeX_;
    if( x > ux_ ) {
      y += binSizeY_;
      x = lx_; 
    }

    bins_.push_back( &bin );
  }
  cout << "TotalBinCnt    : " << bins_.size() << endl;
}

void
BinGrid::updateBinsNonplaceArea(std::vector<Instance*>& nonPlaceInsts) {
}


// Core Part
void
BinGrid::updateBinsArea(std::vector<GCell*>& cells) {
  // clear the Bin-area info
  for(auto& bin : binStor_) {
    bin.placedArea_ = bin.fillerArea_ = 0;
  }

  for(auto& cell : cells) {
    std::pair<int, int> pairX = getMinMaxIdxX(cell);
    std::pair<int, int> pairY = getMinMaxIdxY(cell);
   
    if( cell->isInstance() ) {
      for(int i = pairX.first; i <= pairX.second; i++) {
        for(int j = pairY.first; j <= pairY.second; j++) {
          Bin* bin = &binStor_[ j * binCntX_ + i ];
          bin->placedArea() += getOverlapArea(bin, cell); 
        }
      }
    }
    else if( cell->isFiller() ) {
      for(int i = pairX.first; i <= pairX.second; i++) {
        for(int j = pairY.first; j <= pairY.second; j++) {
          Bin* bin = &binStor_[ j * binCntX_ + i ];
          bin->fillerArea() += getOverlapArea(bin, cell); 
        }
      }
    }
  }  
}


std::pair<int, int>
BinGrid::getMinMaxIdxX(GCell* gcell) {
  int lowerIdx = (gcell->lx() - lx())/binSizeX_;
  int upperIdx = 
   ( fastModulo((gcell->ux() - lx()), binSizeX_) == 0)? 
   (gcell->ux() - lx()) / binSizeX_ : (gcell->ux()-lx())/binSizeX_ + 1;
  return std::make_pair(lowerIdx, upperIdx);
}

std::pair<int, int>
BinGrid::getMinMaxIdxY(GCell* gcell) {
  int lowerIdx = (gcell->ly() - ly())/binSizeY_;
  int upperIdx =
   ( fastModulo((gcell->uy() - ly()), binSizeY_) == 0)? 
   (gcell->uy() - ly()) / binSizeY_ : (gcell->uy()-ly())/binSizeY_ + 1;
  return std::make_pair(lowerIdx, upperIdx);
}





////////////////////////////////////////////////
// NesterovBaseVars
NesterovBaseVars::NesterovBaseVars() 
: targetDensity(1.0), minAvgCut(0.1), maxAvgCut(0.9),
isSetBinCntX(0), isSetBinCntY(0), binCntX(0), binCntY(0) {}

void 
NesterovBaseVars::reset() {
  targetDensity = 1.0;
  minAvgCut = 0.1;
  maxAvgCut = 0.9;
  isSetBinCntX = isSetBinCntY = 0;
  binCntX = binCntY = 0;
}


////////////////////////////////////////////////
// NesterovBase 

NesterovBase::NesterovBase()
  : pb_(nullptr) {}

NesterovBase::NesterovBase(
    NesterovBaseVars nbVars, 
    std::shared_ptr<PlacerBase> pb)
  : NesterovBase() {
  nbVars_ = nbVars;
  pb_ = pb;
  init();
}

NesterovBase::~NesterovBase() {
  pb_ = nullptr;
}

void
NesterovBase::init() {
  // gCellStor init
  gCellStor_.reserve(pb_->insts().size());
  for(auto& inst: pb_->insts()) {
    GCell myGCell(inst); 
    gCellStor_.push_back(myGCell);
  }

  cout << "InstGCells     : " << gCellStor_.size() << endl;

  // gNetStor init
  gNetStor_.reserve(pb_->nets().size());
  for(auto& net : pb_->nets()) {
    GNet myGNet(net);
    gNetStor_.push_back(myGNet);
  }

  // gPinStor init
  gPinStor_.reserve(pb_->pins().size());
  for(auto& pin : pb_->pins()) {
    GPin myGPin(pin);
    gPinStor_.push_back(myGPin);
  }

  // update gFillerCells
  initFillerGCells();

  // send param into binGrid structure
  if( nbVars_.isSetBinCntX ) {
    bg_.setBinCntX(nbVars_.binCntX);
  }
  
  if( nbVars_.isSetBinCntY ) {
    bg_.setBinCntY(nbVars_.binCntY);
  }

  bg_.setPlacerBase(pb_);
  bg_.setCoordi(&(pb_->die()));
  bg_.setTargetDensity(nbVars_.targetDensity);
  
  // update binGrid info
  bg_.initBins();
}


// virtual filler GCells
void
NesterovBase::initFillerGCells() {
  // extract average dx/dy in range (10%, 90%)
  vector<int> dxStor;
  vector<int> dyStor;

  dxStor.reserve(pb_->placeInsts().size());
  dyStor.reserve(pb_->placeInsts().size());
  for(auto& placeInst : pb_->placeInsts()) {
    dxStor.push_back(placeInst->dx());
    dyStor.push_back(placeInst->dy());
  }
  
  // sort
  std::sort(dxStor.begin(), dxStor.end());
  std::sort(dyStor.begin(), dyStor.end());

  // average from (10 - 90%) .
  uint32_t dxSum = 0, dySum = 0;

  int minIdx = dxStor.size()*0.10;
  int maxIdx = dxStor.size()*0.90;
  for(int i=minIdx; i<maxIdx; i++) {
    dxSum += dxStor[i];
    dySum += dyStor[i];
  }

  // the avgDx and avgDy will be used as filler cells' 
  // width and height
  int avgDx = static_cast<int>(dxSum / (maxIdx - minIdx));
  int avgDy = static_cast<int>(dySum / (maxIdx - minIdx));

  cout << "FillerSize     : ( " 
    << avgDx << ", " << avgDy << " )" << endl;

  int64_t coreArea = 
    static_cast<int64_t>(pb_->die().coreDx()) *
    static_cast<int64_t>(pb_->die().coreDy()); 

  int64_t whiteSpaceArea = coreArea - 
    static_cast<int64_t>(pb_->nonPlaceInstsArea());

  int64_t movableArea = whiteSpaceArea * nbVars_.targetDensity;
  int64_t totalFillerArea = movableArea 
    - static_cast<int64_t>(pb_->placeInstsArea());

  if( totalFillerArea < 0 ) {
    cout << "ERROR: Filler area is negative!!" << endl;
    cout << "       Please put higher target density or " << endl;
    cout << "       Re-floorplan to have enough coreArea" << endl;
    exit(1);
  }

  int fillerCnt = 
    static_cast<int>(totalFillerArea 
        / static_cast<int32_t>(avgDx * avgDy));

  cout << "FillerGCells   : " << fillerCnt << endl;

  // 
  // mt19937 supports huge range of random values.
  // rand()'s RAND_MAX is only 32767.
  //
  mt19937 randVal(0);
  for(int i=0; i<fillerCnt; i++) {
    // place filler cells on random coordi and
    // set size as avgDx and avgDy
    GCell myGCell(
        randVal() % pb_->die().coreDx() + pb_->die().coreLx(), 
        randVal() % pb_->die().coreDy() + pb_->die().coreLy(),
        avgDx, avgDy );

    gCellStor_.push_back(myGCell);
  }
}


void
NesterovBase::reset() { 
  pb_ = nullptr;
}


// https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op
static int 
fastModulo(const int input, const int ceil) {
  return input >= ceil? input % ceil : input;
}

static uint32_t 
getOverlapArea(Bin* bin, GCell* cell) {
  int rectLx = max(bin->lx(), cell->lx()), 
      rectLy = max(bin->ly(), cell->ly()),
      rectUx = min(bin->ux(), cell->ux()), 
      rectUy = min(bin->uy(), cell->uy());

  if( rectLx >= rectUx || rectLy >= rectUy ) {
    return 0;
  }
  else {
    return static_cast<int32_t>(rectUx - rectLx) 
      * static_cast<int32_t>(rectUy - rectLy);
  }
}

static uint32_t 
getDensityOverlapArea(Bin* bin, GCell* cell) {
  int rectLx = max(bin->lx(), cell->dLx()), 
      rectLy = max(bin->ly(), cell->dLy()),
      rectUx = min(bin->ux(), cell->dUx()), 
      rectUy = min(bin->uy(), cell->dUy());

  if( rectLx >= rectUx || rectLy >= rectUy ) {
    return 0;
  }
  else {
    return static_cast<int32_t>(rectUx - rectLx) 
      * static_cast<int32_t>(rectUy - rectLy);
  }
}


static uint32_t
getOverlapArea(Bin* bin, Instance* inst) {
  int rectLx = max(bin->lx(), inst->lx()), 
      rectLy = max(bin->ly(), inst->ly()),
      rectUx = min(bin->ux(), inst->ux()), 
      rectUy = min(bin->uy(), inst->uy());

  if( rectLx >= rectUx || rectLy >= rectUy ) {
    return 0;
  }
  else {
    return static_cast<int32_t>(rectUx - rectLx) 
      * static_cast<int32_t>(rectUy - rectLy);
  }
}



}

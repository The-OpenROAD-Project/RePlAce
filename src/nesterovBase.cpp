#include "opendb/db.h"

#include "nesterovBase.h"
#include "placerBase.h"


namespace replace {

using namespace std;
using namespace odb;

GCell::GCell() :
  lx_(0), ly_(0), ux_(0), uy_(0),
  dLx_(0), dLy_(0), dUx_(0), dUy_(0),
  densityScale_(0), gradientX_(0), gradientY_(0) {}


GCell::GCell(Instance* inst) : GCell() {
  setInstance(inst);
}

GCell::GCell(std::vector<Instance*>& insts) : GCell() {
  setClusteredInstance(insts);
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
GCell::lx() {
  return lx_;
}
int
GCell::ly() {
  return ly_; 
}

int
GCell::ux() { 
  return ux_; 
}

int
GCell::uy() {
  return uy_; 
}

int
GCell::cx() {
  return (lx_ + ux_)/2;
}

int
GCell::cy() {
  return (ly_ + uy_)/2;
}

int 
GCell::dx() {
  return ux_ - lx_; 
}

int
GCell::dy() {
  return uy_ - ly_;
}

int
GCell::dLx() {
  return dLx_;
}

int
GCell::dLy() {
  return dLy_;
}

int
GCell::dUx() {
  return dUx_;
}

int
GCell::dUy() {
  return dUy_;
}

int
GCell::dCx() {
  return (dUx_ - dLx_)/2;
}

int
GCell::dCy() {
  return (dUy_ - dLy_)/2;
}

int
GCell::dDx() {
  return dUx_ - dLx_; 
}

int
GCell::dDy() {
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
GCell::isInstance() {
  return (insts_.size() == 1);
}

bool
GCell::isClusteredInstance() {
  return (insts_.size() > 0);
}

bool
GCell::isFiller() {
  return (insts_.size() == 0);
}

////////////////////////////////////////////////
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
Bin::lx() {
  return lx_;
}
int
Bin::ly() { 
  return ly_;
}

int
Bin::ux() { 
  return ux_;
}

int
Bin::uy() { 
  return uy_;
}

int
Bin::cx() { 
  return (ux_ - lx_)/2;
}

int
Bin::cy() { 
  return (uy_ - ly_)/2;
}

int
Bin::dx() { 
  return (ux_ - lx_);
} 
int
Bin::dy() { 
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
Bin::phi() {
  return phi_;
}

float
Bin::density() {
  return density_;
}

float
Bin::electroForce() {
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


BinGrid::BinGrid()
  : lx_(0), ly_(0), ux_(0), uy_(0),
  binCntX_(0), binCntY_(0),
  binSizeX_(0), binSizeY_(0),
  isSetBinCntX_(0), isSetBinCntY_(0) {}

BinGrid::BinGrid(Die* die) : BinGrid() {
  updateCoordi(die);
}

BinGrid::~BinGrid() {
  std::vector<Bin> ().swap(bins_);
  std::vector<Bin*> ().swap(binsPtr_);
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

void
BinGrid::updateBinsArea(std::vector<GCell>& cells) {
  
}

int
BinGrid::lx() {
  return lx_;
}
int
BinGrid::ly() { 
  return ly_;
}

int
BinGrid::ux() { 
  return ux_;
}

int
BinGrid::uy() { 
  return uy_;
}

int
BinGrid::cx() { 
  return (ux_ - lx_)/2;
}

int
BinGrid::cy() { 
  return (uy_ - ly_)/2;
}

int
BinGrid::dx() { 
  return (ux_ - lx_);
} 
int
BinGrid::dy() { 
  return (uy_ - ly_);
}

void
BinGrid::initBins() {
  // setBinCntX_;
  if( !isSetBinCntX_ ) {
  }

  // setBinCntY_;
  if( !isSetBinCntY_ ) {
  }
  
  binSizeX_ = ceil(
      static_cast<float>((ux_ - lx_))/binCntX_);
  binSizeY_ = ceil(
      static_cast<float>((uy_ - ly_))/binCntY_);

  // initialize bins_, binsPtr_ vector
  bins_.resize(binCntX_ * binCntY_);
  int x = lx_, y = ly_;
  for(auto& bin : bins_) {

    int sizeX = (x + binSizeX_ > ux_)? 
      ux_ : x + binSizeX_;
    int sizeY = (y + binSizeY_ > uy_)? 
      uy_ : y + binSizeY_;

    bin = Bin(x, y, x+sizeX, y+sizeY);
    
    // move x, y coordinates.
    x += binSizeX_;
    if( x > ux_ ) {
      y += binSizeY_;
      x = lx_; 
    }

    binsPtr_.push_back( &bin );
  }
}




}

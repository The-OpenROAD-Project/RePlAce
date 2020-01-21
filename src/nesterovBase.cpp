#include "opendb/db.h"

#include "nesterovBase.h"
#include "placerBase.h"


namespace replace {

using namespace std;
using namespace odb;

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

}

void
GPin::updateLocation(GCell* gCell) {
  cx_ = gCell->lx() + offsetCx_;
  cy_ = gCell->ly() + offsetCy_;
}


////////////////////////////////////////////////
// NesterovBase 

NesterovBase::NesterovBase()
  : pb_(nullptr) {}

NesterovBase::NesterovBase(PlacerBase* pb)
  : NesterovBase() {
  pb_ = pb;
}

NesterovBase::~NesterovBase() {
  pb_ = nullptr;
}

void
NesterovBase::init() {

}

void
NesterovBase::reset() { 

}








}

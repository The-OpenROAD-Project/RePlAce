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


}

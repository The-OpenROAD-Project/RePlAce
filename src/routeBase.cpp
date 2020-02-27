#include "routeBase.h"
#include <opendb/db.h>

namespace replace {

Tile::Tile()
: x_(0), y_(0), 
  lx_(0), ly_(0), ux_(0), uy_(0),
  sumUsageH_(0), sumUsageV_(0),
  supplyH_(0), supplyV_(0),
  inflationRatioH_(0),
  inflationRatioV_(0),
  inflationRatio_(0),
  inflationArea_(0),
  inflationAreaDelta_(0),
  inflatedRatio_(0) {}

Tile::Tile(int x, int y, int lx, int ly, int ux, int uy, int layers) 
: Tile() {
  x_ = x;
  y_ = y;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux; 
  uy_ = uy;

  blockage_.resize(layers, 0);
  capacity_.resize(layers, 0);
  route_.resize(layers, 0);

  usageHL_.resize(layers, 0);
  usageHR_.resize(layers, 0);
  usageVL_.resize(layers, 0);
  usageVR_.resize(layers, 0);
}

Tile::~Tile() {
  reset();
}

void
Tile::reset() {
  x_ = y_ = lx_ = ly_ = ux_ = uy_ = 0;
  blockage_.clear();
  capacity_.clear();
  route_.clear();

  usageHL_.clear();
  usageHR_.clear();
  usageVL_.clear();
  usageVR_.clear();
  
  usageHL_.shrink_to_fit();
  usageHR_.shrink_to_fit();
  usageVL_.shrink_to_fit();
  usageVR_.shrink_to_fit();
}

int
Tile::blockage(int layer) const {
  return blockage_[layer];
}

int
Tile::capacity(int layer) const {
  return capacity_[layer];
}

int
Tile::route(int layer) const {
  return route_[layer];
}

int
Tile::usageHL(int layer) const {
  return usageHL_[layer];
}

int
Tile::usageHR(int layer) const {
  return usageHR_[layer];
}

int
Tile::usageVL(int layer) const {
  return usageVL_[layer];
}

int 
Tile::usageVR(int layer) const {
  return usageVR_[layer];
}

void
Tile::setBlockage(int layer, int blocakge) {
  blockage_[layer] = blockage;
}

void
Tile::setCapacity(int layer, int capacity) {
  capacity_[layer] = capacity;
}

void
Tile::setRoute(int layer, int route) {
  route_[layer] = route;
}

void
Tile::setUsageHL(int layer, int usage) { 
  usageHL_[layer] = usage;
}

void
Tile::setUsageHR(int layer, int usage) { 
  usageHR_[layer] = usage;
}

void
Tile::setUsageVL(int layer, int usage) { 
  usageVL_[layer] = usage;
}

void
Tile::setUsageVR(int layer, int usage) { 
  usageVR_[layer] = usage;
}

float 
Tile::sumUsageH() const {
  return sumUsageH_;
}

float
Tile::sumUsasgeV() const {
  return sumSuageV_; 
}

float
Tile::supplyH() const {
  return supplyH_;
}

float
Tile::supplyV() const {
  return supplyV_;
}


TileGrid::TileGrid()
  : db_(nullptr), 
  lx_(0), ly_(0), 
  ux_(0), uy_(0) {}

TileGrid::~TileGrid() {
  reset();
}

void
TileGrid::reset() {
  db_ = nullptr;
  lx_ = ly_ = ux_ = uy_ = 0;

  tileStor_.clear();
  tiles_.clear();

  tileStor_.shrink_to_fit();
  tiles_.shrink_to_fit();
}

void
TileGrid::setDb(odb::dbDatabase* db) {
  db_ = db;
}

void
TileGrid::initTiles() {
  const int numLayer = db_->getTech()->getRoutingLayerCount();
  
}

void
TileGrid::initFromGuide(const char* fileName) {

}


RouteBase::RouteBase()
  : {};

RouteBase::~RouteBase() {
  reset();
}


void 
RouteBase::reset() {

}

void
RouteBase::init() {

}











}

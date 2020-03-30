#include "routeBase.h"
#include "logger.h"
#include "nesterovBase.h"
#include <opendb/db.h>
#include <string>
#include <iostream>
#include <cmath>

using std::vector;
using std::string;
using std::to_string; 

namespace replace {

Tile::Tile()
: x_(0), y_(0), 
  lx_(0), ly_(0), ux_(0), uy_(0),
  pinCnt_(0),
  usageH_(0), usageV_(0),
  supplyH_(0), supplyV_(0),
  supplyHL_(0), supplyHR_(0),
  supplyVL_(0), supplyVR_(0), 
  inflationRatioH_(0),
  inflationRatioV_(0),
  inflationRatio_(0),
  inflationArea_(0),
  inflationAreaDelta_(0),
  inflatedRatio_(0), 
  isMacroIncluded_(false) {}

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
  x_ = y_ = lx_ = ly_ = ux_ = uy_ = pinCnt_ = 0;
  usageH_ = usageV_ = supplyH_ = supplyV_ = 0;
  supplyHL_ = supplyHR_ = supplyVL_ = supplyVR_ = 0;
  inflationRatioH_ = inflationRatioV_ = 0;
  inflationRatio_ = 0;

  inflationArea_ = inflationAreaDelta_ = 0;
  inflatedRatio_ = 0;
  isMacroIncluded_ = false;

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

float 
Tile::usageH() const {
  return usageH_;
}

float
Tile::usageV() const {
  return usageV_; 
}

float
Tile::supplyHL() const {
  return supplyHL_;
}
float
Tile::supplyHR() const {
  return supplyHR_;
}

float
Tile::supplyVL() const {
  return supplyVL_;
}
float
Tile::supplyVR() const {
  return supplyVR_;
}

float
Tile::supplyH() const {
  return supplyH_; 
}

float
Tile::supplyV() const {
  return supplyV_;
}

float 
Tile::inflationRatioH() const {
  return inflationRatioH_;
}

float 
Tile::inflationRatioV() const {
  return inflationRatioV_;
}

float
Tile::inflationRatio() const {
  return inflationRatio_;
}

float
Tile::inflationArea() const {
  return inflationArea_;
}

float
Tile::inflationAreaDelta() const {
  return inflationAreaDelta_;
}

float
Tile::inflatedRatio() const {
  return inflatedRatio_;
}

bool
Tile::isMacroIncluded() const {
  return isMacroIncluded_;
}

int
Tile::pinCnt() const {
  return pinCnt_;
}



void
Tile::setBlockage(int layer, int block) {
  blockage_[layer] = block;
}

void
Tile::setCapacity(int layer, int capacity) {
  capacity_[layer] = capacity;
}

void
Tile::setCapacity(vector<int>& capacity) {
  capacity_ = capacity; 
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


void
Tile::setSupplyH(float supply) {
  supplyH_ = supply;
}

void
Tile::setSupplyV(float supply) {
  supplyV_ = supply;
}

// set func for supply*
void
Tile::setSupplyHL(float supply) {
  supplyHL_ = supply;
}

void
Tile::setSupplyHR(float supply) {
  supplyHR_ = supply;
}

void
Tile::setSupplyVL(float supply) {
  supplyVL_ = supply; 
}

void
Tile::setSupplyVR(float supply) {
  supplyVR_ = supply; 
}

void
Tile::setInflationRatioH(float val) {
  inflationRatioH_ = val;
}

void
Tile::setInflationRatioV(float val) {
  inflationRatioV_ = val;
}

void
Tile::setInflationRatio(float val) {
  inflationRatio_ = val;
}

void
Tile::setInflationArea(float area) {
  inflationArea_ = area;
}

void
Tile::setInflationAreaDelta(float delta) {
  inflationAreaDelta_ = delta;
}


void
Tile::setMacroIncluded(bool mode) {
  isMacroIncluded_ = mode;
}

void 
Tile::setPinCnt(int cnt) {
  pinCnt_ = cnt;
}

void 
Tile::updateUsages() {
  usageH_ = 0;
  usageV_ = 0;

  int grSumUsageH = 0, grSumUsageV = 0;

  using std::cout;
  using std::endl;

  for(int i=0; i<usageHL_.size(); i++) {
    cout << "final usage " << x_ << " " << y_ << " " << i << " " << usageHL_[i] << " " << usageHR_[i] << " " << usageVL_[i] << " " << usageVR_[i] << endl;
    grSumUsageH += std::max( usageHL_[i], usageHR_[i] );
    grSumUsageV += std::max( usageVL_[i], usageVR_[i] );
  }

  // scaled by tileSizeX and tileSizeY
  usageH_ = static_cast<float>(grSumUsageH) 
    * static_cast<float>(ux() - lx());
  usageV_ = static_cast<float>(grSumUsageV) 
    * static_cast<float>(uy() - ly());
}



TileGrid::TileGrid()
  : lx_(0), ly_(0), 
  tileCntX_(0), tileCntY_(0), 
  tileSizeX_(0), tileSizeY_(0),
  numRoutingLayers_(0) {}

TileGrid::~TileGrid() {
  reset();
}

void
TileGrid::reset() {
  lx_ = ly_ = 0; 
  tileCntX_ = tileCntY_ = 0;
  tileSizeX_ = tileSizeY_ = 0;
  numRoutingLayers_ = 0;

  tileStor_.clear();
  tiles_.clear();

  tileStor_.shrink_to_fit();
  tiles_.shrink_to_fit();
}

void
TileGrid::setLogger(std::shared_ptr<Logger> log) {
  log_ = log;
}

void
TileGrid::setTileCnt(int tileCntX, int tileCntY) {
  tileCntX_ = tileCntX;
  tileCntY_ = tileCntY;
}

void
TileGrid::setTileCntX(int tileCntX) {
  tileCntX_ = tileCntX;
}

void
TileGrid::setTileCntY(int tileCntY) {
  tileCntY_ = tileCntY;  
}

void
TileGrid::setTileSize(int tileSizeX, int tileSizeY) {
  tileSizeX_ = tileSizeX;
  tileSizeY_ = tileSizeY;
}

void
TileGrid::setTileSizeX(int tileSizeX) {
  tileSizeX_ = tileSizeX;
}

void
TileGrid::setTileSizeY(int tileSizeY) {
  tileSizeY_ = tileSizeY; 
}

void
TileGrid::setNumRoutingLayers(int num) {
  numRoutingLayers_ = num;
}

void
TileGrid::setLx(int lx) {
  lx_ = lx;
}

void
TileGrid::setLy(int ly) {
  ly_ = ly;
}

void
TileGrid::initTiles() {
  log_->infoIntPair("TileLxLy", lx_, ly_);
  log_->infoIntPair("TileSize", tileSizeX_, tileSizeY_);
  log_->infoIntPair("TileCnt", tileCntX_, tileCntY_);
  log_->infoInt("numRoutingLayers", numRoutingLayers_);

  // 2D tile grid structure init
  int x = lx_, y = ly_;
  int idxX = 0, idxY = 0;
  tileStor_.resize(tileCntX_ * tileCntY_);
  for(auto& tile : tileStor_) {
    tile = Tile(idxX, idxY, x, y, 
        x + tileSizeX_, y + tileSizeY_,
        numRoutingLayers_); 

    x += tileSizeX_;
    idxX += 1;
    if( x >= ux() ) {
      y += tileSizeY_;
      x = lx_;

      idxY ++;
      idxX = 0;
    }

    tiles_.push_back( &tile );
  }
  log_->infoInt("NumTiles", tiles_.size());
}

int
TileGrid::lx() const {
  return lx_; 
}

int 
TileGrid::ly() const {
  return ly_;
}

    // this is points
int 
TileGrid::ux() const {
  return lx_ + tileCntX_ * tileSizeX_;
}
int 
TileGrid::uy() const {
  return ly_ + tileCntY_ * tileSizeY_;
}

int
TileGrid::tileCntX() const {
  return tileCntX_;
}

int
TileGrid::tileCntY() const {
  return tileCntY_; 
}

int
TileGrid::tileSizeX() const {
  return tileSizeX_;
}

int
TileGrid::tileSizeY() const {
  return tileSizeY_;
}

int
TileGrid::numRoutingLayers() const {
  return numRoutingLayers_;
}

/////////////////////////////////////////////
// EdgeCapacityInfo 
//
EdgeCapacityInfo::EdgeCapacityInfo()
: lx(0), ly(0), ll(0), ux(0), uy(0), ul(0), capacity(0) {}

EdgeCapacityInfo::EdgeCapacityInfo(int lx1, int ly1, int ll1,
  int ux1, int uy1, int ul1, int capacity1)
  : lx(lx1), ly(ly1), ll(ll1), ux(ux1), uy(uy1), ul(ul1), capacity(capacity1) {}


/////////////////////////////////////////////
// RoutingTrack  
//
RoutingTrack::RoutingTrack() 
: lx(0), ly(0), ux(0), uy(0), layer(0), gNet(nullptr) {}

RoutingTrack::RoutingTrack(int lx1, int ly1, int ux1, int uy1, int layer1, GNet* gNet1) 
: RoutingTrack() {
  lx = lx1;
  ly = ly1;
  ux = ux1;
  uy = uy1;
  layer = layer1;
  gNet = gNet1;
}


/////////////////////////////////////////////
// RouteBaseVars

RouteBaseVars::RouteBaseVars()
  : gRoutePitchScale(1.09), 
  edgeAdjustmentCoef(1.19),
  pinCoef(1.66), 
  inflationRatioCoef(2.33), 
  maxInflationRatio(2.5), 
  blockagePorosity(0) {}

void 
RouteBaseVars::reset() {
  gRoutePitchScale = 1.09;
  edgeAdjustmentCoef = 1.19;
  pinCoef = 1.66;
  inflationRatioCoef = 2.33;
  maxInflationRatio = 2.5;
  blockagePorosity = 0;
}

/////////////////////////////////////////////
// RouteBase

RouteBase::RouteBase()
  : rbVars_(), 
  db_(nullptr), nb_(nullptr), log_(nullptr) {}

RouteBase::RouteBase(
    RouteBaseVars rbVars, 
    odb::dbDatabase* db, 
    std::shared_ptr<NesterovBase> nb,
    std::shared_ptr<Logger> log)
  : RouteBase() {
  rbVars_ = rbVars;
  db_ = db;
  nb_ = nb;
  log_ = log;

  init();
}

RouteBase::~RouteBase() {
  reset();
}

void 
RouteBase::reset() {
  rbVars_.reset();
  db_ = nullptr;
  nb_ = nullptr;
  log_ = nullptr;

  verticalCapacity_.clear();
  horizontalCapacity_.clear();
  minWireWidth_.clear();
  minWireSpacing_.clear();
  edgeCapacityStor_.clear();
  routingTracks_.clear();

  verticalCapacity_.shrink_to_fit();
  horizontalCapacity_.shrink_to_fit();
  minWireWidth_.shrink_to_fit();
  minWireSpacing_.shrink_to_fit();
  edgeCapacityStor_.shrink_to_fit();
  routingTracks_.shrink_to_fit();
}

void
RouteBase::init() {
  tg_.setLogger(log_);

  importRoute("input.route");
  log_->infoString("input.route parsing is done");

  importEst("out.guide.est");
  log_->infoString("out.guide.est parsing is done");

  tg_.initTiles();
}

void
RouteBase::updateCongestionMap() {
  updateSupplies();
  updateUsages();
  updatePinCount();
  updateInflationRatio();

  log_->infoString("Congestion Map building is done");
}

// fill 
//
// TileGrids' 
// lx_ ly_ 
// tileCntX_ tileCntY_
// tileSizeX_ tileSizeY_ 
//
// rbVars_' 
// blockagePorosity_
//
// RouteBases' 
// verticalCapacity_
// horizontalCapacity_
//
// minWireWidth_
// minWireSpacing_
//
// edgeCapacityStor_

// following code is temp!
void
RouteBase::importRoute(const char* fileName) {
  char *token = NULL;
  char temp[255];
  char line[255];
  bool blockageFlag = false;
  bool beolFlag = false;
  bool edgeFlag = false;

  FILE *fp = nullptr;
  if((fp = fopen(fileName, "r")) == NULL) {
    log_->error("Cannot open " + string(fileName) + " file!", 999);
    exit(1);
  }
  

  int tileCntX = 0, tileCntY = 0, numRoutingLayers = 0;

  while(!feof(fp)) {
    *line = '\0';
    char* ptr = fgets(line, 255, fp);
    sscanf(line, "%s%*s", temp);

    if(strlen(line) < 5 || temp[0] == '#' || strcmp(temp, "route") == 0)
      continue;
    if(strcmp(temp, "NumBlockageNodes") == 0) {
      blockageFlag = true;
      continue;
    }
    if(strcmp(temp, "NumEdgeCapacityAdjustments") == 0) {
      blockageFlag = false;
      edgeFlag = true;
      continue;
    }
    if(strcmp(temp, "Grid") == 0) {
      beolFlag = true;
    }
    if(strcmp(temp, "NumNiTerminals") == 0) {
      beolFlag = false;
      continue;
    }

    if(beolFlag) {
      sscanf(line, "%s :%*s", temp);
      if(strcmp(temp, "Grid") == 0) {

        sscanf(line, "%*s : %d %d %d", &tileCntX, &tileCntY, &numRoutingLayers);

        tg_.setTileCnt(tileCntX, tileCntY);
        tg_.setNumRoutingLayers(numRoutingLayers);
      }
      else if(strcmp(temp, "VerticalCapacity") == 0) {
        verticalCapacity_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < numRoutingLayers; i++) {
          verticalCapacity_.push_back(atoi(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "HorizontalCapacity") == 0) {
        horizontalCapacity_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < numRoutingLayers; i++) {
          horizontalCapacity_.push_back(atoi(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "MinWireWidth") == 0) {
        minWireWidth_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < numRoutingLayers; i++) {
          minWireWidth_.push_back(atof(token));

          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "MinWireSpacing") == 0) {
        minWireSpacing_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < numRoutingLayers; i++) {
          minWireSpacing_.push_back(atof(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "ViaSpacing") == 0) {
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < numRoutingLayers; i++) {
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "GridOrigin") == 0) {
        double temp_gridLLx, temp_gridLLy;
        sscanf(line, "%*s : %lf %lf", &temp_gridLLx, &temp_gridLLy);

        int lx = 0, ly = 0;
        lx = static_cast<int>(round(temp_gridLLx));
        ly = static_cast<int>(round(temp_gridLLy));

        tg_.setLx(lx);
        tg_.setLy(ly);
      }
      else if(strcmp(temp, "TileSize") == 0) {
        double temp_tileWidth, temp_tileHeight;
        sscanf(line, "%*s : %lf %lf", &temp_tileWidth, &temp_tileHeight);
        int tileSizeX = 0, tileSizeY = 0;
        tileSizeX = static_cast<int>(round(temp_tileWidth));
        tileSizeY = static_cast<int>(round(temp_tileHeight));

        tg_.setTileSize(tileSizeX, tileSizeY);
      }
      else if(strcmp(temp, "BlockagePorosity") == 0) {
        double temp_blockagePorosity;
        sscanf(line, "%*s : %lf", &temp_blockagePorosity);
        // blockagePorosity changes..!
        rbVars_.blockagePorosity 
          = static_cast<float>(temp_blockagePorosity);
      }
    }

    // No need to care
    if(blockageFlag) {
    }

    if(edgeFlag) {
      int e1, e2, e3, e4, e5, e6, e7;
      sscanf(line, "%d %d %d %d %d %d %d ", &e1, &e2, &e3, &e4, &e5, &e6, &e7);
      edgeCapacityStor_.push_back(EdgeCapacityInfo(e1, e2, e3, e4, e5, e6, e7));
    }
  }
}

// Fill routingTracks_;
// following code is temp!
void 
RouteBase::importEst(const char* fileName) {

  // *.est importing.
  //

  FILE *fp = fopen(fileName, "r");
  if(fp == NULL) {
    log_->error("Cannot open " + string(fileName) + " file!", 999);
		exit(1);
  }
      
  char temp[255] = {0, };
  char netName[255] = {0, };
  char line[255] = {0, };
  bool flag = false;

  while(!feof(fp)) {
    *line = '\0';
    fgets(line, 255, fp);
    sscanf(line, "%s%*s", temp);

    if(strlen(line) < 3)
      continue;

    if(temp[0] != '(') {
			int netIdx = 0;
      sscanf(line, "%s %d%*s", netName, &netIdx);
      flag = true;
      continue;
    }

    if(temp[0] == '!') {
      flag = false;
      continue;
    }

    if(flag) {
      if(temp[0] == '(') {
  			int lx = 0, ly = 0, ll = 0, 
            ux = 0, uy = 0, ul = 0;

        sscanf(line, "(%d,%d,%d)-(%d,%d,%d)%*s", &lx, &ly, &ll, 
            &ux, &uy, &ul);

        // This is unexpected...!
        if(ll != ul)
          continue;

        odb::dbNet* dbNet = 
          db_->getChip()->getBlock()->findNet(netName);
        //log_->infoString("net " + string(netName) + " " + string(dbNet->getConstName()));
        GNet* gNet = nb_->dbToNb(dbNet);
        routingTracks_.push_back(
            RoutingTrack(lx, ly, ux, uy, ll, gNet));
      }
    }
  }
}

void
RouteBase::updateSupplies() {
  int numHTracks = 0;
  int numVTracks = 0;
  for(int i=0; i<tg_.numRoutingLayers(); i++) {
    numVTracks += 
      verticalCapacity_[i] 
      / (minWireWidth_[i] + minWireSpacing_[i]);
    numHTracks +=
      horizontalCapacity_[i] 
      / (minWireWidth_[i] + minWireSpacing_[i]);
  }
  
  log_->infoInt("NumHTracks", numHTracks);
  log_->infoInt("NumVTracks", numVTracks);
  
  int pitchH = std::round(static_cast<float>(tg_.tileSizeY()) 
      / numHTracks * rbVars_.gRoutePitchScale);
  int pitchV = std::round(static_cast<float>(tg_.tileSizeX()) 
      / numVTracks * rbVars_.gRoutePitchScale);

  for(auto& tile : tg_.tiles()) {
    tile->setSupplyHL( tile->area() / pitchH );
    tile->setSupplyHR( tile->supplyHL() );
    tile->setSupplyVL( tile->area() / pitchV );
    tile->setSupplyVR( tile->supplyVL() );
  }

  log_->infoInt("PitchH", pitchH );
  log_->infoInt("PitchV", pitchV );

  // apply edgeCapacityInfo from *.route
  // update supplyH/V
  for(auto& ecInfo : edgeCapacityStor_) {
    bool isHorizontal = (ecInfo.ly == ecInfo.uy);
    
    // l : lower
    // u : upper
    // index
    int lx = std::min( ecInfo.lx, ecInfo.ux );
    int ux = std::max( ecInfo.lx, ecInfo.ux );
    int ly = std::min( ecInfo.ly, ecInfo.uy );
    int uy = std::max( ecInfo.ly, ecInfo.uy );

    // Note that ecInfo.ll == ecInfo.ul
    assert( ecInfo.ll == ecInfo.ul );
    int layer = ecInfo.ll - 1;
    int capacity = ecInfo.capacity;

    Tile* lTile = tg_.tiles()[ly * tg_.tileCntX() + lx];
    Tile* uTile = tg_.tiles()[uy * tg_.tileCntX() + ux];

    if( isHorizontal ) {
      // lower -> right edge
      lTile->setSupplyHR( 
          lTile->supplyHR() - 
          static_cast<float>((horizontalCapacity_[layer] - capacity)) /
          static_cast<float>((minWireWidth_[layer] + minWireSpacing_[layer])) /
          rbVars_.edgeAdjustmentCoef * tg_.tileSizeX() );

      // upper -> left edge
      uTile->setSupplyHL(
          uTile->supplyHL() -
          static_cast<float>((horizontalCapacity_[layer] - capacity)) /
          static_cast<float>((minWireWidth_[layer] + minWireSpacing_[layer])) /
          rbVars_.edgeAdjustmentCoef * tg_.tileSizeX() );

      // lower layer check
      if( layer <= 4 && horizontalCapacity_[layer] > 0 &&
          capacity < 0.01 ) {
        lTile->setMacroIncluded(true);
      }
    }
    else {
      // lower -> right edge
      lTile->setSupplyVR( 
          lTile->supplyVR() - 
          static_cast<float>((verticalCapacity_[layer] - capacity)) /
          static_cast<float>((minWireWidth_[layer] + minWireSpacing_[layer])) /
          rbVars_.edgeAdjustmentCoef * tg_.tileSizeY() );
      
      // upper -> left edge
      uTile->setSupplyVL(
          uTile->supplyVL() -
          static_cast<float>((verticalCapacity_[layer] - capacity)) /
          static_cast<float>((minWireWidth_[layer] + minWireSpacing_[layer])) /
          rbVars_.edgeAdjustmentCoef * tg_.tileSizeY() );
      
      // lower layer check
      if( layer <= 4 && verticalCapacity_[layer] > 0 &&
          capacity < 0.01 ) {
        lTile->setMacroIncluded(true);
      }
    }
  } 
  
  // fill capacity
  std::vector<int> capacity(tg_.numRoutingLayers(), 0);
  for(int i=0; i<tg_.numRoutingLayers(); i++) {
    if( horizontalCapacity_[i] > 0 ) {
      capacity[i] = horizontalCapacity_[i]; 
    }
    else {
      capacity[i] = verticalCapacity_[i];
    }
  }

  for(auto& tile : tg_.tiles()) {
    // set H, V from L, R
    tile->setSupplyH( std::fmin( tile->supplyHL(), tile->supplyHR() ) );
    tile->setSupplyV( std::fmin( tile->supplyVL(), tile->supplyVR() ) );

    // set capacity initially
    tile->setCapacity( capacity );
  }
  
  // apply edgeCapacityInfo from *.route
  // update blockage from possible capacity
  for(auto& ecInfo : edgeCapacityStor_) {
    
    int lx = std::min( ecInfo.lx, ecInfo.ux );
    int ly = std::min( ecInfo.ly, ecInfo.uy );
    int layer = ecInfo.ll - 1;
    int capacity = ecInfo.capacity;
    
    Tile* tile = tg_.tiles()[ly * tg_.tileCntX() + lx];
    tile->setBlockage(layer, 
        tile ->blockage(layer) 
        + tile->capacity(layer) - capacity);
  }
}


// update tiles' usageHR, usageHL, usageVR, usage VL
void
RouteBase::updateUsages() {
  for (auto& rTrack : routingTracks_) {
    bool isHorizontal = ( rTrack.ly == rTrack.uy );
    
    using std::cout;
    using std::endl;

    cout << "rTrack: " << rTrack.lx << " " << rTrack.ly << " " 
      << rTrack.ux << " " << rTrack.uy << " " << rTrack.layer -1 << endl; 

    // points
    int lx = std::min( rTrack.lx, rTrack.ux );
    int ux = std::max( rTrack.lx, rTrack.ux );
    int ly = std::min( rTrack.ly, rTrack.uy );
    int uy = std::max( rTrack.ly, rTrack.uy );

    int layer = rTrack.layer - 1;

    // getIdx from coordinates.
    int lIdxX = (lx - tg_.lx())/tg_.tileSizeX();
    int lIdxY = (ly - tg_.ly())/tg_.tileSizeY();
    int uIdxX = (ux - tg_.lx())/tg_.tileSizeX();
    int uIdxY = (uy - tg_.ly())/tg_.tileSizeY();


    if( lIdxX < 0 || lIdxX >= tg_.tileCntX() ) {
      log_->error("lIdxX is wrong. Check the *.est file. lIdx: " + to_string(lIdxX), 100);
    }
    if( lIdxY < 0 || lIdxY >= tg_.tileCntY() ) {
      log_->error("lIdxY is wrong. Check the *.est file. lIdxY: " + to_string(lIdxY), 100);
    }
    if( uIdxX < 0 || uIdxX >= tg_.tileCntX() ) {
      log_->error("uIdxX is wrong. Check the *.est file. uIdxX: " + to_string(uIdxX), 100);
    }
    if( uIdxY < 0 || uIdxY >= tg_.tileCntY() ) {
      log_->error("uIdxY is wrong. Check the *.est file. uIdxY: " + to_string(uIdxY), 100);
    }
   
    // get lTile and uTile using lx, ly, ux, uy 
    Tile* lTile = tg_.tiles()[lIdxY * tg_.tileCntX() + lIdxX];
    Tile* uTile = tg_.tiles()[uIdxY * tg_.tileCntX() + uIdxX];
    // horizontal
    if( isHorizontal ) {
      lTile->setUsageHR( layer, lTile->usageHR(layer) + 1 );
      uTile->setUsageHL( layer, uTile->usageHL(layer) + 1 );
      cout << "HR " << lIdxX << " " << lIdxY << " " << layer << " " << lTile->usageHR(layer) << endl;
      cout << "HL " << uIdxX << " " << uIdxY << " " << layer << " " << uTile->usageHL(layer) << endl;


    }
    // vertical
    else {
      lTile->setUsageVR( layer, lTile->usageVR(layer) + 1 );
      uTile->setUsageVL( layer, uTile->usageVL(layer) + 1 );
      
      cout << "VR " << lIdxX << " " << lIdxY << " " << layer << " " << lTile->usageVR(layer) << endl;
      cout << "VL " << uIdxX << " " << uIdxY << " " << layer << " " << uTile->usageVL(layer) << endl;
    }

    // update route info
    lTile->setRoute(layer, lTile->route(layer) 
        + minWireWidth_[layer] + minWireSpacing_[layer] );
  }

  
  // update usageH and usageV
  for(auto& tile : tg_.tiles()) {
    tile->updateUsages();
  }

}

// update pin density on tiles
void
RouteBase::updatePinCount() {
  for(auto& gCell : nb_->gCells()) {
    for(auto& gPin : gCell->gPins()) {
      int idxX = (gPin->cx() - tg_.lx())/tg_.tileSizeX();
      int idxY = (gPin->cy() - tg_.ly())/tg_.tileSizeY();
      Tile* tile = tg_.tiles()[idxY * tg_.tileCntX() + idxX];
      tile->setPinCnt( tile->pinCnt() + 1 );
    }
  }
}

// inflationRatio
void
RouteBase::updateInflationRatio() {
  float maxInflV = 0, maxInflH = 0;

  for(auto& tile : tg_.tiles()) {

    float newRatioV = (tile->usageH() + rbVars_.pinCoef * tile->pinCnt()) / tile->supplyH();
    newRatioV = pow(newRatioV, rbVars_.inflationRatioCoef);

    // Vertical InflationRatio
    tile->setInflationRatioV( 
        std::fmax(
          tile->inflationRatioV(), newRatioV));

    // upper bound
    tile->setInflationRatioV(
        std::fmin( 
          rbVars_.maxInflationRatio, 
          tile->inflationRatioV()));

    maxInflV = std::max(maxInflV, tile->inflationRatioV());
    
    float newRatioH = (tile->usageV() + rbVars_.pinCoef * tile->pinCnt()) / tile->supplyV();
    newRatioH = pow(newRatioH, rbVars_.inflationRatioCoef);

    // Horizontal InflationRatio
    tile->setInflationRatioH( 
        std::fmax(
          tile->inflationRatioH(), newRatioH));

    // upper bound
    tile->setInflationRatioH(
        std::fmin( 
          rbVars_.maxInflationRatio, 
          tile->inflationRatioH()));

    maxInflH = std::max(maxInflH, tile->inflationRatioH());

    log_->infoIntPair("xy", tile->x(), tile->y(), 5); 
    log_->infoIntPair("minxy", tile->lx(), tile->ly(), 5);
    log_->infoIntPair("maxxy", tile->ux(), tile->uy(), 5);
    log_->infoFloatPair("usageHV", 
        tile->usageH(), tile->usageV(), 5); 
    log_->infoFloatPair("supplyHV", 
        tile->supplyH(), tile->supplyV(), 5); 
    log_->infoInt("pinCnt", tile->pinCnt(), 5);
//    log_->infoFloatPair("calcInflRatioHV", 
//          (tile->usageV() + rbVars_.pinCoef * tile->pinCnt()) / tile->supplyV(),
//          (tile->usageH() + rbVars_.pinCoef * tile->pinCnt()) / tile->supplyH(), 5);
    log_->infoFloatPair("calcInflRatioHV", 
          tile->inflationRatioH(),
          tile->inflationRatioV(), 5);
//    std::cout << std::endl;
  }
  log_->infoFloatPair("MaxInflationRatioHV", maxInflH, maxInflV);
}


}

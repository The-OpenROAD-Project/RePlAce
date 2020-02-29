#include "routeBase.h"
#include "logger.h"
#include "placerBase.h"
#include <opendb/db.h>
#include <string>

using std::vector;
using std::string;

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
Tile::setBlockage(int layer, int block) {
  blockage_[layer] = block;
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
Tile::sumUsageV() const {
  return sumUsageV_; 
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


TileGrid::TileGrid()
  : db_(nullptr), 
  lx_(0), ly_(0), 
  ux_(0), uy_(0),
  tileCntX_(0), tileCntY_(0), tileSizeX_(0), tileSizeY_(0) {}

TileGrid::~TileGrid() {
  reset();
}

void
TileGrid::reset() {
  db_ = nullptr;
  lx_ = ly_ = ux_ = uy_ = 0;
  tileCntX_ = tileCntY_ = 0;
  tileSizeX_ = tileSizeY_ = 0;

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
TileGrid::setLogger(std::shared_ptr<Logger> log) {
  log_ = log;
}

void
TileGrid::setDiePoints(Die* die) {
  lx_ = die->coreLx();
  ly_ = die->coreLy();
  ux_ = die->coreUx();
  uy_ = die->coreUy();
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
TileGrid::initTiles() {
  const int numLayer = db_->getTech()->getRoutingLayerCount();
  int numHTracks = 0;
  int numVTracks = 0;
  for(int i=0; i<numLayer; i++) {

  }

  log_->infoInt("NumHTracks", numHTracks, 3);
  log_->infoInt("NumVTracks", numVTracks, 3);

}

int
TileGrid::lx() const {
  return lx_; 
}

int 
TileGrid::ly() const {
  return ly_;
}

int 
TileGrid::ux() const {
  return ux_;
}
int 
TileGrid::uy() const {
  return uy_;
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


EdgeCapacityInfo::EdgeCapacityInfo()
: lx(0), ly(0), ll(0), ux(0), uy(0), ul(0), capacity(0) {}

EdgeCapacityInfo::EdgeCapacityInfo(int lx1, int ly1, int ll1,
  int ux1, int uy1, int ul1, int capacity1)
  : lx(lx1), ly(ly1), ll(ll1), ux(ux1), uy(uy1), ul(ul1), capacity(capacity1) {}

// RoutingTrack structure
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




RouteBase::RouteBase()
  : nb_(nullptr), log_(nullptr), 
  tileLx_(0), tileLy_(0), tileCntX_(0), tileCntY_(0),
  tileNumLayers_(0), tileSizeX_(0), tileSizeY_(0),
  blockagePorosity_(0), gRoutePitchScale_(0) {}

RouteBase::RouteBase(std::shared_ptr<NesterovBase> nb,
    std::shared_ptr<Logger> log)
  : RouteBase() {
  nb_ = nb;
  log_ = log;
}

RouteBase::~RouteBase() {
  reset();
}

void
RouteBase::initFromRoute(const char* fileName) {
  char name[255];
  char *token = NULL;
  char temp[255];
  char line[255];
  bool blockageFlag = false;
  bool beolFlag = false;
  bool edgeFlag = false;
  std::vector< int > blockageLayers;

  FILE *fp = nullptr;
  if((fp = fopen(fileName, "r")) == NULL) {
    log_->error("Cannot open " + string(fileName) + " file!");
    exit(1);
  }

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
        sscanf(line, "%*s : %d %d %d", &tileCntX_, &tileCntY_, &tileNumLayers_);
      }
      else if(strcmp(temp, "VerticalCapacity") == 0) {
        verticalCapacity_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < tileNumLayers_; i++) {
          verticalCapacity_.push_back(atoi(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "HorizontalCapacity") == 0) {
        horizontalCapacity_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < tileNumLayers_; i++) {
          horizontalCapacity_.push_back(atoi(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "MinWireWidth") == 0) {
        minWireWidth_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < tileNumLayers_; i++) {
          minWireWidth_.push_back(atof(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "MinWireSpacing") == 0) {
        minWireSpacing_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < tileNumLayers_; i++) {
          minWireSpacing_.push_back(atof(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "ViaSpacing") == 0) {
        viaSpacing_.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < tileNumLayers_; i++) {
          viaSpacing_.push_back(atof(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "GridOrigin") == 0) {
        double temp_gridLLx, temp_gridLLy;
        sscanf(line, "%*s : %lf %lf", &temp_gridLLx, &temp_gridLLy);
        tileLx_ = temp_gridLLx;
        tileLy_ = temp_gridLLy;
      }
      else if(strcmp(temp, "TileSize") == 0) {
        double temp_tileWidth, temp_tileHeight;
        sscanf(line, "%*s : %lf %lf", &temp_tileWidth, &temp_tileHeight);
        tileSizeX_ = temp_tileWidth;
        tileSizeY_ = temp_tileHeight;
      }
      else if(strcmp(temp, "BlockagePorosity") == 0) {
        double temp_blockagePorosity;
        sscanf(line, "%*s : %lf", &temp_blockagePorosity);
        blockagePorosity_ = (float)temp_blockagePorosity;
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


void 
RouteBase::reset() {
  tileLx_ = tileLy_ = tileCntX_ = tileCntY_ = 0;
  tileNumLayers_ = 0;
  tileSizeX_ = tileSizeY_ = 0;
  blockagePorosity_ = 0;
  edgeCapacityStor_.clear();
  routingTracks_.clear();

  edgeCapacityStor_.shrink_to_fit();
  routingTracks_.shrink_to_fit();

  gRoutePitchScale_ = 0;
}

void
RouteBase::init() {
  initFromRoute("input.route");
}





}

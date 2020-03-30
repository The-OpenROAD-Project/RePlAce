#ifndef __REPLACE_ROUTE_BASE__
#define __REPLACE_ROUTE_BASE__

#include <memory>
#include <vector>

namespace odb {
  class dbDatabase;
}

namespace replace {

class Logger;
class NesterovBase;
class GNet;
class Die;

// for GGrid
class Tile {
  public:
    Tile();
    Tile(int x, int y, int lx, int ly, 
        int ux, int uy, int layers);
    ~Tile();

    // getter funcs
    int x() const;
    int y() const;

    int lx() const;
    int ly() const;
    int ux() const;
    int uy() const;

    // only area is needed
    const int64_t area() const;

    int blockage(int layer) const;
    int capacity(int layer) const;
    int route(int layer) const;

    int usageHL(int layer) const;
    int usageHR(int layer) const;
    int usageVL(int layer) const;
    int usageVR(int layer) const;

    float usageH() const;
    float usageV() const;

    float supplyHL() const;
    float supplyHR() const;
    float supplyVL() const;
    float supplyVR() const;

    float supplyH() const;
    float supplyV() const;

    float inflationRatioH() const;
    float inflationRatioV() const;

    float inflationRatio() const;
    float inflationArea() const;
    float inflationAreaDelta() const;
    float inflatedRatio() const;

    bool isMacroIncluded() const;
    int pinCnt() const;

    // setter funcs
    void setBlockage(int layer, int blockage);
    void setCapacity(int layer, int capacity);
    void setCapacity(std::vector<int>& capacity);
    void setRoute(int layer, int route);

    void setUsageHL(int layer, int usage);
    void setUsageHR(int layer, int usage);
    void setUsageVL(int layer, int usage);
    void setUsageVR(int layer, int usage);

    void setSupplyH(float supply);
    void setSupplyV(float supply);

    void setSupplyHL(float supply);
    void setSupplyHR(float supply);
    void setSupplyVL(float supply);
    void setSupplyVR(float supply);

    void setInflationRatioH(float val);
    void setInflationRatioV(float val);
    void setInflationRatio(float ratio);

    void setInflationArea(float area);
    void setInflationAreaDelta(float delta);

    void setPinCnt(int cnt);
    void setMacroIncluded(bool mode);


    void updateUsages();

  private:
    // the followings will store
    // blockage / capacity / route-ability
    // idx : metalLayer

    // Note that the layerNum starts from 0 in this Tile class
    std::vector<int> blockage_;
    std::vector<int> capacity_;
    std::vector<int> route_;

    // H : Horizontal
    // V : Vertical
    // L : Left
    // R : Right
    //
    std::vector<int> usageHL_;
    std::vector<int> usageHR_;
    std::vector<int> usageVL_;
    std::vector<int> usageVR_;

    int x_;
    int y_;

    int lx_;
    int ly_;
    int ux_;
    int uy_;

    // pin counts
    int pinCnt_;

    float usageH_;
    float usageV_;

    float supplyH_;
    float supplyV_;

    // supply cals for four edges
    float supplyHL_;
    float supplyHR_;
    float supplyVL_;
    float supplyVR_;

    // to bloat cells in tile
    float inflationRatioH_;
    float inflationRatioV_;
    float inflationRatio_;

    float inflationArea_;
    float inflationAreaDelta_;

    float inflatedRatio_;

    bool isMacroIncluded_;

    void reset();
};

inline int
Tile::x() const {
  return x_;
}

inline int
Tile::y() const {
  return y_;
}

inline int
Tile::lx() const {
  return lx_;
}

inline int
Tile::ly() const {
  return ly_;
}

inline int
Tile::ux() const {
  return ux_;
}

inline int
Tile::uy() const {
  return uy_;
}

inline const int64_t
Tile::area() const {
  return
    static_cast<int64_t>(ux_ - lx_) *
    static_cast<int64_t>(uy_ - ly_);
}


class TileGrid {
  public:
    TileGrid();
    ~TileGrid();

    void setLogger(std::shared_ptr<Logger> log);
    void setTileCnt(int tileCntX, int tileCntY);
    void setTileCntX(int tileCntX);
    void setTileCntY(int tileCntY);
    void setTileSize(int tileSizeX, int tileSizeY);
    void setTileSizeX(int tileSizeX);
    void setTileSizeY(int tileSizeY);
    void setNumRoutingLayers(int num);

    void setLx(int lx);
    void setLy(int ly);

    int lx() const;
    int ly() const;
    int ux() const;
    int uy() const;

    int tileCntX() const;
    int tileCntY() const;
    int tileSizeX() const;
    int tileSizeY() const;

    int numRoutingLayers() const;

    const std::vector<Tile*> & tiles() const;

    void initTiles();

  private:
    // for traversing layer info!
    std::shared_ptr<Logger> log_;

    std::vector<Tile> tileStor_;
    std::vector<Tile*> tiles_;

    int lx_;
    int ly_;
    int tileCntX_;
    int tileCntY_;
    int tileSizeX_;
    int tileSizeY_;
    int numRoutingLayers_;

    void reset();
};

inline const std::vector<Tile*> &
TileGrid::tiles() const {
  return tiles_;
}

// For *.route EdgeCapacityAdjustment
struct EdgeCapacityInfo {
  int lx;
  int ly;
  int ll;
  int ux;
  int uy;
  int ul;
  int capacity;
  EdgeCapacityInfo();
  EdgeCapacityInfo(int lx, int ly, int ll,
      int ux, int uy, int ul, int capacity);
};

// For *.est file communication
struct RoutingTrack {
  int lx, ly, ux, uy;
  int layer;
  GNet* gNet;
  RoutingTrack();
  RoutingTrack(int lx, int ly, int ux, int uy,
      int layer, GNet* net);
};

class RouteBaseVars {
public:
  float gRoutePitchScale;
  float edgeAdjustmentCoef;
  float pinInflationCoef;
  float pinBlockageFactor;
  float inflationRatioCoef;
  float maxInflationRatio;
  float blockagePorosity;

  RouteBaseVars();
  void reset();
};


class RouteBase {
  public:
    RouteBase();
    RouteBase(RouteBaseVars rbVars,
        odb::dbDatabase* db,
        std::shared_ptr<NesterovBase> nb,
        std::shared_ptr<Logger> log);
    ~RouteBase();

    // temp func
    void importRoute(const char* fileName);
    void importEst(const char* fileName);

    void updateCongestionMap();

  private:
    RouteBaseVars rbVars_;
    odb::dbDatabase* db_;

    std::shared_ptr<NesterovBase> nb_;
    std::shared_ptr<Logger> log_;

    TileGrid tg_;

    // from *.route file
    std::vector<int> verticalCapacity_;
    std::vector<int> horizontalCapacity_;
    std::vector<int> minWireWidth_;
    std::vector<int> minWireSpacing_;

    std::vector<EdgeCapacityInfo> edgeCapacityStor_;

    // from *.est file
    std::vector<RoutingTrack> routingTracks_;

    void init();
    void reset();

    // init congestion maps based on given points
    void updateSupplies();
    void updateUsages();
    void updatePinCount();
    void updateRoutes();
    void updateInflationRatio();
};
}

#endif

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
    Tile(int x, int y, int lx, int ly, int ux, int uy, int layers);
    ~Tile();

    int x() const;
    int y() const;

    int lx() const;
    int ly() const;
    int ux() const;
    int uy() const;

    int blockage(int layer) const;
    int capacity(int layer) const;
    int route(int layer) const;

    int usageHL(int layer) const;
    int usageHR(int layer) const;
    int usageVL(int layer) const;
    int usageVR(int layer) const;

    void setBlockage(int layer, int blockage);
    void setCapacity(int layer, int capacity);
    void setRoute(int layer, int route);

    void setUsageHL(int layer, int usage);
    void setUsageHR(int layer, int usage);
    void setUsageVL(int layer, int usage);
    void setUsageVR(int layer, int usage);

    float sumUsageH() const;
    float sumUsageV() const;

    float supplyHL() const;
    float supplyHR() const;
    float supplyVL() const;
    float supplyVR() const;


    float inflationRatioH() const;
    float inflationRatioV() const;

    float inflationRatio() const;
    float inflationArea() const;
    float inflationAreaDelta() const;
    float inflatedRatio() const;

    bool isMacroIncluded() const;

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

    float sumUsageH_;
    float sumUsageV_;

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

class TileGrid {
  public:
    TileGrid();
    ~TileGrid();

    void setDb(odb::dbDatabase* db);
    void setLogger(std::shared_ptr<Logger> log);
    void setDiePoints(Die* die);
    void setTileCnt(int tileCntX, int tileCntY);
    void setTileCntX(int tileCntX);
    void setTileCntY(int tileCntY);

    int lx() const;
    int ly() const;
    int ux() const;
    int uy() const;

    int tileCntX() const;
    int tileCntY() const;
    int tileSizeX() const;
    int tileSizeY() const;

    void reset();

    const std::vector<Tile*> & tiles() const;

    void initTiles();

  private:
    std::vector<Tile> tileStor_;
    std::vector<Tile*> tiles_;

    // extract routing layer info
    odb::dbDatabase* db_;
    std::shared_ptr<Logger> log_;

    int lx_;
    int ly_;
    int ux_;
    int uy_;
    int tileCntX_;
    int tileCntY_;
    int tileSizeX_;
    int tileSizeY_;
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

class RouteBase {
  public:
    RouteBase();
    RouteBase(std::shared_ptr<NesterovBase> nb,
        std::shared_ptr<Logger> log);
    ~RouteBase();

    void estimateCongestion();
    void initFromRoute(const char* fileName);
    void importEst(const char* fileName);

  private:
    TileGrid tg_;

    std::shared_ptr<NesterovBase> nb_;
    std::shared_ptr<Logger> log_;

    // from *.route file
    std::vector<int> verticalCapacity_;
    std::vector<int> horizontalCapacity_;
    std::vector<int> minWireWidth_;
    std::vector<int> minWireSpacing_;
    std::vector<int> viaSpacing_;

    int tileLx_;
    int tileLy_;
    int tileCntX_;
    int tileCntY_;
    int tileNumLayers_;
    int tileSizeX_;
    int tileSizeY_;

    float blockagePorosity_;
    std::vector<EdgeCapacityInfo> edgeCapacityStor_;

    // from *.est file
    std::vector<RoutingTrack> routingTracks_;

    float gRoutePitchScale_;


    void init();
    void reset();
};
}

#endif

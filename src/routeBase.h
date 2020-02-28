#ifndef __REPLACE_ROUTE_BASE__
#define __REPLACE_ROUTE_BASE__

#include <memory>

namespace odb {  
  class dbDatabase;
}

namespace replace {

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
Tile::x() {
  return x_; 
}

inline int
Tile::y() {
  return y_; 
}

inline int 
Tile::lx() {
  return lx_;
}

inline int
Tile::ly() {
  return ly_;
}

inline int
Tile::ux() {
  return ux_;  
}

inline int
Tile::uy() {
  return uy_;
}

class TileGrid {
  public:
    TileGrid();
    ~TileGrid();

    int lx() const; 
    int ly() const;
    int ux() const;
    int uy() const;
    
    void reset();

    const std::vector<Tile*> & tiles() const;
    void setDb(odb::dbDatabase* db);

    void initTiles();
    void initFromGuide(const char* fileName);
  
  private:
    vector<Tile> tileStor_;
    vector<Tile*> tiles_;

    // extract routing layer info
    odb::dbDatabase* db_;
    std::shared_ptr<Logger> log_;

    int lx_; 
    int ly_;
    int ux_; 
    int uy_;
};

inline const std::vector<Tile*> & 
TileGrid::tiles() const {
  return tiles_;
}

struct EdgeCapacityInfo {
  int col1;
  int row1;
  int layer1;
  int col2;
  int row2;
  int layer2;
  int capacity;
  EdgeCapacityInfo();
};

class RouteBase {
  public:
    RouteBase(std::shared_ptr<NesterovBase> nb,
        std::shared_ptr<Logger> log);
    ~RouteBase();

    void estimateCongestion();
    void initFromGuide(const char* fileName);
    void importEst(const char* fileName);

  private:
    TileGrid tg_;

    std::shared_ptr<NesterovBase> nb_;
    std::shared_ptr<Logger> log_;
    
    // from *.route file
    std::vector<int> verticalCapacity_;
    std::vector<int> minWireWidth_;
    std::vector<int> minWireSpacing_;

    float gRoutePitchScale_;

    // from *.est file
    std::vector<EdgeCapacityInfo> edgeCapacityStor_;


    void init();
    void reset();
};
}

#endif

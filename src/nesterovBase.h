#ifndef __NESTEROV_BASE__
#define __NESTEROV_BASE__

#include <vector>
#include <cstdint>
#include <pair>

namespace replace {

class Instance;
class Die;
class PlacerBase;

class GCell {
public:
  GCell();
  GCell(Instance* inst);
  GCell(std::vector<Instance*>& insts);
  ~GCell();

  Instance* instance();
  std::vector<Instance*> & insts() { return insts_; }

  void setClusteredInstance(std::vector<Instance*>& insts);
  void setInstance(Instance* inst);
  void setFiller();

  // normal coordinates
  int lx();
  int ly();
  int ux();
  int uy();
  int cx();
  int cy();
  int dx();
  int dy();

  // virtual density coordinates
  int dLx();
  int dLy();
  int dUx();
  int dUy();
  int dCx();
  int dCy();
  int dDx();
  int dDy();


  void setLocation(int lx, int ly);
  void setCenterLocation(int cx, int cy);
  void setSize(int dx, int dy);

  void setDensityLocation(int dLx, int dLy);
  void setDensityCenterLocation(int dCx, int dCy);
  void setDensitySize(int dDx, int dDy);

  void setDensityScale(float densityScale);
  void setGradientX(float gradX);
  void setGradientY(float gradY);

  float gradientX() { return gradientX_; }
  float gradientY() { return gradientY_; }
  float densityScale() { return densityScale_; }

  bool isInstance();
  bool isClusteredInstance();
  bool isFiller();


private:
  std::vector<Instance*> insts_;
  int lx_;
  int ly_;
  int ux_;
  int uy_;

  int dLx_;
  int dLy_;
  int dUx_;
  int dUy_;

  float densityScale_;
  float gradientX_;
  float gradientY_;
};

// TODO
//class GNet : class Net {};
//class GPin : class Pin {};

class Bin {
public:
  Bin();
  Bin(int lx, int ly, int ux, int uy);
  ~Bin();

  int lx();
  int ly();
  int ux();
  int uy();
  int cx();
  int cy();
  int dx();
  int dy();

  float phi();
  float density();
  float electroForce();

  void setPhi(float phi);
  void setDensity(float density);
  void setElectroForce(float electroForce);

protected:
  uint32_t & nonPlaceArea();
  uint32_t & placedArea();
  uint32_t & fillerArea();

private:
  int lx_;
  int ly_;
  int ux_;
  int uy_;

  uint32_t nonPlaceArea_;
  uint32_t placedArea_;
  uint32_t fillerArea_;

  float phi_;
  float density_;
  float electroForce_;

  friend class BinGrid;
};

//
// The bin can be non-uniform because of
// "integer" coordinates
//
class BinGrid {
public:
  BinGrid();
  BinGrid(Die* die);
  ~BinGrid();

  void setCoordi(Die* die);
  void setBinCnt(int binCntX, int binCntY);
  void setBinCntX(int binCntX);
  void setBinCntY(int binCntY);
  void updateCoordi(Die* die);
  void updateBinsArea(std::vector<GCell>& cells);

  void initBins();

  // lx, ly, ux, uy will hold coreArea
  int lx();
  int ly();
  int ux();
  int uy();
  int cx();
  int cy();
  int dx();
  int dy();

  int binCntX();
  int binCntY();
  int binSizeX();
  int binSizeY();

  // return bins_ index with given gcell
  std::pair<int, int> getMinMaxIdxX(GCell* gcell);
  std::pair<int, int> getMinMaxIdxY(GCell* gcell);

  std::vector<Bin*> & bins() { return binsPtr_; }

private:
  std::vector<Bin> bins_;
  std::vector<Bin*> binsPtr_;
  int lx_;
  int ly_;
  int ux_;
  int uy_;
  int binCntX_;
  int binCntY_;
  int binSizeX_;
  int binSizeY_;
  unsigned char isSetBinCntX_:1;
  unsigned char isSetBinCntY_:1;
};

class NesterovBase {
  public:
    NesterovBase();
    NesterovBase(PlacerBase* pb);
    ~NesterovBase();

    std::vector<GCell*> & gCells();
    std::vector<GCell*> & gcellInsts();
    std::vector<GCell*> & gcellFillers();

    void initGCells();
    void initBinGrid();

  private:
    PlacerBase* pb_;
    BinGrid binGrid_;
    std::vector<GCell> gCells_;
    std::vector<GCell*> gcellsPtr_;
    std::vector<GCell*> gcellInsts_;
    std::vector<GCell*> gcellFillers_;

};

}



#endif

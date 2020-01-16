#ifndef __NESTEROV_BASE__
#define __NESTEROV_BASE__

#include <vector>

namespace replace {

class Instance;
class GCell {
public:
  GCell();
  GCell(Instance* inst);
  GCell(std::vector<Instance*>& insts);
  ~GCell();

  Instance* instance();
  std::vector<Instance*> & insts() { return insts_; }

  void setClusterdInstance(std::vector<Instance*>& insts);
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

  enum {
    InstField,
    clusteredInstField,
    fillerField
  };

  uint8_t attribute_;

  float densityScale_;
  float gradX_;
  float gradY_;
};


class Bin {
public:
  Bin();
  ~Bin();

  int lx();
  int ly();
  int ux();
  int uy();
  int cx();
  int cy();

protected:
  uint32_t & fixedArea();
  uint32_t & placeArea();
  uint32_t & fillerArea();

private:
  int lx_;
  int ly_;
  int ux_;
  int uy_;

  uint32_t fixedArea_;
  uint32_t placeArea_;
  uint32_t fillerArea_;

  float phi_;
  float density_;

  friend class BinGrid;
};

class BinGrid {
public:
  BinGrid();
  ~BinGrid();

  void updateBins(std::vector<GCell>& cells);

private:
  std::vector<Bin> bins_;
  int lx_;
  int ly_;
  int ux_;
  int uy_;
  int binCntX_;
  int binCntY_;
  int binSizeX_;
  int binSizeY_;

};



}



#endif

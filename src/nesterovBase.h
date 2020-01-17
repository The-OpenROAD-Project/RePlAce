#ifndef __NESTEROV_BASE__
#define __NESTEROV_BASE__

#include <vector>

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
// class GNet : class Net {};
// class GPin : class Pin {};


class NesterovBase {
  public:
    NesterovBase();
    NesterovBase(PlacerBase* pb);
    ~NesterovBase();

    std::vector<GCell*> & gCells();
    std::vector<GCell*> & gcellInsts();
    std::vector<GCell*> & gcellFillers();

    void initGCells();

  private:
    PlacerBase* pb_;
    std::vector<GCell> gCells_;
    std::vector<GCell*> gcellsPtr_;
    std::vector<GCell*> gcellInsts_;
    std::vector<GCell*> gcellFillers_;

};

}



#endif

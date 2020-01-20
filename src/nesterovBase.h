#ifndef __NESTEROV_BASE__
#define __NESTEROV_BASE__

#include <vector>

namespace replace {

class Instance;
class Die;
class PlacerBase;

class Instance;
class Pin;
class Net;

class GPin;

class GCell {
public:
  GCell();
  GCell(Instance* inst);
  GCell(std::vector<Instance*>& insts);
  ~GCell();

  Instance* instance();
  std::vector<Instance*> & insts() { return insts_; }
  std::vector<GPin*> & gPins() { return gPins_; }

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
  std::vector<GPin*> gPins_;
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

class GNet {
  public:
    GNet();
    GNet(Net* net);
    GNet(std::vector<Net*>& nets);
    ~GNet();

    Net* net();
    std::vector<Net*> & nets() { return nets_; }

    std::vector<GPin*> & gPins() { return gPins_; }

    void setCustomNetWeight();
    float netWeight() { return netWeight_; }

  private:
    std::vector<GPin*> gPins_;
    std::vector<Net*> nets_;
    int lx_;
    int ly_;
    int ux_;
    int uy_;

    float netCustomWeight_;
    float netWeight_;

    //
    // weighted average WL model stor for better indexing
    // Please check the equation (4) in the ePlace-MS paper.
    //
    // WA: weighted Average
    // saving four variable will be helpful for
    // calculating the WA gradients/wirelengths.
    //
    // gamma: modeling accuracy.
    //
    // X forces.
    //
    // waExpMinSumStorX_: store sigma {exp(x_i/gamma)}
    // waXExpMinSumStorX_: store signa {x_i*exp(e_i/gamma)}
    // waExpMaxSumStorX_ : store sigma {exp(-x_i/gamma)}
    // waXExpMaxSumStorX_: store sigma {x_i*exp(-x_i/gamma)}
    //
    float waExpMinSumStorX_;
    float waXExpMinSumStorX_;

    float waExpMaxSumStorX_;
    float waXExpMaxSumStorX_;

    //
    // Y forces.
    //
    // waExpMinSumStorY_: store sigma {exp(y_i/gamma)}
    // waYExpMinSumStorY_: store signa {y_i*exp(e_i/gamma)}
    // waExpMaxSumStorY_ : store sigma {exp(-y_i/gamma)}
    // waYExpMaxSumStorY_: store sigma {y_i*exp(-y_i/gamma)}
    //
    float waExpMinSumStorY_;
    float waYExpMinSumStorY_;

    float waExpMaxSumStorY_;
    float waYExpMaxSumStorY_;

};

class GPin {
  public:
    GPin();
    GPin(Pin* pin);
    GPin(std::vector<Pin*>& pins);
    ~GPin();

    Pin* pin();
    std::vector<Pin*> & pins() { return pins_; }

    GCell* gCell() { return gCell_; }
    GNet* gNet() { return gNet_; }
  private:
    GCell* gCell_;
    GNet* gNet_;
    std::vector<Pin*> pins_;

    int offsetCx_;
    int offsetCy_;
    int cx_;
    int cy_;

    //
    // weighted average WL vals stor for better indexing
    // Please check the equation (4) in the ePlace-MS paper.
    //
    // posExpSum_: holds exp(x_i/gamma)
    // negExpSum_: holds exp(-x_i/gamma)
    // the x_i is equal to cx_ variable.
    //
    float posExpSum_;
    float negExpSum_;

    // flag variables
    // whether this pins should be considered
    // in a WA models.
    unsigned char hasPosExpSum_:1;
    unsigned char hasNegExpSum_:1;
};


class NesterovBase {
  public:
    NesterovBase();
    NesterovBase(PlacerBase* pb);
    ~NesterovBase();

    std::vector<GCell*> & gCells() { return gCellsPtr_; }
    std::vector<GCell*> & gCellInsts() { return gCellInsts_; }
    std::vector<GCell*> & gCellFillers() { return gCellFillers_; }

    std::vector<GNet*> & gNets() { return gNetsPtr_; }
    std::vector<GPin*> & gPins() { return gPinsPtr_; }

    void initGBases();

  private:
    PlacerBase* pb_;
    std::vector<GCell> gCells_;
    std::vector<GNet> gNets_;
    std::vector<GPin> gPins_;

    std::vector<GCell*> gCellsPtr_;
    std::vector<GCell*> gCellInsts_;
    std::vector<GCell*> gCellFillers_;

    std::vector<GNet*> gNetsPtr_;
    std::vector<GPin*> gPinsPtr_;

};

}



#endif

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

  Instance* instance() const;
  const std::vector<Instance*> & insts() const { return insts_; }
  const std::vector<GPin*> & gPins() const { return gPins_; }

  void setClusteredInstance(std::vector<Instance*>& insts);
  void setInstance(Instance* inst);
  void setFiller();

  // normal coordinates
  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  // virtual density coordinates
  int dLx() const;
  int dLy() const;
  int dUx() const;
  int dUy() const;
  int dCx() const;
  int dCy() const;
  int dDx() const;
  int dDy() const;


  void setLocation(int lx, int ly);
  void setCenterLocation(int cx, int cy);
  void setSize(int dx, int dy);

  void setDensityLocation(int dLx, int dLy);
  void setDensityCenterLocation(int dCx, int dCy);
  void setDensitySize(int dDx, int dDy);

  void setDensityScale(float densityScale);
  void setGradientX(float gradX);
  void setGradientY(float gradY);

  float gradientX() const { return gradientX_; }
  float gradientY() const { return gradientY_; }
  float densityScale() const { return densityScale_; }

  bool isInstance() const;
  bool isClusteredInstance() const;
  bool isFiller() const;


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

    Net* net() const;
    const std::vector<Net*> & nets() const { return nets_; }
    const std::vector<GPin*> & gPins() const { return gPins_; }

    void setCustomWeight( float customWeight );
    float customWeight() const { return customWeight_; }
    float netWeight() const { return weight_; }

    void addGPin(GPin* gPin);

  private:
    std::vector<GPin*> gPins_;
    std::vector<Net*> nets_;
    int lx_;
    int ly_;
    int ux_;
    int uy_;

    float customWeight_;
    float weight_;

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

    Pin* pin() const;
    const std::vector<Pin*> & pins() const { return pins_; }

    GCell* gCell() const { return gCell_; }
    GNet* gNet() const { return gNet_; }

    int cx() const { return cx_; }
    int cy() const { return cy_; }

    float posExpSum() const { return posExpSum_; }
    float negExpSum() const { return negExpSum_; }

    bool hasPosExpSum() const { return (hasPosExpSum_ == 1); }
    bool hasNegExpSum() const { return (hasNegExpSum_ == 1); }

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

    const std::vector<GCell*> & gCells() const { return gCellsPtr_; }
    const std::vector<GCell*> & gCellInsts() const { return gCellInsts_; }
    const std::vector<GCell*> & gCellFillers() const { return gCellFillers_; }

    const std::vector<GNet*> & gNets() const { return gNetsPtr_; }
    const std::vector<GPin*> & gPins() const { return gPinsPtr_; }


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

    void init();
    void reset();
};

}



#endif

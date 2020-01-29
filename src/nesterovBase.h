#ifndef __NESTEROV_BASE__
#define __NESTEROV_BASE__

#include <vector>
#include <memory>
#include <unordered_map>

#include "coordi.h"

namespace replace {

class Instance;
class Die;
class PlacerBase;

class Instance;
class Pin;
class Net;

class GPin;
class FFT;

class GCell {
public:
  GCell();

  // instance cells
  GCell(Instance* inst);
  GCell(std::vector<Instance*>& insts);

  // filler cells
  GCell(int cx, int cy, int dx, int dy);
  ~GCell();

  Instance* instance() const;
  const std::vector<Instance*> & insts() const { return insts_; }
  const std::vector<GPin*> & gPins() const { return gPins_; }

  void addGPin(GPin* gPin);

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

    int lx() const { return lx_; }
    int ly() const { return ly_; }
    int ux() const { return ux_; }
    int uy() const { return uy_; }

    void setCustomWeight( float customWeight );
    float customWeight() const { return customWeight_; }
    float netWeight() const { return weight_; }

    void addGPin(GPin* gPin);
    void updateBox();
    int32_t hpwl();

    void setDontCare();
    bool isDontCare();

    // clear WA(Weighted Average) variables.
    void clearWaVars();

    void addWaExpMinSumX(float waExpMinSumX);
    void addWaXExpMinSumX(float waExpXMinSumX);

    void addWaExpMinSumY(float waExpMinSumY);
    void addWaYExpMinSumY(float waExpXMinSumY);

    void addWaExpMaxSumX(float waExpMaxSumX);
    void addWaXExpMaxSumX(float waExpXMaxSumX);

    void addWaExpMaxSumY(float waExpMaxSumY);
    void addWaYExpMaxSumY(float waExpXMaxSumY);

    float waExpMinSumX() const { return waExpMinSumStorX_; }
    float waXExpMinSumX() const { return waXExpMinSumStorX_; }

    float waExpMinSumY() const { return waExpMinSumStorY_; }
    float waYExpMinSumY() const { return waYExpMinSumStorY_; }

    float waExpMaxSumX() const { return waExpMaxSumStorX_; }
    float waXExpMaxSumX() const { return waXExpMaxSumStorX_; }

    float waExpMaxSumY() const { return waExpMaxSumStorY_; }
    float waYExpMaxSumY() const { return waYExpMaxSumStorY_; }


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

    unsigned char isDontCare_:1;

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

    void setGCell(GCell* gCell);
    void setGNet(GNet* gNet);

    int cx() const { return cx_; }
    int cy() const { return cy_; }
    
    // clear WA(Weighted Average) variables.
    void clearWaVars();

    void setMaxExpSumX(float maxExpSumX);
    void setMaxExpSumY(float maxExpSumY);
    void setMinExpSumX(float minExpSumX);
    void setMinExpSumY(float minExpSumY);

    float maxExpSumX() const { return maxExpSumX_; }
    float maxExpSumY() const { return maxExpSumY_; }
    float minExpSumX() const { return minExpSumX_; }
    float minExpSumY() const { return minExpSumY_; }

    bool hasMaxExpSumX() const { return (hasMaxExpSumX_ == 1); }
    bool hasMaxExpSumY() const { return (hasMaxExpSumY_ == 1); }
    bool hasMinExpSumX() const { return (hasMinExpSumX_ == 1); }
    bool hasMinExpSumY() const { return (hasMinExpSumY_ == 1); }

    void setCenterLocation(int cx, int cy);
    void updateLocation(const GCell* gCell);
    void updateDensityLocation(const GCell* gCell);

  private:
    GCell* gCell_;
    GNet* gNet_;
    std::vector<Pin*> pins_;

    int offsetCx_;
    int offsetCy_;
    int cx_;
    int cy_;

    // weighted average WL vals stor for better indexing
    // Please check the equation (4) in the ePlace-MS paper.
    //
    // maxExpSum_: holds exp(x_i/gamma)
    // minExpSum_: holds exp(-x_i/gamma)
    // the x_i is equal to cx_ variable.
    //
    float maxExpSumX_;
    float maxExpSumY_;

    float minExpSumX_;
    float minExpSumY_;

    // flag variables
    //
    // check whether
    // this pin is considered in a WA models.
    unsigned char hasMaxExpSumX_:1;
    unsigned char hasMaxExpSumY_:1;

    unsigned char hasMinExpSumX_:1;
    unsigned char hasMinExpSumY_:1;
};

class Bin {
public:
  Bin();
  Bin(int x, int y, int lx, int ly, int ux, int uy);

  ~Bin();

  int x() const;
  int y() const;

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  float electroPhi() const;
  float electroForceX() const;
  float electroForceY() const;
  float density() const;

  void setDensity(float density);
  void setElectroForce(float electroForceX, float electroForceY);
  void setElectroPhi(float phi);

  void setNonPlaceArea(int64_t area);
  void setInstPlacedArea(int64_t area);
  void setFillerArea(int64_t area);

  void addNonPlaceArea(int64_t area);
  void addInstPlacedArea(int64_t area);
  void addFillerArea(int64_t area);

  const int64_t binArea() const;
  const int64_t nonPlaceArea() const { return nonPlaceArea_; }
  const int64_t instPlacedArea() const { return instPlacedArea_; }
  const int64_t fillerArea() const { return fillerArea_; }

private:
  // index
  int x_;
  int y_;

  // coordinate
  int lx_;
  int ly_;
  int ux_;
  int uy_;

  int64_t nonPlaceArea_;
  int64_t instPlacedArea_;
  int64_t fillerArea_;

  float density_;
  float electroPhi_;
  float electroForceX_;
  float electroForceY_;
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

  void setPlacerBase(std::shared_ptr<PlacerBase> pb);
  void setCoordi(Die* die);
  void setBinCnt(int binCntX, int binCntY);
  void setBinCntX(int binCntX);
  void setBinCntY(int binCntY);
  void setTargetDensity(float density);
  void updateBinsGCellArea(std::vector<GCell*>& cells);
  void updateBinsGCellDensityArea(std::vector<GCell*>& cells);


  void initBins();

  // lx, ly, ux, uy will hold coreArea
  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  int binCntX() const;
  int binCntY() const;
  int binSizeX() const;
  int binSizeY() const;

  int64_t overflowArea() const;

  // return bins_ index with given gcell
  std::pair<int, int> getMinMaxIdxX(GCell* gcell);
  std::pair<int, int> getMinMaxIdxY(GCell* gcell);

  std::pair<int, int> getDensityMinMaxIdxX(GCell* gcell);
  std::pair<int, int> getDensityMinMaxIdxY(GCell* gcell);

  std::pair<int, int> getMinMaxIdxX(Instance* inst);
  std::pair<int, int> getMinMaxIdxY(Instance* inst);

  const std::vector<Bin*> & bins() const { return bins_; }

private:
  std::vector<Bin> binStor_;
  std::vector<Bin*> bins_;
  std::shared_ptr<PlacerBase> pb_;
  int lx_;
  int ly_;
  int ux_;
  int uy_;
  int binCntX_;
  int binCntY_;
  int binSizeX_;
  int binSizeY_;
  float targetDensity_;
  int64_t overflowArea_;
  unsigned char isSetBinCntX_:1;
  unsigned char isSetBinCntY_:1;

  void updateBinsNonPlaceArea();
};

class NesterovBaseVars {
public:
  float targetDensity;
  float minAvgCut;
  float maxAvgCut;
  int binCntX;
  int binCntY;
  float minWireLengthForceBar;
  unsigned char isSetBinCntX:1;
  unsigned char isSetBinCntY:1;

  NesterovBaseVars();
  void reset();
};

class NesterovBase {
public:
  NesterovBase();
  NesterovBase(NesterovBaseVars nbVars, std::shared_ptr<PlacerBase> pb);
  ~NesterovBase();

  const std::vector<GCell*> & gCells() const { return gCells_; }
  const std::vector<GCell*> & gCellInsts() const { return gCellInsts_; }
  const std::vector<GCell*> & gCellFillers() const { return gCellFillers_; }

  const std::vector<GNet*> & gNets() const { return gNets_; }
  const std::vector<GPin*> & gPins() const { return gPins_; }

  //
  // placerBase To NesterovBase functions
  //
  GCell* placerToNesterov(Instance* inst);
  GPin* placerToNesterov(Pin* pin);
  GNet* placerToNesterov(Net* net);

  // update gCells with lx, ly
  void updateGCellLocation(
      std::vector<FloatCoordi>& coordis);

  // update gCells with cx, cy
  void updateGCellCenterLocation(
      std::vector<FloatCoordi>& coordis);

  void updateGCellDensityCenterLocation(
      std::vector<FloatCoordi>& coordis);

  int binCntX() const;
  int binCntY() const;
  int binSizeX() const;
  int binSizeY() const;

  int64_t overflowArea() const;
  float sumPhi() const;

  void updateDensityCoordiLayoutInside(GCell* gcell);

  float getDensityCoordiLayoutInsideX(GCell* gCell, float cx);
  float getDensityCoordiLayoutInsideY(GCell* gCell, float cy);

  // WL force update based on WeightedAverage model
  // wlCoeffX : WireLengthCoefficient for X.
  //            equal to 1 / gamma_x
  // wlCoeffY : WireLengthCoefficient for Y.
  //            equal to 1 / gamma_y
  //
  // Gamma is described in the ePlaceMS paper.
  //
  void updateWireLengthForceWA(
      float wlCoeffX,
      float wlCoeffY);

  FloatCoordi
    getWireLengthGradientPinWA(GPin* gPin,
        float wlCoeffX, float wlCoeffY);

  FloatCoordi
    getWireLengthGradientWA(GCell* gCell,
        float wlCoeffX, float wlCoeffY);

  // for preconditioner
  FloatCoordi
    getWireLengthPreconditioner(GCell* gCell);

  FloatCoordi
    getDensityPreconditioner(GCell* gCell);

  FloatCoordi
    getDensityGradient(GCell* gCell);

  int32_t getHpwl();

  // update electrostatic forces within Bin
  void updateDensityForceBin();

private:
  NesterovBaseVars nbVars_;
  std::shared_ptr<PlacerBase> pb_;

  BinGrid bg_;
  std::unique_ptr<FFT> fft_;

  std::vector<GCell> gCellStor_;
  std::vector<GNet> gNetStor_;
  std::vector<GPin> gPinStor_;

  std::vector<GCell*> gCells_;
  std::vector<GCell*> gCellInsts_;
  std::vector<GCell*> gCellFillers_;

  std::vector<GNet*> gNets_;
  std::vector<GPin*> gPins_;

  std::unordered_map<Instance*, GCell*> gCellMap_;
  std::unordered_map<Pin*, GPin*> gPinMap_;
  std::unordered_map<Net*, GNet*> gNetMap_;

  float sumPhi_;

  void init();
  void initFillerGCells();
  void initBinGrid();

  void reset();
};

}

#endif

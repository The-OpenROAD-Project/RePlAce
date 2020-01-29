#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

#include "coordi.h"
#include <memory>
#include <vector>

namespace replace
{

class PlacerBase;
class Instance;
class NesterovBase;

class NesterovPlaceVars {
  public:
  int maxNesterovIter;
  int maxBackTrack;
  float initDensityPanelty; // INIT_LAMBDA
  float initWireLengthCoeff; // base_wcof
  float targetOverflow; // overflow
  float minBoundMuK; // pcof_min
  float maxBoundMuK; // pcof_max
  float initialPrevCoordiUpdateCoeff; // z_ref_alpha
  NesterovPlaceVars();
};

class NesterovPlace {
public:
  NesterovPlace();
  NesterovPlace(NesterovPlaceVars npVars,
      std::shared_ptr<PlacerBase> pb,
      std::shared_ptr<NesterovBase> nb);
  ~NesterovPlace();

  void doNesterovPlace();

  void updateCoordi(std::vector<FloatCoordi>& coordi);
  void updateBins();
  void updateWireLength();

  void updateGradients(std::vector<FloatCoordi>& sumGrads,
      std::vector<FloatCoordi>& wireLengthGrads,
      std::vector<FloatCoordi>& densityGrads );

  void updateWireLengthCoef(float overflow);

  void updateInitialPrevCoordi();

  float getStepLength(
      std::vector<FloatCoordi>& prevCoordi_, 
      std::vector<FloatCoordi>& prevSumGrads_,
      std::vector<FloatCoordi>& curCoordi_,
      std::vector<FloatCoordi>& curSumGrads_ );

private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  NesterovPlaceVars npVars_;

  // y_st, y_dst, y_wdst, w_pdst
  std::vector<FloatCoordi> curCoordi_;
  std::vector<FloatCoordi> curWireLengthGrads_;
  std::vector<FloatCoordi> curDensityGrads_;
  std::vector<FloatCoordi> curSumGrads_;

  // y0_st, y0_dst, y0_wdst, y0_pdst
  std::vector<FloatCoordi> newCoordi_;
  std::vector<FloatCoordi> newWireLengthGrads_;
  std::vector<FloatCoordi> newDensityGrads_;
  std::vector<FloatCoordi> newSumGrads_;

  // z_st, z_dst, z_wdst, z_pdst
  std::vector<FloatCoordi> prevCoordi_;
  std::vector<FloatCoordi> prevWireLengthGrads_;
  std::vector<FloatCoordi> prevDensityGrads_;
  std::vector<FloatCoordi> prevSumGrads_;

  float wireLengthGradSum_;
  float densityGradSum_;

  // alpha
  float stepLength_;

  // opt_phi_cof
  float densityPanelty_;

  // base_wcof
  float baseWireLengthCoeff_;

  // wlen_cof
  float wireLengthCoeffX_;
  float wireLengthCoeffY_;

  // phi is described in ePlace paper.
  float sumPhi_;
  float sumOverflow_;

  // half-parameter-wire-length
  float hpwl_;

  float getWireLengthCoeff(float overflow);

  void init();
  void reset();

};
}

#endif

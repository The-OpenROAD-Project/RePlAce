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
  float initDensityPanelty;
  float initWireLengthCoeff;
  float targetOverflow;
  float minBoundMuK;
  float maxBoundMuK;
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
  float backTrackStepLength_;

  // opt_phi_cof
  float densityPanelty_;

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

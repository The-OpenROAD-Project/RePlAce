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
  int verboseLevel;
  float initDensityPenalty; // INIT_LAMBDA
  float initWireLengthCoef; // base_wcof
  float targetOverflow; // overflow
  float minPhiCoef; // pcof_min
  float maxPhiCoef; // pcof_max
  float minPreconditioner; // MIN_PRE
  float initialPrevCoordiUpdateCoef; // z_ref_alpha
  float referenceHpwl; // refDeltaHpwl
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

  void updateCoordi(
      std::vector<FloatCoordi>& coordi);
  void updateBins();
  void updateWireLength();

  void updateGradients(
      std::vector<FloatCoordi>& sumGrads,
      std::vector<FloatCoordi>& wireLengthGrads,
      std::vector<FloatCoordi>& densityGrads );

  void updateWireLengthCoef(float overflow);

  void updateInitialPrevSLPCoordi();

  float getStepLength(
      std::vector<FloatCoordi>& prevCoordi_,
      std::vector<FloatCoordi>& prevSumGrads_,
      std::vector<FloatCoordi>& curCoordi_,
      std::vector<FloatCoordi>& curSumGrads_ );

  void updateNextIter();
  float getPhiCoef(float scaledDiffHpwl);

  void updateDb();

private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  NesterovPlaceVars npVars_;

  // SLP is Step Length Prediction.
  //
  // y_st, y_dst, y_wdst, w_pdst
  std::vector<FloatCoordi> curSLPCoordi_;
  std::vector<FloatCoordi> curSLPWireLengthGrads_;
  std::vector<FloatCoordi> curSLPDensityGrads_;
  std::vector<FloatCoordi> curSLPSumGrads_;

  // y0_st, y0_dst, y0_wdst, y0_pdst
  std::vector<FloatCoordi> nextSLPCoordi_;
  std::vector<FloatCoordi> nextSLPWireLengthGrads_;
  std::vector<FloatCoordi> nextSLPDensityGrads_;
  std::vector<FloatCoordi> nextSLPSumGrads_;

  // z_st, z_dst, z_wdst, z_pdst
  std::vector<FloatCoordi> prevSLPCoordi_;
  std::vector<FloatCoordi> prevSLPWireLengthGrads_;
  std::vector<FloatCoordi> prevSLPDensityGrads_;
  std::vector<FloatCoordi> prevSLPSumGrads_;

  // x_st and x0_st
  std::vector<FloatCoordi> curCoordi_;
  std::vector<FloatCoordi> nextCoordi_;

  float wireLengthGradSum_;
  float densityGradSum_;

  // alpha
  float stepLength_;

  // opt_phi_cof
  float densityPenalty_;

  // base_wcof
  float baseWireLengthCoef_;

  // wlen_cof
  float wireLengthCoefX_;
  float wireLengthCoefY_;

  // phi is described in ePlace paper.
  float sumPhi_;
  float sumOverflow_;

  // half-parameter-wire-length
  int64_t prevHpwl_;

  float getWireLengthCoef(float overflow);

  void init();
  void reset();

};
}

#endif

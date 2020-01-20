#ifndef __REPLACE_HEADER__
#define __REPLACE_HEADER__

namespace odb {
  class dbDatabase;
}
namespace sta {
  class dbSta;
}

namespace replace {

class InitialPlace;
class NesterovPlace;
class PlacerBase;

class Replace
{
  public:
    Replace();
    ~Replace();

    void init();
    void reset();

    void setDb(odb::dbDatabase* odb);
    void setSta(sta::dbSta* dbSta);

    void doInitialPlace();
    void doNesterovPlace();

    void setMaxInitialPlaceIter(int iter);
    void setMaxNesvPlaceIter(int iter);

    void setBinGridCntX(int binGridCntX);
    void setBinGridCntY(int binGridCntY);

    void setTargetDensity(float density);
    void setTargetOverflow(float overflow);
    void setInitPenalityFactor(float penaltyFactor);
    void setMinPCoef(float minPCoef);
    void setMaxPCoef(float maxPCoef);

    // HPWL: half-parameter wire length.
    void setDeltaHpwl(float deltaHpwl);
    void setVerboseLevel(int verbose);

  private:
    odb::dbDatabase* db_;
    sta::dbSta* sta_;

    InitialPlace* ip_;
    NesterovPlace* np_;
    PlacerBase* pb_;

    int maxInitialPlaceIter_;
    int maxNesterovPlaceIter_;
    int binGridCntX_;
    int binGridCntY_;
    float overflow_;
    float density_;
    float initPenalityFactor_;
    float minPCoef_;
    float maxPCoef_;
    float deltaHpwl_;
    int verbose_;
};
}

#endif

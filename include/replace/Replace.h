#ifndef __REPLACE_HEADER__
#define __REPLACE_HEADER__

namespace odb {
  class dbDatabase;
}
namespace sta {
  class dbSta;
}

namespace replace {

class InitPlace;
class NesterovPlace;
class PlacerBase;

class Replace
{
  public:
    Replace();
    ~Replace();
    void init();
    void clear();

    void setDb(odb::dbDatabase* odb);
    void setSta(sta::dbSta* dbSta);

    void doInitPlace();
    void doNesterovPlace();

    void setMaxInitPlaceIter(int iter);
    void setMaxNesvPlaceIter(int iter);

    void setBinGridCntX(int binGridCntX);
    void setBinGridCntY(int binGridCntY);

    void setTargetDensity(float density);
    void setTargetOverflow(float overflow);
    void setInitLambda(float lambda);
    void setMinPCoef(float minPCoef);
    void setMaxPCoef(float maxPCoef);
    void setDeltaHpwl(float deltaHpwl);

    void setVerboseLevel(int verbose);

  private:
    odb::dbDatabase* db_;
    sta::dbSta* sta_;
    InitPlace* ip_;
    NesterovPlace* np_;
    PlacerBase* pb_;

    int maxInitPlaceIter_;
    int maxNesterovPlaceIter_;
    int binGridCntX_;
    int binGridCntY_;
    float overflow_;
    float density_;
    float lambda_;
    float minPCoef_;
    float maxPCoef_;
    float deltaHpwl_;
    int verbose_;
};
}

#endif

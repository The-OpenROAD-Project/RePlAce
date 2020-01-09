#ifndef __REPLACE_HEADER__
#define __REPLACE_HEADER__


namespace odb {
  class dbDatabase;
}
namespace sta {
  class dbSta;
}

namespace GlobalPlacer {

class Replace
{
  public:
    void setDb(odb::dbDatabase* odb);
    void setSta(sta::dbSta* dbSta);

    void doInitPlace();
    void doNesterovPlace();

    void setBinCntX(int binCntX);
    void setBinCntY(int binCntY);

    void setTargetDensity(float density);
    void setTargetOverflow(float overflow);
    void setInitLambda(float lambda);
    void setMinPCoef(float minPCoef);
    void setMaxPCoef(float maxPCoef);
    void setDeltaHpwl(float deltaHpwl);

  private:
    odb::dbDatabase* odb_;
    sta::dbSta* sta_;

    int maxInitPlaceIter_;
    int maxNesterovPlaceIter_;
    int binCntX_;
    int binCntY_;
    float overflow_;
    float density_;
    float lambda_;
    float minPCoef_;
    float maxPCoef_;
    float deltaHpwl_;
};

}

#endif

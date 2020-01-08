#ifndef __REPLACE_HEADER__
#define __REPLACE_HEADER__


namespace odb {
  class dbDatabase;
}
namespace sta {
  class dbSta;
}

namespace GlobalPlacer {

enum InitPlaceMode
{
  BiCGSTAB
}


class Replace
{
  public:
    void setDb(odb::dbDatabase* odb);
    void setSta(sta::dbSta* dbSta);

    void initPlace(InitPlaceMode initPlace);
    void setBinCntX(int binCntX);
    void setBinCntY(int binCntY);

    void setInitLambda(float lambda);
    void setMinPCoef(float minPCoef);
    void setMaxPCoef(float maxPCoef);
    void setDeltaHpwl(float deltaHpwl);

  private:
    odb::dbDatabase* odb_;
    sta::dbSta* sta_;
};

}

#endif

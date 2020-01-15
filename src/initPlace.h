#ifndef __REPLACE_INIT_PLACE__
#define __REPLACE_INIT_PLACE__

#include <Eigen/SparseCore>
#include <opendb/db.h>

namespace replace {

class PlacerBase;
class InitPlaceVars {
public:
  int maxInitPlaceIter;
  int minDiffLength;
  int verbose;
  InitPlaceVars();
  void clear();
};

typedef Eigen::SparseMatrix<int, Eigen::RowMajor> SMatrix;

class InitPlace {
  public:
    InitPlace();
    InitPlace(PlacerBase* placerBase);
    ~InitPlace();
    
    void clear();
    void setInitPlaceVars(InitPlaceVars initPlaceVars);
    void doInitPlace();

  private:
    InitPlaceVars initPlaceVars_;
    PlacerBase* pb_;
    Eigen::VectorXf xcgX_, xcgB_;
    Eigen::VectorXf ycgX_, ycgB_;
    SMatrix matX_, matY_;

    void placeInstsCenter();
    void setPlaceInstExtId();
    void updatePinInfo();
    void createSparseMatrix();
};

}
#endif

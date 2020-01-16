#ifndef __REPLACE_INIT_PLACE__
#define __REPLACE_INIT_PLACE__

#include <Eigen/SparseCore>
#include <opendb/db.h>

namespace replace {

class PlacerBase;
class InitialPlaceVars {
public:
  int maxInitialPlaceIter;
  int minDiffLength;
  int solverIterMax;
  float netWeightScale;
  int verbose;
  InitialPlaceVars();
  void clear();
};

typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;

class InitialPlace {
  public:
    InitialPlace();
    InitialPlace(PlacerBase* placerBase);
    ~InitialPlace();
    
    void clear();
    void setInitialPlaceVars(InitialPlaceVars initialPlaceVars);
    void doBicgstabPlace();

  private:
    InitialPlaceVars initialPlaceVars_;
    PlacerBase* pb_;
    Eigen::VectorXf xcgX_, xcgB_;
    Eigen::VectorXf ycgX_, ycgB_;
    SMatrix matX_, matY_;

    void placeInstsCenter();
    void setPlaceInstExtId();
    void updatePinInfo();
    void createSparseMatrix();
    void updateCoordi();
};

}
#endif

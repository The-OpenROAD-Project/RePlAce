#ifndef __REPLACE_INIT_PLACE__
#define __REPLACE_INIT_PLACE__

#include <Eigen/Core>
#include <opendb/db.h>

namespace replace {

class InitPlaceVars {
public:
  int maxInitPlaceIter;
  InitPlaceVars();
  void clear();
};


typedef Eigen::SparseMatrix<int, Eigen::RowMajor> SMatrix;

class InitPlace {
  public:
    InitPlace();
    ~InitPlace();

    void init();
    void clear();
    void setDb(odb::dbDatabase* db);
    void setPlaceInsts(odb::dbSet<odb::dbInst>* placeInst);
    void setInitPlaceVars(InitPlaceVars initPlaceVars);
    void doInitPlace();

  private:
    odb::dbDatabase* db_;
    InitPlaceVars initPlaceVars_;
    odb::dbSet<odb::dbInst>* placeInsts_;
    Eigen::VectorXf xcgX_, xcgB_;
    Eigen::VectorXf ycgX_, ycgB_;
    SMatrix matX_, matY_;
    void createSparseMatrix();
};

}
#endif

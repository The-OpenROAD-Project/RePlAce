#ifndef __PLACER_BASE__
#define __PLACER_BASE__

#include <vector>
#include <unordered_map>

namespace odb {
class dbDatabase;

class dbInst;
class dbITerm;
class dbBTerm;
class dbNet;

class dbPlacementStatus;
class dbSigType;

class dbBox;

class adsRect;
}

namespace replace {

class Pin;
class Net;
class GCell;

class Instance {
public:
  Instance();
  Instance(odb::dbInst* inst);
  Instance(int lx, int ly, int ux, int uy);
  ~Instance();

  odb::dbInst* dbInst() const { return inst_; }

  // a cell that no need to be moved.
  bool isFixed() const;

  // a instance that need to be moved.
  bool isInstance() const;

  // Dummy is virtual instance to fill in 
  // empty fragmented row structures.
  // will have inst_ as nullptr
  bool isDummy() const;

  void setLocation(int x, int y);
  void setCenterLocation(int x, int y);

  void dbSetPlaced();
  void dbSetPlacementStatus(odb::dbPlacementStatus ps);
  void dbSetLocation();
  void dbSetLocation(int x, int y);
  void dbSetCenterLocation(int x, int y);

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  void setExtId(int extId);
  int extId() const { return extId_; }

  void addPin(Pin* pin);
  const std::vector<Pin*> & pins() const { return pins_; }

private:
  odb::dbInst* inst_;
  std::vector<Pin*> pins_;
  int lx_;
  int ly_;
  int ux_;
  int uy_;
  int extId_;
};

class Pin {
public:
  Pin();
  Pin(odb::dbITerm* iTerm);
  Pin(odb::dbBTerm* bTerm);
  ~Pin();

  odb::dbITerm* dbITerm() const;
  odb::dbBTerm* dbBTerm() const;

  bool isITerm() const;
  bool isBTerm() const;
  bool isMinPinX() const;
  bool isMaxPinX() const;
  bool isMinPinY() const;
  bool isMaxPinY() const;

  void setITerm();
  void setBTerm();
  void setMinPinX();
  void setMinPinY();
  void setMaxPinX();
  void setMaxPinY();
  void unsetMinPinX();
  void unsetMinPinY();
  void unsetMaxPinX();
  void unsetMaxPinY();

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;

  void updateLocation();
  void updateLocation(const Instance* inst);

  void setInstance(Instance* inst);
  void setNet(Net* net);

  Instance* instance() const { return inst_; }
  Net* net() const { return net_; }

private:
  void* term_;
  Instance* inst_;
  Net* net_;

  int offsetLx_;
  int offsetLy_;
  int offsetUx_;
  int offsetUy_;
  int lx_;
  int ly_;
  
  unsigned char iTermField_:1;
  unsigned char bTermField_:1;
  unsigned char minPinXField_:1;
  unsigned char minPinYField_:1;
  unsigned char maxPinXField_:1;
  unsigned char maxPinYField_:1;

  void updateOffset();
  void updateOffset(odb::dbITerm* iTerm);
  void updateOffset(odb::dbBTerm* bTerm);
  void updateLocation(odb::dbITerm* iTerm);
  void updateLocation(odb::dbBTerm* bTerm);
};

class Net {
public:
  Net();
  Net(odb::dbNet* net);
  ~Net();

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;

  // HPWL: half-parameter-wire-length
  int hpwl() const;

  void updateBox();

  const std::vector<Pin*> & pins() const { return pins_; }

  odb::dbNet* dbNet() const { return net_; }
  odb::dbSigType getSigType() const;

  void addPin(Pin* pin);

private:
  odb::dbNet* net_;
  std::vector<Pin*> pins_;
  int lx_;
  int ly_;
  int ux_;
  int uy_;
};

class Die {
public:
  Die();
  Die(odb::dbBox* dieBox, odb::adsRect* coreRect);
  ~Die();

  void setDieBox(odb::dbBox* dieBox);
  void setCoreBox(odb::adsRect* coreBox);

  int dieLx() const { return dieLx_; }
  int dieLy() const { return dieLy_; }
  int dieUx() const { return dieUx_; }
  int dieUy() const { return dieUy_; }

  int coreLx() const { return coreLx_; }
  int coreLy() const { return coreLy_; }
  int coreUx() const { return coreUx_; }
  int coreUy() const { return coreUy_; }

  int dieCx() const;
  int dieCy() const;
  int coreCx() const;
  int coreCy() const;

private:
  int dieLx_;
  int dieLy_;
  int dieUx_;
  int dieUy_;
  int coreLx_;
  int coreLy_;
  int coreUx_;
  int coreUy_;
};

class Bin {
public:
  Bin();
  Bin(int lx, int ly, int ux, int uy);
  ~Bin();

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  float phi() const;
  float density() const;
  float electroForce() const;

  void setPhi(float phi);
  void setDensity(float density);
  void setElectroForce(float electroForce);

protected:
  uint32_t & nonPlaceArea();
  uint32_t & placedArea();
  uint32_t & fillerArea();

private:
  int lx_;
  int ly_;
  int ux_;
  int uy_;

  uint32_t nonPlaceArea_;
  uint32_t placedArea_;
  uint32_t fillerArea_;

  float phi_;
  float density_;
  float electroForce_;

  friend class BinGrid;
};

//
// The bin can be non-uniform because of
// "integer" coordinates
//
class BinGrid {
public:
  BinGrid();
  BinGrid(Die* die);
  ~BinGrid();

  void setCoordi(Die* die);
  void setBinCnt(int binCntX, int binCntY);
  void setBinCntX(int binCntX);
  void setBinCntY(int binCntY);
  void updateBinsArea(std::vector<GCell*>& cells);
  void updateBinsNonplaceArea(std::vector<Instance*>& fixedCells);

  void initBins();

  // lx, ly, ux, uy will hold coreArea
  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;
  int cx() const;
  int cy() const;
  int dx() const;
  int dy() const;

  int binCntX() const;
  int binCntY() const;
  int binSizeX() const;
  int binSizeY() const;

  // return bins_ index with given gcell
  std::pair<int, int> getMinMaxIdxX(GCell* gcell);
  std::pair<int, int> getMinMaxIdxY(GCell* gcell);

  const std::vector<Bin*> & bins() const { return binsPtr_; }

private:
  std::vector<Bin> bins_;
  std::vector<Bin*> binsPtr_;
  int lx_;
  int ly_;
  int ux_;
  int uy_;
  int binCntX_;
  int binCntY_;
  int binSizeX_;
  int binSizeY_;
  unsigned char isSetBinCntX_:1;
  unsigned char isSetBinCntY_:1;
};

class PlacerBase {
public:
  PlacerBase();
  PlacerBase(odb::dbDatabase* db);
  ~PlacerBase();

  void init();
  void reset();

  const std::vector<Instance*>& insts() const { return insts_; }
  const std::vector<Pin*>& pins() const { return pins_; }
  const std::vector<Net*>& nets() const { return nets_; }

  // 
  // placeInsts : a real instance that need to be placed
  // fixedInsts : a real instance that is fixed (e.g. macros, tapcells)
  // dummyInsts : a fake instance that is for fragmented-row handling
  //
  // nonPlaceInsts : fixedInsts + dummyInsts to enable fast-iterate on Bin-init
  //
  const std::vector<Instance*>& placeInsts() const { return placeInsts_; }
  const std::vector<Instance*>& fixedInsts() const { return fixedInsts_; }
  const std::vector<Instance*>& dummyInsts() const { return dummyInsts_; }
  const std::vector<Instance*>& nonPlaceInsts() const { return nonPlaceInsts_; }

  Die& die() { return die_; }

  Instance* dbToPlace(odb::dbInst* inst) const;
  Pin* dbToPlace(odb::dbITerm* pin) const;
  Pin* dbToPlace(odb::dbBTerm* pin) const;
  Net* dbToPlace(odb::dbNet* net) const;

  int siteSizeX() const { return siteSizeX_; }
  int siteSizeY() const { return siteSizeY_; }

  int hpwl() const;
  void printInfo() const;

private:
  odb::dbDatabase* db_;
  
  BinGrid binGrid_;
  Die die_;

  std::vector<Instance> instStor_;
  std::vector<Pin> pinStor_;
  std::vector<Net> netStor_;

  std::vector<Instance*> insts_;
  std::vector<Pin*> pins_;
  std::vector<Net*> nets_;

  std::unordered_map<odb::dbInst*, Instance*> instMap_;
  std::unordered_map<void*, Pin*> pinMap_;
  std::unordered_map<odb::dbNet*, Net*> netMap_;

  std::vector<Instance*> placeInsts_;
  std::vector<Instance*> fixedInsts_;
  std::vector<Instance*> dummyInsts_;
  std::vector<Instance*> nonPlaceInsts_;

  int siteSizeX_;
  int siteSizeY_;

  uint64_t placeInstsArea_;
  uint64_t nonPlaceInstsArea_;
  
  void initBinGrid();
  void initInstsForFragmentedRow();
};

}

#endif

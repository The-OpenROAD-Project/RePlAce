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

class Instance {
public:
  Instance();
  Instance(odb::dbInst* inst);
  ~Instance();

  odb::dbInst* inst() { return inst_; }
  bool isFixed();

  void setLocation(int x, int y);
  void setCenterLocation(int x, int y);

  void dbSetPlaced();
  void dbSetPlacementStatus(odb::dbPlacementStatus ps);
  void dbSetLocation();
  void dbSetLocation(int x, int y);
  void dbSetCenterLocation(int x, int y);
  
  int lx();
  int ly();
  int ux();
  int uy();
  int cx();
  int cy();
  
  void setExtId(int extId);
  int extId() { return extId_; }

  void addPin(Pin* pin);
  std::vector<Pin*> & pins() { return pins_; }

private:
  odb::dbInst* inst_;
  std::vector<Pin*> pins_;
  int lx_;
  int ly_;
  int extId_;
};

class Pin {
public:
  Pin();
  Pin(odb::dbITerm* iTerm);
  Pin(odb::dbBTerm* bTerm);
  ~Pin();

  odb::dbITerm* iTerm();
  odb::dbBTerm* bTerm();

  bool isITerm();
  bool isBTerm();
  bool isMinPinX();
  bool isMaxPinX();
  bool isMinPinY();
  bool isMaxPinY();

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
 
  int offsetLx();
  int offsetLy();
  int offsetUx();
  int offsetUy();

  int lx();
  int ly();
  int ux();
  int uy();
  int cx();
  int cy();
  
  void updateLocation();

  void setInstance(Instance* inst);
  void setNet(Net* net);

  Instance* instance() { return inst_; }
  Net* net() { return net_; }

private:
  void* term_;
  Instance* inst_;
  Net* net_;
  
  uint8_t attribute_;

  int offsetLx_;
  int offsetLy_;
  int offsetUx_;
  int offsetUy_;
  int lx_;
  int ly_;
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

  int lx();
  int ly();
  int ux();
  int uy();
  int cx();
  int cy();
  int hpwl();

  void updateBox();

  std::vector<Pin*> & pins() { return pins_; }

  odb::dbNet* net() { return net_; }
  odb::dbSigType getSigType();

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

  int dieLx() { return dieLx_; }
  int dieLy() { return dieLy_; }
  int dieUx() { return dieUx_; }
  int dieUy() { return dieUy_; }

  int coreLx() { return coreLx_; }
  int coreLy() { return coreLy_; }
  int coreUx() { return coreUx_; }
  int coreUy() { return coreUy_; }

  int dieCx(); 
  int dieCy();
  int coreCx();
  int coreCy();

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

class PlacerBase {
public:
  PlacerBase();
  PlacerBase(odb::dbDatabase* db);
  ~PlacerBase();

  void init();
  void clear();

  std::vector<Instance>& insts() { return insts_; }
  std::vector<Pin>& pins() { return pins_; }
  std::vector<Net>& nets() { return nets_; }

  std::vector<Instance*>& placeInsts() { return placeInsts_; }
  std::vector<Instance*>& fixedInsts() { return fixedInsts_; }

  Die& die() { return die_; }

  Instance* dbToPlace(odb::dbInst* inst);
  Pin* dbToPlace(odb::dbITerm* pin);
  Pin* dbToPlace(odb::dbBTerm* pin);
  Net* dbToPlace(odb::dbNet* net);

  int hpwl();
  void printInfo();

private:
  odb::dbDatabase* db_;
  Die die_;
  std::vector<Instance> insts_;
  std::vector<Pin> pins_;
  std::vector<Net> nets_;
  std::unordered_map<odb::dbInst*, Instance*> instMap_;
  std::unordered_map<void*, Pin*> pinMap_;
  std::unordered_map<odb::dbNet*, Net*> netMap_;
  
  std::vector<Instance*> placeInsts_;
  std::vector<Instance*> fixedInsts_;
};

}

#endif

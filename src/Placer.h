#ifndef __PLACER_BASE__
#define __PLACER_BASE__

#include <opendb/db.h>

namespace replace {

class instance {
public:
  dbInst* inst() { return inst_; }
  int lx() { return lx_; }
  int ly() { return ly_; }
  int ux() { return lx_ + inst()->getMaster()->}
  bool placed() { return placed_; }
  void setLocation(int x, int y);
  void setCenterLocation(int x, int y);

  instance();
  ~instance();

private:
  dbInst* inst_;
  bool placed_;
};

class pin {
public:
  dbITerm* iTerm() { return iTerm_; }
  int lx() { return lx_; }
  int ly() { return ly_; }
  int ux()
  int uy()

  pin();
  ~pin();

  bool isITerm();
  bool isBTerm();
  bool isMinPinX();
  bool isMaxPinX();
  bool isMinPinY();
  bool isMaxPinY();

private:
  union{
    dbITerm* iTerm;
    dbBTerm* bTerm;
  }
  uint8_t attribute_;
};

class net {
  public:
    dbNet* net() { return net_; }
    int lx() { return }
};

class PlacerBase {
public:
  PlacerBase(odb::dbDatabase* db);
  ~PlacerBase();

  std::vector<instance>& insts() { return insts_; }
  std::vector<pin>& pins() { return pins_; }
  std::vector<net>& nets() { return nets_; }

  instance* dbToPlace(dbInst* inst);
  pin* dbToPlace(dbITerm* pin);
  pin* dbToPlace(dbBTerm* pin);
  net* dbToPlace(dbNet* net);

private:
  odb::dbDatabase* db_;
  std::vector<instance> insts_;
  std::vector<pin> pins_;
  std::vector<net> nets_;
  map<dbInst*, instance*> instMap_;
  map<dbITerm*, pin*> pinMap_;
  map<dbNet*, net*> netMap_;
}

}

#endif

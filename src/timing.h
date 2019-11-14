#ifndef __REPLACE_TIMING__
#define __REPLACE_TIMING__ 0

#include "replace_private.h"
#include "lefdefIO.h"

#include <fstream>
#include <flute.h>
#include <boost/functional/hash.hpp>
#include <tcl.h>
#include <limits>

#define DBU_MAX std::numeric_limits< DBU >::max()

#define CAP_SCALE 1.0e-12
#define RES_SCALE 1.0

#define MAX_WIRE_SEGMENT_IN_MICRON 20.0
#define TIMING_PIN_CAP 4e-15

namespace sta {
class Sta;
}


namespace Timing { 

struct pin {
  int origIdx;      // pinInst's idx
  bool isTerm;      // whether this pin is from terminal or module
  int owner;        /* The owners of PIs or POs are UINT_MAX */
  int net;          // netInst's idx
  int type;         // 0 : input, 1 : output
  bool isFlopInput; /* is this pin an input  of a clocked element? */
  bool isFlopCkPort;

  // from .sdc
  double cap;          /* PO load if exists (in Farad) */
  double delay;        /* PI/PO input/output delay if exists (in sec) */
  double rTran, fTran; /* input rise/fall transition if exits (in sec) */
  int driverType;      /* input driver for PIs */

  bool isClock;  // terminal is always fixed

  // from timer
  double earlySlk, lateSlk;
  pin(); 
};

struct net {
  std::string name;
  double lumpedCap;

  // pin Index
  vector< int > sources;
  vector< int > sinks;

  vector< pair< pair< std::string, std::string >, double > > wireSegs;

  int origIdx;  // netInstance; clock_net --> INT_MAX

  // constructor
  net();
  net(std::string netname, double lcap, 
      vector< int > sources, vector< int > xsinks, int _origIdx);
};

#define PINNUM_TYPE uint64_t

class PinInfo {
 private:
  // 0 ~ 1 << 7 : module
  // 1 << 7 ~ 1 << 8 - 1 : terminal
  // uint32_t_max (== 1<<8 -1) : steiner point
  uint32_t data;

  PINNUM_TYPE pNum;
  int netIdx;

 public:
  // constructor
  PinInfo();
  PinInfo(PIN* curPin);
  
  // copy constructor
  PinInfo(const PinInfo& k);

  uint32_t GetData() const;
  PINNUM_TYPE GetPinNum() const;
  uint32_t GetIdx() const;

  void SetPinInfo(PIN* curPin);

  // pinIdx is Terminals' pin's index
  void SetTerminal(int termIdx, PINNUM_TYPE pinIdx);

  // pinIdx is Modules' pin's index
  void SetModule(int moduleIdx, PINNUM_TYPE pinIdx);
  void SetSteiner(PINNUM_TYPE stnIdx, int _netIdx);

  // required for generating hash-map's key
  void SetImpossible();

  // pinName Return function
  std::string GetPinName(
      void* ptr, 
      vector< vector< string > >& pNameStor, 
      bool isEscape = true);

  // only for steiner point
  std::string GetStnPinName( bool isEscape = true);

  bool isTerminal(); 
  bool isModule(); 
  bool isSteiner();

  void Print();
};

struct wire {
  PinInfo iPin;
  PinInfo oPin;
  double length;

  wire(PinInfo ipin, PinInfo opin, double length);
  void Print();
};

class Timing {
 private:
  
  MODULE* _modules;
  TERM* _terms;

  NET* _nets;
  int _netCnt;

  PIN* _pins;
  int _pinCnt;
  
  // due to weird structure,
  // it stores variable names in below structure...
  vector< vector< std::string > >& _mPinName;
  vector< vector< std::string > >& _tPinName;

  int _unitX;
  int _unitY;

  // clock Info  
  std::string _clkName;
  float _clkPeriod;
  
  // required for script usages 
  // for openSTA; 
  // SPEF write
  int scriptIterCnt;

  // lef to def variable
  int _l2d;

  sta::Sta* _sta;
  Tcl_Interp* _interp;

  float _targetTop;

  // wire segment stor
  vector< vector< wire > > wireSegStor;
  vector< double > lumpedCapStor;


  // Fill Net and Pin Information again for clock-based placement
  void FillNetPin();

  // helper function for return the pinName
  inline std::string GetPinName(PIN* curPin, bool isEscape = true);

  // helper function for return the pinName
  inline std::string GetPinName(PinInfo& curPin, bool isEscape = true);

  void CleanSteiner();

  // additional write for clock net
  void WriteSpefClockNet(std::stringstream& feed);

  void WriteSpefClockNetDef(std::stringstream& feed);
  void WriteSpefClockNetVerilog(std::stringstream& feed);

  // For OpenSTA
  void FillSpefForSta();
  void MakeParasiticsForSta();

  void GenerateClockSta();
  void UpdateTimingSta();
  void UpdateNetWeightSta();

 public:
  Timing(MODULE* modules, TERM* terms, NET* nets, int netCnt, PIN* pins,
         int pinCnt, 
         vector< vector< std::string > >& mPinName,
         vector< vector< std::string > >& tPinName, 
         std::string clkName, float clkPeriod);

  // Steiner point generating
  // it assumes that pin location is updated
  //
  // store wireSegment into wireSegStor
  void BuildSteiner(bool scaleApplied = false);

  // ? not sure, but needs to be sliced?
  //        void SliceLongWire();

  // copy from lefdefio.cpp
  void SetLefDefEnv();

  void WriteSpef(const std::string& spefFile);
  void ExecuteStaFirst(std::string topCellName, std::string verilogName,
                       vector< std::string >& libName, std::string sdcName);
  void ExecuteStaLater();
};

inline bool operator==(const PinInfo& lhs, const PinInfo& rhs) {
  return (lhs.GetData() == rhs.GetData()) &&
         (lhs.GetPinNum() == rhs.GetPinNum());
}

inline bool operator!=(const PinInfo& lhs, const PinInfo& rhs) {
  return !(lhs == rhs);
}

}

// for hash-map enhancement
// cusom hash -function
template < class T >
struct MyHash;

template <>
struct MyHash< std::pair< DBU, DBU > > {
  std::size_t operator()(const pair< DBU, DBU >& k) const {
    using boost::hash_combine;
    size_t seed = 0;
    hash_combine(seed, k.first);
    hash_combine(seed, k.second);

    return seed;
  }
};

template <>
struct MyHash< Timing::PinInfo > {
  std::size_t operator()(const Timing::PinInfo& k) const {
    using boost::hash_combine;
    size_t seed = 0;
    hash_combine(seed, k.GetData());
    hash_combine(seed, k.GetPinNum());
    return seed;
  }
};

long GetTimingHPWL();

#endif

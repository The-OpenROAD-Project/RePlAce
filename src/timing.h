#ifndef __REPLACE_TIMING__
#define __REPLACE_TIMING__ 0

#include "global.h"
#include "lefdefIO.h"

#include <fstream>
#include <flute.h>
#include <boost/functional/hash.hpp>
#include <tcl.h>
#include <limits>

#define TIMING_NAMESPACE_OPEN namespace Timing {
#define TIMING_NAMESPACE_CLOSE }

#define DBU_MAX std::numeric_limits< DBU >::max()

#define CAP_SCALE 1.0e-12
#define RES_SCALE 1.0

#define MAX_WIRE_SEGMENT_IN_MICRON 20.0
#define TIMING_PIN_CAP 4e-15

using std::stringstream;
using std::numeric_limits;

namespace sta {
class Sta;
}

TIMING_NAMESPACE_OPEN

struct pin {
  // from verilog
  //    string name;                      //separately saved
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

  // from .def
  //    double x_coord, y_coord;             /* (in DBU) */
  //    double x_offset, y_offset;           /* COG of VIA relative to the
  //    origin of a cell, (in DBU) */

  bool isClock;  // terminal is always fixed

  // from timer
  double earlySlk, lateSlk;

  pin()
      : origIdx(INT_MAX),
        owner(INT_MAX),
        net(INT_MAX),
        type(INT_MAX),
        isFlopInput(false),
        isFlopCkPort(false),
        cap(0.0),
        delay(0.0),
        rTran(0.0),
        fTran(0.0),
        driverType(INT_MAX),
        //    x_coord(0.0), y_coord(0.0), x_offset(0.0), y_offset(0.0),
        isClock(false),
        earlySlk(0.0),
        lateSlk(0.0) {
  }
};

struct net {
  string name;
  double lumpedCap;

  // pin Index
  vector< int > sources;
  vector< int > sinks;

  vector< pair< pair< string, string >, double > > wireSegs;

  int origIdx;  // netInstance; clock_net --> INT_MAX

  net() : name(""), lumpedCap(0.0), origIdx(INT_MAX){};
  net(string _name, double _lCap, vector< int > _sources, vector< int > _sinks,
      int _origIdx)
      : name(_name),
        lumpedCap(_lCap),
        sources(_sources),
        sinks(_sinks),
        origIdx(_origIdx){};
};

#define PINNUM_TYPE uint64_t

struct PinInfo {
 private:
  uint32_t data;
  // 0 ~ 1 << 7 : module
  // 1 << 7 ~ 1 << 8 - 1 : terminal
  // uint32_t_max (== 1<<8 -1) : steiner point

  PINNUM_TYPE pNum;
  int netIdx;

 public:
  PinInfo()
      : data(0), pNum(numeric_limits< PINNUM_TYPE >::max()), netIdx(INT_MAX){};

  // For Module/terminal
  PinInfo(PIN* curPin) {
    SetPinInfo(curPin);
  }

  //        PinInfo( uint64_t stnIdx ) {
  //            SetSteiner(stnIdx);
  //        }

  PinInfo(const PinInfo& k) {
    data = k.data;
    pNum = k.pNum;
    netIdx = k.netIdx;
  }

  uint32_t GetData() const {
    return data;
  };
  PINNUM_TYPE GetPinNum() const {
    return pNum;
  };

  uint32_t GetIdx() const {
    return data & ((uint32_t)(1 << 31) - 1);
  };

  inline void SetPinInfo(PIN* curPin) {
    netIdx = curPin->netID;
    if(curPin->term) {
      SetTerminal(curPin->moduleID, curPin->pinIDinModule);
    }
    else {
      SetModule(curPin->moduleID, curPin->pinIDinModule);
    }
  }

  // pinIdx is Terminals' pin's index
  inline void SetTerminal(int termIdx, PINNUM_TYPE pinIdx) {
    data = termIdx;
    data |= (1 << 31);
    pNum = pinIdx;
  }

  // pinIdx is Modules' pin's index
  inline void SetModule(int moduleIdx, PINNUM_TYPE pinIdx) {
    data = moduleIdx;
    pNum = pinIdx;
  }

  inline void SetSteiner(PINNUM_TYPE stnIdx, int _netIdx) {
    data = numeric_limits< uint32_t >::max();
    pNum = stnIdx;
    netIdx = _netIdx;
  }

  // required for generating hash-map's key
  inline void SetImpossible() {
    data = numeric_limits< uint32_t >::max();
    pNum = numeric_limits< PINNUM_TYPE >::max();
    netIdx = numeric_limits< int >::max();
  }

  inline string GetPinName(void* ptr, vector< vector< string > >& pNameStor, bool isEscape = true) {
    if(isSteiner()) {
      cout << "ERROR: GetPinName must be executed only when NOT Steiner Point"
           << endl;
      exit(1);
    }

    if(isModule()) {
      string moduleName(((MODULE*)ptr)[GetIdx()].name);
      if( isEscape ) {
        SetEscapedStr(moduleName);
      }
      return moduleName + ":" + pNameStor[GetIdx()][pNum];
    }
    else if(isTerminal()) {
      //                cout << "data: " << data << endl;
      //                cout << "idx: " << GetIdx() << endl;
      //                cout << "pidx: " << pNum  << endl;
      //                cout << "name: " << ((TERM* )ptr)[GetIdx()].name <<
      //                endl;
      //                cout << "pname: " << pNameStor[GetIdx()][pNum] << endl;

      ///!!!!!!
      if(((TERM*)ptr)[GetIdx()].isTerminalNI) {
        return pNameStor[GetIdx()][pNum];
      }
      else {
        string termName(((TERM*)ptr)[GetIdx()].name);
        if( isEscape ) {
          SetEscapedStr(termName);
        }
        return termName + ":" + pNameStor[GetIdx()][pNum];
      }
    }
  }

  // only for steiner point
  inline string GetStnPinName( bool isEscape = true) {
    if(!isSteiner()) {
      cout << "ERROR: GetStnPinName must be executed only when Steiner Point"
           << endl;
      exit(1);
    }
    //            return "sp_" + to_string(pNum);
    //            cout << netIdx << endl;
    string netStr(netInstance[netIdx].name);
    if( isEscape ) {
      SetEscapedStr(netStr);
    }

    return netStr + ":" + to_string(pNum);
  }

  bool isTerminal() {
    return (1 & (data >> 31)) && data != numeric_limits< uint32_t >::max();
  };
  bool isModule() {
    //            cout << "data: " << data << " -> " << (data >> 31) << endl;
    return (data >> 31) == 0;
  };
  bool isSteiner() {
    return data == numeric_limits< uint32_t >::max();
  };

  void Print() {
    //            cout << data << " " << pNum;

    if(isSteiner()) {
      cout << "(sp_" << pNum << ")";
    }
    else if(isTerminal()) {
      cout << "(T_" << GetIdx() << " " << pNum << ")";
    }
    else {
      cout << "(M_" << GetIdx() << " " << pNum << ")";
    }
  }
};

struct wire {
  PinInfo iPin;
  PinInfo oPin;
  double length;
  wire(PinInfo _iPin, PinInfo _oPin, double _length)
      : iPin(_iPin), oPin(_oPin), length(_length){};
  void print() {
    iPin.Print();
    cout << " -> ";
    oPin.Print();
    cout << " : " << length << endl;
  }
};

class Timing {
 private:
  NET* _nets;
  int _netCnt;

  PIN* _pins;
  int _pinCnt;

  MODULE* _modules;
  TERM* _terms;

  int _unitX;
  int _unitY;

  // lef to def variable
  int _l2d;

  sta::Sta* _sta;
  Tcl_Interp* _interp;

  string _clkName;
  float _clkPeriod;
  float _targetTop;

  // wire segment stor
  vector< vector< wire > > wireSegStor;

  vector< double > lumpedCapStor;

  // due to weird structure,
  // it stores variable names in below structure...
  vector< vector< string > >& _mPinName;
  vector< vector< string > >& _tPinName;

  // source, sink, wireLength
  // share its idx with original net idx

  // required for script usages for openSTA; SPEF write
  int scriptIterCnt;

  // Fill Net and Pin Information again for clock-based placement
  void FillNetPin();

  // helper function for return the pinName
  inline string GetPinName(PIN* curPin, bool isEscape = true);

  // helper function for return the pinName
  inline string GetPinName(PinInfo& curPin, bool isEscape = true);

  void CleanSteiner();

  // additional write for clock net
  void WriteSpefClockNet(stringstream& feed);

  void WriteSpefClockNetDef(stringstream& feed);
  void WriteSpefClockNetVerilog(stringstream& feed);
  void UpdateSpefClockNetVerilog();

  // For OpenSTA
  void FillSpefForSta();
  void GenerateClockSta();
  void UpdateTimingSta();
  void UpdateNetWeightSta();

  // For OpenSTA with Script Modes
  //
  //        void ScriptWriteTcl(string& tclName,
  //            string& topCellName, string& verilogName, vector<string>&
  //            libStor,
  //            string& sdcName, string& spefName, string& outName,
  //            string& slackName);
  //        void ScriptUpdateSta(string& outCutName, int targetCnt);

 public:
  Timing(MODULE* modules, TERM* terms, NET* nets, int netCnt, PIN* pins,
         int pinCnt, vector< vector< string > >& mPinName,
         vector< vector< string > >& tPinName, string clkName, float clkPeriod)
      : _modules(modules),
        _terms(terms),
        _nets(nets),
        _netCnt(netCnt),
        _pins(pins),
        _pinCnt(pinCnt),
        _mPinName(mPinName),
        _tPinName(tPinName),
        _unitX(0.0),
        _unitY(0.0),
        _clkName(clkName),
        _clkPeriod(clkPeriod),
        scriptIterCnt(0) {
    wireSegStor.resize(netCnt);
    lumpedCapStor.resize(netCnt);
    SetLefDefEnv();
    //            FillNetPin();
  };

  /*
  void Init( MODULE* modules, TERM* terms, NET* nets, int netCnt,
          PIN* pins, int pinCnt,
          vector< vector<string> >& mPinName,
          vector< vector<string> >& tPinName ) {
      _modules = modules;
      _terms = terms;
      _nets = nets;
      _pins = pins;
      _netCnt = netCnt;
      _pinCnt = pinCnt;
      _mPinName = mPinName;
      _tPinName = tPinName;
      wireSegStor.resize(netCnt);
  }*/

  // Steiner point generating
  // it assumes that pin location is updated
  //
  // store wireSegment into wireSegStor
  void BuildSteiner(bool scaleApplied = false);

  // ? not sure, but needs to be sliced?
  //        void SliceLongWire();

  // copy from lefdefio.cpp
  void SetLefDefEnv();

  void WriteSpef(const string& spefFile);

  // for openSTA's engine modes
  //        void ExecuteSta( string topCellName, string verilogName,
  //                vector<string> &libName, string ffLibName);
  void ExecuteStaFirst(string topCellName, string verilogName,
                       vector< string >& libName, string sdcName);
  void ExecuteStaLater();
};

inline bool operator==(const PinInfo& lhs, const PinInfo& rhs) {
  return (lhs.GetData() == rhs.GetData()) &&
         (lhs.GetPinNum() == rhs.GetPinNum());
}

inline bool operator!=(const PinInfo& lhs, const PinInfo& rhs) {
  return !(lhs == rhs);
}

TIMING_NAMESPACE_CLOSE

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

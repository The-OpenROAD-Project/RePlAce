#include "timing.h"
#include "timingSta.h"

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <chrono>

#include <tcl.h>

using namespace sta;
using std::string;
using std::to_string;

//static const char *
//escapeDividers(const char *token,
//             const sta::Network *network);
static float 
GetMaxResistor(sta::Sta* sta, sta::Net* net);

namespace Timing { 

inline string Timing::GetPinName(PIN* curPin, bool isEscape) {
  // itself is PINS in def.
  if(curPin->term && _terms[curPin->moduleID].isTerminalNI) {
    return string(_terms[curPin->moduleID].Name());
  }

  // below is common
  string name = (curPin->term) ? string(_terms[curPin->moduleID].Name())
                               : string(_modules[curPin->moduleID].Name());

  if( isEscape ) {
    SetEscapedStr(name);
  }


  // bookshelf cases, it must be empty
  if(_mPinName.size() == 0 && _tPinName.size() == 0) {
    string pinPrefix = (curPin->IO == 0) ? "I" : "O";
    return name + "/" + pinPrefix + to_string(curPin->pinIDinModule);
  }
  // other LEF/DEF/VERILOG cases.
  else {
    return (curPin->term == 0)
               ? name + "/" + _mPinName[curPin->moduleID][curPin->pinIDinModule]
               : name + "/" +
                     _tPinName[curPin->moduleID][curPin->pinIDinModule];
  }
}

inline string Timing::GetPinName(PinInfo& curPin, bool isEscape) {
  if(curPin.isSteiner()) {
    return curPin.GetStnPinName(isEscape);
  }

  return (curPin.isModule()) ? 
    curPin.GetPinName((void*)_modules, _mPinName, isEscape)
    : curPin.GetPinName((void*)_terms, _tPinName, isEscape);
}


void TimingPathPrint(sta::Sta* sta, sta::PathEnd* end) {
  cout << "===========================================================" << endl;
  cout << "pathName: " << end->path()->name(sta) << endl;
  cout << "pathslack: " << end->slack(sta) << " seconds" << endl;

  PathExpanded expanded(end->path(), sta);

  for(size_t i = 0; i < expanded.size(); i++) {
    PathRef* path1 = expanded.path(i);
    // TimingArc* prevArc = expanded.prevArc(i);
    Vertex* vertex = path1->vertex(sta);
    Pin* pin = vertex->pin();
    // Arrival time = path1->arrival(sta) + timeOffset;
    // Arrival incr(0.0);

    // bool isClkStart = sta->network()->isRegClkPin(pin);
    bool isClk = path1->isClock(sta->search());

    // if( prevArc == NULL) {
    //   cout << "is first node" << endl;
    // }

    if(isClk) {
      continue;
    }
    cout << ((isClk) ? "C " : "UC ");

    Net* pinNet = NULL;
    if(sta->network()->isTopLevelPort(pin)) {
      const char* pinName = sta->cmdNetwork()->pathName(pin);
      cout << "(TL:" << pinName << ") ";
    }
    else {
      Net* net = sta->network()->net(pin);
      if(net) {
        Net* higestNet = sta->network()->highestNetAbove(net);
        const char* netName = sta->cmdNetwork()->pathName(higestNet);
        cout << "(" << netName << ") ";
        pinNet = higestNet;
      }
    }
    cout << sta->network()->pathName(sta->network()->instance(pin)) << " "
         << sta->network()->portName(pin) << " ("
         << sta->network()->vertexIndex(pin) << ")" << endl;
    //        cout << vertex->name(sta->network()) << endl;

    PortDirection* dir = sta->network()->direction(pin);
    // only output pins..!!!
    if(dir->isInput()) {
      continue;
    }

    if(pinNet) {
      NetConnectedPinIterator* connPinIter =
          sta->network()->connectedPinIterator(pinNet);

      while(connPinIter->hasNext()) {
        // Pin* curPin = connPinIter->next();
        // PortDirection* dir = sta->network()->direction(curPin);
        // if( dir->isInput() ) {
        //   continue;
        // }
//        cout << sta->network()->name(curPin) << " "
//             << GetMaxResistor(sta, curPin) << endl;
      }
    }

    /*
    VertexOutEdgeIterator voIter( vertex, sta->graph());
    float maxRes = 0.0f;
    while( voIter.hasNext() ) {
        Edge* edge = voIter.next();
        cout << "  Out " << edge->from(sta->graph())->name(sta->network())
            << " -> "
            << edge->to(sta->graph())->name(sta->network()) <<endl;

        float newRes = GetMaxResistor(sta, edge->to(sta->graph())->pin());
        maxRes = (maxRes < newRes)? newRes : maxRes;
        edge = voIter.next();
    }
    cout << "  OutMaxRes: " << maxRes << endl;

    VertexInEdgeIterator viIter( vertex, sta->graph());
    while( viIter.hasNext()) {
        Edge* edge = viIter.next();
        cout << "  In  " << edge->from(sta->graph())->name(sta->network())
            << " -> "
            << edge->to(sta->graph())->name(sta->network()) <<endl;

        float newRes = GetMaxResistor(sta, edge->to(sta->graph())->pin());
        maxRes = (maxRes < newRes)? newRes : maxRes;
        edge = viIter.next();
    }
    cout << "  InMaxRes: " << maxRes << endl;
    */
  }
  cout << "===========================================================" << endl;
  cout << endl;
}

void Timing::ExecuteStaFirst(string topCellName, string verilogName,
                             vector< string >& libStor, string sdcName) {
  cout << "Execute STA" << endl;
  cout << "topCellName: " << topCellName << endl;
  cout << "verilog    : " << verilogName << endl;
  for(auto& libName : libStor) {
    cout << "liberty    : " << libName << endl;
  }
  cout << "sdcName    : " << sdcName << endl << endl;

  // STA object create
  _sta = new Sta;

  // Tcl Interpreter settings
  //    Tcl_FindExecutable(argv[0]);
  _interp = Tcl_CreateInterp();

  // Initialize the TCL interpreter
  Tcl_Init(_interp);

  // define swig commands
  Sta_Init(_interp);

  // load encoded TCL functions
  evalTclInit(_interp, tcl_inits);
  // initialize TCL commands
  Tcl_Eval(_interp, "sta::show_splash");
  Tcl_Eval(_interp, "namespace import sta::*");
  
  Tcl_Eval(_interp, "define_sta_cmds");

  // initialize STA objects
  initSta();
  Sta::setSta(_sta);
  _sta->makeComponents();
  _sta->setTclInterp(_interp);
  _sta->setThreadCount(numThread);

  // environment settings

  string cornerName = "wst";
  // string cornerNameFF="bst";

  StringSet cornerNameSet;
  cornerNameSet.insert(cornerName.c_str());
  // cornerNameSet.insert(cornerNameFF.c_str());

  // define_corners
  _sta->makeCorners(&cornerNameSet);
  Corner* corner = _sta->findCorner(cornerName.c_str());
  // Corner *corner_FF = _sta->findCorner(cornerNameFF.c_str());

  // read_liberty
  for(auto& libName : libStor) {
    _sta->readLiberty(libName.c_str(), corner, MinMaxAll::max(), false);
  }

  // LibertyLibrary *FFlib1 =
  // _sta->readLiberty(ffLibName.c_str(), corner_FF, MinMaxAll::min(), false);

  // read_netlist
  NetworkReader* network = _sta->networkReader();
  if(!network) {
    cout << "ERROR: Internal OpenSTA has problem for generating networkReader" << endl;
    exit(1);
  }

  // Parsing the Verilog
  _sta->readNetlistBefore();
  if( !readVerilogFile(verilogName.c_str(), _sta->networkReader()) ) {
    cout << "ERROR: OpenSTA failed to read Verilog file!" << endl;
    exit(1);
  }

  // link_design
  PrintProcBegin("Timing: LinkDesign " + topCellName);
  Tcl_Eval(_interp, string("set link_make_black_boxes 0").c_str() ); 
  Tcl_Eval(_interp, string("link_design " + topCellName ).c_str() );

  bool is_linked = network->isLinked();
  if(is_linked) {
    PrintProcEnd("Timing: LinkDesign " + topCellName);
  }
  else {
    cout << "ERROR:  Linking Fail. Please put liberty files ";
    cout << "to instantiate OpenSTA correctly" << endl;
    exit(1);
  }

  // read_parasitics
  //    cout << spefFileName << endl;
  //  bool parasitics =
  //      _sta->readParasitics("simple.spef",
  //              _sta->currentInstance(),
  //              MinMaxAll::max(), false, true, 0.0,
  //              reduce_parasitics_to_pi_elmore, false, true, true);

  MakeParasiticsForSta(); 

  if(isClockGiven) {
    GenerateClockSta();
  }
  else {
    Tcl_Eval(_interp, string("sta::read_sdc " + sdcName).c_str());
  }

  // find_timing -full_update (true->full, false-> incremental)
  UpdateTimingSta();

  UpdateNetWeightSta();

  // WNS / TNS report
  const MinMax* cnst_min_max;
  cnst_min_max = MinMax::max();

  Slack wns; 
  Vertex *worstVertex;
  _sta->worstSlack(cnst_min_max, wns, worstVertex);

  Slack tns = _sta->totalNegativeSlack(cnst_min_max);
  PrintInfoPrecSignificant("Timing: WNS", wns);
  PrintInfoPrecSignificant("Timing: TNS", tns);
  globalWns = wns;
  globalTns = tns;

  float tol = 0.0;
  _sta->setIncrementalDelayTolerance(tol);
  Tcl_DeleteInterp(_interp);

  // _sta->clear();
  // _sta = NULL;
  // delete _sta;
}

void Timing::ExecuteStaLater() {
  for(int i = 0; i < _netCnt; i++) {
    _nets[i].timingWeight = 0;
  }
  // _sta->parasitics()->deleteParasitics();
  // _sta->network()->clear();

  auto start = std::chrono::steady_clock::now();
  MakeParasiticsForSta(); 
  auto finish = std::chrono::steady_clock::now();
  double elapsed_seconds =
      std::chrono::duration_cast< std::chrono::duration< double > >(finish -
                                                                    start)
          .count();

  PrintInfoRuntime("Timing: FillParasitcsForSta", elapsed_seconds, 1);

  _sta->setIncrementalDelayTolerance(1e-6);

  start = std::chrono::steady_clock::now();
  UpdateTimingSta();
  finish = std::chrono::steady_clock::now();

  elapsed_seconds =
      std::chrono::duration_cast< std::chrono::duration< double > >(finish -
                                                                    start)
          .count();

  PrintInfoRuntime("Timing: UpdateTimingSta", elapsed_seconds, 1);

  start = std::chrono::steady_clock::now();
  UpdateNetWeightSta();
  finish = std::chrono::steady_clock::now();
  elapsed_seconds =
      std::chrono::duration_cast< std::chrono::duration< double > >(finish -
                                                                    start)
          .count();
  PrintInfoRuntime("Timing: UpdateNetWeight", elapsed_seconds, 1);

  // WNS / TNS report
  const MinMax* cnst_min_max;
  cnst_min_max = MinMax::max();
  Slack wns; 
  Vertex *worstVertex;
  _sta->worstSlack(cnst_min_max, wns, worstVertex);
  Slack tns = _sta->totalNegativeSlack(cnst_min_max);
  
  PrintInfoPrecSignificant("Timing: WNS", wns);
  PrintInfoPrecSignificant("Timing: TNS", tns);
  globalWns = wns;
  globalTns = tns;
}

char* GetNewStr(const char* inp) {
  char* ret = new char[strlen(inp) + 1];
  strcpy(ret, inp);
  return ret;
}

// 
// Fill OpenSTA's parasitic models to have Cap / Res from FLUTE.
//
void Timing::MakeParasiticsForSta() {
  sta::Network* network = _sta->network();
  sta::Parasitics* parasitics = _sta->parasitics();

  HASH_MAP< PinInfo, bool, MyHash< PinInfo > > pin_cap_written;
  // map from pin name -> cap
  HASH_MAP< PinInfo, double, MyHash< PinInfo > > lumped_cap_at_pin;


  // 1. calc. lump sum caps from wire segments (PI2-model) + load caps
  for(int i = 0; i < _netCnt; i++) {
    for(auto& curWireSeg : wireSegStor[i]) {
      lumpedCapStor[i] += curWireSeg.length / (double)(_l2d)*capPerMicron;
      lumped_cap_at_pin[curWireSeg.iPin] +=
          curWireSeg.length / (double)(_l2d)*capPerMicron * 0.5;
      lumped_cap_at_pin[curWireSeg.oPin] +=
          curWireSeg.length / (double)(_l2d)*capPerMicron * 0.5;
      pin_cap_written[curWireSeg.iPin] = false;
      pin_cap_written[curWireSeg.oPin] = false;
    }

    for(int j = 0; j < _nets[i].pinCNTinObject; j++) {
      PIN* curPin = _nets[i].pin[j];
      if(curPin->term && _terms[curPin->moduleID].isTerminalNI &&
         curPin->IO == 1) {
        // This must be read from SDC file,
        // but in ICCAD contest, only outPin have PIN_CAP
        // as 4e-15
        lumpedCapStor[i] += TIMING_PIN_CAP;
        lumped_cap_at_pin[PinInfo(curPin)] += TIMING_PIN_CAP;
      }
    }
  }

  
  // get maximum pin counts in current sta network structure:
  int newPinIdx = network->pinCount()+1;

  const sta::Corner* corner = _sta->corners()->findCorner(0);
  _sta->corners()->makeParasiticAnalysisPtsMinMax();

  const sta::MinMax* min_max = MinMax::max();
  const sta::ParasiticAnalysisPt *ap = corner->findParasiticAnalysisPt(min_max); 
  
//  cout << "corner: " << corner << endl;
//  cout << "min_max: " << min_max << endl;
//  cout << "ap: " << ap << endl;

  // for each net
  for(int i=0; i<_netCnt; i++) {
    NET* curNet = &netInstance[i];
//    cout << "run: " << i << " " << curNet->Name() << endl;
    sta::Net* curStaNet = network->findNet( curNet->Name() );
    if( !curStaNet ) {
      cout << "cannot find: " << curNet->Name() << endl;
      cout << "Verilog and DEF are mismatched. Please check your input" << endl;
      exit(1);
    }

    Parasitic* parasitic = parasitics->makeParasiticNetwork(curStaNet, false, ap);
    for(auto& curSeg : wireSegStor[i]) {
      // check for IPin cap
      
      ParasiticNode *n1 = NULL, *n2 = NULL;

      // existed pin cases
      if( !curSeg.iPin.isSteiner() ) {
        string pinName = GetPinName(curSeg.iPin, false);
        sta::Pin* pin = network->findPin(pinName.c_str());
        if( !pin ) {
          cout << "cannot find: " << pinName << " in " 
            << curNet->Name() << " net." << endl;
          cout << "Verilog and DEF are mismatched. Please check your input" << endl;
          exit(1);
        }
        n1 = parasitics->ensureParasiticNode(parasitic, pin);
      }
      // virtual steinerPin cases
      // Set steiner pins' index as newPinIdx+1, newPinIdx+2, ....
      else {
        n1 = parasitics->ensureParasiticNode(parasitic, curStaNet, 
            newPinIdx + curSeg.iPin.GetPinNum());
      }

      // insert cap
      if(!pin_cap_written[curSeg.iPin]) {
        parasitics->incrCap(n1, lumped_cap_at_pin[curSeg.iPin], ap);
        pin_cap_written[curSeg.iPin] = true;
      }
      
      // existed pin cases
      if( !curSeg.oPin.isSteiner() ) {
        string pinName = GetPinName(curSeg.oPin, false);
        sta::Pin* pin = network->findPin(pinName.c_str());
        if( !pin ) {
          cout << "cannot find: " << curNet->Name() << ":" << pinName << endl;
          cout << "Verilog and DEF are mismatched. Please check your input" << endl;
          exit(1);
        }
        n2 = parasitics->ensureParasiticNode(parasitic, pin);
      }
      // virtual steinerPin cases
      // Set steiner pins' index as newPinIdx+1, newPinIdx+2, ....
      else {
        n2 = parasitics->ensureParasiticNode(parasitic, curStaNet, 
            newPinIdx + curSeg.oPin.GetPinNum());
      }

      if(!pin_cap_written[curSeg.oPin]) {
        parasitics->incrCap(n2, lumped_cap_at_pin[curSeg.oPin], ap);
        pin_cap_written[curSeg.oPin] = true;
      }

      // insert resistor.
      parasitics->makeResistor(nullptr, n1, n2, 
          curSeg.length / static_cast<double>(_l2d) * resPerMicron, ap);
      
    }
  }
  _sta->graphDelayCalc()->delaysInvalid();
  _sta->search()->arrivalsInvalid(); 
}


void Timing::GenerateClockSta() {
  // sdc -> clock definition (unit=second)
  // float clk_period = 70e-9;
  FloatSeq* waveform = new FloatSeq;
  waveform->push_back(0.0);
  waveform->push_back(_clkPeriod / 2);

  PinSet* pins = new PinSet;
  Pin* pin =
      _sta->network()->findPin(_sta->currentInstance(), _clkName.c_str());
  pins->insert(pin);

  _sta->makeClock(_clkName.c_str(), pins, true, _clkPeriod, waveform, NULL);

  // Clock *clock_found =
  _sta->findClock(_clkName.c_str());

  // write_sdc
  // _sta->writeSdc("test.sdc",false,false,5);
}

void Timing::UpdateTimingSta() {
  _sta->updateTiming(true);
}


void Timing::UpdateNetWeightSta() {
  // To enable scaling 
  // boundary values
  netWeightMin = FLT_MAX;
  netWeightMax = FLT_MIN;
 
  // extract WNS 
  Slack wns; 
  Vertex* worstVertex = NULL;
  
  const MinMax* cnst_min_max = MinMax::max();
  _sta->worstSlack(cnst_min_max, wns, worstVertex);

//  cout << "WNS: " << wns << endl;

  float minRes = FLT_MAX;
  float maxRes = FLT_MIN;

  // for normalize
  for(int i=0; i<_netCnt; i++) {
    NET* curNet = &_nets[i];
    sta::Net* curStaNet = _sta->network()->findNet(curNet->Name());
    if( !curStaNet ) {
      cout << "cannot find: " << curNet->Name() << endl;
      cout << "Verilog and DEF are mismatched. Please check your input" << endl;
      exit(1);
    }
    float netRes = GetMaxResistor(_sta, curStaNet);
    minRes = (minRes > netRes)? netRes : minRes;
    maxRes = (maxRes < netRes)? netRes : maxRes;
  }
    

  // for all nets
  for(int i=0; i<_netCnt; i++) {
    NET* curNet = &_nets[i];
    sta::Net* curStaNet = _sta->network()->findNet(curNet->Name());
    if( !curStaNet ) {
      cout << "cannot find: " << curNet->Name() << endl;
      cout << "Verilog and DEF are mismatched. Please check your input" << endl;
      exit(1);
    }

    
    float netSlack = _sta->netSlack(curStaNet, cnst_min_max);
    netSlack = (fabs(netSlack - MinMax::min()->initValue()) <= FLT_EPSILON) ? 
      0 : netSlack;

    float criticality = (wns>0)? 0 : max(0.0f, netSlack / wns);

//    cout << "diff: " << fabs(netSlack - MinMax::min()->initValue()) << endl;
//    cout << curNet->Name() << " netSlack: " << netSlack << " crit: " << criticality;

    // get normalized resistor
    float netRes = GetMaxResistor(_sta, curStaNet);
    float normRes = (netRes - minRes)/(maxRes - minRes);

    int netDegree = max(2, netInstance[i].pinCNTinObject);
    float netWeight = 1 + normRes * (1 + criticality) / (netDegree - 1);


    // TODO
    // following two lines are temporal magic codes at this moment.
    // Need to be replaced/tuned later
    netWeight = (netWeight >= 1.9)? 1.9 : netWeight;
    netWeight = (netSlack < 0)? 1.8 : 1;

//    cout << " normRes: " << normRes << " deg: " << netDegree 
//      << " nw: " << netWeight << endl;

    // update timingWeight 
    netInstance[i].timingWeight = netWeight;

    // update netWeightMin / netWeightMax    
    netWeightMin = (netWeightMin < netWeight) ? netWeightMin : netWeight;
    netWeightMax = (netWeightMax > netWeight) ? netWeightMax : netWeight;
  }
}

}

// static void ExecuteCommand( const char* inp ){
//    return system(inp);
//}

// static std::string ExecuteCommand(const char* cmd) {
//   cout << "COMMAND: " << cmd << endl;
//   std::array< char, 128 > buffer;
//   std::string result;
//   std::shared_ptr< FILE > pipe(popen(cmd, "r"), pclose);
//   if(!pipe)
//     throw std::runtime_error("popen() failed!");
//   while(!feof(pipe.get())) {
//     if(fgets(buffer.data(), 128, pipe.get()) != nullptr)
//       result += buffer.data();
//   }
//   return result;
// }

static float 
GetMaxResistor(sta::Sta* sta, sta::Net* net) {
  float retRes = 0.0f;

  const MinMax* cnst_min_max = MinMaxAll::max()->asMinMax();
  ParasiticAnalysisPt* ap =
      sta->corners()->findCorner(0)->findParasiticAnalysisPt(cnst_min_max);

  // find parasitic object from Net
  Parasitics* parasitics = sta->parasitics();
  Parasitic* parasitic = parasitics->findParasiticNetwork(net, ap);

  // Resistor Traverse from Net
  ParasiticDeviceIterator* paraDevIter = parasitics->deviceIterator(parasitic);

  while(paraDevIter->hasNext()) {
    ParasiticDevice* paraDev = paraDevIter->next();
    if(!parasitics->isResistor(paraDev)) {
      continue;
    }

    float newRes = parasitics->value(paraDev, ap);
    retRes = (retRes < newRes) ? newRes : retRes;
  }
  return retRes;
}

//static const char *
//escapeDividers(const char *token,
//             const sta::Network *network)
//{
//  return sta::escapeChars(token, network->pathDivider(), '\0',
//               network->pathEscape());
//}


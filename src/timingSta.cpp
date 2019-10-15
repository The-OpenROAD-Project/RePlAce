#include "timing.h"
#include "timingSta.h"

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <chrono>

#include <tcl.h>

using std::to_string;

TIMING_NAMESPACE_OPEN

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
    return name + ":" + pinPrefix + to_string(curPin->pinIDinModule);
  }
  // other LEF/DEF/VERILOG cases.
  else {
    return (curPin->term == 0)
               ? name + ":" + _mPinName[curPin->moduleID][curPin->pinIDinModule]
               : name + ":" +
                     _tPinName[curPin->moduleID][curPin->pinIDinModule];
  }
}

inline string Timing::GetPinName(PinInfo& curPin, bool isEscape) {
  if(curPin.isSteiner()) {
    return curPin.GetStnPinName(isEscape);
  }

  return (curPin.isModule()) ? curPin.GetPinName((void*)_modules, _mPinName, isEscape)
                             : curPin.GetPinName((void*)_terms, _tPinName, isEscape);
}

float GetMaxResistor(sta::Sta* sta, Pin* pin) {
  float retRes = 0.0f;

  const MinMax* cnst_min_max = MinMaxAll::max()->asMinMax();
  ParasiticAnalysisPt* ap =
      sta->corners()->findCorner(0)->findParasiticAnalysisPt(cnst_min_max);

  Parasitics* parasitics = sta->parasitics();

  Parasitic* parasitic = parasitics->findParasiticNetwork(pin, ap);
  ParasiticNode* paraNode = parasitics->findNode(parasitic, pin);

  if(!paraNode) {
    return 0.0f;
  }
  // Resistor Traverse
  ParasiticDeviceIterator* paraDevIter = parasitics->deviceIterator(paraNode);

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
        Pin* curPin = connPinIter->next();
        // PortDirection* dir = sta->network()->direction(curPin);
        // if( dir->isInput() ) {
        //   continue;
        // }
        cout << sta->network()->name(curPin) << " "
             << GetMaxResistor(sta, curPin) << endl;
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

  // network = _sta->networkReader();
  // Instance* top_inst = network->topInstance();
  // cout << top_inst << endl;
  // NetSeq* nets = new NetSeq;
  // PatternMatch pattern(string("*").c_str());
  // cout << &pattern << endl;
  // network->findNetsHierMatching(top_inst, &pattern, nets);
  //
  // NetSeq::Iterator netIter(nets);
  // while(netIter.hasNext()) {
  //     Net* net = netIter.next();
  //     string netName = network->pathName(net);
  //     cout << netName << endl;
  // }

  // exit(1);

  // read_parasitics
  //    cout << spefFileName << endl;
  /*
  bool parasitics =
      _sta->readParasitics("/home/mgwoo/RePlACE-TD/output/etc/simple/experiment7/simple.spef",
              _sta->currentInstance(),
              MinMaxAll::max(), false, true, 0.0,
              reduce_parasitics_to_pi_elmore, false, true, true);

              */
  // from search/Sta.cc:readParasitics
  FillSpefForSta();

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

  // exit(0);
}

void Timing::ExecuteStaLater() {
  for(int i = 0; i < _netCnt; i++) {
    _nets[i].timingWeight = 0;
  }
  // _sta->parasitics()->deleteParasitics();
  // _sta->network()->clear();

  auto start = std::chrono::steady_clock::now();
  // do something
  FillSpefForSta();

  auto finish = std::chrono::steady_clock::now();
  double elapsed_seconds =
      std::chrono::duration_cast< std::chrono::duration< double > >(finish -
                                                                    start)
          .count();

  PrintInfoRuntime("Timing: FillSpefForSta", elapsed_seconds, 1);

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

void Timing::FillSpefForSta() {
  const Corner* corner = _sta->corners()->findCorner(0);
  const MinMax* cnst_min_max;
  ParasiticAnalysisPt* ap;

  // assume always MinMaxAll::max()
  _sta->corners()->makeParasiticAnalysisPtsMinMax();
  cnst_min_max = MinMaxAll::max()->asMinMax();
  ap = corner->findParasiticAnalysisPt(cnst_min_max);

  // cout << ap->index() << endl;

  const OperatingConditions* op_cond =
      _sta->sdc()->operatingConditions(cnst_min_max);

  SpefReader* reader = new SpefReader(
      "", NULL, _sta->currentInstance(), ap, false, false, false, 0.0,
      ReduceParasiticsTo::pi_elmore, false, op_cond, corner, cnst_min_max,
      true, _sta->report(), _sta->network(), _sta->parasitics());
  sta::spef_reader = reader;

  // spef parsing manually
  // by default
  sta::spef_reader->setDivider('/');
  sta::spef_reader->setDelimiter(':');
  sta::spef_reader->setBusBrackets('[', ']');

  char* timeStr = new char[3];
  char* resStr = new char[5];
  char* inductStr = new char[3];
  char* capStr = new char[3];

  strcpy(timeStr, "PS");
  strcpy(capStr, "PF");
  strcpy(resStr, "OHM");
  strcpy(inductStr, "UH");

  sta::spef_reader->setTimeScale(1, timeStr);
  sta::spef_reader->setCapScale(1, capStr);
  sta::spef_reader->setResScale(1, resStr);
  sta::spef_reader->setInductScale(1, inductStr);

  HASH_MAP< PinInfo, bool, MyHash< PinInfo > > pin_cap_written;
  // map from pin name -> cap
  HASH_MAP< PinInfo, double, MyHash< PinInfo > > lumped_cap_at_pin;

  PinInfo tmpPin;
  tmpPin.SetImpossible();

#ifdef USE_GOOGLE_HASH
  pin_cap_written.set_empty_key(tmpPin);
  lumped_cap_at_pin.set_empty_key(tmpPin);
#endif

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
        // it was must be read from SDC file,
        // but in ICCAD contest, only outPin have PIN_CAP
        // as 4e-15
        lumpedCapStor[i] += TIMING_PIN_CAP;
        lumped_cap_at_pin[PinInfo(curPin)] += TIMING_PIN_CAP;
      }
    }
  }

  bool isEscape = true;
  for(int i=0; i<_netCnt; i++) {
    char* tmpStr = GetEscapedStr(netInstance[i].Name(), isEscape);
    sta::Net* net = sta::spef_reader->findNet(tmpStr);
    if( !net ) {
      isEscape = false;
      break;
    }
  }
//  isEscape = false;

  for(int i = 0; i < _netCnt; i++) {
    NET* curNet = &_nets[i];
    // char* netName = new char[strlen(curNet->name)+1];
    // strcpy(netName, curNet->name);

    char* tmpStr = GetEscapedStr(curNet->Name(), isEscape);
//    cout << "find: " << tmpStr << endl;
    sta::Net* net = sta::spef_reader->findNet(tmpStr);
    if( !net ) {
      cout << "ERROR  : Net " << tmpStr << " is not found in Verilog" << endl;
      cout << "Verilog is mismatched with DEF files" << endl;
      exit(1);
    }
    // cout << "SPEF: " << tmpStr << endl;
    free(tmpStr);

    SpefTriple* netCap = new SpefTriple(lumpedCapStor[i] / CAP_SCALE);
    sta::spef_reader->dspfBegin(net, netCap);

    for(auto& curSeg : wireSegStor[i]) {
      if(!pin_cap_written[curSeg.iPin]) {
        // feed << cnt++ << " " << GetPinName(curSeg.iPin)
        // << " " << lumped_cap_at_pin[ curSeg.iPin ] / CAP_SCALE<<endl;
        string pinName = GetPinName(curSeg.iPin, isEscape);
        //char* pinNamePtr = GetEscapedStr( pinName.c_str(), isEscape );
        char* pinNamePtr = GetNewStr(pinName.c_str());

        SpefTriple* pinCap =
            new SpefTriple(lumped_cap_at_pin[curSeg.iPin] / CAP_SCALE);

//        cout << "PinPTR: " << pinNamePtr << endl;
        sta::spef_reader->makeCapacitor(INT_MAX, pinNamePtr, pinCap);
        pin_cap_written[curSeg.iPin] = true;
      }
      if(!pin_cap_written[curSeg.oPin]) {
        // feed << cnt++ << " " << GetPinName(curSeg.oPin)
        // << " " << lumped_cap_at_pin[ curSeg.oPin ] / CAP_SCALE<<endl;

        string pinName = GetPinName(curSeg.oPin, isEscape);
        //char* pinNamePtr = GetEscapedStr( pinName.c_str(), isEscape );
        char* pinNamePtr = GetNewStr(pinName.c_str());

        SpefTriple* pinCap =
            new SpefTriple(lumped_cap_at_pin[curSeg.oPin] / CAP_SCALE);

//        cout << "PinPTR: " << pinNamePtr << endl;
        sta::spef_reader->makeCapacitor(INT_MAX, pinNamePtr, pinCap);
        pin_cap_written[curSeg.oPin] = true;
      }
    }
    unsigned cnt = 1;
    for(auto& curSeg : wireSegStor[i]) {
      // feed << cnt++ << " " << GetPinName(curSeg.iPin) << " "
      //   << GetPinName(curSeg.oPin) << " "
      //   << curSeg.length / static_cast<double>(_l2d)
      //   * LOCAL_WIRE_RES_PER_MICRON / RES_SCALE << endl;

      string pinName1 = GetPinName(curSeg.iPin, isEscape);
      // char* pinNamePtr1 = GetEscapedStr( pinName1.c_str(), isEscape );
      char* pinNamePtr1 = GetNewStr(pinName1.c_str());
//      cout << "PinPTR2: " << pinNamePtr1 << endl;

      string pinName2 = GetPinName(curSeg.oPin, isEscape);
      // char* pinNamePtr2 = GetEscapedStr( pinName2.c_str(), isEscape );
      char* pinNamePtr2 = GetNewStr(pinName2.c_str());
//      cout << "PinPTR2: " << pinNamePtr2 << endl;
      SpefTriple* pinRes =
          new SpefTriple(curSeg.length / static_cast< double >(_l2d) *
                         resPerMicron / RES_SCALE);

      // cout << pinName1 << " " << pinName2 << " " <<
      // pinRes->value(0) << endl;
      sta::spef_reader->makeResistor(cnt++, pinNamePtr1, pinNamePtr2, pinRes);
    }
    sta::spef_reader->dspfFinish();
  }

  UpdateSpefClockNetVerilog();
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
  // report_checks -path_delay min_max
  Corner* corner = _sta->corners()->findCorner(0);

  PathEndSeq* ends =
      _sta->findPathEnds(NULL, NULL, NULL,          // from, thru, to
                        false,                      // unconstrained
                         corner, MinMaxAll::max(),  // corner, delay_min_max
                         INT_MAX, 1, false,  // max_paths, nworst, unique_pins
                         -INF, INF,          // slack_min, slack_max
                         true, NULL,         // sort_by_slack, group_name
                         true, true,         // setup, hold
                         true, true,         // recovery, removal
                         true, true);        // clk gating setup, hold

  if(ends->empty()) {
    cout << "ERROR: There is no valid timing path. " << endl;
    cout << "       Please double check your SDC and Design" << endl;
    cout << "       or, try to use non Timing-Driven mode" << endl;
    exit(1);
  }

  PathEndSeq::Iterator tmpIter(ends), pathEndIter(ends);
  int resultCnt = 0;
  while(tmpIter.hasNext()) {
    tmpIter.next();
    resultCnt++;
  }

  //    int limintCnt = resultCnt * netCut;
  int limintCnt = resultCnt;

  PrintInfoInt("Timing: NumPaths", resultCnt, 1);

  //    _sta->setReportPathOptions(report_path_full, true, true, true, true,
  //    true, 2);
  //    _sta->reportPathEnds(ends) ;

  PathEnd* end = pathEndIter.next();

  // vectex iterator and index check
  /*
  int i=1;
  VertexIterator vIter(_sta->graph());
  while(vIter.hasNext()) {
      Vertex * vertex = vIter.next();
      Pin* pin = vertex->pin();
      if( i != _sta->network()->vertexIndex(pin)) {
          cout << vertex->name(_sta->network()) << " " << i << " "
              << _sta->network()->vertexIndex(pin) << endl;
      }
      i++;
  }*/

  const MinMax* cnst_min_max = MinMax::max();

  // boundary value
  netWeightMin = FLT_MAX;
  netWeightMax = FLT_MIN;

  HASH_SET<sta::Net*> netSet;
#ifdef USE_GOOGLE_HASH
  netSet.set_empty_key(NULL);
#endif

  // Reported path check based on reportPath5 function (search/ReportPath.cc)
  for(int i = 0; i < limintCnt; i++) {
//    cout << "pathName: " << end->path()->name(_sta) << endl;
//    if( i >= 46748 ) {
//      TimingPathPrint( _sta, end );
//    }
    
    if(i % 10000 == 0 ) {
      PrintProc("Timing: Critical path " + to_string(i) + " has been updated", 1);
    }
    PathExpanded expanded(end->path(), _sta);

    int pinCnt = 0;

    for(size_t j = 0; j < expanded.size(); j++) {
      PathRef* path1 = expanded.path(j);
      // TimingArc *prevArc = expanded.prevArc(j);
      Vertex* vertex = path1->vertex(_sta);
      Pin* pin = vertex->pin();
      // Arrival time = path1->arrival(_sta) + timeOffset;
      // Arrival incr(0.0);

      bool isClk = path1->isClock(_sta->search());

      // if( prevArc == NULL) {
      //   cout << "is first node" << endl;
      // }

      if(isClk) {
        continue;
      }

      PortDirection* dir = _sta->network()->direction(pin);
      // only output pins..!!!
      if(dir->isInput()) {
        continue;
      }

      if(_sta->network()->isTopLevelPort(pin)) {
        continue;
      }

      Net* net = _sta->network()->net(pin);
      if(!net) {
        continue;
      }

      Net* higestNet = _sta->network()->highestNetAbove(net);
      // skip for already pushed Nets
      if( netSet.find(higestNet) != netSet.end() ) {
        continue;
      }
      string netName = string(_sta->cmdNetwork()->pathName(higestNet));
      

      auto nnPtr = netNameMap.find(netName);

      if(nnPtr == netNameMap.end()) {
        cout << "**ERROR : Cannot Find net: " << netName
             << " (netNameMap/timingSta)" << endl;
        // exit(0);
      }
      else {
        // cout << nnPtr->second << endl;

        // already calculated net is just passed!!
//        if(abs(netInstance[nnPtr->second].timingWeight) >
//           std::numeric_limits< prec >::epsilon()) {
//          continue;
//        }

        NetConnectedPinIterator* connPinIter =
            _sta->network()->connectedPinIterator(higestNet);

        // extract maximum resistance for road Resistor
        float highRes = 0.0f;
        while(connPinIter->hasNext()) {
          Pin* curPin = connPinIter->next();
          // cout << sta->network()->name(curPin) << " "
          //   << GetMaxResistor(sta, curPin) << endl;
          float curRes = GetMaxResistor(_sta, curPin);
          highRes = (highRes < curRes) ? curRes : highRes;
          pinCnt ++;
        }

        // cout << "high Resistor: " << highRes << endl;
        Slack wns; 
        Vertex *worstVertex;
        _sta->worstSlack(cnst_min_max, wns, worstVertex);

        float criticality =
            max(0.0f, end->slack(_sta) / wns);
        int netDegree = max(2, netInstance[nnPtr->second].pinCNTinObject);
        float netWeight = highRes * (1 + criticality) / (netDegree - 1);
        netInstance[nnPtr->second].timingWeight = netWeight;
        netWeightMin = (netWeightMin < netWeight) ? netWeightMin : netWeight;
        netWeightMax = (netWeightMax > netWeight) ? netWeightMax : netWeight;
        
        netSet.insert( higestNet );  

        // cout << "netdeg: " << netDegree << endl;
        // cout << "crit: " << criticality<< endl;
        // cout << "netWeight: " << netWeight << endl;
      }
    }

    // cout << endl;

    if(!pathEndIter.hasNext()) {
      break;
    }
    end = pathEndIter.next();
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

TIMING_NAMESPACE_CLOSE

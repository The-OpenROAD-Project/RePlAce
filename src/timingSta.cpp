#include "timing.h"
#include "timingSta.h"

#include <cstdlib>
#include <memory>
#include <stdexcept>

#include <tcl.h>

TIMING_NAMESPACE_OPEN

inline string Timing::GetPinName(PIN* curPin) {
    // itself is PINS in def.
    if( curPin->term && _terms[curPin->moduleID].isTerminalNI ) {
        return string(_terms[curPin->moduleID].name);
    }

    // below is common
    string name = (curPin->term)? 
        string(_terms[curPin->moduleID].name) : string(_modules[curPin->moduleID].name);

    // bookshelf cases, it must be empty
    if( _mPinName.size() == 0 && _tPinName.size() == 0) {
        string pinPrefix = (curPin->IO == 0)? "I" : "O";
        return name + ":" + pinPrefix + to_string(curPin->pinIDinModule);
    } 
    // other LEF/DEF/VERILOG cases.
    else {
        return (curPin->term == 0)? 
            name + ":" + _mPinName[curPin->moduleID][curPin->pinIDinModule]
            : name + ":" + _tPinName[curPin->moduleID][curPin->pinIDinModule];
    }
}

inline string Timing::GetPinName(PinInfo & curPin) {
    if( curPin.isSteiner() ) {
        return curPin.GetStnPinName();
    } 

    return (curPin.isModule())? 
        curPin.GetPinName( (void*)_modules, _mPinName ) :
        curPin.GetPinName( (void*)_terms, _tPinName );
}

float GetMaxResistor( sta::Sta* sta, Pin* pin ) {
    float retRes = 0.0f;

    const MinMax* cnst_min_max = MinMaxAll::max()->asMinMax();
    ParasiticAnalysisPt* ap = sta->corners()->defaultCorner()
        ->findParasiticAnalysisPt(cnst_min_max);
    
    Parasitics* parasitics = sta->parasitics();

    Parasitic* parasitic = parasitics->findParasiticNetwork(pin, ap);
    ParasiticNode* paraNode = parasitics->findNode(parasitic, pin);

    if( !paraNode ) {
//        const char* pinName = sta->cmdNetwork()->pathName(pin);
//        cout << pinName << endl;
//        exit(1);
        return 0.0f;
    }
    // Resistor Traverse
    ParasiticDeviceIterator* paraDevIter 
        = parasitics->deviceIterator(paraNode);

    while( paraDevIter-> hasNext() ) {
        ParasiticDevice* paraDev = paraDevIter->next();
        if( !parasitics->isResistor(paraDev)) {
            continue;
        }
    
        float newRes = parasitics->value( paraDev, ap);
        retRes = (retRes < newRes)? newRes : retRes;
//        cout << "R: " << newRes << endl; 
//        paraDev = paraDevIter->next();
    }
    return retRes;
}

void TimingPathPrint( sta::Sta* sta, sta::PathEnd* end ) {

    cout << "===========================================================" << endl;
    cout << "pathName: " << end->path()->name(sta) << endl;
    cout << "pathslack: " << end->slack(sta) << " seconds" << endl;

    PathExpanded expanded(end->path(), sta);
//    cout << "iterSize: " << expanded.size() << endl;
    float timeOffset = 0.0f;



    for(int i=0; i<expanded.size(); i++) {
        PathRef *path1 = expanded.path(i);
        TimingArc *prevArc = expanded.prevArc(i);
        Vertex *vertex = path1->vertex(sta);
        Pin *pin = vertex->pin();
        Arrival time = path1->arrival(sta) + timeOffset; 
        Arrival incr(0.0);

        bool isClkStart = sta->network()->isRegClkPin(pin);
        bool isClk = path1->isClock(sta->search());
        
//        if( prevArc == NULL) { 
//            cout << "is first node" << endl;
//        }
    
        if( isClk ) {
            continue;
        }
        cout << ((isClk)? "C " : "UC ");

        Net* pinNet = NULL;
        if( sta->network()->isTopLevelPort(pin)) {
            const char* pinName = sta->cmdNetwork()->pathName(pin);
            cout << "(TL:" << pinName << ") ";
        }
        else {
            Net* net = sta->network()->net(pin);
            if (net) {
                Net *higestNet = sta->network()->highestNetAbove(net);
                const char* netName = sta->cmdNetwork() ->pathName(higestNet);
                cout << "(" << netName << ") ";
                pinNet = higestNet;
            }
        }
        cout << sta->network()->pathName(sta->network()->instance(pin)) << " " 
            << sta->network()->portName(pin) << " ("
            << sta->network()->vertexIndex(pin) << ")" <<  endl;
//        cout << vertex->name(sta->network()) << endl;

        PortDirection* dir = sta->network()->direction(pin);
        // only output pins..!!!
        if(dir->isInput()) {
            continue;
        }

        if( pinNet ) {
            NetConnectedPinIterator* connPinIter 
                = sta->network()->connectedPinIterator(pinNet);
           
            while( connPinIter->hasNext() ) { 
                Pin* curPin = connPinIter->next();
                PortDirection* dir = sta->network()->direction(curPin);
//                if( dir->isInput() ) {
//                    continue;
//                }
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

void Timing::ExecuteStaFirst(string topCellName,
        string verilogName, vector<string>& libStor, string ffLibName) {

    cout << "Execute STA" << endl;
    cout << "topCellName: " << topCellName << endl;
    cout << "verilog    : " << verilogName << endl;
    for(auto& libName : libStor) {
        cout << "liberty    : " << libName << endl;
    }
    cout << "ffLiberty  : " << ffLibName << endl << endl;


    // STA object create
    _sta = new Sta;

    // Tcl Interpreter settings 
//    Tcl_FindExecutable(argv[0]);
    _interp = Tcl_CreateInterp();

    // define swig commands
    Sta_Init(_interp);

    // load encoded TCL functions
    evalTclInit(_interp, tcl_inits);
    // initialize TCL commands
    Tcl_Eval(_interp, "sta::show_splash");
//    Tcl_Eval(_interp, "clear");
    Tcl_Eval(_interp, "sta::define_sta_cmds");
    Tcl_Eval(_interp, "namespace import sta::*");
    
    // initialize STA objects
    initSta();
    Sta::setSta(_sta);
    _sta->makeComponents();
    _sta->setTclInterp(_interp);
    _sta->setThreadCount(numThread);
   
    // environment settings
//    string dirPos = "./design/" + topCellName;      // *.v, *.lib
//    string outPos = dirPos;                         // *.spef

//    string verilogFileName = dirPos + "/" + topCellName + ".v";
//    string sdcFileName = dirPos + "/" + topCellName + ".sdc";
//    string spefFileName = outPos + "/" + topCellName + ".spef";

//    string libName= dirPos + "/" + topCellName +"_Late.lib";
//    string ffLibName= dirPos + "/" + topCellName + "_Early.lib";

//    string clk_name="clk";
    
    string cornerName="wst";
//    string cornerNameFF="bst";

    StringSet cornerNameSet;
    cornerNameSet.insert(cornerName.c_str());
//    cornerNameSet.insert(cornerNameFF.c_str());

    //define_corners
    _sta->makeCorners(&cornerNameSet);
    Corner *corner = _sta->findCorner(cornerName.c_str());
//    Corner *corner_FF = _sta->findCorner(cornerNameFF.c_str());
    
    
    //read_liberty
    for(auto& libName : libStor) {
        _sta->readLiberty(libName.c_str(), corner, MinMaxAll::max(), false);
    }

//    LibertyLibrary *FFlib1 = 
//        _sta->readLiberty(ffLibName.c_str(), corner_FF, MinMaxAll::min(), false);

    //read_netlist
    NetworkReader *network = _sta->networkReader();
    bool readVerilog = false;
    if (network) {
        _sta->readNetlistBefore();       
        readVerilog = 
            readVerilogFile(verilogName.c_str(), _sta->report(), 
                    _sta->debug(), _sta->networkReader());
    }

    //link_design
    cout << "Now linking; " << topCellName << endl;
    bool link = _sta->linkDesign(topCellName.c_str());

    bool is_linked = network->isLinked();
    if(is_linked) {
        cout << "linked: " << network->cellName(_sta->currentInstance()) << endl;
    }
    
//    network = _sta->networkReader();
//    Instance* top_inst = network->topInstance();
//    cout << top_inst << endl;
//    NetSeq* nets = new NetSeq;
//    PatternMatch pattern(string("*").c_str());
//    cout << &pattern << endl;
//    network->findNetsHierMatching(top_inst, &pattern, nets);
//
//    NetSeq::Iterator netIter(nets);
//    while(netIter.hasNext()) {
//        Net* net = netIter.next();
//        string netName = network->pathName(net);
//        cout << netName << endl;
//    }
    
//    exit(1);

    //read_parasitics
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
    
    GenerateClockSta();

    //find_timing -full_update (true->full, false-> incremental)
    UpdateTimingSta();

    UpdateNetWeightSta();

    
    //WNS / TNS report
    const MinMax *cnst_min_max;
    cnst_min_max = MinMax::max();
    Slack wns = _sta->worstSlack(cnst_min_max);
    Slack tns = _sta->totalNegativeSlack(cnst_min_max);
    cout << "timing summary" << endl;
    cout << "WNS = " << wns << " seconds" << endl;
    cout << "TNS = " << tns << " seconds" << endl;

    float tol=0.0;
    _sta->setIncrementalDelayTolerance(tol);
    Tcl_DeleteInterp(_interp);

//    _sta->clear();
//    _sta = NULL;
//    delete _sta;
    
//    exit(0);
}

void Timing::ExecuteStaLater() {
    for(int i=0; i<_netCnt; i++) {
        _nets[i].timingWeight = 0;
    }
//    _sta->parasitics()->deleteParasitics();
//    _sta->network()->clear();

    FillSpefForSta();
    GenerateClockSta();
    UpdateTimingSta();
    UpdateNetWeightSta();
    
    //WNS / TNS report
    const MinMax *cnst_min_max;
    cnst_min_max = MinMax::max();
    Slack wns = _sta->worstSlack(cnst_min_max);
    Slack tns = _sta->totalNegativeSlack(cnst_min_max);
    cout << "timing summary" << endl;
    cout << "WNS = " << wns << " seconds" << endl;
    cout << "TNS = " << tns << " seconds" << endl;
}

void Timing::FillSpefForSta() {
    Corner* corner = _sta->corners()->defaultCorner();
    const MinMax *cnst_min_max;
    ParasiticAnalysisPt* ap;

    // assume always MinMaxAll::max()
    _sta->corners()->makeParasiticAnalysisPtsMinMax();
    cnst_min_max = MinMaxAll::max()->asMinMax();
    ap = corner->findParasiticAnalysisPt(cnst_min_max);

    // cout << ap->index() << endl;

    const OperatingConditions *op_cond = _sta->sdc()->operatingConditions(cnst_min_max);

    SpefReader* reader = new SpefReader("", NULL, 0, _sta->currentInstance(), ap,
            false, false, 0.0, reduce_parasitics_to_pi_elmore, 
            false, op_cond, corner, cnst_min_max, 
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

    sta::spef_reader->setTimeScale(1, timeStr );
    sta::spef_reader->setCapScale(1, capStr);
    sta::spef_reader->setResScale(1, resStr);
    sta::spef_reader->setInductScale(1, inductStr);

    dense_hash_map<PinInfo, bool> pin_cap_written;
    // map from pin name -> cap
    dense_hash_map<PinInfo, double> lumped_cap_at_pin;              

    PinInfo tmpPin;
    tmpPin.SetImpossible();

    pin_cap_written.set_empty_key(tmpPin);
    lumped_cap_at_pin.set_empty_key(tmpPin);

    // 1. calc. lump sum caps from wire segments (PI2-model) + load caps
    for(int i=0; i<_netCnt; i++) {
        double lumpedCap = 0.0f; 

        int cnt=0;
        for(auto& curWireSeg : wireSegStor[i]) {
            lumpedCapStor[i] += curWireSeg.length / (double)(_l2d) 
                * LOCAL_WIRE_CAP_PER_MICRON;
            lumped_cap_at_pin [ curWireSeg.iPin ] += curWireSeg.length
                / (double)(_l2d) * LOCAL_WIRE_CAP_PER_MICRON * 0.5;
            lumped_cap_at_pin [ curWireSeg.oPin ] += curWireSeg.length
                / (double)(_l2d) * LOCAL_WIRE_CAP_PER_MICRON * 0.5;
            pin_cap_written [ curWireSeg.iPin ] = false;
            pin_cap_written [ curWireSeg.oPin ] = false;
        }

        for(int j=0; j<_nets[i].pinCNTinObject; j++) {

            PIN* curPin = _nets[i].pin[j];
            if( curPin->term && _terms[curPin->moduleID].isTerminalNI && curPin->IO == 1 ) {
                // it was must be read from SDC file, 
                // but in ICCAD contest, only outPin have PIN_CAP
                // as 4e-15
                lumpedCapStor[i] += TIMING_PIN_CAP;
                lumped_cap_at_pin [ PinInfo(curPin) ] += TIMING_PIN_CAP;
            }
        }
    }

    for(int i=0; i<_netCnt; i++) {
        NET* curNet = &_nets[i];
        //            char* netName = new char[strlen(curNet->name)+1];
        //            strcpy(netName, curNet->name);

        sta::Net* net = sta::spef_reader->findNet(curNet->name);
        SpefTriple* netCap = new SpefTriple ( lumpedCapStor[i] / CAP_SCALE );
        sta::spef_reader->dspfBegin( net , netCap );

        for(auto& curSeg : wireSegStor[i] ) {
            if(!pin_cap_written[ curSeg.iPin ]) {
                // feed << cnt++ << " " << GetPinName(curSeg.iPin)
                // << " " << lumped_cap_at_pin[ curSeg.iPin ] / CAP_SCALE<<endl;
                string pinName = GetPinName(curSeg.iPin);
                char* pinNamePtr = new char[pinName.length()+1];
                strcpy( pinNamePtr, pinName.c_str() );

                SpefTriple* pinCap = 
                    new SpefTriple( lumped_cap_at_pin[ curSeg.iPin ]/CAP_SCALE); 

                sta::spef_reader->makeCapacitor(INT_MAX, pinNamePtr, pinCap ); 

                pin_cap_written[ curSeg.iPin ]=true;
            }
            if(!pin_cap_written[ curSeg.oPin ]) {
                // feed << cnt++ << " " << GetPinName(curSeg.oPin)
                // << " " << lumped_cap_at_pin[ curSeg.oPin ] / CAP_SCALE<<endl;

                string pinName = GetPinName(curSeg.oPin);
                char* pinNamePtr = new char[pinName.length()+1];
                strcpy( pinNamePtr, pinName.c_str() );

                SpefTriple* pinCap = 
                    new SpefTriple( lumped_cap_at_pin[ curSeg.oPin ]/CAP_SCALE); 

                sta::spef_reader->makeCapacitor(INT_MAX, pinNamePtr, pinCap ); 

                pin_cap_written[ curSeg.oPin ]=true;
            }
        }
        unsigned cnt = 1;
        for(auto& curSeg : wireSegStor[i]) {            
            // feed << cnt++ << " " << GetPinName(curSeg.iPin) << " " 
            //   << GetPinName(curSeg.oPin) << " " 
            //   << curSeg.length / static_cast<double>(_l2d) 
            //   * LOCAL_WIRE_RES_PER_MICRON / RES_SCALE << endl;

            string pinName1 = GetPinName(curSeg.iPin);
            char* pinNamePtr1 = new char[pinName1.length()+1];
            strcpy( pinNamePtr1, pinName1.c_str() );

            string pinName2 = GetPinName(curSeg.oPin);
            char* pinNamePtr2 = new char[pinName2.length()+1];
            strcpy( pinNamePtr2, pinName2.c_str() );

            SpefTriple* pinRes = 
                new SpefTriple( curSeg.length / static_cast<double>(_l2d)
                        * LOCAL_WIRE_RES_PER_MICRON / RES_SCALE );

//            cout << pinName1 << " " << pinName2 << " " << pinRes->value(0) << endl;
            sta::spef_reader->makeResistor(cnt++, pinNamePtr1, pinNamePtr2, pinRes );
        }
        sta::spef_reader->dspfFinish();
    }

    UpdateSpefClockNetVerilog(); 
    parasiticsChangedAfter(_sta);
}
void Timing::GenerateClockSta() {
    //sdc -> clock definition (unit=second)
//    float clk_period = 70e-9;
    FloatSeq *waveform = new FloatSeq;
    waveform->push_back(0.0);
    waveform->push_back(_clkPeriod/2);

    PinSet *pins = new PinSet;
    Pin *pin = _sta->network()->findPin(_sta->currentInstance(),_clkName.c_str());
    pins->insert(pin);
    
    _sta->makeClock(_clkName.c_str(),pins,true,_clkPeriod,waveform, NULL);
    
//    Clock *clock_found =
    _sta->findClock(_clkName.c_str());
    
    //write_sdc
//    _sta->writeSdc("test.sdc",false,false,5);
}

void Timing::UpdateTimingSta() {
    _sta->updateTiming(true);
}



void Timing::UpdateNetWeightSta() {
    //report_checks -path_delay min_max
    Corner* corner = _sta->corners()->defaultCorner();

    PathEndSeq *ends = _sta->findPathEnds(NULL, NULL, NULL, //from, thru, to
		corner, MinMaxAll::max(), // corner, delay_min_max
        INT_MAX, 1, false, //max_paths, nworst, unique_pins
        -INF, INF, //slack_min, slack_max
        true, NULL, //sort_by_slack, group_name
        true, true, // setup, hold
        true, true, // recovery, removal
        true, true); // clk gating setup, hold

    if (ends->empty()) cout << "NO PATH !" << endl;

    PathEndSeq::Iterator tmpIter(ends), pathEndIter(ends);
    int resultCnt = 0;
    while( tmpIter.hasNext() ) {
        tmpIter.next();
        resultCnt ++; 
    }

//    int limintCnt = resultCnt * netCut; 
    int limintCnt = resultCnt; 

    cout << "INFO: Total " << resultCnt << " worst path!" << endl;

//    _sta->setReportPathOptions(report_path_full, true, true, true, true, true, 2);
//    _sta->reportPathEnds(ends) ;


	PathEnd *end = pathEndIter.next();
   
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
    
    const MinMax *cnst_min_max = MinMax::max();

    // boundary value
    netWeightMin = FLT_MAX;
    netWeightMax = FLT_MIN;


    // Reported path check based on reportPath5 function (search/ReportPath.cc)
    for( int i=0; i<limintCnt; i++ ) {
//        cout << "!!" << i << endl;
//        TimingPathPrint( _sta, end );
        
        PathExpanded expanded(end->path(), _sta);
        for(int j=0; j<expanded.size(); j++) {
            PathRef *path1 = expanded.path(j);
//            TimingArc *prevArc = expanded.prevArc(j);
            Vertex *vertex = path1->vertex(_sta);
            Pin *pin = vertex->pin();
//            Arrival time = path1->arrival(_sta) + timeOffset; 
//            Arrival incr(0.0);

            bool isClk = path1->isClock(_sta->search());
            
    //        if( prevArc == NULL) { 
    //            cout << "is first node" << endl;
    //        }
        
            if( isClk ) {
                continue;
            }
        
            PortDirection* dir = _sta->network()->direction(pin);
            // only output pins..!!!
            if(dir->isInput()) {
                continue;
            }


            if( _sta->network()->isTopLevelPort(pin) ) {
                continue;
            }
            
            Net* net = _sta->network()->net(pin);
            if (!net) {
                continue;
            }
            
            Net *higestNet = _sta->network()->highestNetAbove(net);
            string netName = string(_sta->cmdNetwork() ->pathName(higestNet));
//            ReplaceStringInPlace( netName, "[", "\\[");
//            ReplaceStringInPlace( netName, "]", "\\]");


            auto nnPtr = netNameMap.find( netName );

            if( nnPtr == netNameMap.end() ) {
                cout << "**ERROR : Cannot Find net: " << netName 
                    << " (netNameMap/timingSta)"<< endl;
//                exit(0); 
            }
            else {
//                cout << nnPtr->second << endl;

                // already calculated net is just passed!!
                if( abs(netInstance[nnPtr->second].timingWeight) 
                        > std::numeric_limits<prec>::epsilon()) {
                    continue;
                }

                NetConnectedPinIterator* connPinIter 
                    = _sta->network()->connectedPinIterator(higestNet);

                // extract maximum resistance for road Resistor
                float highRes = 0.0f;
                while( connPinIter->hasNext() ) { 
                    Pin* curPin = connPinIter->next();
                    PortDirection* dir = _sta->network()->direction(curPin);
//                    cout << sta->network()->name(curPin) << " "
//                        << GetMaxResistor(sta, curPin) << endl;
                    float curRes = GetMaxResistor(_sta, curPin);
                    highRes = (highRes < curRes)? curRes : highRes;
                }

//                cout << "high Resistor: " << highRes << endl;
                float criticality = max( 0.0f, end->slack(_sta) /_sta->worstSlack(cnst_min_max) );
                int netDegree = max(2, netInstance[nnPtr->second].pinCNTinObject) ;
                float netWeight = highRes*(1+criticality) / (netDegree - 1);
                netInstance[nnPtr->second].timingWeight = netWeight;

                netWeightMin = (netWeightMin < netWeight)? netWeightMin : netWeight;
                netWeightMax = (netWeightMax > netWeight)? netWeightMax : netWeight;

//                cout << "netdeg: " << netDegree << endl;
//                cout << "crit: " << criticality<< endl;
//                cout << "netWeight: " << netWeight << endl;
            }
        }

//        cout << endl;
        
        if( !pathEndIter.hasNext() ) {
            break;
        }
        end = pathEndIter.next();
    }
//    cout << "netWeightMin: " << netWeightMin << endl;
//    cout << "netWeightMax: " << netWeightMax << endl;

//    for(auto& curName: netNameMap) {
//        cout << "Map: " << curName.first << endl;
//    }
}


//static void ExecuteCommand( const char* inp ){ 
//    return system(inp); 
//}

static std::string ExecuteCommand( const char* cmd ) {
    cout << "COMMAND: " << cmd << endl;
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

///////////////////////////////////////////////////////////////////////
//
// for Script usage calling for each iteration
// global variable : dir_bnd , gbch
//
/*
void Timing::ScriptExecuteSta(string topCellName,
        string verilogName, vector<string>& libStor, string ffLibName) {

    string iterName = string(dir_bnd) + "/"
                        + topCellName + "_" + to_string(scriptIterCnt);
    string spefName = iterName + ".spef";
    string tclName = iterName + ".tcl";
    string outName = iterName + ".out";
    string outCutName = iterName + ".outcut";
    string slackName = iterName + ".slk";
//    cout << "spefName " << spefName << endl; 
//    cout << "tclName " << spefName << endl; 

    WriteSpef( spefName ); 
    ScriptWriteTcl( tclName, topCellName, verilogName, 
            libStor, 
            spefName, outName, slackName);

    ExecuteCommand(
            string("../module/OpenSTA/bin/sta -f " + tclName).c_str());
    ExecuteCommand(
            string("cat " + outName + " | grep '(net)\\|Startpoint:' > " + outCutName).c_str());
    string pathCntStr = ExecuteCommand(
            string("cat " + outName + "cut | grep 'Startpoint:' | wc -l").c_str());

    int pathCnt = stoi(pathCntStr);
    int targetCnt = pathCnt * 0.03;
    cout << "Path CNT: " << pathCnt << ", and Target CNT: " << targetCnt << endl;
    
    ScriptUpdateSta( outCutName, targetCnt );

//    cout << netList << endl;

    scriptIterCnt++;
//    exit(0); 
}

// tcl writing function
void Timing::ScriptWriteTcl(string& tclName, 
        string& topCellName, string& verilogName, vector<string>& libStor,
        string& sdcName, string& spefName, string& outName,
        string& slackName) {

    ofstream tclFile( tclName );
    if( !tclFile.good() ) {
        cout << "** ERROR: cannot Open TCL file to write: " << tclName << endl;
        exit(1); 
    }
    
    stringstream feed;
    feed << "## Script generated by RePlAce-TD" << endl; 
    feed << "set list_lib \"";
    for(auto& curLib : libStor) {
        feed << curLib << "\\" << endl;
    }
    feed << "\""<< endl;
//    feed << "set log_begin \"opensta.log\"" << endl;
    feed << "set link_library $list_lib" << endl;
    feed << "set target_library $list_lib" << endl;
//    feed << "set netlist " << verilogName << endl;
    feed << "set sdc " << sdcName << endl;
    feed << "set spef " << spefName << endl;
    feed << "define_corners wst" << endl;
    feed << "foreach lib $list_lib {" << endl;
    feed << "\tread_liberty -corner wst $lib" << endl;
    feed << "}" << endl;
    feed << "read_verilog " << verilogName << endl;
    feed << "link_design " << topCellName << endl;
    feed << "current_design " << topCellName << endl;
    feed << "read_parasitics -keep_capacitive_coupling -max " << spefName << endl;
    feed << "read_sdc " << sdcName << endl;
    feed << "find_timing -full_update" << endl;
    feed << "report_checks -path_delay min_max -sort_by_slack -field net -group_count "
         << INT_MAX << " > " << outName <<  endl;
    feed << "report_check_types -format slack_only > " << slackName<<  endl;
    feed << "exit" << endl;
//    feed << "exit" << endl;

    tclFile << feed.str();
    tclFile.close();
    feed.clear();
}

void Timing::ScriptUpdateSta(string& outCutName, int targetCnt) {

    // clear all timingWeight in here
    for(int i=0; i<_netCnt; i++) {
        _nets[i].timingWeight = 0;
    }

    // file Iteration up to Path CNT
    string line;
    ifstream pathFile( outCutName );
    int iterCnt = 0;
    while(getline(pathFile, line)) {
        // when the string starts with this prefixes;
        if( line.rfind("Startpoint:", 0) == 0 ) {
            iterCnt ++;
            if( iterCnt >= targetCnt ) {
                break;
            }
        }
        else {
            auto netName = line.substr(0, line.find(' '));
            auto nnPtr = netNameMap.find( netName );
            if( nnPtr == netNameMap.end() ) {
                cout << "**ERROR: cannot find " << netName 
                    << " in netNameMap(ScriptUpdateSta)" << endl;
                exit(1);
            }
//            cout << "found Net Idx: " << nnPtr->second << endl; 

            _nets[nnPtr->second].timingWeight++;
        }
    }
    pathFile.close();
}

void Timing::ExecuteInnovus(string topCellName,
        string verilogName, vector<string>& libStor, string ffLibName) {
    
    Tcl_Interp *interp = Tcl_CreateInterp();

    int res = Tcl_Eval(interp, "set tcl_library \"/usr/share/tcl8.5\"");
    cout << res << endl;
    res = Tcl_Eval(interp, "source [file join $tcl_library init.tcl]");
    cout << res << endl;

    //    cout << res << endl;
    
    res = Tcl_Eval(interp, "exec /home/tool/cadence/INNOVUS171/tools/bin/innovus -nowin");
    cout << res << endl; 
    cout << Tcl_GetString( Tcl_GetObjResult(interp)) << endl;
    
    Tcl_Eval(interp, "exec /bin/ls");
    cout << Tcl_GetString( Tcl_GetObjResult(interp)) << endl;
    
    fflush(stdout);
}
*/

TIMING_NAMESPACE_CLOSE

void parasiticsChangedAfter(sta::Sta* sta_) {
    CornerIterator corner_iter(sta_);
    while( corner_iter.hasNext() ) {
        Corner* corner = corner_iter.next();
        MinMaxIterator mm_iter;
        while(mm_iter.hasNext()) {
            MinMax *min_max = mm_iter.next();
            ParasiticAnalysisPt *ap = corner->findParasiticAnalysisPt(min_max);
            DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(min_max);
            dcalc_ap->setParasiticAnalysisPt(ap);
        }
    }
    sta_->graphDelayCalc()->delaysInvalid();
    sta_->search()->arrivalsInvalid();
}


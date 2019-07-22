#include "global.h"
#include "timing.h"
#include <sstream>
#include <fstream>

using std::ofstream;
using std::stringstream;
using std::string;
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
    return curPin.GetStnPinName();
  }

  return (curPin.isModule()) ? curPin.GetPinName((void*)_modules, _mPinName, isEscape)
                             : curPin.GetPinName((void*)_terms, _tPinName, isEscape);
}

// stn stands for steiner
void Timing::BuildSteiner(bool scaleApplied) {
  CleanSteiner();
  using namespace Flute;
  Flute::readLUT("./POWV9.dat", "./PORT9.dat");

  uint64_t stnPointCnt = 0;
  long long int totalStnWL = 0;

  for(int i = 0; i < _netCnt; i++) {
    NET* curNet = &_nets[i];
    // below cases were exists.....
    if(curNet->pinCNTinObject <= 1) {
      continue;
    }
    // two pin nets
    if(curNet->pinCNTinObject == 2) {
      PIN* firPin = curNet->pin[0];
      PIN* secPin = curNet->pin[1];
      int wl = (!scaleApplied)
                   ? fabs(firPin->fp.x - secPin->fp.x) +
                         fabs(firPin->fp.y - secPin->fp.y)
                   : fabs(firPin->fp.x - secPin->fp.x) * _unitX +
                         fabs(firPin->fp.y - secPin->fp.y) * _unitY;
      //            cout << GetPinName(firPin) << " " <<  GetPinName(secPin) <<
      //            " " << wl << endl;
      wireSegStor[i].push_back(wire(PinInfo(firPin), PinInfo(secPin), wl));
      totalStnWL += wl;
      //            cout << wl << endl;
    }
    else {
      DBU* x = new DBU[curNet->pinCNTinObject];
      DBU* y = new DBU[curNet->pinCNTinObject];

      int* mapping = new int[curNet->pinCNTinObject];

      // x, y coordi --> pin's index
      HASH_MAP< pair< DBU, DBU >, PinInfo, MyHash< pair< DBU, DBU > > >
          pinMap;
#ifdef USE_GOOGLE_HASH
      pinMap.set_empty_key(make_pair(DBU_MAX, DBU_MAX));
#endif

      //            cout << "pinMapBuilding" << endl;
      for(int j = 0; j < curNet->pinCNTinObject; j++) {
        PIN* curPin = curNet->pin[j];
        x[j] = (!scaleApplied) ? (DBU)(curPin->fp.x + 0.5f)
                               : (DBU)(curPin->fp.x * _unitX + 0.5f);
        y[j] = (!scaleApplied) ? (DBU)(curPin->fp.y + 0.5f)
                               : (DBU)(curPin->fp.y * _unitY + 0.5f);

        //                cout << curPin->term << " - "
        //                    << ((curPin->term)? _terms[curPin->moduleID].name
        //                    :
        //                        _modules[curPin->moduleID].name )
        //                    << ":" << curPin->pinIDinModule << endl;

        pinMap[make_pair(x[j], y[j])] = PinInfo(curPin);
        //                cout << "pin: ";
        //                pinMap[ make_pair(x[j], y[j]) ].Print();
        //                cout << endl;

        //                cout << GetPinName(curPin) << "("<< x[j] << ", " <<
        //                y[j] << ") ";
      }
      //            cout << endl;

      Tree fluteTree =
          flute(curNet->pinCNTinObject, x, y, FLUTE_ACCURACY, mapping);
      //            for(int i=0; i<curNet->pinCNTinObject; i++) {
      //                cout << i << " " << mapping[i] << endl;
      //            }
      delete[] x;
      delete[] y;
      delete[] mapping;

      //            printtree(fluteTree);

      int branchNum = 2 * fluteTree.deg - 2;
      for(int j = 0; j < branchNum; j++) {
        int n = fluteTree.branch[j].n;
        if(j == n) {
          continue;
        }
        //                cout << j << "<->" <<  n << endl;

        int wl = fabs((int)fluteTree.branch[j].x - (int)fluteTree.branch[n].x) +
                 fabs((int)fluteTree.branch[j].y - (int)fluteTree.branch[n].y);
        totalStnWL += wl;

        PinInfo pin1, pin2;
        auto pinPtr1 = pinMap.find(
            make_pair(fluteTree.branch[j].x, fluteTree.branch[j].y));

        if(pinPtr1 == pinMap.end()) {
          stnPointCnt++;
          pin1.SetSteiner(stnPointCnt, i);
          pinMap[make_pair(fluteTree.branch[j].x, fluteTree.branch[j].y)] =
              pin1;
        }
        else {
          pin1 = pinPtr1->second;
        }

        auto pinPtr2 = pinMap.find(
            make_pair(fluteTree.branch[n].x, fluteTree.branch[n].y));
        if(pinPtr2 == pinMap.end()) {
          stnPointCnt++;
          pin2.SetSteiner(stnPointCnt, i);
          pinMap[make_pair(fluteTree.branch[n].x, fluteTree.branch[n].y)] =
              pin2;
        }
        else {
          pin2 = pinPtr2->second;
        }
        //                if( string(curNet->name) == "u_logic_Xahvx4" ) {
        //                    pin1.Print();
        //                    cout << "to";
        //                    pin2.Print();
        //                    cout << endl;
        //                }
        //                wire(pin1, pin2, wl).print();

        if(pin1 != pin2) {
          wireSegStor[i].push_back(wire(pin1, pin2, wl));
          //                    (wireSegStor[i].end()-1)->print();
          //                    if( pin1.GetData() == 0 && pin1.GetPinNum() ==
          //                    0) {
          //                        cout << curNet->name << endl;
          //                        exit(0);
          //                    }
        }
      }

      free(fluteTree.branch);
      //            cout << endl;
    }
    //        if( i == 2 )
    //            exit(0);
  }
  cout << "total Steiner HPWL: " << totalStnWL << endl;
  //    exit(0);
}

void Timing::CleanSteiner() {
  for(int i = 0; i < _netCnt; i++) {
    vector< wire >().swap(wireSegStor[i]);
    //        wireSegStor[i].swap(vector<wire>());
  }

  vector< double >().swap(lumpedCapStor);
  lumpedCapStor.resize(_netCnt);
}

void Timing::WriteSpef(const string& spefLoc) {
  ofstream spefFile(spefLoc);
  //    ofstream spefFile( string(dir_bnd) + "/"+ benchName + ".spef");
  if(!spefFile.good()) {
    cout << "** ERROR : Cannot Open SPEF file to write : " << spefLoc << endl;
    exit(1);
  }
  stringstream feed;
  feed.precision(5);

  time_t rawtime;
  time(&rawtime);
  string t(ctime(&rawtime));
  feed << "*SPEF \"IEEE 1481-1998\"" << endl;
  feed << "*DESIGN \"" << benchName << "\"" << endl;
  feed << "*DATE \"" << t.substr(0, t.length() - 1) << "\"" << endl;
  feed << "*VENDOR \"ICCAD 2015 Contest\"" << endl;
  feed << "*PROGRAM \"ICCAD 2015 Contest Spef Generator\"" << endl;
  feed << "*VERSION \"0.0\"" << endl;
  feed << "*DESIGN_FLOW \"\"" << endl;
  feed << "*DIVIDER /" << endl;
  feed << "*DELIMITER :" << endl;
  feed << "*BUS_DELIMITER [ ]" << endl;
  feed << "*T_UNIT 1 PS" << endl;
  feed << "*C_UNIT 1 PF" << endl;
  feed << "*R_UNIT 1 OHM" << endl;
  feed << "*L_UNIT 1 UH" << endl << endl;

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

  // 2. write parasitics
  //    for(vector<net>::iterator theNet = partial_nets.begin() ; theNet !=
  //    partial_nets.end() ; ++theNet)
  for(int i = 0; i < _netCnt; i++) {
    NET* curNet = &_nets[i];

    // 0. write net name and lumped sum of downstream cap
    //        cout << "*D_NET "<< curNet->name << " " << lumpedCapStor[i] /
    //        CAP_SCALE << endl;
    feed << "*D_NET " << GetEscapedStr(curNet->Name()) << " "
         << lumpedCapStor[i] / CAP_SCALE << endl;

    // 1. write connections
    feed << "*CONN" << endl;

    for(int j = 0; j < curNet->pinCNTinObject; j++) {
      PIN* curPin = curNet->pin[j];

      feed << ((curPin->term && _terms[curPin->moduleID].isTerminalNI) ? "*P "
                                                                       : "*I ")
           << GetPinName(curPin) << ((curPin->IO == 1) ? " O" : " I") << endl;
    }

    // no parasitics bewteen clock source and LCBs
    //        if(theNet->name == clock_port) {
    //            feed << "*END" <<endl <<endl;
    //            continue;
    //        }

    // 2. write wire cap + load cap
    unsigned cnt = 1;
    feed << "*CAP" << endl;
    //        for(vector< pair< pair<string, string>, double > >::iterator
    //        theSeg=theNet->wire_segs.begin() ;
    //                theSeg != theNet->wire_segs.end() ; ++theSeg)
    for(auto& curSeg : wireSegStor[i]) {
      if(!pin_cap_written[curSeg.iPin]) {
        feed << cnt++ << " " << GetPinName(curSeg.iPin) << " "
             << lumped_cap_at_pin[curSeg.iPin] / CAP_SCALE << endl;
        pin_cap_written[curSeg.iPin] = true;
      }
      if(!pin_cap_written[curSeg.oPin]) {
        feed << cnt++ << " " << GetPinName(curSeg.oPin) << " "
             << lumped_cap_at_pin[curSeg.oPin] / CAP_SCALE << endl;
        pin_cap_written[curSeg.oPin] = true;
      }
    }

    // 3. write wire resistance
    cnt = 1;
    feed << "*RES" << endl;
    //        for(vector< pair< pair<string, string>, double > >::iterator
    //        theSeg=theNet->wire_segs.begin() ;
    //                theSeg != theNet->wire_segs.end() ; ++theSeg)

    for(auto& curSeg : wireSegStor[i]) {
      feed << cnt++ << " " << GetPinName(curSeg.iPin) << " "
           << GetPinName(curSeg.oPin) << " "
           << curSeg.length / (double)_l2d * resPerMicron / RES_SCALE << endl;
    }

    feed << "*END" << endl << endl;
  }

  pin_cap_written.clear();
  lumped_cap_at_pin.clear();

  WriteSpefClockNet(feed);

  spefFile << feed.str();
  spefFile.close();
  feed.clear();
}

TIMING_NAMESPACE_CLOSE

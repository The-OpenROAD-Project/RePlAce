///////////////////////////////////////////////////////////////////////////////
// Authors: Ilgweon Kang and Lutong Wang
//          (respective Ph.D. advisors: Chung-Kuan Cheng, Andrew B. Kahng),
//          based on Dr. Jingwei Lu with ePlace and ePlace-MS
//
//          Many subsequent improvements were made by Mingyu Woo
//          leading up to the initial release.
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////
//
//  This code was implemented by mgwoo at 18.01.17
//
//          http://mgwoo.github.io/
//
///////////////////////////////////////////////////////

#ifndef __circuit__
#define __circuit__ 0

#define REPLACE_NAMESPACE_OPEN namespace Replace {
#define REPLACE_NAMESPACE_CLOSE }

#ifndef CIRCUIT_FPRINTF
#define CIRCUIT_FPRINTF(fmt, ...)  \
  {                                \
    if(fmt) {                      \
      fprintf(fmt, ##__VA_ARGS__); \
    }                              \
  }
#endif

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <climits>
#include <cfloat>
#include <limits>
#include <queue>

#include <stdint.h>

#include <lefrReader.hpp>
#include <lefwWriter.hpp>
#include <lefiDebug.hpp>
#include <lefiEncryptInt.hpp>
#include <lefiUtil.hpp>

#include <defrReader.hpp>
#include <defiAlias.hpp>

#include "replace_private.h"
#define INIT_STR "!@#!@#"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::pair;

REPLACE_NAMESPACE_OPEN

class Circuit {
 public:
  Circuit();

  //
  // LEF/DEF is essential,
  // but verilog is optional
  //
  Circuit(vector< string >& lefStor, string defFilename, bool isVerbose = false); 
  void Init(vector< string >& lefStor, string defFilename,
            bool isVerbose = false);
  
  // Parsing function
  int ParseLef(vector< string >& lefStor, bool isVerbose);
  int ParseDef(string filename, bool isVerbose);
  
  void WriteLef(FILE* _fout);
  void WriteDef(FILE* _fout);

  /////////////////////////////////////////////////////
  // LEF parsing
  //
  double lefVersion;
  string lefDivider;
  string lefBusBitChar;

  lefiUnits lefUnit;
  double lefManufacturingGrid;
  vector< lefiMacro > lefMacroStor;
  vector< lefiLayer > lefLayerStor;
  vector< lefiVia > lefViaStor;
  vector< lefiSite > lefSiteStor;

  // Macro, via, Layer's unique name -> index of lefXXXStor.
  HASH_MAP< string, int > lefMacroMap;
  HASH_MAP< string, int > lefViaMap;
  HASH_MAP< string, int > lefLayerMap;
  HASH_MAP< string, int > lefSiteMap;

  // this will maps
  // current lefMacroStor's index
  // -> lefiPin, lefiObstruction
  //
  // below index is same with lefMacroStor
  vector< vector< lefiPin > > lefPinStor;  // macroIdx -> pinIdx -> pinObj
  vector< HASH_MAP< string, int > >
      lefPinMapStor;  // macroIdx -> pinName -> pinIdx
  vector< vector< lefiObstruction > >
      lefObsStor;  // macroIdx -> obsIdx -> obsObj

  /////////////////////////////////////////////////////
  // DEF parsing
  //
  string defVersion;
  string defDividerChar;
  string defBusBitChar;
  string defDesignName;

  vector< defiProp > defPropStor;

  double defUnit;
  defiBox defDieArea;
  vector< defiRow > defRowStor;
  vector< defiTrack > defTrackStor;
  vector< defiGcellGrid > defGcellGridStor;
  vector< defiVia > defViaStor;

  defiComponentMaskShiftLayer defComponentMaskShiftLayer;
  vector< defiComponent > defComponentStor;
  vector< defiPin > defPinStor;
  vector< defiBlockage > defBlockageStor;
  vector< defiNet > defNetStor;
  vector< defiNet > defSpecialNetStor;

  // Component's unique name -> index of defComponentStor.
  HASH_MAP< string, int > defComponentMap;
  HASH_MAP< string, int > defPinMap;

  // ROW's Y coordinate --> Orient info
  HASH_MAP< int, int > defRowY2OrientMap;

  // this will maps
  // current defComponentStor's index + string pin Name
  // -> defNetStor indexes.
  //
  // below index is same with defComponentStor
  vector< HASH_MAP< string, int > > defComponentPinToNet;

 private:

  /////////////////////////////////////////////////////
  // LEF Writing
  //

  // for Dump Lef
  void DumpLefVersion();
  void DumpLefDivider();
  void DumpLefBusBitChar();

  void DumpLefUnit();
  void DumpLefManufacturingGrid();
  void DumpLefLayer();
  void DumpLefSite();
  void DumpLefMacro();

  void DumpLefPin(lefiPin* pin);
  void DumpLefObs(lefiObstruction* obs);

  void DumpLefVia();
  void DumpLefDone();

  /////////////////////////////////////////////////////
  // DEF Writing
  //

  void DumpDefVersion();
  void DumpDefDividerChar();
  void DumpDefBusBitChar();
  void DumpDefDesignName();

  void DumpDefProp();
  void DumpDefUnit();

  void DumpDefDieArea();
  void DumpDefRow();
  void DumpDefTrack();
  void DumpDefGcellGrid();
  void DumpDefVia();

  void DumpDefComponentMaskShiftLayer();
  void DumpDefComponent();
  void DumpDefPin();
  void DumpDefSpecialNet();
  void DumpDefNet();
  void DumpDefDone();
  void DumpDefBlockage();

  void DumpDefComponentPinToNet();
};

// Verilog net Info Storage
struct NetInfo {

  int macroIdx;
  int compIdx;
  int pinIdx;

  NetInfo(int _macroIdx, int _compIdx, int _pinIdx)
      : macroIdx(_macroIdx), compIdx(_compIdx), pinIdx(_pinIdx){};
};

extern Circuit __ckt;

REPLACE_NAMESPACE_CLOSE

void ParseInput();
void ParseLefDef();

// required for timing
void SetVerilogTopModule();

void ReadPl(const char* fileName, bool isNameConvert = false);
void ReadPlLefDef(const char* fileName, bool isNameConvert = false);

void WriteDef(const char* defOutput);

void GenerateModuleTerminal(Replace::Circuit& __ckt);
void GenerateRow(Replace::Circuit& __ckt);
void GenerateFullRow(Replace::Circuit& __ckt);
void GenerateDummyCell(Replace::Circuit& __ckt);

void GenerateNetDefOnly(Replace::Circuit& __ckt);

/////////////////////////////////////////////////////
// DieRect struct  
struct DieRect {
  float llx, lly, urx, ury;
  DieRect() : 
    llx(FLT_MAX), lly(FLT_MAX), urx(FLT_MIN), ury(FLT_MIN) {};
  DieRect(float _llx, float _lly, float _urx, float _ury) :
    llx(_llx), lly(_lly), urx(_urx), ury(_ury) {};

  bool isNotInitialize() {
    return (llx == FLT_MAX || lly == FLT_MAX || 
            urx == FLT_MIN || ury == FLT_MIN);
  }
  void Dump() {
    cout << "(" << llx << ", " << lly << ") - (" 
      << urx << ", " << ury << ")" << endl;
  }
};

DieRect GetDieFromProperty(bool isScaleDown = true);
DieRect GetDieFromDieArea(bool isScaleDown = true);
DieRect GetCoreFromRow();

/////////////////////////////////////////////////////
// ArrayInfo class
class ArrayInfo {
private:
  float lx_;
  float ly_;
  float siteSizeX_;
  float siteSizeY_;
public:
  enum CellInfo {Empty, Row, Cell};
  ArrayInfo(float lx, float ly, float siteSizeX, float siteSizeY) :
    lx_(lx), ly_(ly), siteSizeX_(siteSizeX), siteSizeY_(siteSizeY) {};
  int GetCoordiX( float x );
  int GetCoordiY( float y );
  
  int GetLowerX( float x );
  int GetLowerY( float y );
  
  int GetUpperX( float x );
  int GetUpperY( float y );
};


// return Component Index
inline int GetDefComponentIdx(Replace::Circuit& __ckt, string& compName) {
  auto dcPtr = __ckt.defComponentMap.find(compName);
  if(dcPtr == __ckt.defComponentMap.end()) {
    cout << "** ERROR:  Component Instance ( " << compName
         << " ) does not exist in COMPONENT statement (defComponentMap) "
         << endl;
    exit(1);
  }
  return dcPtr->second;
}

// return Macro Index
inline int GetLefMacroIdx(Replace::Circuit& __ckt, string& macroName) {
  auto mcPtr = __ckt.lefMacroMap.find(macroName);
  if(mcPtr == __ckt.lefMacroMap.end()) {
    cout << "** ERROR:  Macro Instance ( " << macroName
         << " ) does not exist in COMPONENT statement (lefMacroMap) " << endl;
    exit(1);
  }
  return mcPtr->second;
}

// return Pin Index
inline int GetLefMacroPinIdx(Replace::Circuit& __ckt, int macroIdx,
                             string& pinName) {
  auto pinPtr = __ckt.lefPinMapStor[macroIdx].find(pinName);
  if(pinPtr == __ckt.lefPinMapStor[macroIdx].end()) {
    cout << "** ERROR:  Pin Instance ( " << pinName
         << " ) does not exist in MACRO statement (lefPinMapStor) " << endl;
    exit(1);
  }
  return pinPtr->second;
}

inline int GetDefPinIdx(Replace::Circuit& __ckt, string& pinName) {
  auto pinPtr = __ckt.defPinMap.find(pinName);
  if(pinPtr == __ckt.defPinMap.end()) {
    cout << "** ERROR:  Pin Instance ( " << pinName
         << " ) does not exist in PINS statement (defPinMap) " << endl;
    exit(1);
  }
  return pinPtr->second;
}

#endif

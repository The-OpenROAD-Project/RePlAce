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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
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

#define CIRCUIT_NAMESPACE_OPEN namespace Circuit{
#define CIRCUIT_NAMESPACE_CLOSE }

#ifndef CIRCUIT_FPRINTF
#define CIRCUIT_FPRINTF(fmt, ...) {if(fmt) { fprintf(fmt, ##__VA_ARGS__); }}
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
#include <sparsehash/dense_hash_map>

#include <stdint.h>

#include <lefrReader.hpp>
#include <lefwWriter.hpp>
#include <lefiDebug.hpp>
#include <lefiEncryptInt.hpp>
#include <lefiUtil.hpp>

#include <defrReader.hpp>
#include <defiAlias.hpp>

#define INIT_STR "!@#!@#"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::pair;
using google::dense_hash_map;

CIRCUIT_NAMESPACE_OPEN
class Circuit {
    public:
        Circuit()
        : lefManufacturingGrid(DBL_MIN) {
            lefMacroMap.set_empty_key(INIT_STR);
            lefViaMap.set_empty_key(INIT_STR);
            lefLayerMap.set_empty_key(INIT_STR);
            lefSiteMap.set_empty_key(INIT_STR);

            defComponentMap.set_empty_key(INIT_STR);
            defPinMap.set_empty_key(INIT_STR);
            defRowY2OrientMap.set_empty_key(INT_MAX);
        }

        // 
        // LEF/DEF is essential, 
        // but verilog is optional
        //
        Circuit(vector<string>& lefStor, 
                string defFilename, string verilogFilename = "" )
        : lefManufacturingGrid(DBL_MIN) {
            lefMacroMap.set_empty_key(INIT_STR);
            lefViaMap.set_empty_key(INIT_STR);
            lefLayerMap.set_empty_key(INIT_STR);
            lefSiteMap.set_empty_key(INIT_STR);

            defComponentMap.set_empty_key(INIT_STR);
            defPinMap.set_empty_key(INIT_STR);
            defRowY2OrientMap.set_empty_key(INT_MAX);

            Init( lefStor, defFilename, verilogFilename );
        }

        void Init( vector<string>& lefStor, string defFilename, 
                   string verilogFilename = "" ) {
            ParseLef(lefStor);
            ParseDef(defFilename);
            if( verilogFilename != "" ) {
                // ParseVerilog(verilogFilename);   
            }
        };

        void WriteLef( FILE* _fout );
        void WriteDef( FILE* _fout );
        
        /////////////////////////////////////////////////////
        // LEF parsing
        //
        double lefVersion;
        string lefDivider;
        string lefBusBitChar;

        lefiUnits lefUnit;
        double lefManufacturingGrid;
        vector<lefiMacro> lefMacroStor;
        vector<lefiLayer> lefLayerStor;
        vector<lefiVia> lefViaStor;
        vector<lefiSite> lefSiteStor;

        // Macro, via, Layer's unique name -> index of lefXXXStor.
        dense_hash_map<string, int> lefMacroMap;
        dense_hash_map<string, int> lefViaMap;
        dense_hash_map<string, int> lefLayerMap;
        dense_hash_map<string, int> lefSiteMap;

        // this will maps 
        // current lefMacroStor's index
        // -> lefiPin, lefiObstruction
        //
        // below index is same with lefMacroStor
        vector<vector<lefiPin>> lefPinStor; // macroIdx -> pinIdx -> pinObj
        vector<dense_hash_map<string, int>> lefPinMapStor; // macroIdx -> pinName -> pinIdx
        vector<vector<lefiObstruction>> lefObsStor; // macroIdx -> obsIdx -> obsObj
        
        /////////////////////////////////////////////////////
        // DEF parsing
        //
        string defVersion;
        string defDividerChar;
        string defBusBitChar;
        string defDesignName;

        vector<defiProp> defPropStor;

        double defUnit;
        defiBox defDieArea;
        vector<defiRow> defRowStor;
        vector<defiTrack> defTrackStor;
        vector<defiGcellGrid> defGcellGridStor;
        vector<defiVia> defViaStor;

        defiComponentMaskShiftLayer defComponentMaskShiftLayer;
        vector<defiComponent> defComponentStor;
        vector<defiPin> defPinStor;
        vector<defiNet> defNetStor;
        vector<defiNet> defSpecialNetStor;

        // Component's unique name -> index of defComponentStor.
        dense_hash_map<string, int> defComponentMap;
        dense_hash_map<string, int> defPinMap;

        // ROW's Y coordinate --> Orient info
        dense_hash_map<int, int> defRowY2OrientMap;

        // this will maps
        // current defComponentStor's index + string pin Name
        // -> defNetStor indexes.
        //
        // below index is same with defComponentStor
        vector<dense_hash_map<string, int>> defComponentPinToNet;

    private:
        // Parsing function
        void ParseLef(vector<string>& lefStor);
        void ParseDef(string filename);
        // void ParseVerilog(string filename);


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

        void DumpDefComponentPinToNet();

};

// Verilog net Info Storage
struct NetInfo {
//    string macroName;
//    string compName;
//    string pinName;
    
    int macroIdx;
    int compIdx;
    int pinIdx;

//    NetInfo(char* _macroName, char* _compName, char* _pinName) {
//        macroName = string(_macroName);
//        compName = string(_compName);
//        pinName = string(_pinName);
//    }
    NetInfo( int _macroIdx, int _compIdx, int _pinIdx) 
        : macroIdx(_macroIdx), compIdx(_compIdx), pinIdx(_pinIdx) {};
};

extern Circuit __ckt; 

CIRCUIT_NAMESPACE_CLOSE

struct DieRect {
  int llx, lly, urx, ury;
  DieRect() : llx(INT_MAX), lly(INT_MAX), urx(INT_MIN), ury(INT_MIN) {};
  bool isNotInitialize () {
    return ( llx == INT_MAX || lly == INT_MAX
        || urx == INT_MIN || ury == INT_MIN) ;
  }
  void Dump() {
    cout << "(" << llx << ", " << lly << ") - (" << urx << ", " << ury << ")" << endl;
  }
};

DieRect GetDieFromProperty();
DieRect GetDieFromDieArea();


void ParseInput();
void ParseLefDef();

void ReadPl(const char* fileName);
void ReadPlLefDef(const char* fileName);

void WriteDef(const char* defOutput);

void GenerateModuleTerminal(Circuit::Circuit& __ckt);
void GenerateRow(Circuit::Circuit& __ckt);

void GenerateNetDefOnly(Circuit::Circuit& __ckt);
void GenerateNetDefVerilog(Circuit::Circuit& __ckt);

// custom scale down parameter setting during the stage
void SetUnitY(float  _unitY);
void SetUnitY(double _unitY);

#endif
 

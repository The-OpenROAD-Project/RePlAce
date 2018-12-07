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

#include "global.h"
#include "lefdefIO.h"
#include "bookShelfIO.h"

#include "verilog_writer.h"
#include "verilog_parser.h"
#include "verilog_ast_util.h"

#include "mkl.h"

#include "timing.h"
#include "timingSta.h"
#include <iostream>
#include <boost/functional/hash.hpp>

using namespace std;
using Circuit::NetInfo;
using Circuit::Circuit;

// 
// The below global variable is updated.
//
//
// terminalInstance // terminalCNT
// moduleInstance // moduleCNT
// netInstnace // netCNT
// pinInstance // pinCNT
// row_st // row_cnt
//
// shapeMap // (Fixed cell can have rectilinear polygon)
// 
// numNonRectangularNodes // required bookshelf writing - *.shape aware
// totalShapeCount // required bookshelf writing - *.shape aware
//
// terminal_pmin
// terminal_pmax
//
// grow_pmin
// grow_pmax
//
// rowHeight
//
//

// global variable.. to write output as DEF
Circuit::Circuit __ckt;

// lef 2 def unit convert
static prec l2d = 0.0f;

// 
// To support book-shelf based tools, scale down all values..
// 
// Set unitX as siteWidth
// Set unitY as Vertical routing pitch (in lowest metal) 
//
static prec unitX = 0.0f;
static prec unitY = 0.0f;

//
// To prevent mis-matching in ROW offset.
//
static prec offsetX = 0.0f;
static prec offsetY = 0.0f;


// module & terminal Makp
// 
// first : nodeName
// second, first : isModule (true -> moduleInst // false -> termInst)
// second, second : corresponding index
//
static dense_hash_map< string, pair<bool, int> > moduleTermMap;

// 
// Metal1 Name.
// It usually depends on the lef files
//
static string metal1Name;

// helper function for LEF/DEF in siteorient
static char* orientStr(int orient) {
    switch (orient) {
        case 0: return ((char*)"N");
        case 1: return ((char*)"W");
        case 2: return ((char*)"S");
        case 3: return ((char*)"E");
        case 4: return ((char*)"FN");
        case 5: return ((char*)"FW");
        case 6: return ((char*)"FS");
        case 7: return ((char*)"FE");
    };
    return ((char*)"BOGUS");
}

// for Saving verilog information
// declaired here to send verilog -> timing inst.. 
static dense_hash_map<string, vector<NetInfo>, boost::hash<std::string>> netMap; 

// save clockNets' index
// for DEF parsing
static vector<int> clockNetsDef;

// for Verilog parsing
static vector<string> clockNetsVerilog;


// from main.cpp
void ParseInput() {
    if( auxCMD == "" && lefStor.size() != 0 && defCMD != "" ) {
        inputMode = InputMode::lefdef; 
        ParseLefDef();
    }
    else if( auxCMD != "" && lefStor.size() == 0 && defCMD == "") {
        inputMode = InputMode::bookshelf; 
        ParseBookShelf();
    }

    if( verilogCMD != "") {
        SetVerilogTopModule();
    }
}

void SetUnitY(float _unitY) {
    unitY = _unitY;
}

void SetUnitY(double _unitY) {
    unitY = _unitY;
}

inline static bool IsPrecEqual(prec a, prec b) {
    return std::fabs(a-b) < std::numeric_limits<float>::epsilon();
}


// 
// this will set below (global) parameter, needed to parse LEF/DEF file
//
// l2d,                 - lef to def scale difference
// metal1Name,          - metal1 Layer Name
// unitX, unitY         - scale down parameter for bookshelf porting
// offsetX, offsetY     - offset parameter to exactly fit on 'ROW' structure
//
void SetParameter() {
    // lef 2 def unit info setting
    l2d = __ckt.defUnit;
    cout << "INFO:  DEF SCALE UNIT: " << l2d << endl;

    if( __ckt.lefLayerStor.size() == 0) {
        cout << "\n** ERROR : LAYER statements not exists in lef file!" << endl;
        exit(1);
    }
    // Metal1 Name extract 
    for(auto& curLayer : __ckt.lefLayerStor) {
        if(!curLayer.hasType()) {
            continue;   
        }
        if( strcmp(curLayer.type(), "ROUTING") == 0 ) {
            metal1Name = string(curLayer.name());
            break;
        }
    }
    cout << "INFO:  METAL1 NAME IN LEF: " << metal1Name << endl;

    // required for FIXED components(DEF) -> *.shape bookshelf
    shapeMap.set_empty_key(INIT_STR);

    // required for net(DEF) -> module fast access
    moduleTermMap.set_empty_key(INIT_STR);

    // unitX setting : CORE SITE's Width
    for(auto& curSite : __ckt.lefSiteStor) {
        if( !curSite.hasClass() || !curSite.hasSize() ) {
            continue;
        }
        if( strcmp( curSite.siteClass(), "CORE" ) == 0 ) {
            unitX = l2d* curSite.sizeX();
            break;
        }
    }
  
    // unitY setting : first metal1 LAYER's yPitch 
    // 
    // unitY was not initialized by SetUnitY function before.
    // (is equal to 0.0f)
    if( IsPrecEqual(unitY, 0.0f) ) {
        for(auto& curLayer : __ckt.lefLayerStor) {
            if( metal1Name == string(curLayer.name()) ) {
                if( curLayer.hasPitch() ) {
                    unitY = l2d * curLayer.pitch();
                }
                else if( curLayer.hasXYPitch() ){
                    unitY = l2d * curLayer.pitchY();
                }
                break;
            }
        }
    } 

//    unitY *= 1.5;

    cout << "INFO:  SCALE DOWN UNIT: ( " << unitX << ", " << unitY << " )" << endl;

    // offsetX & offsetY : Minimum coordinate of ROW's x/y
    offsetX = offsetY = PREC_MAX; 
    for(auto& curRow : __ckt.defRowStor) {
        offsetX = (offsetX > curRow.x())? curRow.x() : offsetX;
        offsetY = (offsetY > curRow.y())? curRow.y() : offsetY;
    }
//    cout << INT_CONVERT(offsetX) << endl;
//    cout << INT_CONVERT(offsetY) << endl;

    offsetX = (INT_CONVERT(offsetX) % INT_CONVERT(unitX) == 0)? 
              0 : 
              unitX - (INT_CONVERT(offsetX) % (INT_CONVERT(unitX)));

    offsetY = (INT_CONVERT(offsetY) % INT_CONVERT(unitY) == 0)? 
              0 : 
              unitY - (INT_CONVERT(offsetY) % (INT_CONVERT(unitY)));

    cout << "INFO:  OFFSET COORDINATE: ( " << offsetX << ", " << offsetY << " )" << endl << endl;
    
}

void SetVerilogTopModule() {
  /*
    using namespace verilog;

    FILE* verilogInput = fopen(verilogCMD.c_str(), "rb");
    if( !verilogInput ) {
        cout << "** ERROR:  Cannot open verilog file: " << verilogCMD << endl;
        exit(1);
    }

    verilog::verilog_parser_init();
    int result = verilog::verilog_parse_file( verilogInput );
    if( result != 0 ) {
        cout << "** ERROR:  Verilog Parse Failed: " << verilogCMD << endl;
        exit(1);
    }

    verilog::verilog_source_tree* tree = verilog::yy_verilog_source_tree;
    verilog::verilog_resolve_modules(tree);
   
    if( tree->modules->items > 2 ) {
        cout << "WARNING:  # Modules in Verilog: " << tree->modules->items
             << ", so only use the 'First' module" << endl;
    }

    // extract the '1st' module
    ast_module_declaration* module = (ast_module_declaration*)ast_list_get(tree->modules, 0);
  */
    verilogTopModule = __ckt.defDesignName; 
}


void ParseLefDef() {
    
    // for input parse only
    // 
    // Circuit::Circuit __ckt(lefStor, defCMD, "");
    //
    __ckt.Init(lefStor, defCMD, isVerbose);

    SetParameter(); 

    GenerateRow(__ckt);
    GenerateModuleTerminal(__ckt);
    if( defMacroCnt > 0) {
        GenerateFullRow(__ckt);
    }

    if( __ckt.defNetStor.size() > 0 ) {
        cout << "INFO:  EXTRACT NET INFO FROM DEF ONLY" << endl;
        GenerateNetDefOnly(__ckt);
    }
    else {
        cout << "INFO:  EXTRACT NET INFO FROM DEF & VERILOG" << endl;
        if( verilogCMD == "" ) {
            cout << "** ERROR:  Cannot Find any Net information "
                 << "(Check NETS statement in DEF) "
                 << "or use -verilog command instead"  << endl;
            exit(1);
        }

        GenerateNetDefVerilog(__ckt);
    }


    cout << "INFO:  SUCCESSFULLY LEF/DEF PARSED" << endl; 


    // do weird things..
    POS tier_min, tier_max;
    int tier_row_cnt = 0;

    get_mms_3d_dim( &tier_min, &tier_max, &tier_row_cnt);
    transform_3d(&tier_min, &tier_max, tier_row_cnt);
    post_read_3d();
}

void WriteDef(const char* defOutput) {
    MODULE* curModule = NULL;

    // moduleInstnace -> defComponentStor
    for(int i=0; i<moduleCNT; i++) {
        curModule = &moduleInstance[i];
        auto cmpPtr = __ckt.defComponentMap.find(string(curModule-> name));
        if( cmpPtr == __ckt.defComponentMap.end() ) {
            cout << "** ERROR:  Module Instance ( "<< curModule->name
                << " ) does not exist in COMPONENT statement (defComponentMap) " << endl;
            exit(1);
        }

        // update into PLACED status
        if( !__ckt.defComponentStor[cmpPtr->second].isPlaced() ) {
            __ckt.defComponentStor[cmpPtr->second].setPlacementStatus( DEFI_COMPONENT_PLACED );
        }
        
        // update into corresponding coordinate
        //
        // unitX & unitY is used to recover scaling
        
        int x = INT_CONVERT( curModule->pmin.x * unitX ) - offsetX;
        int y = INT_CONVERT( curModule->pmin.y * unitY ) - offsetY;

        // x-coordinate, y-coordinate, cell-orient
        auto orientPtr = __ckt.defRowY2OrientMap.find(y);

        __ckt.defComponentStor[cmpPtr->second].
            setPlacementLocation( x, y, 
                                    (orientPtr != __ckt.defRowY2OrientMap.end())? 
                                    orientPtr->second : -1 );

        // cout << curModule->name << ": " << curModule->pmin.x << " " << curModule->pmin.y << endl;
    }


    FILE* fp = fopen( defOutput, "w"); 
    if( !fp ) {
        cout << "** ERROR:  Cannot open " << defOutput << " (DEF WRITING)" << endl;
        exit(1);
    }

    __ckt.WriteDef(fp);
    cout << "INFO:  SUCCESSFULLY WRITE DEF FILE INTO " << defOutput << endl; 
}

////////
//
// Generate Module&Terminal Instance
//
// helper function for FIXED cells(in DEF Components), which can have multiple rectangles, 
// i.e. it is corresponded to *.shape
//
// The shapeMap & shapeStor is updated in here.
// numNonRectangularNodes, totalShapeCount also updated in here
//
// COMPONENTS -> MACRO(TYPE: BLOCK) -> OBS -> LAYER -> RECT 
//
// return type :
// true -- the sub-rectangular has been found and added
// false -- the sub-rectangular has not been found.
bool AddShape(int defCompIdx, int lx, int ly) {
    
    string compName = string(__ckt.defComponentStor[defCompIdx].id());
    string macroName = string(__ckt.defComponentStor[defCompIdx].name());

    // reference MACRO in lef
    auto mcPtr = __ckt.lefMacroMap.find( macroName );
    if( mcPtr == __ckt.lefMacroMap.end() ) {
        cout << "** ERROR:  Macro Instance ( "
            << macroName 
            << " ) does not exist in COMPONENT statement (lefMacroMap) " << endl;
        exit(1);
    }

    // Check whether MACRO have CLASS statement
    int macroIdx = mcPtr->second;
    if( !__ckt.lefMacroStor[macroIdx].hasClass() ) {
        cout << "** ERROR: Macro Instance ( "
             << __ckt.lefMacroStor[macroIdx].name()
             << " ) does not have Class! (lefMacroStor) " << endl;
        exit(1);
    }
    
    // 
    // only for MACRO TYPE == BLOCK cases 
    // & OBS command must exist in LEF
    //
    if( strcmp( __ckt.lefMacroStor[macroIdx].macroClass(), "BLOCK") != 0 ||
        __ckt.lefObsStor[macroIdx].size() == 0 ) {
        return false;
    }

    // already updated into shapeMap, then skip
//    if( shapeMap.find( compName ) != shapeMap.end() ) {
//        cout << "already updated: " << compName << endl;
//        return ;
//    }
//    cout << "compName: " << compName << endl;

    bool isMetal1 = false;
    int shapeCnt = 0;

    for(auto& curObs : __ckt.lefObsStor[macroIdx]) {
        lefiGeometries* curGeom = curObs.geometries();
        
        // LAYER Metal1 <-- lefiGeomLayerE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //
        //
        // LAYER VIA1 <-- Skip for this 
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
       
        for(int j=0; j<curGeom->numItems(); j++) {
            // Meets 'Metal1' Layer
            if( curGeom->itemType(j) == lefiGeomLayerE &&
                    string(curGeom->getLayer(j)) == metal1Name ) {
//                cout << j << " " << curGeom->getLayer(j) << endl;
                isMetal1 = true;
                continue;
            }
            
            // calculate BBox
            if( isMetal1 && curGeom->itemType(j) == lefiGeomRectE ) {
                lefiGeomRect* rect = curGeom->getRect(j);

                // shape Name -> bunch of shapeStor's index
                if( shapeMap.find(compName) == shapeMap.end() ) {
                    vector<int> tmpStor;
                    tmpStor.push_back( shapeStor.size() );
                    shapeMap[compName] = tmpStor;
                }
                else {
                    shapeMap[compName].push_back( shapeStor.size() );
                }
                totalShapeCount ++;

                // finally pushed into shapeStor
                shapeStor.push_back( 
                    SHAPE( string("shape_")+to_string( shapeCnt ),
                           compName, shapeStor.size(), 
                           (l2d * rect->xl + lx + offsetX)/unitX,        // lx  
                           (l2d * rect->yl + ly + offsetY)/unitY,        // ly
                           l2d*(rect->xh - rect->xl)/unitX,         // xWidth
                           l2d*(rect->yh - rect->yl)/unitY ) );     // yWidth

//                cout << compName << ", " << string("shape_")+to_string(shapeCnt) << endl;
                shapeCnt++;
            }
            // now, meets another Layer
            else if( isMetal1 && curGeom->itemType(j) == lefiGeomLayerE ) {
                break;
            }
        }
        // firstMetal was visited
        if( isMetal1 ) {
            break;
        }
    }

    if( isMetal1 ) {
        numNonRectangularNodes ++;
    }
    
    return (isMetal1)? true : false;
}


void SetSizeForObsMacro( int macroIdx, MODULE* curModule )  {
    bool isMetal1 = false;
    for(auto& curObs : __ckt.lefObsStor[macroIdx]) {
        lefiGeometries* curGeom = curObs.geometries();
        
        // LAYER Metal1 <-- lefiGeomLayerE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //
        //
        // LAYER VIA1 <-- Skip for this 
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
       
        for(int j=0; j<curGeom->numItems(); j++) {
            // Meets 'Metal1' Layer
            if( curGeom->itemType(j) == lefiGeomLayerE &&
                    string(curGeom->getLayer(j)) == metal1Name ) {
//                cout << j << " " << curGeom->getLayer(j) << endl;
                isMetal1 = true;
                continue;
            }
            
            // calculate BBox
            if( isMetal1 && curGeom->itemType(j) == lefiGeomRectE ) {
                lefiGeomRect* rect = curGeom->getRect(j);
                
                curModule->size.Set( l2d*(rect->xh - rect->xl)/unitX, l2d*(rect->yh - rect->yl)/unitY, 1 );
//                cout << rect->xl << " " << rect->yl << endl;
//                cout << rect->xh << " " << rect->yh << endl;
            }
            // now, meets another Layer
            else if( isMetal1 && curGeom->itemType(j) == lefiGeomLayerE ) {
                break;
            }
        }
        // firstMetal was visited
        if( isMetal1 ) {
            break;
        }
    }
//    cout << "func end" << endl;
}

//
// defComponentStor(non FIXED cell) -> moduleInstance
// defComponentStor(FIXED cell), defPinStor -> terminalInstnace
//
// terminal_pmin & terminal_pmax must be updated...
void GenerateModuleTerminal(Circuit::Circuit &__ckt) {
    
    moduleInstance = (MODULE*) mkl_malloc( sizeof(MODULE)* 
                                __ckt.defComponentStor.size(), 64 );

    // to fast traverse when building TerminalInstance
    vector<int> fixedComponent;
    
    MODULE* curModule = NULL;
    defiComponent* curComp = NULL;
    lefiMacro* curMacro = NULL;

    moduleCNT = 0;
   
    // not 1-to-1 mapping (into moduleInstnace), so traverse by index 
    for(int i=0; i<__ckt.defComponentStor.size(); i++) {
        curComp = &(__ckt.defComponentStor[i]);

        curModule = &moduleInstance[moduleCNT];
        new (curModule) MODULE();
    
        if( curComp->isFixed() ) {
            fixedComponent.push_back(i);
            continue;
        }
    
        // pmin info update
        if( curComp->isPlaced() ) {
            // only when is already placed
            curModule->pmin.Set( ((prec)curComp->placementX() + offsetX)/unitX, 
                    ((prec)curComp->placementY() + offsetY)/unitY, (prec)0);
        }
        else {
            curModule->pmin.SetZero();
        }

        auto macroPtr = __ckt.lefMacroMap.find( string(curComp->name()) );
        if( macroPtr == __ckt.lefMacroMap.end() ) {
            cout << "\n** ERROR : Cannot find MACRO cell in lef files: " 
                 << curComp->name() << endl;
            exit(1);
        }
        
        curMacro = &__ckt.lefMacroStor[ macroPtr->second ];

        if( !curMacro->hasSize() ) {
            cout << "\n** ERROR : Cannot find MACRO SIZE in lef files: " 
                 << curComp->name() << endl;
            exit(1);
        }

        if( strcmp( curMacro->macroClass(), "BLOCK") == 0 &&
            __ckt.lefObsStor[macroPtr->second].size() != 0 ) {
//            cout << "BLOCK/OBS: " << curMacro->name() << endl; 
            SetSizeForObsMacro(macroPtr->second, curModule);
//            curModule.size.Set( l2d * curMacro);
        }
        else {
            // size info update from LEF Macro
            curModule->size.Set( l2d * curMacro->sizeX()/unitX, 
                    l2d * curMacro->sizeY()/unitY, 1 );
//            curModule->size.Dump("size");
        }
        
        if( fabs(curModule->size.y - rowHeight) > PREC_EPSILON  ) {
            defMacroCnt ++;
            cout << "MACRO: " << curComp->id() << endl; 
        }

        // set half_size
        curModule->half_size.Set( curModule->size.x / 2, 
                                  curModule->size.y / 2, 0);
        
        // set center coordi
        curModule->center.SetAdd( curModule->pmin, curModule->half_size );

        // set pinMax coordi
        curModule->pmax.SetAdd( curModule->pmin, curModule->size ); 
        
        // set area
        curModule->area = curModule->size.GetProduct();

        // set which tier
        curModule->tier = 0;

        string moduleName = curComp->id();

        // set Name
        strcpy( curModule->name, moduleName.c_str() ); 

        // set Index
        curModule->idx = moduleCNT;

        moduleTermMap[ moduleName ] = 
                make_pair( true, moduleCNT ); 
      
        // check 
//        curModule->Dump(string("Macro ") + to_string(moduleCNT)); 
        moduleCNT++;
    }
//    cout << moduleCNT << endl;

    // memory cutting
    moduleInstance = (MODULE*) mkl_realloc( moduleInstance, sizeof(MODULE) * moduleCNT );

   
    // 
    // Terminal Update
    //
    TERM* curTerm = NULL;
    terminalCNT = 0;
    terminalInstance = (TERM*) mkl_malloc( sizeof(TERM)*
        ( fixedComponent.size() + __ckt.defPinStor.size() ), 64);

    // for fixed cells.
    for(auto& curIdx : fixedComponent) {
        curTerm = &terminalInstance[terminalCNT];
        new (curTerm) TERM();

        curComp = &(__ckt.defComponentStor[curIdx]);
        
        curTerm->idx = terminalCNT;

        // check whether this nodes contains sub-rectangular sets
        bool shapeFound = AddShape( curIdx, curComp->placementX(), curComp->placementY() );

        // pmin info update
        curTerm->pmin.Set( ((prec)curComp->placementX() + offsetX)/unitX, 
                           ((prec)curComp->placementY() + offsetY)/unitY, (prec)0);
   
//        cout << "Fixed: " << curComp->name() << endl;
        auto macroPtr = __ckt.lefMacroMap.find( string(curComp->name()) );
        if( macroPtr == __ckt.lefMacroMap.end() ) {
            cout << "\n** ERROR : Cannot find MACRO cell in lef files: " 
                 << curComp->name() << endl;
            exit(1);
        }
        
        curMacro = &__ckt.lefMacroStor[ macroPtr->second ];
        int macroIdx = macroPtr->second;

        if( !curMacro->hasSize() ) {
            cout << "\n** ERROR : Cannot find MACRO SIZE in lef files: " 
                 << curComp->name() << endl;
            exit(1);
        }

        // 
        // ** terminal/terminal_NI determination rule
        //
        // if OBS exists,
        //   1) if M1 exists -> (Then shapeFound has 'true') ->  terminal Cell
        //   2) if M1 does not exists in OBS -> terminal_NI Cell
        //
        // if OBS not exists -> terminal Cell (All Metal layer cannot accross this region)
        //
        //
        // size info update from LEF Macro
//        if( shapeFound || __ckt.lefObsStor[macroIdx].size() == 0) {
            // terminal nodes (in bookshelf)
            curTerm->size.Set( l2d * curMacro->sizeX()/unitX, 
                               l2d * curMacro->sizeY()/unitY, 1 );  
            curTerm->isTerminalNI = false;
//        }
//        else {
            // terminal_NI nodes (in bookshelf)
//            curTerm->size.Set(0, 0, 1);
//            curTerm->isTerminalNI = true;
//        }
        
//        curTerm->isTerminalNI = (shapeFound || __ckt.lefObsStor[macroIdx].size() == 0)? false : true;

        // set center coordi
        curTerm->center.x = curTerm->pmin.x + curTerm->size.x/2;
        curTerm->center.y = curTerm->pmin.y + curTerm->size.y/2;

        // set pinMax coordi
        curTerm->pmax.SetAdd( curTerm->pmin, curTerm->size ); 
        
        // set area
        curTerm->area = curTerm->size.GetProduct();
        
        string termName = curComp->id();

        // set Name
        strcpy( curTerm->name, termName.c_str() ); 

        // set tier
        moduleTermMap[ termName ] = 
                make_pair( false, terminalCNT ); 

//        curTerm->Dump();
        terminalCNT++;
    }

    // for pin
    for(auto& curPin : __ckt.defPinStor) {
        curTerm = &terminalInstance[terminalCNT];
        new (curTerm) TERM();

        strcpy( curTerm->name, curPin.pinName() );
        curTerm->idx = terminalCNT;
        curTerm->IO = (strcmp( curPin.direction(), "INPUT") == 0)? 0 : 1;
        curTerm->pmin.Set( (curPin.placementX() + offsetX)/unitX, 
                           (curPin.placementY() + offsetY)/unitY, 0 );
        curTerm->isTerminalNI = true;

        // since size == 0, pmin == center == pmax; 
        curTerm->pmax.Set(curTerm->pmin);
        curTerm->center.Set(curTerm->pmin);
        
        moduleTermMap[ string(curTerm->name) ] = 
                make_pair( false, terminalCNT ); 

//        curTerm->Dump();
        terminalCNT++;
    }
    cout << "INFO:  #MODULE: " << moduleCNT << ", #TERMINAL: " << terminalCNT << endl;
}


/////////////////////////////////////////////
//  DieRect Instances
//
DieRect GetDieFromProperty() {

    // Set DieArea from PROPERTYDEFINITIONS
    string llxStr = "FE_CORE_BOX_LL_X", urxStr = "FE_CORE_BOX_UR_X";
    string llyStr = "FE_CORE_BOX_LL_Y", uryStr = "FE_CORE_BOX_UR_Y";

    DieRect retRect;
    for(auto& prop: __ckt.defPropStor) {
        if (strcmp( prop.propType(), "design") != 0) {
            continue;
        }

        if( string(prop.propName()) == llxStr ) {
            retRect.llx = INT_CONVERT(prop.number() * l2d / unitX); 
        }
        else if( string(prop.propName()) == llyStr ) {
            retRect.lly = INT_CONVERT(prop.number() * l2d / unitY);
        }
        else if( string(prop.propName()) == urxStr ) {
            retRect.urx = INT_CONVERT(prop.number() * l2d / unitX);
        }
        else if( string(prop.propName()) == uryStr ) {
            retRect.ury = INT_CONVERT(prop.number() * l2d / unitY);
        }
    }
    return retRect; 
}

DieRect GetDieFromDieArea() {
    DieRect retRect;
    defiPoints points = __ckt.defDieArea.getPoint();
    if( points.numPoints >=2 ) {
        retRect.llx = points.x[0] / unitX;
        retRect.lly = points.y[0] / unitY;
        retRect.urx = points.x[1] / unitX;
        retRect.ury = points.y[1] / unitY;
    }
    for(int i=0; i<points.numPoints; i++) {
        cout << points.x[i] << " " << points.y[i] << endl;
    }
    
    retRect.Dump();
    return retRect;
}

/////////////////////////////////////////////
//  Generate RowInstance
// defRowStor -> rowInstance
// this must update grow_pmin / grow_pmax, rowHeight
void GenerateRow(Circuit::Circuit &__ckt) {
    // Get the sites from ROW statements
    // usual cases
    bool isFirst = true;
    int i=0;
    
    row_cnt = __ckt.defRowStor.size();
    row_st = (ROW*) mkl_malloc( sizeof(ROW)*row_cnt, 64 );

    for(auto& row : __ckt.defRowStor) {
        auto sitePtr = __ckt.lefSiteMap.find( string(row.macro()) ) ;
        if( sitePtr == __ckt.lefSiteMap.end() ) {
            cout << "\n** ERROR:  Cannot find SITE in lef files: " 
                << row.macro() << endl;
            exit(1);
        }

        ROW* curRow = &row_st[i];
    
        new (curRow) ROW();

        curRow->pmin.Set( (row.x()+offsetX)/unitX, (row.y()+offsetY)/unitY, 0 );
        curRow->size.Set( 
                INT_CONVERT( 
                    l2d * __ckt.lefSiteStor[sitePtr->second].sizeX() / unitX ) 
                * row.xNum(),
                INT_CONVERT(
                    l2d * __ckt.lefSiteStor[sitePtr->second].sizeY() / unitY ) 
                * row.yNum(),
                1 );

        curRow->pmax.Set( 
                curRow->pmin.x + curRow->size.x,
                curRow->pmin.y + curRow->size.y,
                1 );

        if( isFirst ) {
            grow_pmin.Set(curRow->pmin);
            grow_pmax.Set(curRow->pmax);

            rowHeight = INT_CONVERT( l2d* __ckt.lefSiteStor[sitePtr->second].sizeY()/unitY );

            if( INT_CONVERT( l2d * __ckt.lefSiteStor[sitePtr->second].sizeY()) % 
                    INT_CONVERT( unitY ) != 0 ) {
                int _rowHeight = INT_CONVERT( l2d * __ckt.lefSiteStor[sitePtr->second].sizeY());
                cout << endl 
                    << "** ERROR: rowHeight \% unitY is not zero,  " << endl
                    << "          ( rowHeight : " 
                    << _rowHeight << ", unitY : " << INT_CONVERT(unitY) 
                    << ", rowHeight \% unitY : " 
                    << _rowHeight % INT_CONVERT(unitY) << " )" << endl;
                cout << "          so it causes serious problem in RePlACE" << endl << endl;
                cout << "          Use custom unitY in here using -unitY command, as a divider of rowHeight" << endl;
                exit(1);
            }
            isFirst = false;
        }
        else {
            grow_pmin.x = min( grow_pmin.x, (prec)curRow->pmin.x);
            grow_pmin.y = min( grow_pmin.y, (prec)curRow->pmin.y);

            grow_pmax.x = max( grow_pmax.x, (prec)curRow->pmax.x);
            grow_pmax.y = max( grow_pmax.y, (prec)curRow->pmax.y);
        }

        curRow->x_cnt = row.xNum();

        // 
        // this is essential to DetailPlacer
        //
        // scale down to 1.
        //  
        curRow->site_wid = curRow->site_spa = SITE_SPA = row.xStep()/unitX; 

        curRow->ori = string( orientStr(row.orient()) );
        curRow->isXSymmetry = (__ckt.lefSiteStor[sitePtr->second].
                hasXSymmetry() )? true : false;

        curRow->isYSymmetry = (__ckt.lefSiteStor[sitePtr->second].
                hasYSymmetry() )? true : false;

        curRow->isR90Symmetry = (__ckt.lefSiteStor[sitePtr->second].
                has90Symmetry() )? true : false;

//        curRow->Dump(to_string(i));
        i++;
    }
    cout << "INFO:  ROW SIZE: ( " << SITE_SPA << ", " 
        << rowHeight << " ) "<< endl;
    cout << "INFO:  #ROW: " << row_cnt << endl;
}

// MS-Placement requires this!
void GenerateFullRow(Circuit::Circuit &__ckt) {
    cout << "INFO:  NEW ROW IS CREATING... (Mixed-Size Mode) " << endl;
    mkl_free(row_st);
    // Newly create the all ROW area for floorplan.
    // In here, I've used DESIGN FE_CORE_BOX_LL_X statements in PROPERTYDEFINITIONS 
    DieRect dieArea = GetDieFromProperty();
    if( dieArea.isNotInitialize() ) {
        dieArea = GetDieFromDieArea(); 
    } 
    if( dieArea.isNotInitialize() ) {
        cout << "ERROR: DIEAREA ERROR" << endl;
        exit(1);
    }
    
    cout << "INFO:  DIEAREA: (" << dieArea.llx << " " << dieArea.lly << ") - (" << dieArea.urx << " " << dieArea.ury << ")" << endl; 
    
    
    // this portion is somewhat HARD_CODING
    // it regards there only one SITE definition per each design!

    defiRow* minRow = &__ckt.defRowStor[0];
    
    // get the lowest one
    for(auto curRow : __ckt.defRowStor ){
        if( minRow->y() < minRow->y() ) {
            minRow = &curRow;
        }
    }

    auto sitePtr = __ckt.lefSiteMap.find( string(minRow->macro()) ) ;
    if( sitePtr == __ckt.lefSiteMap.end() ) {
        cout << "\n** ERROR:  Cannot find SITE in lef files: " 
            << minRow->macro() << endl;
        exit(1);
    }

    int siteX = INT_CONVERT( 
                    l2d * __ckt.lefSiteStor[sitePtr->second].sizeX() / unitX );
    int siteY = INT_CONVERT(
                    l2d * __ckt.lefSiteStor[sitePtr->second].sizeY() / unitY );

    int rowCntX = (dieArea.urx - dieArea.llx) / siteX; 
    int rowCntY = (dieArea.ury - dieArea.lly) / siteY;

    int rowSizeX = rowCntX * siteX;
    int rowSizeY = rowHeight = siteY;

    row_cnt = rowCntY;
    row_st = (ROW*) mkl_malloc( sizeof(ROW)*row_cnt, 64 );

    /////////////////////////
    // HARD CODE PART!!!!!
    //
    // 0 ~ cut : 0 ~ 1640064 
    // cut ~ 1640064 : 451298 ~ 1640064
    /*
    
    int cut = 635904;
    int cutY = (1.0*cut / unitY) / siteY;
    int rowCntX1 = (1.0*(1640064 - 0) / unitX) / siteX;
    int rowCntX2 = (1.0*(1640064 - 451298) / unitX) / siteX;
    int rowSizeX1 = rowCntX1 * siteX;
    int rowSizeX2 = rowCntX2 * siteX;
    
//    cout << siteX << endl;
//    exit(0);

    for(int i=0; i<cutY; i++) {
        ROW* curRow = &row_st[i];
        new (curRow) ROW();

        curRow->pmin.Set(llx, lly + i * siteY, 0);
        curRow->size.Set(rowSizeX1, rowSizeY, 1);
        curRow->pmax.Set(llx + rowSizeX1, lly + i * siteY + rowSizeY, 1);

        if( i == 0 ) {
            grow_pmin.Set(curRow->pmin);
        }

        curRow->x_cnt = rowCntX1;
        curRow->site_wid = curRow->site_spa = SITE_SPA = minRow->xStep()/unitX;
        curRow->Dump(to_string(i));
    } 

    for(int i=cutY; i<row_cnt; i++) {
        ROW* curRow = &row_st[i];
        new (curRow) ROW();

        curRow->pmin.Set(451298 / unitX , lly + i * siteY, 0);
        curRow->size.Set(rowSizeX2, rowSizeY, 1);
        curRow->pmax.Set(451298 / unitX + rowSizeX2, lly + i * siteY + rowSizeY, 1);

        if( i == row_cnt-1 ) {
            grow_pmax.Set(curRow->pmax);
        }

        curRow->x_cnt = rowCntX2;
        curRow->site_wid = curRow->site_spa = SITE_SPA = minRow->xStep()/unitX;
        curRow->Dump(to_string(i));
    }

//    exit(1);
    */
    //
    ///////////////////////////
    // ORIGINAL!!!

    for(int i=0; i<row_cnt; i++) {
        ROW* curRow = &row_st[i];
        new (curRow) ROW();

        curRow->pmin.Set(dieArea.llx, dieArea.lly + i * siteY, 0);
        curRow->size.Set(rowSizeX, rowSizeY, 1);
        curRow->pmax.Set(dieArea.llx + rowSizeX, dieArea.lly + i * siteY + rowSizeY, 1);

        if( i == 0 ) {
            grow_pmin.Set(curRow->pmin);
        }
        else if( i == row_cnt-1 ) {
            grow_pmax.Set(curRow->pmax);
        }

        curRow->x_cnt = rowCntX;
        curRow->site_wid = curRow->site_spa = SITE_SPA = minRow->xStep()/unitX;
//        curRow->Dump(to_string(i));
    }
    
    cout << "INFO:  NEW ROW SIZE: ( " << SITE_SPA << ", " 
        << rowHeight << " ) "<< endl;
    cout << "INFO:  NEW #ROW: " << row_cnt << endl;
}



// return Component Index
inline int GetDefComponentIdx( Circuit::Circuit &__ckt, string& compName ) {
    auto dcPtr = __ckt.defComponentMap.find( compName );
    if( dcPtr == __ckt.defComponentMap.end() ) {
        cout << "** ERROR:  Net Instance ( "<< compName 
            << " ) does not exist in COMPONENT statement (defComponentMap) " << endl;
        exit(1);
    }
    return dcPtr->second;
}

// return Macro Index
inline int GetLefMacroIdx( Circuit::Circuit &__ckt, string& macroName ) {
    auto mcPtr = __ckt.lefMacroMap.find( macroName );
    if( mcPtr == __ckt.lefMacroMap.end() ) {
        cout << "** ERROR:  Macro Instance ( "
            << macroName  
            << " ) does not exist in COMPONENT statement (lefMacroMap) " << endl;
        exit(1);
    }
    return mcPtr->second;
}

// return Pin Index
inline int GetLefMacroPinIdx( Circuit::Circuit &__ckt, int macroIdx, string& pinName ) {
    auto pinPtr = __ckt.lefPinMapStor[macroIdx].find( pinName );
    if( pinPtr == __ckt.lefPinMapStor[macroIdx].end() ) {
        cout << "** ERROR:  Pin Instance ( "
            << pinName 
            << " ) does not exist in MACRO statement (lefPinMapStor) " << endl;
        exit(1);
    }
    return pinPtr->second;
}

inline int GetDefPinIdx( Circuit::Circuit &__ckt, string& pinName) { 
    auto pinPtr = __ckt.defPinMap.find(pinName);
    if( pinPtr == __ckt.defPinMap.end() ) {
        cout << "** ERROR:  Pin Instance ( "
            << pinName 
            << " ) does not exist in PINS statement (defPinMap) " << endl;
        exit(1);
    }
    return pinPtr->second;
}

//
// helper function for building Net Instance; Get IO info
//
// 0 : Input
// 1 : Output
// 2 : Both (INOUT)
//
// NETS -> COMPONENTS -> MACRO -> PIN -> DIRECTION 
//
// If macroName is given (like verilog), 
// then skip for NETS->COMPONENTS->MACRO 
//
int GetIO( Circuit::Circuit &__ckt, 
           string& instName, string& pinName,
           int macroIdx = INT_MAX) {
    
    if( macroIdx == INT_MAX ) {
        // First, it references COMPONENTS
        int defCompIdx = GetDefComponentIdx(__ckt, instName);

        // Second, it reference MACRO in lef
        string compName = string( __ckt.defComponentStor[defCompIdx].name());
        macroIdx = GetLefMacroIdx( __ckt, compName);
    }

    // Finally, it reference PIN from MACRO in lef
    int pinIdx = GetLefMacroPinIdx( __ckt, macroIdx, pinName );
    
    if( !__ckt.lefPinStor[macroIdx][pinIdx].hasDirection() ) {
        cout << "** WARNING: Macro Instance ( "
             << __ckt.lefMacroStor[macroIdx].name() 
             << " - " 
             << __ckt.lefPinStor[macroIdx][pinIdx].name()
             << " ) does not have Direction! (lefPinStor) " << endl
             << "            Use INOUT(both direction) instead " << endl;
        return 2;
    }

    string pinDir = string(__ckt.lefPinStor[macroIdx][pinIdx].direction());
    return (pinDir == "INPUT")? 0 : (pinDir == "OUTPUT")? 1 : (pinDir == "INOUT")? 2:2;
}

////////
//
// Generate NetInstnace
// 
// helper function for building Net Instance; Get OFFSET coordinate
// NETS -> COMPONENTS -> MACRO -> PORT -> RECT
//
// If macroName is given (like verilog), 
// then skip for NETS->COMPONENTS->MACRO 
//
FPOS GetOffset( Circuit::Circuit& __ckt, 
                string& instName, string& pinName, 
                int macroIdx = INT_MAX) {

//    int defCompIdx = INT_MAX;
    if( macroIdx == INT_MAX ) {
        // First, it references COMPONENTS
        int defCompIdx = GetDefComponentIdx(__ckt, instName);

        // Second, it reference MACRO in lef
        string compName = string( __ckt.defComponentStor[defCompIdx].name());
        macroIdx = GetLefMacroIdx( __ckt, compName);
    }

    // Finally, it reference PIN from MACRO in lef
    int pinIdx = GetLefMacroPinIdx( __ckt, macroIdx, pinName );
    
    // extract center Macro Cell Coordinate
    prec centerX = l2d * __ckt.lefMacroStor[macroIdx].sizeX() / 2;
    prec centerY = l2d * __ckt.lefMacroStor[macroIdx].sizeY() / 2;
    

    bool isFirstMetal = false;

    // prepare for bbox
    prec lx = PREC_MAX, ly = PREC_MAX, ux = PREC_MIN, uy = PREC_MIN;

    // MACRO PORT traverse
    for( int i=0; i<__ckt.lefPinStor[macroIdx][pinIdx].numPorts(); i++ ) {
        lefiGeometries* curGeom = __ckt.lefPinStor[macroIdx][pinIdx].port(i);

        // LAYER Metal1 <-- lefiGeomLayerE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //
        //
        // LAYER VIA1 <-- Skip for this 
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
        //   RECT XXXX XXXX XXXX XXXX <-- lefiGeomRectE
       
        for(int j=0; j<curGeom->numItems(); j++) {
            // First 'Metal' Layer (Metal layer have 'ROUTING' type)
            if( !isFirstMetal && curGeom->itemType(j) == lefiGeomLayerE &&
                    string( __ckt.lefLayerStor[ 
                                __ckt.lefLayerMap[ string(curGeom->getLayer(j)) ] ].type() ) 
                    == "ROUTING" ) {
//                cout << j << " " << curGeom->getLayer(j) << endl;
                isFirstMetal = true;
                continue;
            }
            
            // calculate BBox when rect type
            if( isFirstMetal && curGeom->itemType(j) == lefiGeomRectE ) {
                lefiGeomRect* rect = curGeom->getRect(j);
                // generate BBox
                lx = (lx > rect->xl)? rect->xl : lx;
                ly = (ly > rect->yl)? rect->yl : ly;
                ux = (ux < rect->xh)? rect->xh : ux;
                uy = (uy < rect->yh)? rect->yh : uy;
            }
            // calculate BBox when polygon type
            else if( isFirstMetal && curGeom->itemType(j) == lefiGeomPolygonE ) {
                lefiGeomPolygon* polygon = curGeom->getPolygon(j);
                // generate BBox
                for( int k=0; k < polygon->numPoints; k++ ) {
                    lx = (lx > polygon->x[k])? polygon->x[k] : lx;
                    ly = (ly > polygon->y[k])? polygon->y[k] : ly;
                    ux = (ux < polygon->x[k])? polygon->x[k] : ux;
                    uy = (uy < polygon->y[k])? polygon->y[k] : uy;
                }
            }
            // now, meets another Layer
            else if( isFirstMetal && curGeom->itemType(j) == lefiGeomLayerE ) {
                break;
            }
        }
        // firstMetal was visited
        if( isFirstMetal ) {
            break;
        }
    }
    
    if( !isFirstMetal ) { 
        cout << "**ERROR:  CANNOT find first metal information : " << instName 
            << " - " << pinName << endl;
        exit(1);
    }

    // calculate center offset
    return FPOS( (l2d*(lx + ux)/2 - centerX)/unitX, 
            (l2d*(ly + uy)/2 - centerY)/unitY, 0);
}


////// 
//
// defNetStor -> netInstnace
// defPinStor -> pinInstance
//
void GenerateNetDefOnly(Circuit::Circuit &__ckt) {
    pinCNT = 0;
    netCNT = __ckt.defNetStor.size();
    for(auto& curNet : __ckt.defNetStor) {
        pinCNT += curNet.numConnections();
    }

    // memory reserve
    netInstance = (NET*) mkl_malloc( sizeof(NET)* netCNT, 64 );
    pinInstance = (PIN*) mkl_malloc( sizeof(PIN)* pinCNT, 64 );
    for(int i=0; i<pinCNT; i++) {
        new (&pinInstance[i]) PIN;
    }

    TERM* curTerm = NULL;
    MODULE* curModule = NULL;

    NET* curNet = NULL;
    PIN* curPin = NULL;

    int pinIdx = 0;
    int netIdx = 0;
    int cNetIdx = 0;

    tPinName.resize(terminalCNT);
    mPinName.resize(moduleCNT);

    netNameMap.set_empty_key(INIT_STR);
    for(auto& net : __ckt.defNetStor) {
        if( strcmp (net.name(), "iccad_clk") == 0 ||
                strcmp( net.name(), "clk") == 0 ||
                strcmp( net.name(), "clock") == 0 ) {

            cout << "** WARNING:  " << net.name() << " is detected. "
                 << " It'll be automatically excluded"
                 << " (defNetStor)" << endl; 

            clockNetsDef.push_back( &net - &__ckt.defNetStor[0]);
            continue;
        }
        // skip for Power/Ground/Reset Nets
        if( net.hasUse() ) {
            continue;
        }

        curNet = &netInstance[netIdx];
        string netName = string(net.name());
//        ReplaceStringInPlace(netName, "[", "\\[");
//        ReplaceStringInPlace(netName, "]", "\\]");
//        ReplaceStringInPlace(netName, "/", "\\/");

        strcpy( curNet->name, netName.c_str() );
//        cout << "copied net Name: " << curNet->name << endl;
        netNameMap[ netName ] = netIdx;
        curNet->idx = netIdx;
        curNet->timingWeight = 0;
    
        curNet->pinCNTinObject = net.numConnections();
//        cout << "connection: " << net.numConnections() << endl;
        curNet->pin = (PIN**) mkl_malloc( 
                        sizeof(PIN*)*net.numConnections(), 64);
        
        for(int i=0; i<net.numConnections(); i++) {
            // net.pin(i) itself exists on termInst
            if( strcmp( net.instance(i), "PIN") == 0 ) {

                auto mtPtr = moduleTermMap.find( string(net.pin(i)) );
                if(mtPtr == moduleTermMap.end()) {
                    cout << "** ERROR:  Net Instance ( "<<net.pin(i) 
                         << " ) does not exist in PINS statement (moduleTermMap) " << endl;
                    exit(1);
                }
               
                int termIdx = mtPtr->second.second; 
                curTerm = &terminalInstance[ termIdx ];
//                assert( termIdx < terminalCNT );
//                cout << "foundTermIdx: " << termIdx << endl;
               
                // extract Input/Output direction from PINS statement
                auto pinPtr = __ckt.defPinMap.find( string(net.pin(i)) );
                if( pinPtr == __ckt.defPinMap.end() ) {
                    cout << "** ERROR:  Net Instance ( "<<net.pin(i) 
                         << " ) does not exist in PINS statement (defPinMap) " << endl;
                    exit(1);
                }

                int defPinIdx = pinPtr->second;
//                cout << "foundPinIdx: " << defPinIdx << endl;
//                assert( defPinIdx < __ckt.defPinStor.size() );

                int io = INT_MAX;
                if( !__ckt.defPinStor[defPinIdx].hasDirection() ) {
                    io = 2; // both direction
                }
                else {
                    // input : 0
                    // output : 1
                    io = (strcmp( __ckt.defPinStor[defPinIdx].
                                    direction(), "INPUT") == 0 )? 0 : 1;
                }
               
                // pin Instnace mapping 
                curPin = &pinInstance[pinIdx];
                curNet->pin[i] = curPin;

                // save terminal pin Name into tPinName
                tPinName[termIdx].push_back(string(net.pin(i)));

                AddPinInfoForModuleAndTerminal( 
                    &curTerm->pin, &curTerm->pof,
                    curTerm->pinCNTinObject++,
                    FPOS(0, 0, 0), termIdx,  
                    netIdx, i, pinIdx++, 
                    io, true); 
            }
            // net.instance(i) must exist on moduleInst or termInst
            else {
                string instName = string(net.instance(i));
                string pinName = string(net.pin(i));

                auto mtPtr = moduleTermMap.find( instName );
                if(mtPtr == moduleTermMap.end()) {
                    cout << "** ERROR:  Net Instance ( "<< instName
                         << " ) does not exist in COMPONENTS/PINS statement "
                         << "(moduleTermMap) " << endl;
                    exit(1);
                }
               
                // Get Offset Infomation 
                FPOS curOffset = GetOffset( __ckt, instName, pinName );
//                cout << "instName: " << instName << ", pinName: " << pinName << endl;
//                curOffset.Dump("offset"); cout << endl; 
                
                // Get IO information
                int io = GetIO( __ckt, instName, pinName );
//                cout << io << endl;
                
                // pin Instnace mapping 
                curPin = &pinInstance[pinIdx];
                curNet->pin[i] = curPin;

                // module case
                if( mtPtr->second.first ) {
                    curModule = &moduleInstance[mtPtr->second.second];
                
                    // save module pin Name into mPinName
                    mPinName[mtPtr->second.second].push_back(pinName);

                    AddPinInfoForModuleAndTerminal( 
                            &curModule->pin, &curModule->pof,
                            curModule->pinCNTinObject++,
                            curOffset, curModule->idx,  
                            netIdx, i, pinIdx++, 
                            io, false); 
                }
                // terminal case
                else {
                    curTerm = &terminalInstance[mtPtr->second.second];
                    
                    // save terminal pin Name into tPinName
                    tPinName[mtPtr->second.second].push_back(pinName);

                    AddPinInfoForModuleAndTerminal( 
                            &curTerm->pin, &curTerm->pof,
                            curTerm->pinCNTinObject++,
                            curOffset, curTerm->idx,  
                            netIdx, i, pinIdx++, 
                            io, true); 
                }
            }
        }
        netIdx++;
    }

    // update instance number
    pinCNT = pinIdx;
    netCNT = netIdx;

    // memory cutting (shrink)
    netInstance = (NET*) mkl_realloc( netInstance, sizeof(NET)* netCNT );
    pinInstance = (PIN*) mkl_realloc( pinInstance, sizeof(PIN)* pinCNT );
    
    cout << "INFO:  #NET: " << netCNT << ", #PIN: " << pinCNT << endl;
}



//////
//
// defPinStor -> pinInstnace -- (verilog cannot store pin's coordinate)!
// verilog -> netInstnace
//
void GenerateNetDefVerilog(Circuit::Circuit &__ckt) {
    using namespace verilog;

    FILE* verilogInput = fopen(verilogCMD.c_str(), "rb");
    if( !verilogInput ) {
        cout << "** ERROR:  Cannot open verilog file: " << verilogCMD << endl;
        exit(1);
    }

    verilog::verilog_parser_init();
    int result = verilog::verilog_parse_file( verilogInput );
    if( result != 0 ) {
        cout << "** ERROR:  Verilog Parse Failed: " << verilogCMD << endl;
        exit(1);
    }

    verilog::verilog_source_tree* tree = verilog::yy_verilog_source_tree;
    verilog::verilog_resolve_modules(tree);
   
    if( tree->modules->items > 2 ) {
        cout << "WARNING:  # Modules in Verilog: " << tree->modules->items
             << ", so only use the 'First' module" << endl;
    }

    // extract the '1st' module
    ast_module_declaration* module = (ast_module_declaration*)ast_list_get(tree->modules, 0);

    /*
    // IO map update 
    // (used in Net Instance Mapping)
    dense_hash_map<string, int> ioMap; 
    ioMap.set_empty_key(INIT_STR);

    int portCnt = module->module_ports->items;
    for(int i=0; i< portCnt; i++) {
        ast_port_declaration* port = (ast_port_declaration*)ast_list_get(module->module_ports, i);

        // 
        // input : 0
        // output : 1
        // inout : 2
        //
        int io = (port->direction == PORT_INPUT)? 0:
                 (port->direction == PORT_OUTPUT)? 1:2; 

        for(int j=0; j<port->port_names->items; j++) {
            ast_identifier name = (ast_identifier) ast_list_get( port->port_names, j);
            char* tmp = ast_identifier_tostring(name);
            ioMap[string(tmp)] = io;
            free(tmp);
        }
    }
    */

    // 
    // first : netName
    // second : NetInfo (macroName / compName / pinName)
    netMap.set_empty_key(INIT_STR);
  
    bool warnIccadClk = false;
    bool warnLcb = false;
    string lcbStr = "lcb_";

    // pinCNT check && netMap build
    pinCNT = 0;
    for(int i=0; i<module->module_instantiations->items; i++) {
        ast_module_instantiation* inst =
            (ast_module_instantiation*) ast_list_get( module->module_instantiations, i);

        ast_module_instance* innerInst = 
            (ast_module_instance*) ast_list_get( inst->module_instances, 0 );
      
        /*
         * memory leak?  
        char* macroNamePtr = ast_identifier_tostring( inst-> module_identifer );
        char* compNamePtr = ast_identifier_tostring( innerInst->instance_identifier );
        string macroName = string(macroNamePtr);
        string compName = string(compNamePtr);

        int macroIdx = GetLefMacroIdx( __ckt, macroName );
        int compIdx = GetDefComponentIdx( __ckt, compName );
        free(macroNamePtr);
        free(compNamePtr);
        */
        
        string macroName =  string(ast_identifier_tostring( inst-> module_identifer ));
        string compName = string(ast_identifier_tostring( innerInst->instance_identifier ));

        int macroIdx = GetLefMacroIdx( __ckt, macroName );
        int compIdx = GetDefComponentIdx( __ckt, compName );
       
        if( !innerInst->port_connections ) {
            cout << "** WARNING:  " << compName << " has empty connection in verilog."<< endl;
            continue;
        } 
        pinCNT += innerInst->port_connections->items;
        for(int j=0; j<innerInst->port_connections->items; j++) {
            ast_port_connection* port =
                (ast_port_connection*) ast_list_get( innerInst->port_connections, j );

            char* netNamePtr = ast_identifier_tostring(
                                port->expression->primary->value.identifier);

            char* rhs = NULL;
            if( port->expression->right ) { 
                rhs = strdup("[");
                char* cont = ast_expression_tostring(port->expression->right);
                strcat( rhs , cont );
                free(cont);
            }

            if( rhs ) {
                strcat( netNamePtr, rhs );
                free(rhs);
            }
            string netName( netNamePtr );
            free(netNamePtr);
        
//            if( !warnIccadClk && netName == "iccad_clk" ) {
//                cout << "** WARNING:  iccad_clk is detected. It'll be automatically excluded"
//                    << " (netMap)" << endl;
//                warnIccadClk = true;
//                continue;
//            }

//            if( !warnLcb && std::equal( lcbStr.begin(), lcbStr.end(), netName.begin()) ) {
//                cout << "** WARNING:  lcb_ prefix is detected. It'll be automatically excluded"
//                    << " (netMap)" << endl;
//                warnLcb = true;
//                continue;
//            }

//            if( netName == "iccad_clk" ||
//                    std::equal( lcbStr.begin(), lcbStr.end(), netName.begin() ) ) {
//                continue;
//            }
//            if( netName == "iccad_clk" ) {
//                continue;
//            }

            char* pinNamePtr = ast_identifier_tostring(port-> port_name);
            string pinName = string(pinNamePtr);
            int pinIdx = GetLefMacroPinIdx( __ckt, macroIdx, pinName );
            free(pinNamePtr);
             
            // netMap build 
            if(netMap.find( netName ) == netMap.end()) {
                vector<NetInfo> tmpStor;
                tmpStor.push_back( NetInfo(macroIdx, compIdx, pinIdx ) );
                netMap[netName] = tmpStor;   
            }
            else {
                netMap[netName].push_back( NetInfo( macroIdx, compIdx, pinIdx ) );
            }
        }

    }

    // insert PIN information into nets based on the DefPinStor
    if( __ckt.defPinStor.size() > 0 ) {
        for(auto& curPin : __ckt.defPinStor) {

            auto nmPtr = netMap.find( string(curPin.netName()) );
            if( nmPtr == netMap.end() ) {
                netMap[ string(curPin.netName()) ].push_back( 
                        NetInfo( INT_MAX, INT_MAX, &curPin - &__ckt.defPinStor[0] ) );
            }
            else {
                // save defPinStor's index
                nmPtr->second.push_back( 
                        NetInfo( INT_MAX, INT_MAX, &curPin - &__ckt.defPinStor[0] ) );
            }
            pinCNT++;
        }
    }
    // this is worst cases. It must reference all information from Verilog files
    // TODO
    else {
        if( module->continuous_assignments ) {
            int assignCnt = module->continuous_assignments->items;
            for(int i=0; i<assignCnt; i++) {
//                ast_continuous_assignment* 
            }
        }
    }
    //    cout << "NET MAP BUILD DONE" << endl; 
    netCNT = netMap.size();
    
    // memory reserve
    netInstance = (NET*) mkl_malloc( sizeof(NET)* netCNT, 64 );
    pinInstance = (PIN*) mkl_malloc( sizeof(PIN)* pinCNT, 64 );

    PIN* curPin = NULL;

    struct MODULE* curModule = NULL;
    struct TERM* curTerm = NULL;

    int netIdx = 0;
    int pinIdx = 0;

    tPinName.resize(terminalCNT);
    mPinName.resize(moduleCNT);
  
    netNameMap.set_empty_key(INIT_STR);

    // net traverse
    // netMap is silimar with __ckt.defNetStor
    for(auto& net : netMap) {
        
        if( net.first ==  "iccad_clk" ||
            net.first ==  "clk" ||
            net.first == "clock" ) {

            cout << "** WARNING:  " << net.first << " is detected. "
                 << " It'll be automatically excluded"
                 << " (defNetStor)" << endl; 

            clockNetsVerilog.push_back( net.first );
            continue;
        }
//        cout << net.first << endl; 
        NET* curNet = &netInstance[netIdx];
        netNameMap[ net.first ] = netIdx;
        curNet->idx = netIdx;

        curNet->pinCNTinObject = net.second.size();
        curNet->pin = (PIN**) mkl_malloc( 
                        sizeof(PIN*)*net.second.size(), 64);
        // net name initialize
        strcpy( curNet->name, net.first.c_str() );
        // all names are saved in here.
        for(auto& connection : net.second) {

            int connectIdx = &connection - &net.second[0];
            /////////////// Net Info Mapping part
            //
            // This means external PIN cases
            if( connection.macroIdx == INT_MAX && connection.compIdx == INT_MAX) {
                string pinName = string(__ckt.defPinStor[connection.pinIdx].pinName());
                auto mtPtr = moduleTermMap.find( pinName );
                if(mtPtr == moduleTermMap.end()) {
                    cout << "** ERROR:  Net Instance ( "
                        << pinName
                        << " ) does not exist in COMPONENTS/PINS statement "
                        << "(moduleTermMap) " << endl;
                    exit(1);
                }

                // terminal Index setting
                int termIdx = mtPtr->second.second;

                int io = INT_MAX;
                if( !__ckt.defPinStor[connection.pinIdx].hasDirection() ) {
                    io = 2; // both direction
                }
                else {
                    // input : 0
                    // output : 1
                    io = (strcmp( __ckt.defPinStor[connection.pinIdx].
                                    direction(), "INPUT") == 0 )? 0 : 1;
                }
                
                // pin Instnace mapping 
                curPin = &pinInstance[pinIdx];
                curNet->pin[ connectIdx ] = curPin;
                
                curTerm = &terminalInstance[termIdx];

                // save terminal pin Name into tPinName
                tPinName[termIdx].push_back(pinName);

                AddPinInfoForModuleAndTerminal( 
                        &curTerm->pin, &curTerm->pof,
                        curTerm->pinCNTinObject++,
                        FPOS(0, 0, 0), curTerm->idx,  
                        netIdx, connectIdx, pinIdx++, 
                        io, true); 

            }
            else {
                // 
                // find current component is in module or terminal
                //
                auto mtPtr = moduleTermMap.find( string(__ckt.defComponentStor[connection.compIdx].id() ) );
                if(mtPtr == moduleTermMap.end()) {
                    cout << "** ERROR:  Net Instance ( "
                        << __ckt.defComponentStor[connection.compIdx].id()
                        << " ) does not exist in COMPONENTS/PINS statement "
                        << "(moduleTermMap) " << endl;
                    exit(1);
                }

                // pin Info set
                curPin = &pinInstance[pinIdx];
                curNet->pin[ connectIdx ] = curPin;

                string compName = string( __ckt.defComponentStor[connection.compIdx].id());
                string pinName = string( __ckt.lefPinStor[connection.macroIdx][connection.pinIdx].name()); 

                FPOS curOffset = GetOffset( __ckt, 
                        compName, 
                        pinName, 
                        connection.macroIdx );
                int io = GetIO( __ckt, 
                        compName, 
                        pinName, 
                        connection.macroIdx ); 

                // module case
                if( mtPtr->second.first ) {
                    curModule = &moduleInstance[mtPtr->second.second];
                    mPinName[mtPtr->second.second].push_back(pinName);

                    AddPinInfoForModuleAndTerminal( 
                            &curModule->pin, &curModule->pof,
                            curModule->pinCNTinObject++,
                            curOffset, curModule->idx,  
                            netIdx, connectIdx, pinIdx++, 
                            io, false); 
                }
                // terminal case
                else {
                    curTerm = &terminalInstance[mtPtr->second.second];
                    tPinName[mtPtr->second.second].push_back(pinName);

                    AddPinInfoForModuleAndTerminal( 
                            &curTerm->pin, &curTerm->pof,
                            curTerm->pinCNTinObject++,
                            curOffset, curTerm->idx,  
                            netIdx, connectIdx, pinIdx++, 
                            io, true); 
                }

            }
            // curOffset.Dump("curOffset");
            // cout << macroName << " " << compName << " " << pinName << endl;
        }
        netIdx++;
    }
    netCNT = netIdx;
    pinCNT = pinIdx;
   
    netInstance = (NET*) mkl_realloc( netInstance, sizeof(NET)* netCNT );
    // pinInstance is not re-allocable!!!

    /*
    // fully traverse
    for(int i=0; i<module->module_instantiations->items; i++) {
        ast_module_instantiation* inst =
            (ast_module_instantiation*) ast_list_get( module->module_instantiations, i);

        char* macroNamePtr = ast_identifier_tostring( inst->module_identifier );
        string macroName(macroNamePtr);
        free(macroNamePtr);

        // 
        // it is only composed of module-module pair
        //
        ast_module_instance* innerInst = 
            (ast_module_instance*) ast_list_get( inst->module_instances, 0 );
        
        char* compNamePtr = ast_identifier_tostring( innerInst-> instance_identifier );
        string compName(compNamePtr);
        free(compNamePtr); 

        // 
        // find current component is in module or terminal
        //
        auto mtPtr = moduleTermMap.find( compName );
        if(mtPtr == moduleTermMap.end()) {
            cout << "** ERROR:  Net Instance ( "<< compName 
                << " ) does not exist in COMPONENTS/PINS statement "
                << "(moduleTermMap) " << endl;
            exit(1);
        }

        for(int j=0; j<innerInst->port_connections->items; j++) {
            ast_port_connection* port =
                (ast_port_connection*) ast_list_get( innerInst->port_connections, j );

            char* pinNamePtr = ast_identifier_tostring(port->port_name);
            string pinName( pintNamePtr );
            free(pinNamePtr);

            char* netNamePtr = ast_identifier_tostring(
                                port->expression->primary->value.identifier);
            string netName( netNamePtr );
            free(netNamePtr);
            
//            ast_identifier_tostring( port-> );

        }
    }
    

    */ 
//    ast_free_all();
    cout << "INFO:  SUCCESSFULLY VERILOG PARSED" << endl; 
}

// 
// this function is indended for 
// re-parsing the result of detail-placer
//
// ReadPlLefDef -- moduleTermMap -- lefdefIO.cpp
// ReadBookshelf -- nodesMap -- bookshelfIO.cpp
//
void ReadPl(const char* fileName) {
    if( auxCMD == "" && lefStor.size() != 0 && defCMD != "" ) {
        // it references moduleTermMap 
        ReadPlLefDef(fileName);
    }
    else if( auxCMD != "" && lefStor.size() == 0 && defCMD == "") {
        // it references nodesMap
        ReadPlBookshelf(fileName);
    }
}

//////
// 
// Read PL back from Detailed Placer -- to use moduleTermMap again
//
// See also ReadPlBookshelf(const char* fileName) in bookShelfIO.cpp
//
void ReadPlLefDef(const char* fileName) {
    FILE *fp = fopen(fileName, "r");
    if( !fp ) {
        runtimeError( fileName + string( " is not Exist!" ) );
    }

    char buf[BUF_SZ], name[BUF_SZ];
    int moduleID = 0;
    FPOS pof;
    while(fgets(buf, BUF_SZ, fp)) {
        char* token = strtok(buf, " \t\n");
        if(!token || token[0] == '#' || !strcmp(token, "UCLA"))
            continue;

        strcpy(name, token);

        if(name[0] == 'f' && name[1] == 'a' && name[2] == 'k' && name[3] == 'e')
            continue;

        // if(!getCellIndex ( name , & moduleID , & isTerminal,
        // &isTerminalNI))
        // continue;

        bool isModule = false;
        auto mtPtr = moduleTermMap.find(string(name));
        if(mtPtr == moduleTermMap.end()) {
            continue;
        }
        else {
            isModule = mtPtr->second.first;
            moduleID = mtPtr->second.second;
        }

        if(isModule) {
            MODULE* curModule = &moduleInstance[moduleID];

            if(curModule->flg == Macro)
                continue;

            token = strtok(NULL, " \t\n");
            curModule->pmin.x = atof(token);

            token = strtok(NULL, " \t\n");
            curModule->pmin.y = atof(token);

            curModule->pmax.x = curModule->pmin.x + curModule->size.x;
            curModule->pmax.y = curModule->pmin.y + curModule->size.y;

            curModule->center.x = 0.5 * (curModule->pmin.x + curModule->pmax.x);
            curModule->center.y = 0.5 * (curModule->pmin.y + curModule->pmax.y);

            for(int i = 0; i < curModule->pinCNTinObject; i++) {
                PIN* pin = curModule->pin[i];
                pof = curModule->pof[i];
                pin->fp.x = curModule->center.x + pof.x;
                pin->fp.y = curModule->center.y + pof.y;
            }
        }
        else {
            continue;
        }
    }

    fclose(fp);
}

TIMING_NAMESPACE_OPEN

// copy scale down parameter into Timing Instance
void Timing::SetLefDefEnv() {
    _unitX = unitX;
    _unitY = unitY;
    _l2d = l2d;
}

void Timing::WriteSpefClockNet( stringstream& feed ) {
    if( clockNetsDef.size() != 0) {
        WriteSpefClockNetDef( feed );
    }
    else if( clockNetsVerilog.size() != 0 ) {
        WriteSpefClockNetVerilog( feed );
    }
}

void Timing::WriteSpefClockNetDef( stringstream& feed ) { 
    for(auto& curClockNet : clockNetsDef) {
        defiNet& net = __ckt.defNetStor[curClockNet];
        feed << "*D_NET " << net.name() << " 0" << endl << "*CONN" << endl;
        for(int i=0; i<net.numConnections(); i++) {
            // net.pin(i) itself exists on termInst
            if( strcmp( net.instance(i), "PIN") == 0 ) {
                feed << "*P " << net.pin(i);
                
                // extract Input/Output direction from PINS statement
                auto pinPtr = __ckt.defPinMap.find( string(net.pin(i)) );
                if( pinPtr == __ckt.defPinMap.end() ) {
                    cout << "** ERROR:  Net Instance ( "<<net.pin(i) 
                         << " ) does not exist in PINS statement (defPinMap) " << endl;
                    exit(1);
                }

                int defPinIdx = pinPtr->second;
//                cout << "foundPinIdx: " << defPinIdx << endl;
//                assert( defPinIdx < __ckt.defPinStor.size() );

                int io = INT_MAX;
                if( !__ckt.defPinStor[defPinIdx].hasDirection() ) {
                    cout << "** ERROR:  PINS statement in DEF, " << net.pin(i)
                        << " have no direction!!" << endl;
                    exit(0);
//                    io = 2; // both direction
                }
                else {
                    // input : 0
                    // output : 1
                    io = (strcmp( __ckt.defPinStor[defPinIdx].
                                    direction(), "INPUT") == 0 )? 0 : 1;
                }
                feed << (( io == 0)? " I" : " O") << endl;
            }
            else {
                string instName = string(net.instance(i));
                string pinName = string(net.pin(i));
                
                // Get IO information
                int io = GetIO( __ckt, instName, pinName );
                
                feed << "*I " << instName << ":" << pinName << (( io == 0)? " I" : " O") << endl;
            }
        }
        feed << "*END" << endl << endl; 
    }
}

void Timing::WriteSpefClockNetVerilog( stringstream& feed ) {
    for(auto& curClockNet : clockNetsVerilog) {
        feed << "*D_NET " << curClockNet << " 0" << endl << "*CONN" << endl;
        for( auto& connection : netMap[curClockNet]) {
            /////////////// Net Info Mapping part
            // 
            // find current component is in module or terminal
            //
            if( connection.macroIdx == INT_MAX && connection.compIdx == INT_MAX) {
                string pinName = string(__ckt.defPinStor[connection.pinIdx].pinName());
                auto mtPtr = moduleTermMap.find( pinName );
                if(mtPtr == moduleTermMap.end()) {
                    cout << "** ERROR:  Net Instance ( "
                        << pinName
                        << " ) does not exist in COMPONENTS/PINS statement "
                        << "(moduleTermMap) " << endl;
                    exit(1);
                }

                // terminal Index setting
                int termIdx = mtPtr->second.second;

                int io = INT_MAX;
                if( !__ckt.defPinStor[connection.pinIdx].hasDirection() ) {
                    io = 2; // both direction
                }
                else {
                    // input : 0
                    // output : 1
                    io = (strcmp( __ckt.defPinStor[connection.pinIdx].
                                    direction(), "INPUT") == 0 )? 0 : 1;
                }
                feed << "*P " << pinName;
                feed << (( io == 0)? " I" : " O") << endl;
            }
            else {
                auto mtPtr = moduleTermMap.find( 
                        string(__ckt.defComponentStor[connection.compIdx].id()) );

                if(mtPtr == moduleTermMap.end()) {
                    cout << "** ERROR:  Net Instance ( "
                        << __ckt.defComponentStor[connection.compIdx].id()
                        << " ) does not exist in COMPONENTS/PINS statement "
                        << "(moduleTermMap) " << endl;
                    exit(1);
                }
                
                string compName = 
                    string( __ckt.defComponentStor[connection.compIdx].id());
                string pinName = 
                    string( __ckt.lefPinStor[connection.macroIdx][connection.pinIdx].name()); 

                int moduleIdx = mtPtr->second.second;

                // 
                // located in terminal cases
                if( !mtPtr->second.first ) {
                    TERM* curTerminal = &terminalInstance[moduleIdx];
                    // outer pin
                    if( curTerminal->isTerminalNI ) {
                        feed << "*P " << compName;
                    }
                    // inner fixed cell's pin
                    else {
                        feed << "*I " << compName << ":" << pinName;
                    }
                }  
                // inner module pin
                else {
                    feed << "*I " << compName << ":" << pinName;
                }

                int io = GetIO( __ckt, 
                        compName, 
                        pinName, 
                        connection.macroIdx ); 
                feed << (( io == 0)? " I" : " O") << endl;

            }
        }
        feed << "*END" << endl << endl; 
    }
}

void Timing::UpdateSpefClockNetVerilog() {
    for(auto& curClockNet : clockNetsVerilog) {
        char* netNamePtr = new char[curClockNet.length()+1];
        strcpy(netNamePtr, curClockNet.c_str());
        sta::Net* net = sta::spef_reader->findNet(netNamePtr);
        SpefTriple* netCap = new SpefTriple ( 0 );
        sta::spef_reader->dspfBegin( net , netCap );
        sta::spef_reader->dspfFinish();
    }
}

TIMING_NAMESPACE_CLOSE

void DummyCellInfo::SetScaleDownParam() {
  unitX_ = unitX;
  unitY_ = unitY;
  l2d_ = l2d;
}

void DummyCellInfo::SetCircuitInst() {
  this->ckt_ = &__ckt;
}

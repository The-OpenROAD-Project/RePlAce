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

#include "replace_private.h"
#include "lefdefIO.h"
#include "bookShelfIO.h"

#include "timing.h"
#include "timingSta.h"
#include <iostream>
#include <boost/functional/hash.hpp>

using namespace std;
using Replace::NetInfo;
using Replace::Circuit;


Replace::Circuit::Circuit() : lefManufacturingGrid(DBL_MIN) {
#ifdef USE_GOOGLE_HASH
  lefMacroMap.set_empty_key(INIT_STR);
  lefViaMap.set_empty_key(INIT_STR);
  lefLayerMap.set_empty_key(INIT_STR);
  lefSiteMap.set_empty_key(INIT_STR);

  defComponentMap.set_empty_key(INIT_STR);
  defPinMap.set_empty_key(INIT_STR);
  defRowY2OrientMap.set_empty_key(INT_MAX);
#endif
};
  
Replace::Circuit::Circuit(vector< string >& lefStor, string defFilename, bool isVerbose)
      : lefManufacturingGrid(DBL_MIN) {
#ifdef USE_GOOGLE_HASH
    lefMacroMap.set_empty_key(INIT_STR);
    lefViaMap.set_empty_key(INIT_STR);
    lefLayerMap.set_empty_key(INIT_STR);
    lefSiteMap.set_empty_key(INIT_STR);

    defComponentMap.set_empty_key(INIT_STR);
    defPinMap.set_empty_key(INIT_STR);
    defRowY2OrientMap.set_empty_key(INT_MAX);
#endif

    Init(lefStor, defFilename, isVerbose);
  }

void Replace::Circuit::Init(vector< string >& lefStor, string defFilename,
    bool isVerbose) {
  PrintProcBegin("LefParsing");
  ParseLef(lefStor, isVerbose);
  PrintProcEnd("LefParsing");
  PrintProcBegin("DefParsing");
  ParseDef(defFilename, isVerbose);
  PrintProcEnd("DefParsing");
}


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
namespace Replace {
  Circuit __ckt;
}

using Replace::__ckt;

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
static prec offsetX = PREC_MAX;
static prec offsetY = PREC_MAX;

// module & terminal Makp
//
// first : nodeName
// second, first : isModule (true -> moduleInst // false -> termInst)
// second, second : corresponding index
//
static HASH_MAP< string, pair< bool, int > > moduleTermMap;

//
// Metal1 Name.
// It usually depends on the lef files
//
static string metal1Name;

// helper function for LEF/DEF in siteorient
static const char* 
orientStr(int orient) {
  switch(orient) {
    case 0:
      return ((const char*)"N");
    case 1:
      return ((const char*)"W");
    case 2:
      return ((const char*)"S");
    case 3:
      return ((const char*)"E");
    case 4:
      return ((const char*)"FN");
    case 5:
      return ((const char*)"FW");
    case 6:
      return ((const char*)"FS");
    case 7:
      return ((const char*)"FE");
  };
  return ((const char*)"BOGUS");
}

// orient coordinate shift 
inline static std::pair<float, float> 
GetOrientPoint( float x, float y, float w, float h, int orient ) {
  switch(orient) {
    case 0: // North
      return std::make_pair(x, y); 
    case 1: // West
      return std::make_pair(h-y, x);
    case 2: // South
      return std::make_pair(w-x, h-y); // x-flip, y-flip
    case 3: // East
      return std::make_pair(y, w-x);
    case 4: // Flipped North
      return std::make_pair(w-x, y); // x-flip
    case 5: // Flipped West
      return std::make_pair(y, x); // x-flip from West
    case 6: // Flipped South
      return std::make_pair(x, h-y); // y-flip
    case 7: // Flipped East
      return std::make_pair(h-y, w-x); // y-flip from West
  }
  
  cout << "Warning: Weird Orientation: Set North... " << endl;
  return std::make_pair(x, y); // default
}

// Get Lower-left coordinates from rectangle's definition
inline static std::pair<float, float> 
GetOrientLowerLeftPoint( float lx, float ly, float ux, float uy,
   float w, float h, int orient ) {
  switch(orient) {
    case 0: // North
      return std::make_pair(lx, ly); // verified
    case 1: // West
      return GetOrientPoint(lx, uy, w, h, orient);
    case 2: // South
      return GetOrientPoint(ux, uy, w, h, orient);
    case 3: // East
      return GetOrientPoint(ux, ly, w, h, orient);
    case 4: // Flipped North
      return GetOrientPoint(ux, ly, w, h, orient); // x-flip
    case 5: // Flipped West
      return GetOrientPoint(lx, ly, w, h, orient); // x-flip from west
    case 6: // Flipped South
      return GetOrientPoint(lx, uy, w, h, orient); // y-flip
    case 7: // Flipped East
      return GetOrientPoint(ux, uy, w, h, orient); // y-flip from west
  }

  cout << "Warning: Weird Orientation: Set North... " << endl;
  return std::make_pair(lx, ly); // default
}

// orient coordinate shift 
inline static std::pair<float, float> 
GetOrientSize( float w, float h, int orient ) {
  switch(orient) {
    // East, West, FlipEast, FlipWest
    case 1:
    case 3:
    case 5:
    case 7:
      return std::make_pair(h, w);
    // otherwise 
    case 0:
    case 2:
    case 4:
    case 6:
      return std::make_pair(w, h); 
  }

  cout << "Warning: Weird Orientation: Set North... " << endl;
  return std::make_pair(w, h); 
}

// for Saving verilog information
// declaired here to send verilog -> timing inst..
static HASH_MAP< string, vector< NetInfo >, boost::hash< std::string > >
    netMap;

// save clockNets' index
// for DEF parsing
static vector< int > clockNetsDef;

// for Verilog parsing
static vector< string > clockNetsVerilog;

// from main.cpp
void ParseInput() {
  if(auxCMD == "" && lefStor.size() != 0 && defName != "") {
    inputMode = InputMode::lefdef;
    ParseLefDef();
  }
  else if(auxCMD != "" && lefStor.size() == 0 && defName == "") {
    inputMode = InputMode::bookshelf;
    ParseBookShelf();
  }

  if(verilogName != "") {
    SetVerilogTopModule();
  }
}

inline static bool IsPrecEqual(prec a, prec b) {
  return std::fabs(a - b) < std::numeric_limits< float >::epsilon();
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
  SetDefDbu( __ckt.defUnit );
  PrintInfoInt("DefUnit", GetDefDbu());

  if(__ckt.lefLayerStor.size() == 0) {
    cout << "\n** ERROR : LAYER statements not exists in lef file!" << endl;
    exit(1);
  }

  metal1Name = "";
  // Metal1 Name extract
  for(auto& curLayer : __ckt.lefLayerStor) {
    if(!curLayer.hasType()) {
      continue;
    }
    if(strcmp(curLayer.type(), "ROUTING") == 0) {
      if( metal1Name == "" ) {
        metal1Name = string(curLayer.name());
        break;
      }
    }
  }
  PrintInfoString("LefMetal1Name", metal1Name);

#ifdef USE_GOOGLE_HASH
  // required for FIXED components(DEF) -> *.shape bookshelf
  shapeMap.set_empty_key(INIT_STR);

  // required for net(DEF) -> module fast access
  moduleTermMap.set_empty_key(INIT_STR);
#endif

  // unitX, unitY setting : RowHeight / 9 --> Converted Height: 9
  // RePlAce's Nesterov iteration parameter is optimized when cell height is around 9.
  PrintInfoString("DefSiteName", __ckt.defRowStor[0].macro());

  auto sitePtr = __ckt.lefSiteMap.find(string(__ckt.defRowStor[0].macro()));
  if( sitePtr == __ckt.lefSiteMap.end() ) {
    cout << "ERROR:  CANNOT find SITE: " 
      << __ckt.defRowStor[0].macro() << " IN DEF" << endl;
    exit(1);
  }

  int siteSizeY = INT_CONVERT( GetDefDbu() * __ckt.lefSiteStor[sitePtr->second].sizeY() );

  SetUnitY(1.0 * siteSizeY / 9.0f);
  SetUnitX(GetUnitY());

  PrintInfoPrec( "ScaleDownUnit", GetUnitX() );

  // offsetX & offsetY : Minimum coordinate of ROW's x/y
  if(IsPrecEqual(offsetX, PREC_MAX)) {
    int rowMin = INT_MAX;
    for(auto& curRow : __ckt.defRowStor) {
      rowMin = (rowMin > curRow.x()) ? curRow.x() : rowMin;
    }

    // Note that OffsetX should follow siteSizeY, because unitX = unitY
    // and unitY comes from siteSizeY
    SetOffsetX( (rowMin % siteSizeY == 0)? 
              0 : siteSizeY - (rowMin % siteSizeY) );
  }

  if(IsPrecEqual(offsetY, PREC_MAX)) { 
    int rowMin = INT_MAX;
    for(auto& curRow : __ckt.defRowStor) {
      rowMin = (rowMin > curRow.y()) ? curRow.y() : rowMin;
    }
   
    SetOffsetY( (rowMin % siteSizeY == 0)? 
              0 : siteSizeY - (rowMin % siteSizeY) );
  }


  PrintInfoPrecPair( "OffsetCoordi", GetOffsetX(), GetOffsetY());

  // init for local global vars
  l2d = GetDefDbu();
  unitX = GetUnitX();
  unitY = GetUnitY();
  offsetX = GetOffsetX();
  offsetY = GetOffsetY();
}

void SetVerilogTopModule() {
  verilogTopModule = __ckt.defDesignName;
}

void ParseLefDef() {
  // for input parse only
  //
  // Replace::Circuit __ckt(lefStor, defName, "");
  //
  __ckt.Init(lefStor, defName, ( gVerbose >= 1 ));

  SetParameter();

  GenerateRow(__ckt);
  GenerateModuleTerminal(__ckt);
  
  GenerateDummyCell(__ckt);
  GenerateFullRow(__ckt);

  if(__ckt.defNetStor.size() > 0) {
    GenerateNetDefOnly(__ckt);
  }
  else {
    cout << "** ERROR:  Cannot Find any Net information "
      << "(Check NETS statement in DEF) " << endl;
      exit(1);
  }

  PrintProc("LEF/DEF Parsing Success!");

  // do weird things..
  FPOS tier_min, tier_max;
  int tier_row_cnt = 0;

  get_mms_3d_dim(&tier_min, &tier_max, &tier_row_cnt);
  transform_3d(&tier_min, &tier_max, tier_row_cnt);
  post_read_3d();
}

void WriteDef(const char* defOutput) {
  MODULE* curModule = NULL;

  // moduleInstnace -> defComponentStor
  for(int i = 0; i < moduleCNT; i++) {
    curModule = &moduleInstance[i];
    auto cmpPtr = __ckt.defComponentMap.find(string(curModule->Name()));
    if(cmpPtr == __ckt.defComponentMap.end()) {
      cout << "** ERROR:  Module Instance ( " << curModule->Name()
           << " ) does not exist in COMPONENT statement (defComponentMap) "
           << endl;
      exit(1);
    }

    // update into PLACED status
    if(!__ckt.defComponentStor[cmpPtr->second].isPlaced()) {
      __ckt.defComponentStor[cmpPtr->second].setPlacementStatus(
          DEFI_COMPONENT_PLACED);
    }

    // update into corresponding coordinate
    //
    // unitX & unitY is used to recover scaling

    int x = INT_CONVERT(curModule->pmin.x * unitX) - offsetX;
    int y = INT_CONVERT(curModule->pmin.y * unitY) - offsetY;

    // x-coordinate, y-coordinate, cell-orient
    auto orientPtr = __ckt.defRowY2OrientMap.find(y);

    __ckt.defComponentStor[cmpPtr->second].setPlacementLocation(
        x, y,
        (orientPtr != __ckt.defRowY2OrientMap.end()) ? orientPtr->second : 0);

    // cout << curModule->name << ": " << curModule->pmin.x << " " <<
    // curModule->pmin.y << endl;
  }

  FILE* fp = fopen(defOutput, "w");
  if(!fp) {
    cout << "** ERROR:  Cannot open " << defOutput << " (DEF WRITING)" << endl;
    exit(1);
  }

  __ckt.WriteDef(fp);
}

////////
//
// Generate Module&Terminal Instance
//
// helper function for FIXED cells(in DEF Components), which can have multiple
// rectangles,
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
bool AddShape(int defCompIdx, int lx, int ly,
    float w, float h, int orient) {
  string compName = string(__ckt.defComponentStor[defCompIdx].id());
  string macroName = string(__ckt.defComponentStor[defCompIdx].name());

  // reference MACRO in lef
  auto mcPtr = __ckt.lefMacroMap.find(macroName);
  if(mcPtr == __ckt.lefMacroMap.end()) {
    cout << "** ERROR:  Macro Instance ( " << macroName
         << " ) does not exist in COMPONENT statement (lefMacroMap) " << endl;
    exit(1);
  }

  // Check whether MACRO have CLASS statement
  int macroIdx = mcPtr->second;
  if(!__ckt.lefMacroStor[macroIdx].hasClass()) {
    cout << "** ERROR: Macro Instance ( " << __ckt.lefMacroStor[macroIdx].name()
         << " ) does not have Class! (lefMacroStor) " << endl;
    exit(1);
  }

  //
  // only for MACRO TYPE == BLOCK cases
  // & OBS command must exist in LEF
  //
  if(strcmp(__ckt.lefMacroStor[macroIdx].macroClass(), "BLOCK") != 0 ||
     __ckt.lefObsStor[macroIdx].size() == 0) {
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

    for(int j = 0; j < curGeom->numItems(); j++) {
      // Meets 'Metal1' Layer
      if(curGeom->itemType(j) == lefiGeomLayerE &&
         string(curGeom->getLayer(j)) == metal1Name) {
        //                cout << j << " " << curGeom->getLayer(j) << endl;
        isMetal1 = true;
  
        continue;
      }

      // calculate BBox
      if(isMetal1 && curGeom->itemType(j) == lefiGeomRectE) {
        lefiGeomRect* rect = curGeom->getRect(j);

        // shape Name -> bunch of shapeStor's index
        if(shapeMap.find(compName) == shapeMap.end()) {
          vector< int > tmpStor;
          tmpStor.push_back(shapeStor.size());
          shapeMap[compName] = tmpStor;
        }
        else {
          shapeMap[compName].push_back(shapeStor.size());
        }
        totalShapeCount++;

        std::pair<float, float> rectLxLy = 
          GetOrientLowerLeftPoint( rect->xl, rect->yl, rect->xh, rect->yh, 
                                    w, h, orient );

        std::pair<float, float> rectSize = 
          GetOrientSize( rect->xh - rect->xl, rect->yh - rect->yl, orient );

        // finally pushed into shapeStor
        /*
        shapeStor.push_back(SHAPE(
            string("shape_") + to_string(shapeCnt), compName, 
            shapeStor.size(),
            (l2d * rect->xl + lx + offsetX) / unitX,  // lx
            (l2d * rect->yl + ly + offsetY) / unitY,  // ly
            l2d * (rect->xh - rect->xl) / unitX,      // xWidth
            l2d * (rect->yh - rect->yl) / unitY));    // yWidth
        */

        // finally pushed into shapeStor
        shapeStor.push_back(SHAPE(
            string("shape_") + to_string(shapeCnt), compName, 
            shapeStor.size(),
            (l2d * rectLxLy.first + lx + offsetX) / unitX,  // lx
            (l2d * rectLxLy.second + ly + offsetY) / unitY,  // ly
            l2d * rectSize.first / unitX,      // xWidth
            l2d * rectSize.second / unitY));    // yWidth

        shapeCnt++;
      }
      // now, meets another Layer
      else if(isMetal1 && curGeom->itemType(j) == lefiGeomLayerE) {
        break;
      }
    }
    // firstMetal was visited
    if(isMetal1) {
      break;
    }
  }

  if(isMetal1) {
    numNonRectangularNodes++;
  }

  return (isMetal1) ? true : false;
}

void SetSizeForObsMacro(int macroIdx, MODULE* curModule, int orient) {
  bool isMetal1 = false;

  prec llx = PREC_MAX, lly = PREC_MAX;
  prec urx = PREC_MIN, ury = PREC_MIN;

  // original macro size info to handle orientation 
  float w = __ckt.lefMacroStor[macroIdx].sizeX();
  float h = __ckt.lefMacroStor[macroIdx].sizeY();

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

    for(int j = 0; j < curGeom->numItems(); j++) {
      // Meets 'Metal1' Layer
      if(curGeom->itemType(j) == lefiGeomLayerE &&
         string(curGeom->getLayer(j)) == metal1Name) {
        //                cout << j << " " << curGeom->getLayer(j) << endl;
        isMetal1 = true;
        continue;
      }

      // calculate BBox
      if(isMetal1 && curGeom->itemType(j) == lefiGeomRectE) {
        lefiGeomRect* rect = curGeom->getRect(j);
        
        // Orientation handling
        std::pair<float, float> rectLxLy = 
          GetOrientLowerLeftPoint( rect->xl, rect->yl, rect->xh, rect->yh, 
                                    w, h, orient );

        std::pair<float, float> rectSize = 
          GetOrientSize( rect->xh - rect->xl, rect->yh - rect->yl, orient );

        // Extract BBox information
        llx = ( llx > rectLxLy.first )?   rectLxLy.first : llx;
        lly = ( lly > rectLxLy.second )?  rectLxLy.second : lly;

        urx = ( urx < rectLxLy.first + rectSize.first )?  
                  rectLxLy.first + rectSize.first : urx;
        ury = ( ury < rectLxLy.second + rectSize.second )?  
                  rectLxLy.second + rectSize.second : ury;

//        cout << rect->xl << " " << rect->yl << endl;
//        cout << rect->xh << " " << rect->yh << endl;
      }
      // now, meets another Layer
      else if(isMetal1 && curGeom->itemType(j) == lefiGeomLayerE) {
        break;
      }
    }
    // firstMetal was visited
    if(isMetal1) {
      break;
    }
  }

  if( !isMetal1 || llx == PREC_MAX ) {
    cout << "ERROR: LEF's OBS statements don't have metal1 statements!!" << endl;
    exit(1);
  }

  // finalize modules' Size 
  curModule->size.Set( 
      (l2d * (urx - llx)) / unitX, 
      (l2d * (ury - lly)) / unitY );
}

//
// defComponentStor(non FIXED cell) -> moduleInstance
// defComponentStor(FIXED cell), defPinStor -> terminalInstnace
//
// terminal_pmin & terminal_pmax must be updated...
void GenerateModuleTerminal(Replace::Circuit& __ckt) {
  moduleInstance =
      (MODULE*)malloc(sizeof(MODULE) * __ckt.defComponentStor.size());

  // to fast traverse when building TerminalInstance
  vector< int > fixedComponent;

  MODULE* curModule = NULL;
  defiComponent* curComp = NULL;
  lefiMacro* curMacro = NULL;

  moduleCNT = 0;

  // not 1-to-1 mapping (into moduleInstnace), so traverse by index
  for(size_t i = 0; i < __ckt.defComponentStor.size(); i++) {
    curComp = &(__ckt.defComponentStor[i]);

    curModule = &moduleInstance[moduleCNT];
    new(curModule) MODULE();

    if(curComp->isFixed()) {
      fixedComponent.push_back(i);
      continue;
    }

    // pmin info update
    if(curComp->isPlaced()) {
      // only when is already placed
      curModule->pmin.Set(((prec)curComp->placementX() + offsetX) / unitX,
                          ((prec)curComp->placementY() + offsetY) / unitY);
    }
    else {
      curModule->pmin.SetZero();
    }

    auto macroPtr = __ckt.lefMacroMap.find(string(curComp->name()));
    if(macroPtr == __ckt.lefMacroMap.end()) {
      cout << "\n** ERROR : Cannot find MACRO cell in lef files: "
           << curComp->name() << endl;
      exit(1);
    }

    curMacro = &__ckt.lefMacroStor[macroPtr->second];

    if(!curMacro->hasSize()) {
      cout << "\n** ERROR : Cannot find MACRO SIZE in lef files: "
           << curComp->name() << endl;
      exit(1);
    }

    if(strcmp(curMacro->macroClass(), "BLOCK") == 0 &&
       __ckt.lefObsStor[macroPtr->second].size() != 0) {
      // cout << "BLOCK/OBS: " << curMacro->name() << endl;

      // Orient handling for MS-Placement
      SetSizeForObsMacro(macroPtr->second, curModule, 
          curComp->isUnplaced()? 
          0 : curComp->placementOrient());
    }
    else {
      // size info update from LEF Macro
      curModule->size.Set(l2d * curMacro->sizeX() / unitX,
                          l2d * curMacro->sizeY() / unitY);
    }

    if(fabs(curModule->size.y - rowHeight) > PREC_EPSILON) {
      defMacroCnt++;
//      cout << "MACRO: " << curComp->id() << " " << curModule->size.y << endl;
    }

    // set half_size
    curModule->half_size.Set(curModule->size.x / 2, curModule->size.y / 2);

    // set center coordi
    curModule->center.SetAdd(curModule->pmin, curModule->half_size);

    // set pinMax coordi
    curModule->pmax.SetAdd(curModule->pmin, curModule->size);

    // set area
    curModule->area = curModule->size.GetProduct();

    // set which tier
    curModule->tier = 0;

    string moduleName = curComp->id();

    // set Name
//    strcpy(curModule->name, moduleName.c_str());
    moduleNameStor.push_back( moduleName );

    // set Index
    curModule->idx = moduleCNT;

    moduleTermMap[moduleName] = make_pair(true, moduleCNT);

    // check
    //        curModule->Dump(string("Macro ") + to_string(moduleCNT));
    moduleCNT++;
  }
  //    cout << moduleCNT << endl;

  // memory cutting
//  moduleInstance =
//      (MODULE*)realloc(moduleInstance, sizeof(MODULE) * moduleCNT);

  //
  // Terminal Update
  //

  // Check the Blockage
  vector< int > blockageIdxStor;
  for(auto& curBlockage : __ckt.defBlockageStor) {
    int idx = &curBlockage - &__ckt.defBlockageStor[0];
    if(curBlockage.hasPlacement()) {
      blockageIdxStor.push_back(idx);
    }
  }

  TERM* curTerm = NULL;
  terminalCNT = 0;
  terminalInstance = (TERM*)malloc(
      sizeof(TERM) * (blockageIdxStor.size() + fixedComponent.size() +
                      __ckt.defPinStor.size()));

  // for fixed cells.
  for(auto& curIdx : fixedComponent) {
    curTerm = &terminalInstance[terminalCNT];
    new(curTerm) TERM();

    curComp = &(__ckt.defComponentStor[curIdx]);

    curTerm->idx = terminalCNT;

    // pmin info update
    curTerm->pmin.Set(((prec)curComp->placementX() + offsetX) / unitX,
                      ((prec)curComp->placementY() + offsetY) / unitY);

    //        cout << "Fixed: " << curComp->name() << endl;
    auto macroPtr = __ckt.lefMacroMap.find(string(curComp->name()));
    if(macroPtr == __ckt.lefMacroMap.end()) {
      cout << "\n** ERROR : Cannot find MACRO cell in lef files: "
           << curComp->name() << endl;
      exit(1);
    }

    int macroIdx = macroPtr->second;
    curMacro = &__ckt.lefMacroStor[macroIdx];

    if(!curMacro->hasSize()) {
      cout << "\n** ERROR : Cannot find MACRO SIZE in lef files: "
           << curComp->name() << endl;
      exit(1);
    }
    
    // check whether this nodes contains sub-rectangular sets
    // Note that unplaced cells is regarded as North orientation (default)
    AddShape(curIdx, curComp->placementX(), curComp->placementY(),
                  curMacro->sizeX(), curMacro->sizeY(), // for orient 
                  curComp->isUnplaced()?
                  0 : curComp->placementOrient());          // for orient 

   
    // Orient consideration
    // Note that unplaced cells is regarded as North orientation (default)
    std::pair<float, float> macroSize = 
      GetOrientSize(curMacro->sizeX(), curMacro->sizeY(), 
          curComp->isUnplaced()? 
          0 : curComp->placementOrient()); 

    curTerm->size.Set(l2d * macroSize.first   / unitX,
                      l2d * macroSize.second  / unitY);

    // Always visible blockage: terminal
    curTerm->isTerminalNI = false;

    // set center coordi
    curTerm->center.x = curTerm->pmin.x + curTerm->size.x / 2;
    curTerm->center.y = curTerm->pmin.y + curTerm->size.y / 2;

    // set pinMax coordi
    curTerm->pmax.SetAdd(curTerm->pmin, curTerm->size);

    // set area
    curTerm->area = curTerm->size.GetProduct();

    string termName = curComp->id();

    // set Name
    terminalNameStor.push_back(termName); 

    // set tier
    moduleTermMap[termName] = make_pair(false, terminalCNT);

    terminalCNT++;
  }

  // for pin
  for(auto& curPin : __ckt.defPinStor) {
    curTerm = &terminalInstance[terminalCNT];
    new(curTerm) TERM();

//    strcpy(curTerm->name, curPin.pinName());
    terminalNameStor.push_back(curPin.pinName());

    curTerm->idx = terminalCNT;
    curTerm->IO = (strcmp(curPin.direction(), "INPUT") == 0) ? 0 : 1;
    curTerm->pmin.Set((curPin.placementX() + offsetX) / unitX,
                      (curPin.placementY() + offsetY) / unitY);
    curTerm->isTerminalNI = true;

    // since size == 0, pmin == center == pmax;
    curTerm->pmax.Set(curTerm->pmin);
    curTerm->center.Set(curTerm->pmin);

    moduleTermMap[string(curTerm->Name())] = make_pair(false, terminalCNT);

    //        curTerm->Dump();
    terminalCNT++;
  }

  int blockageCnt = 0;
  for(auto& curBlockIdx : blockageIdxStor) {
    string blockName = "replace_blockage_" + to_string(blockageCnt++);
    curTerm = &terminalInstance[terminalCNT];
    new(curTerm) TERM();

//    strcpy(curTerm->name, blockName.c_str());
    terminalNameStor.push_back(blockName);

    curTerm->idx = terminalCNT;
    curTerm->IO = 2;

    defiBlockage* curBlockage = &__ckt.defBlockageStor[curBlockIdx];
    if(curBlockage->numRectangles() > 0) {
      curTerm->pmin.Set((curBlockage->xl(0) + offsetX) / unitX,
                        (curBlockage->yl(0) + offsetY) / unitY);
      curTerm->pmax.Set((curBlockage->xh(0) + offsetX) / unitX,
                        (curBlockage->yh(0) + offsetY) / unitY);
      curTerm->center.Set((curTerm->pmin.x + curTerm->pmax.x) / 2.0,
                          (curTerm->pmin.y + curTerm->pmax.y) / 2.0);
    }
    else {
      cout << "** ERROR: RePlAce requires RECT in BLOCKAGES (DEF) " << endl;
      exit(1);
    }
    curTerm->isTerminalNI = false;
    curTerm->size.Set(curTerm->pmax.x - curTerm->pmin.x,
                      curTerm->pmax.y - curTerm->pmin.y);
    curTerm->area = curTerm->size.GetProduct();
    // since size == 0, pmin == center == pmax;
    // curTerm->pmax.Set(curTerm->pmin);
    // curTerm->center.Set(curTerm->pmin);

    moduleTermMap[blockName] = make_pair(false, terminalCNT);

    curTerm->Dump();
    terminalCNT++;
  }
  PrintInfoInt("MacroCount", defMacroCnt);
  PrintInfoInt("NumModules", moduleCNT);
  PrintInfoInt("NumTerminals", terminalCNT);
}

/////////////////////////////////////////////////////
// ArrayInfo class

int ArrayInfo::GetCoordiX( float x ) {
  return INT_CONVERT ( ((x) - lx_ ) / siteSizeX_ );
}

int ArrayInfo::GetCoordiY( float y ) {
  return INT_CONVERT ( ((y) - ly_ ) / siteSizeY_ );
}

int ArrayInfo::GetLowerX( float x ) {
  return int( ((x) - lx_ ) / siteSizeX_ );
}

int ArrayInfo::GetLowerY( float y ) {
  return int( ((y) - ly_ ) / siteSizeY_ );
}

int ArrayInfo::GetUpperX (float x) {
  float val = ( x - lx_ ) / siteSizeX_;
  int intVal = int( val );
  return (fabs(val-intVal) <= 0.0001)? intVal : intVal+1;
}

int ArrayInfo::GetUpperY (float y) {
  float val = ( y - ly_ ) / siteSizeY_;
  int intVal = int( val );
  return (fabs(val-intVal) <= 0.0001)? intVal : intVal+1;
}

/////////////////////////////////////////////////////

// Dummy Cell Generation procedure to be aware of fragmented-Row
void GenerateDummyCell(Replace::Circuit& __ckt) {
  defiRow *minRow = &__ckt.defRowStor[0];
  // get the lowest one
  for(auto curRow : __ckt.defRowStor) {
    if(minRow->y() < minRow->y()) {
      minRow = &curRow;
    }
  }

  auto sitePtr = __ckt.lefSiteMap.find(string(minRow->macro()));
  if(sitePtr == __ckt.lefSiteMap.end()) {
    cout << "\n** ERROR:  Cannot find SITE in lef files: " << minRow->macro()
         << endl;
    exit(1);
  }
  // Set Site Size
  float siteSizeX_ =
       l2d * __ckt.lefSiteStor[sitePtr->second].sizeX() / unitX;
  float siteSizeY_ =
       l2d * __ckt.lefSiteStor[sitePtr->second].sizeY() / unitY;

  // cout << "siteSize: " << siteSizeX_ << " " << siteSizeY_ << endl;

  //Set LayoutArea
  DieRect dieRect_ = GetCoreFromRow();

  // Set Array Counts 
  int numX_ = INT_CONVERT( (dieRect_.urx - dieRect_.llx) / siteSizeX_ );
  int numY_ = INT_CONVERT( (dieRect_.ury - dieRect_.lly) / siteSizeY_ );
  // cout << "rowCnt: " << numX_ << " " << numY_ << endl;

  float rowSizeY_ = siteSizeY_;
  // cout << "rowSize: " << rowSizeX_ << " " << rowSizeY_ << endl;
 
  // Empty Array Fill 
  ArrayInfo::CellInfo* arr_ = new ArrayInfo::CellInfo[numX_ * numY_];
  for(int i = 0; i < numX_ * numY_; i++) {
    arr_[i] = ArrayInfo::CellInfo::Empty;
  }

  ArrayInfo ainfo(dieRect_.llx, dieRect_.lly, siteSizeX_, siteSizeY_); 

  // ROW array fill
  for(int i=0; i<row_cnt ; i++) {
    ROW* curRow = &row_st[i];

    // cout << "ROW lx: " << ainfo.GetCoordiX(curRow->pmin.x) 
    //   << " ux: " << ainfo.GetUpperX(curRow->pmax.x);
    // cout << " ly: " << ainfo.GetCoordiY(curRow->pmin.y) 
    //   << " uy: " << ainfo.GetUpperY(curRow->pmax.y) << endl;

    for(int i = ainfo.GetCoordiX(curRow->pmin.x); 
        i < ainfo.GetCoordiX(curRow->pmax.x); i++) {
      for(int j = ainfo.GetCoordiY(curRow->pmin.y); 
          j < ainfo.GetCoordiY(curRow->pmax.y); j++) {
        arr_[j * numX_ + i] = ArrayInfo::CellInfo::Row;
      }
    } 
  }
  
  // TERM array fill
  for(int i=0; i<terminalCNT; i++) {
    TERM* curTerm = &terminalInstance[i];
    if( curTerm -> isTerminalNI ) {
      continue;
    }

    // TERM-nonShape array fill
    if( shapeMap.find( curTerm->Name() ) == shapeMap.end()) {
      // curTerm->pmin.Dump("curTermPmin");
      // curTerm->pmax.Dump("curTermPmax");
      // cout << "TERM lx: " << ainfo.GetLowerX(curTerm->pmin.x) 
      //   << " ux:" << ainfo.GetUpperX(curTerm->pmax.x);
      // cout << " ly: " << ainfo.GetLowerY(curTerm->pmin.y) 
      //   << " uy:" << ainfo.GetUpperY(curTerm->pmax.y)
      //   << endl;
      for(int i = ainfo.GetLowerX(curTerm->pmin.x); 
          i < ainfo.GetUpperX(curTerm->pmax.x); i++) {
        // out of CoreArea placed-cell handling
        if( i < 0 || i >= numX_ ) {
          continue;
        }
        for(int j = ainfo.GetLowerY(curTerm->pmin.y); 
            j < ainfo.GetUpperY(curTerm->pmax.y); j++) {

          // out of CoreArea placed-cell handling
          if( j < 0 || j >= numY_ ) {
            continue;
          }
          arr_[j * numX_ + i] = ArrayInfo::CellInfo::Cell;
        }
      }
    }
    // TERM-Shape array fill
    else {
      for(auto& curIdx : shapeMap[curTerm->Name()]) {
        float llx = shapeStor[curIdx].llx,
              lly = shapeStor[curIdx].lly,
              width = shapeStor[curIdx].width,
              height = shapeStor[curIdx].height;
        // cout << "SHAPE pmin: " << llx << " " << lly << endl;
        // cout << "      pmax: " << llx + width  << " " << lly + height << endl;
        // cout << "lx: " << ainfo.GetLowerX(llx) 
        //   << " ux:" << ainfo.GetUpperX(llx + width )
        //   << " ly: " << ainfo.GetLowerY(lly) 
        //   << " uy:" << ainfo.GetUpperY(lly + height)
        //   << endl;

        for(int i = ainfo.GetLowerX(llx); 
            i < ainfo.GetUpperX(llx + width); i++) {
          // out of CoreArea placed-cell handling
          if( i < 0 || i >= numX_ ) {
            continue;
          }
          for(int j = ainfo.GetLowerY(lly); 
              j < ainfo.GetUpperY(lly + height); j++) {
            // out of CoreArea placed-cell handling
            if( j < 0 || j >= numY_ ) {
              continue;
            }
            // cout << "[Fixed] i: " << i << ", j: " << j << endl;
            arr_[j * numX_ + i] = ArrayInfo::CellInfo::Cell;
          }
        }
      }
    }
  }


  int idxCnt = terminalCNT;
  vector< TERM > dummyTermStor_;
  for(int j = 0; j < numY_; j++) {
    for(int i = 0; i < numX_; i++) {
      if(arr_[j * numX_ + i] == ArrayInfo::CellInfo::Empty) {
        int startX = i;
        while(i < numX_ && arr_[j * numX_ + i] == ArrayInfo::CellInfo::Empty) {
          i++;
        }
        int endX = i;

        TERM curTerm;
        // strcpy(curTerm.Name(),
        //   string("dummy_inst_" + to_string(dummyTermStor_.size())).c_str());
        //
        terminalNameStor.push_back( 
               string("dummy_inst_" + to_string(dummyTermStor_.size())).c_str());
        curTerm.pmin.Set((prec)(dieRect_.llx + siteSizeX_ * startX),
                         (prec)(dieRect_.lly + j * siteSizeY_));
        curTerm.pmax.Set((prec)(dieRect_.llx + siteSizeX_ * endX),
                         (prec)(dieRect_.lly + j * siteSizeY_ + rowSizeY_));
        curTerm.size.Set((prec)((endX - startX) * siteSizeX_),
                         (prec)(rowSizeY_));
        
        curTerm.center.Set( curTerm.pmin.x + 0.5*curTerm.size.x,
                            curTerm.pmin.y + 0.5*curTerm.size.y); 

        curTerm.isTerminalNI = false;
        curTerm.area = curTerm.size.GetProduct();
        curTerm.idx = idxCnt++;
        dummyTermStor_.push_back(curTerm);
      }
    }
  }
  PrintInfoInt("Inserted Dummy Terms", dummyTermStor_.size());

  // termCnt Updates 
  int prevCnt = terminalCNT;
  terminalCNT += dummyTermStor_.size();
//  terminalInstance = 
//    (TERM*) realloc( terminalInstance, sizeof(TERM) * terminalCNT);
 
  // copy into original instances 
  for(int i=prevCnt; i<terminalCNT; i++) {
    terminalInstance[i] = dummyTermStor_[i - prevCnt];
  }
}



/////////////////////////////////////////////
//  DieRect Instances
//
DieRect GetDieFromProperty( bool isScaleDown ) {
  // Set DieArea from PROPERTYDEFINITIONS
  string llxStr = "FE_CORE_BOX_LL_X", urxStr = "FE_CORE_BOX_UR_X";
  string llyStr = "FE_CORE_BOX_LL_Y", uryStr = "FE_CORE_BOX_UR_Y";

  DieRect retRect;
  for(auto& prop : __ckt.defPropStor) {
    if(strcmp(prop.propType(), "design") != 0) {
      continue;
    }

    if(string(prop.propName()) == llxStr) {
      retRect.llx = (isScaleDown)? 
        (prop.number() * l2d / unitX) : 
        (prop.number() * l2d);
    }
    else if(string(prop.propName()) == llyStr) {
      retRect.lly = (isScaleDown)? 
        (prop.number() * l2d / unitY) : 
        (prop.number() * l2d);
    }
    else if(string(prop.propName()) == urxStr) {
      retRect.urx = (isScaleDown)? 
        (prop.number() * l2d / unitX) : 
        (prop.number() * l2d);
    }
    else if(string(prop.propName()) == uryStr) {
      retRect.ury = (isScaleDown)? 
        (prop.number() * l2d / unitY) : 
        (prop.number() * l2d);
    }
  }
  return retRect;
}

DieRect GetDieFromDieArea( bool isScaleDown ) {
  DieRect retRect;
  defiPoints points = __ckt.defDieArea.getPoint();
  if(points.numPoints >= 2) {
    retRect.llx = (isScaleDown)? points.x[0] / unitX : points.x[0];
    retRect.lly = (isScaleDown)? points.y[0] / unitY : points.y[0];
    retRect.urx = (isScaleDown)? points.x[1] / unitX : points.x[1];
    retRect.ury = (isScaleDown)? points.y[1] / unitY : points.y[1];
  }
  for(int i = 0; i < points.numPoints; i++) {
    cout << points.x[i] << " " << points.y[i] << endl;
  }

  retRect.Dump();
  return retRect;
}

DieRect GetCoreFromRow() {
  
  float minX = FLT_MAX, minY = FLT_MAX;
  float maxX = FLT_MIN, maxY = FLT_MIN;

  for(auto& curRow : __ckt.defRowStor) {
    auto sitePtr = __ckt.lefSiteMap.find(string(curRow.macro()));
    if(sitePtr == __ckt.lefSiteMap.end()) {
      cout << "\n** ERROR:  Cannot find SITE in lef files: " << curRow.macro()
           << endl;
      exit(1);
    }
    lefiSite* lefSite = &__ckt.lefSiteStor[sitePtr->second];

    float curMinX = ( (curRow.x() + offsetX) / unitX );
    float curMinY = ( (curRow.y() + offsetY) / unitY );

    minX = (minX > curMinX) ? curMinX : minX;
    minY = (minY > curMinY) ? curMinY : minY;

    float curMaxX = ( (curRow.x() + offsetX +
          curRow.xNum() * l2d * lefSite->sizeX()) / unitX );
    float curMaxY = ( (curRow.y() + offsetY + 
          curRow.yNum() * l2d * lefSite->sizeY()) / unitY );
  
    maxX = (maxX < curMaxX) ? curMaxX : maxX;
    maxY = (maxY < curMaxY) ? curMaxY : maxY;
  }

  return DieRect(minX, minY, maxX, maxY); 
}

/////////////////////////////////////////////
//  Generate RowInstance
// defRowStor -> rowInstance
// this must update grow_pmin / grow_pmax, rowHeight
void GenerateRow(Replace::Circuit& __ckt) {
  // Get the sites from ROW statements
  // usual cases
  bool isFirst = true;
  int i = 0;

  row_cnt = __ckt.defRowStor.size();
  row_st = (ROW*)malloc(sizeof(ROW) * row_cnt);

  for(auto& row : __ckt.defRowStor) {
    auto sitePtr = __ckt.lefSiteMap.find(string(row.macro()));
    if(sitePtr == __ckt.lefSiteMap.end()) {
      cout << "\n** ERROR:  Cannot find SITE in lef files: " << row.macro()
           << endl;
      exit(1);
    }

    ROW* curRow = &row_st[i];

    new(curRow) ROW();

    curRow->pmin.Set((row.x() + offsetX) / unitX, (row.y() + offsetY) / unitY);
    curRow->size.Set(
        l2d * __ckt.lefSiteStor[sitePtr->second].sizeX() / unitX *
            row.xNum(),
        l2d * __ckt.lefSiteStor[sitePtr->second].sizeY() / unitY *
            row.yNum());

    curRow->pmax.Set(curRow->pmin.x + curRow->size.x,
                     curRow->pmin.y + curRow->size.y);

    if(isFirst) {
      grow_pmin.Set(curRow->pmin);
      grow_pmax.Set(curRow->pmax);

      rowHeight =
          l2d * __ckt.lefSiteStor[sitePtr->second].sizeY() / unitY;

//      if(INT_CONVERT(l2d * __ckt.lefSiteStor[sitePtr->second].sizeY()) %
//             INT_CONVERT(unitY) !=
//         0) {
//        int _rowHeight =
//            INT_CONVERT(l2d * __ckt.lefSiteStor[sitePtr->second].sizeY());
//        cout << endl
//             << "** ERROR: rowHeight \% unitY is not zero,  " << endl
//             << "          ( rowHeight : " << _rowHeight
//             << ", unitY : " << INT_CONVERT(unitY)
//             << ", rowHeight \% unitY : " << _rowHeight % INT_CONVERT(unitY)
//             << " )" << endl;
//        cout << "          so it causes serious problem in RePlACE" << endl
//             << endl;
//        cout << "          Use custom unitY in here using -unitY command, as a "
//                "divider of rowHeight"
//             << endl;
//        exit(1);
//      }
      isFirst = false;
    }
    else {
      grow_pmin.x = min(grow_pmin.x, (prec)curRow->pmin.x);
      grow_pmin.y = min(grow_pmin.y, (prec)curRow->pmin.y);

      grow_pmax.x = max(grow_pmax.x, (prec)curRow->pmax.x);
      grow_pmax.y = max(grow_pmax.y, (prec)curRow->pmax.y);
    }

    curRow->x_cnt = row.xNum();

    //
    // this is essential to DetailPlacer
    //
    // scale down to 1.
    //
    curRow->site_wid = curRow->site_spa = SITE_SPA = row.xStep() / unitX;

    curRow->ori = string(orientStr(row.orient()));
    curRow->isXSymmetry =
        (__ckt.lefSiteStor[sitePtr->second].hasXSymmetry()) ? true : false;

    curRow->isYSymmetry =
        (__ckt.lefSiteStor[sitePtr->second].hasYSymmetry()) ? true : false;

    curRow->isR90Symmetry =
        (__ckt.lefSiteStor[sitePtr->second].has90Symmetry()) ? true : false;

    //        curRow->Dump(to_string(i));
    i++;
  }
  PrintInfoPrecPair( "RowSize", SITE_SPA, rowHeight );
  PrintInfoInt( "NumRows", row_cnt );
}

// MS-Placement requires this!
void GenerateFullRow(Replace::Circuit& __ckt) {
  PrintProcBegin("Generate Un-fragmented Rows");
  if( row_st ) {
    free(row_st);
  }
  // Newly create the all ROW area for floorplan.
  // In here, I've used DESIGN FE_CORE_BOX_LL_X statements in
  // PROPERTYDEFINITIONS
  
  /*
  DieRect dieArea = GetDieFromProperty();
  if(dieArea.isNotInitialize()) {
    dieArea = GetDieFromDieArea();
  }
  if(dieArea.isNotInitialize()) {
    cout << "ERROR: DIEAREA ERROR" << endl;
    exit(1);
  }*/

  DieRect coreArea = GetCoreFromRow();

  PrintInfoPrecPair("CoreAreaLxLy", coreArea.llx, coreArea.lly );
  PrintInfoPrecPair("CoreAreaUxUy", coreArea.urx, coreArea.ury );

  // this portion is somewhat HARD_CODING
  // it regards there only one SITE definition per each design!

  defiRow* minRow = &__ckt.defRowStor[0];

  // get the lowest one
  for(auto curRow : __ckt.defRowStor) {
    if(minRow->y() < minRow->y()) {
      minRow = &curRow;
    }
  }

  auto sitePtr = __ckt.lefSiteMap.find(string(minRow->macro()));
  if(sitePtr == __ckt.lefSiteMap.end()) {
    cout << "\n** ERROR:  Cannot find SITE in lef files: " << minRow->macro()
         << endl;
    exit(1);
  }

  float siteX = l2d * __ckt.lefSiteStor[sitePtr->second].sizeX() / unitX;
  float siteY = l2d * __ckt.lefSiteStor[sitePtr->second].sizeY() / unitY;

  int rowCntX = INT_CONVERT( (coreArea.urx - coreArea.llx) / siteX );
  int rowCntY = INT_CONVERT( (coreArea.ury - coreArea.lly) / siteY );

  float rowSizeX = rowCntX * siteX;
  float rowSizeY = rowHeight = siteY;

  row_cnt = rowCntY;
  row_st = (ROW*)malloc(sizeof(ROW) * row_cnt);

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
      curRow->pmax.Set(451298 / unitX + rowSizeX2, lly + i * siteY + rowSizeY,
1);

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

  for(int i = 0; i < row_cnt; i++) {
    ROW* curRow = &row_st[i];
    new(curRow) ROW();

    curRow->pmin.Set(coreArea.llx, coreArea.lly + i * siteY);
    curRow->size.Set(rowSizeX, rowSizeY);
    curRow->pmax.Set(coreArea.llx + rowSizeX, coreArea.lly + i * siteY + rowSizeY);

    if(i == 0) {
      grow_pmin.Set(curRow->pmin);
    }
    else if(i == row_cnt - 1) {
      grow_pmax.Set(curRow->pmax);
    }

    curRow->x_cnt = rowCntX;
    curRow->site_wid = curRow->site_spa = SITE_SPA = minRow->xStep() / unitX;
//    curRow->Dump(to_string(i));
  }
  
  PrintInfoPrecPair( "RowSize", SITE_SPA, rowHeight );
  PrintInfoInt( "NumRows", row_cnt );
  PrintProcEnd("Generate Un-fragmented Rows"); 
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
int GetIO(Replace::Circuit& __ckt, string& instName, string& pinName,
          int macroIdx = INT_MAX) {
  if(macroIdx == INT_MAX) {
    // First, it references COMPONENTS
    int defCompIdx = GetDefComponentIdx(__ckt, instName);

    // Second, it reference MACRO in lef
    string compName = string(__ckt.defComponentStor[defCompIdx].name());
    macroIdx = GetLefMacroIdx(__ckt, compName);
  }

  // Finally, it reference PIN from MACRO in lef
  int pinIdx = GetLefMacroPinIdx(__ckt, macroIdx, pinName);

  if(!__ckt.lefPinStor[macroIdx][pinIdx].hasDirection()) {
    cout << "** WARNING: Macro Instance ( "
         << __ckt.lefMacroStor[macroIdx].name() << " - "
         << __ckt.lefPinStor[macroIdx][pinIdx].name()
         << " ) does not have Direction! (lefPinStor) " << endl
         << "            Use INOUT(both direction) instead " << endl;
    return 2;
  }

  string pinDir = string(__ckt.lefPinStor[macroIdx][pinIdx].direction());
  return (pinDir == "INPUT")
             ? 0
             : (pinDir == "OUTPUT") ? 1 : (pinDir == "INOUT") ? 2 : 2;
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
FPOS GetOffset(Replace::Circuit& __ckt, string& instName, string& pinName,
               int macroIdx = INT_MAX) {
  //    int defCompIdx = INT_MAX;
  
  // First, it references COMPONENTS
  int defCompIdx = GetDefComponentIdx(__ckt, instName);

  if(macroIdx == INT_MAX) {
    // Second, it reference MACRO in lef
    string compName = string(__ckt.defComponentStor[defCompIdx].name());
    macroIdx = GetLefMacroIdx(__ckt, compName);
  }

  // Finally, it reference PIN from MACRO in lef
  int pinIdx = GetLefMacroPinIdx(__ckt, macroIdx, pinName);
  
  // extract orient information from def
  // default: North orientation
  int orient = 
    (__ckt.defComponentStor[defCompIdx].isUnplaced())? 
    0 : __ckt.defComponentStor[defCompIdx].placementOrient();

  // extract Macro's center points
  // orient-awareness
  float origMacroSizeX = (float) __ckt.lefMacroStor[macroIdx].sizeX();
  float origMacroSizeY = (float) __ckt.lefMacroStor[macroIdx].sizeY();

  std::pair<float, float> macroSize = 
    GetOrientSize( origMacroSizeX, origMacroSizeY, orient );

  // real center points with orient-aware
  prec centerX = l2d * macroSize.first / 2;
  prec centerY = l2d * macroSize.second / 2;

  bool isFirstMetal = false;

  // prepare for bbox
  prec lx = PREC_MAX, ly = PREC_MAX, ux = PREC_MIN, uy = PREC_MIN;

  // MACRO PORT traverse
  for(int i = 0; i < __ckt.lefPinStor[macroIdx][pinIdx].numPorts(); i++) {
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

    for(int j = 0; j < curGeom->numItems(); j++) {
      // First 'Metal' Layer (Metal layer have 'ROUTING' type)
      if(!isFirstMetal && curGeom->itemType(j) == lefiGeomLayerE &&
         string(
             __ckt.lefLayerStor[__ckt.lefLayerMap[string(curGeom->getLayer(j))]]
                 .type()) == "ROUTING") {
        //                cout << j << " " << curGeom->getLayer(j) << endl;
        isFirstMetal = true;
        continue;
      }

      // calculate BBox when rect type
      if(isFirstMetal && curGeom->itemType(j) == lefiGeomRectE) {
        lefiGeomRect* rect = curGeom->getRect(j);
        // generate BBox
        lx = (lx > rect->xl) ? rect->xl : lx;
        ly = (ly > rect->yl) ? rect->yl : ly;
        ux = (ux < rect->xh) ? rect->xh : ux;
        uy = (uy < rect->yh) ? rect->yh : uy;
      }
      // calculate BBox when polygon type
      else if(isFirstMetal && curGeom->itemType(j) == lefiGeomPolygonE) {
        lefiGeomPolygon* polygon = curGeom->getPolygon(j);
        // generate BBox
        for(int k = 0; k < polygon->numPoints; k++) {
          lx = (lx > polygon->x[k]) ? polygon->x[k] : lx;
          ly = (ly > polygon->y[k]) ? polygon->y[k] : ly;
          ux = (ux < polygon->x[k]) ? polygon->x[k] : ux;
          uy = (uy < polygon->y[k]) ? polygon->y[k] : uy;
        }
      }
      // now, meets another Layer
      else if(isFirstMetal && curGeom->itemType(j) == lefiGeomLayerE) {
        break;
      }
    }
    // firstMetal was visited
    if(isFirstMetal) {
      break;
    }
  }

  if(!isFirstMetal) {
    cout << "**ERROR:  CANNOT find first metal information : " << instName
         << " - " << pinName << endl;
    exit(1);
  }
 
  // Get pinCenter points
  // orient-awareness
  std::pair<float, float> pinCenter =
    GetOrientPoint( (lx + ux) / 2, (ly + uy) / 2, 
                    origMacroSizeX, origMacroSizeY, orient  );

  // calculate center offset
  return FPOS((l2d * pinCenter.first - centerX) / unitX,
              (l2d * pinCenter.second - centerY) / unitY);
}

//////
//
// defNetStor -> netInstnace
// defPinStor -> pinInstance
//
void GenerateNetDefOnly(Replace::Circuit& __ckt) {
  PrintProcBegin("DEF Net Parsing");

  pinCNT = 0;
  netCNT = __ckt.defNetStor.size();
  for(auto& curNet : __ckt.defNetStor) {
    pinCNT += curNet.numConnections();
  }

  // memory reserve
  netInstance = (NET*)malloc(sizeof(NET) * netCNT);
  pinInstance = (PIN*)malloc(sizeof(PIN) * pinCNT);
  for(int i = 0; i < pinCNT; i++) {
    new(&pinInstance[i]) PIN;
  }

  TERM* curTerm = NULL;
  MODULE* curModule = NULL;

  NET* curNet = NULL;
  PIN* curPin = NULL;

  int pinIdx = 0;
  int netIdx = 0;

  tPinName.resize(terminalCNT);
  mPinName.resize(moduleCNT);

#ifdef USE_GOOGLE_HASH
  netNameMap.set_empty_key(INIT_STR);
#endif
  for(auto& net : __ckt.defNetStor) {
    if(strcmp(net.name(), "iccad_clk") == 0 || strcmp(net.name(), "clk") == 0 ||
       strcmp(net.name(), "clock") == 0) {
      cout << "** WARNING:  " << net.name() << " is detected. "
           << " It'll be automatically excluded"
           << " (defNetStor)" << endl;

      clockNetsDef.push_back(&net - &__ckt.defNetStor[0]);
      continue;
    }
    // skip for Power/Ground/Reset/Clock Nets
    if(net.hasUse() &&
       (strcmp(net.use(), "CLOCK") == 0 || strcmp(net.use(), "POWER") == 0 ||
        strcmp(net.use(), "GROUND") == 0 || strcmp(net.use(), "RESET") == 0)) {
      continue;
    }

    // skip for empty net definition.
    if( net.numConnections() == 0 ) {
      continue;
    }
   
    // special detection for avoiding signal nets 
    bool isReset = false;
    for(int i = 0; i < net.numConnections(); i++) {
      if( strcmp(net.pin(i), "SI") == 0 || strcmp(net.pin(i), "SE") == 0 ) {
        isReset= true;
        break;
      }
    }
    if( isReset ) {
      continue;
    }


    curNet = &netInstance[netIdx];
    new(curNet) NET();
    string netName = string(net.name());
    //        ReplaceStringInPlace(netName, "[", "\\[");
    //        ReplaceStringInPlace(netName, "]", "\\]");

    // copy original name into netInstance
//    strcpy(curNet->name, netName.c_str());
    netNameStor.push_back( netName );
    
    // But, netNameMap can have escaped strings
//    ReplaceStringInPlace(netName, "\\[", "[");
//    ReplaceStringInPlace(netName, "\\]", "]");
//    ReplaceStringInPlace(netName, "\\/", "/");

    //        cout << "copied net Name: " << curNet->name << endl;
    netNameMap[netName] = netIdx;
    curNet->idx = netIdx;
    curNet->timingWeight = 0;

    curNet->pinCNTinObject = net.numConnections();
    //        cout << "connection: " << net.numConnections() << endl;
    curNet->pin = (PIN**)malloc(sizeof(PIN*) * net.numConnections());

    for(int i = 0; i < net.numConnections(); i++) {
      // net.pin(i) itself exists on termInst
      if(strcmp(net.instance(i), "PIN") == 0) {
        auto mtPtr = moduleTermMap.find(string(net.pin(i)));
        if(mtPtr == moduleTermMap.end()) {
          cout << "** ERROR:  Net Instance ( " << net.pin(i)
               << " ) does not exist in PINS statement (moduleTermMap) "
               << endl;
          exit(1);
        }

        int termIdx = mtPtr->second.second;
        curTerm = &terminalInstance[termIdx];
        //                assert( termIdx < terminalCNT );
        //                cout << "foundTermIdx: " << termIdx << endl;

        // extract Input/Output direction from PINS statement
        auto pinPtr = __ckt.defPinMap.find(string(net.pin(i)));
        if(pinPtr == __ckt.defPinMap.end()) {
          cout << "** ERROR:  Net Instance ( " << net.pin(i)
               << " ) does not exist in PINS statement (defPinMap) " << endl;
          exit(1);
        }

        int defPinIdx = pinPtr->second;
        //                cout << "foundPinIdx: " << defPinIdx << endl;
        //                assert( defPinIdx < __ckt.defPinStor.size() );

        int io = INT_MAX;
        if(!__ckt.defPinStor[defPinIdx].hasDirection()) {
          io = 2;  // both direction
        }
        else {
          // input : 0
          // output : 1
          io = (strcmp(__ckt.defPinStor[defPinIdx].direction(), "INPUT") == 0)
                   ? 0
                   : 1;
        }

        // pin Instnace mapping
        curPin = &pinInstance[pinIdx];
        curNet->pin[i] = curPin;

        // save terminal pin Name into tPinName
        tPinName[termIdx].push_back(string(net.pin(i)));

        AddPinInfoForModuleAndTerminal(&curTerm->pin, &curTerm->pof,
                                       curTerm->pinCNTinObject++, FPOS(0, 0),
                                       termIdx, netIdx, i, pinIdx++, io, true);
      }
      // net.instance(i) must exist on moduleInst or termInst
      else {
        string instName = string(net.instance(i));
        string pinName = string(net.pin(i));

        auto mtPtr = moduleTermMap.find(instName);
        if(mtPtr == moduleTermMap.end()) {
          cout << "** ERROR:  Net Instance ( " << instName
               << " ) does not exist in COMPONENTS/PINS statement "
               << "(moduleTermMap) " << endl;
          exit(1);
        }

        // Get Offset Infomation
        FPOS curOffset = GetOffset(__ckt, instName, pinName);
        //                cout << "instName: " << instName << ", pinName: " <<
        //                pinName << endl;
        //                curOffset.Dump("offset"); cout << endl;

        // Get IO information
        int io = GetIO(__ckt, instName, pinName);
        //                cout << io << endl;

        // pin Instnace mapping
        curPin = &pinInstance[pinIdx];
        curNet->pin[i] = curPin;

        // module case
        if(mtPtr->second.first) {
          curModule = &moduleInstance[mtPtr->second.second];

          // save module pin Name into mPinName
          mPinName[mtPtr->second.second].push_back(pinName);

          AddPinInfoForModuleAndTerminal(
              &curModule->pin, &curModule->pof, curModule->pinCNTinObject++,
              curOffset, curModule->idx, netIdx, i, pinIdx++, io, false);
        }
        // terminal case
        else {
          curTerm = &terminalInstance[mtPtr->second.second];

          // save terminal pin Name into tPinName
          tPinName[mtPtr->second.second].push_back(pinName);

          AddPinInfoForModuleAndTerminal(
              &curTerm->pin, &curTerm->pof, curTerm->pinCNTinObject++,
              curOffset, curTerm->idx, netIdx, i, pinIdx++, io, true);
        }
      }
    }
    netIdx++;
  }

  // update instance number
  pinCNT = pinIdx;
  netCNT = netIdx;

  // memory cutting (shrink)
//  netInstance = (NET*)realloc(netInstance, sizeof(NET) * netCNT);
//  pinInstance = (PIN*)realloc(pinInstance, sizeof(PIN) * pinCNT);

  PrintInfoInt( "NumNets", netCNT ); 
  PrintInfoInt( "NumPins", pinCNT );
  PrintProcEnd("DEF Net Parsing");
}

//
// this function is indended for
// re-parsing the result of detail-placer
//
// ReadPlLefDef -- moduleTermMap -- lefdefIO.cpp
// ReadBookshelf -- nodesMap -- bookshelfIO.cpp
//
void ReadPl(const char* fileName, bool isNameConvert) {
  if(auxCMD == "" && lefStor.size() != 0 && defName != "") {
    // it references moduleTermMap
    ReadPlLefDef(fileName, isNameConvert);
  }
  else if(auxCMD != "" && lefStor.size() == 0 && defName == "") {
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
void ReadPlLefDef(const char* fileName, bool isNameConvert) {
  cout << "READ BACK FROM " << fileName << endl;
  FILE* fp = fopen(fileName, "r");
  if(!fp) {
    runtimeError(fileName + string(" is not Exist!"));
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

    bool isModule = false;
    string mName = (isNameConvert)? 
      ((name[0] == 'o')? _bsMap.GetOrigModuleName( name ) : 
       _bsMap.GetOrigTerminalName( name )) 
      : name;
    
    if( mName == "" ) {
      continue;
    }

    auto mtPtr = moduleTermMap.find(mName);
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
      curModule->pmin.x = GetScaleDownPoint( atof(token) );

      token = strtok(NULL, " \t\n");
      curModule->pmin.y = GetScaleDownPoint( atof(token) );
//      cout << "down: " << curModule->pmin.x << " " << curModule->pmin.y << endl;

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


/////////////////////////////////////////////////////////
//
// Timing Part
//
/////////////////////////////////////////////////////////
namespace Timing { 

// copy scale down parameter into Timing Instance
void Timing::SetLefDefEnv() {
  _unitX = unitX;
  _unitY = unitY;
  _l2d = l2d;
}

void Timing::WriteSpefClockNet(stringstream& feed) {
  if(clockNetsDef.size() != 0) {
    WriteSpefClockNetDef(feed);
  }
  else if(clockNetsVerilog.size() != 0) {
    WriteSpefClockNetVerilog(feed);
  }
}

void Timing::WriteSpefClockNetDef(stringstream& feed) {
  for(auto& curClockNet : clockNetsDef) {
    defiNet& net = __ckt.defNetStor[curClockNet];
    feed << "*D_NET " << net.name() << " 0" << endl << "*CONN" << endl;
    for(int i = 0; i < net.numConnections(); i++) {
      // net.pin(i) itself exists on termInst
      if(strcmp(net.instance(i), "PIN") == 0) {
        feed << "*P " << net.pin(i);

        // extract Input/Output direction from PINS statement
        auto pinPtr = __ckt.defPinMap.find(string(net.pin(i)));
        if(pinPtr == __ckt.defPinMap.end()) {
          cout << "** ERROR:  Net Instance ( " << net.pin(i)
               << " ) does not exist in PINS statement (defPinMap) " << endl;
          exit(1);
        }

        int defPinIdx = pinPtr->second;
        //                cout << "foundPinIdx: " << defPinIdx << endl;
        //                assert( defPinIdx < __ckt.defPinStor.size() );

        int io = INT_MAX;
        if(!__ckt.defPinStor[defPinIdx].hasDirection()) {
          cout << "** ERROR:  PINS statement in DEF, " << net.pin(i)
               << " have no direction!!" << endl;
          exit(0);
          //                    io = 2; // both direction
        }
        else {
          // input : 0
          // output : 1
          io = (strcmp(__ckt.defPinStor[defPinIdx].direction(), "INPUT") == 0)
                   ? 0
                   : 1;
        }
        feed << ((io == 0) ? " I" : " O") << endl;
      }
      else {
        string instName = string(net.instance(i));
        string pinName = string(net.pin(i));

        // Get IO information
        int io = GetIO(__ckt, instName, pinName);

        feed << "*I " << instName << ":" << pinName << ((io == 0) ? " I" : " O")
             << endl;
      }
    }
    feed << "*END" << endl << endl;
  }
}

void Timing::WriteSpefClockNetVerilog(stringstream& feed) {
  for(auto& curClockNet : clockNetsVerilog) {
    feed << "*D_NET " << curClockNet << " 0" << endl << "*CONN" << endl;
    for(auto& connection : netMap[curClockNet]) {
      /////////////// Net Info Mapping part
      //
      // find current component is in module or terminal
      //
      if(connection.macroIdx == INT_MAX && connection.compIdx == INT_MAX) {
        string pinName = string(__ckt.defPinStor[connection.pinIdx].pinName());
        auto mtPtr = moduleTermMap.find(pinName);
        if(mtPtr == moduleTermMap.end()) {
          cout << "** ERROR:  Net Instance ( " << pinName
               << " ) does not exist in COMPONENTS/PINS statement "
               << "(moduleTermMap) " << endl;
          exit(1);
        }

        int io = INT_MAX;
        if(!__ckt.defPinStor[connection.pinIdx].hasDirection()) {
          io = 2;  // both direction
        }
        else {
          // input : 0
          // output : 1
          io = (strcmp(__ckt.defPinStor[connection.pinIdx].direction(),
                       "INPUT") == 0)
                   ? 0
                   : 1;
        }
        feed << "*P " << pinName;
        feed << ((io == 0) ? " I" : " O") << endl;
      }
      else {
        auto mtPtr = moduleTermMap.find(
            string(__ckt.defComponentStor[connection.compIdx].id()));

        if(mtPtr == moduleTermMap.end()) {
          cout << "** ERROR:  Net Instance ( "
               << __ckt.defComponentStor[connection.compIdx].id()
               << " ) does not exist in COMPONENTS/PINS statement "
               << "(moduleTermMap) " << endl;
          exit(1);
        }

        string compName =
            string(__ckt.defComponentStor[connection.compIdx].id());
        string pinName = string(
            __ckt.lefPinStor[connection.macroIdx][connection.pinIdx].name());

        int moduleIdx = mtPtr->second.second;

        //
        // located in terminal cases
        if(!mtPtr->second.first) {
          TERM* curTerminal = &terminalInstance[moduleIdx];
          // outer pin
          if(curTerminal->isTerminalNI) {
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

        int io = GetIO(__ckt, compName, pinName, connection.macroIdx);
        feed << ((io == 0) ? " I" : " O") << endl;
      }
    }
    feed << "*END" << endl << endl;
  }
}

/*
void Timing::UpdateSpefClockNetVerilog() {
  for(auto& curClockNet : clockNetsVerilog) {
    char* netNamePtr = new char[curClockNet.length() + 1];
    strcpy(netNamePtr, curClockNet.c_str());
    sta::Net* net = sta::spef_reader->findNet(netNamePtr);
    SpefTriple* netCap = new SpefTriple(0);
    sta::spef_reader->dspfBegin(net, netCap);
    sta::spef_reader->dspfFinish();
  }
}*/

}



////////////////////////////////////////////////
//
// Routability Part 
//
////////////////////////////////////////////////

void RouteInstance::SetScaleFactor() {
  this->_unitX = (float)unitX;
  this->_unitY = (float)unitY;
  this->_offsetX = (float)offsetX;
  this->_offsetY = (float)offsetY;
  this->_defDbu = (float)l2d;
}

void RouteInstance::SetCircuitInst() {
  this->_ckt = &__ckt;
}

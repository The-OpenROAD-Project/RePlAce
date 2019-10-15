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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <sys/time.h>
#include <sys/stat.h>
#include <fstream>

#include "replace_private.h"
#include "opt.h"
#include "bin.h"
#include "gcell.h"
#include "fft.h"
#include "wlen.h"
#include "bookShelfIO.h"
#include "routeOpt.h"

// global variable due to weird structure
RouteInstance routeInst;
using std::ifstream;
using std::min;
using std::max;
using std::to_string;

void RouteInstance::Init(){
#ifdef USE_GOOGLE_HASH
  _layerMap.set_empty_key(INIT_STR);
#endif
  SetReplaceStructure(); 
  SetScaleFactor();
  SetCircuitInst();

  FillLayerStor();
  FillGCellXY(); 
  FillTrack(); 
  FillGridXY();

  // for Bookshelf's route
  FillForReducedTrackStor();
}

void RouteInstance::SetReplaceStructure() {

}

void RouteInstance::FillLayerStor() {
  if( _ckt->lefLayerStor.size() == 0) {
    cout << "ERROR: RouteInst failed to get LAYER information from LEF" << endl;
    exit(0);
  }
  for(auto& curLayer : _ckt->lefLayerStor) {
    if( !curLayer.hasType() ) {
      continue;
    }
    if(strcmp(curLayer.type(), "ROUTING") != 0) {
      continue;
    }

    string layerName = string(curLayer.name());

    if( !curLayer.hasDirection() ) {
      cout << "ERROR: Layer: " << layerName 
        << " does not have direction" << endl;
      exit(0);
    }
    
    LayerDirection layerDir;
    if( string(curLayer.direction()) == "HORIZONTAL") {
      layerDir = Horizontal;  
    }
    else if( string(curLayer.direction()) == "VERTICAL") {
      layerDir = Vertical;
    }
    else {
      cout << "ERROR: Layer " << layerName 
        << " have weird direction: " << curLayer.direction() << endl;
      exit(0);
    }

    float pitchX = FLT_MIN, pitchY = FLT_MIN;
    if(curLayer.hasPitch()) {
      pitchX = pitchY = curLayer.pitch();
    }

    if(curLayer.hasXYPitch()) {
      pitchX = curLayer.pitchX();
      pitchY = curLayer.pitchY();
    }

    if( !curLayer.hasPitch() && !curLayer.hasXYPitch() ) {
      cout << "ERROR: Layer: " << layerName 
        << " does not have PITCH info" << endl;
      exit(0);
    }
    
    float width = FLT_MIN;
    if( !curLayer.hasWidth() ) {
      cout << "ERROR: Layer: " << layerName 
        << " does not have WIDTH info" << endl;
      exit(0);
    }
    width = curLayer.width();

    _layerMap[ layerName ] = _layerStor.size();
    _layerStor.push_back( Layer( layerName, pitchX, pitchY, width, layerDir) );
  }
//  for(auto& curLayer : _layerStor) {
//    curLayer.Dump();
//  }
}

void RouteInstance::FillGCellXY() {
  if( _layerStor.size() < 2) {
    cout << "ERROR: The # LAYERs must be at least 2, current Size: " 
      << _layerStor.size() << endl;
    exit(0);
  }
  // metal2's height and width
//  _gCellSizeX = 15 * _layerStor[1].layerPitchX;
//  _gCellSizeY = 15 * _layerStor[1].layerPitchY;
  _gCellSizeX = 15 * _layerStor[2].layerPitchY;
  _gCellSizeY = 15 * _layerStor[2].layerPitchX;
//  _gCellSizeX = _ckt->lefSiteStor[0].sizeY();
//  _gCellSizeY = _ckt->lefSiteStor[0].sizeY();

  _tileSizeX =  _defDbu * _gCellSizeX / _unitX ;
  _tileSizeY =  _defDbu * _gCellSizeY / _unitY ;

  cout << "INFO: TILE SIZE for GRouter in Micron: " << _gCellSizeX << " x " << _gCellSizeY << endl;
  cout << "INFO: SCALED TILE SIZE for GRouter: " << _tileSizeX
    << " x " << _tileSizeY << endl << endl;

}

void RouteInstance::FillLayerCapacityRatio( string fileName ) {
#ifdef USE_GOOGLE_HASH
  _layerCapacityMap.set_empty_key(INIT_STR);
#endif
  ifstream inputFile( fileName );
  if( !inputFile.is_open() ) {
    cout << "INFO: FILE: " << fileName << " DOES NOT EXIST" << endl;
    exit(1);
  }

  string metalName = "";
  float metalResource = 0.0f;
  cout << "Metal Resource Control" << endl;
  while(inputFile >> metalName >> metalResource)  {
    cout << metalName << " : " << metalResource << endl;
    _layerCapacityMap[ metalName ] = metalResource;
  }
  inputFile.close();
}

void RouteInstance::FillTrack() {
  for(auto& curLayer : _layerStor) {
    auto lcPtr = _layerCapacityMap.find(curLayer.layerName);

    // control the track resources from M3~M9
    // M1 and M2 is required for pin access
    float currentCapRatio = (lcPtr == _layerCapacityMap.end())? 
      (&curLayer - &_layerStor[0] <= 1)? 
        1.00 : globalRouterCapRatio 
      : lcPtr->second;
    _trackCount.push_back( 
        (curLayer.layerDirection == Horizontal)? 
        floor( 100.0 * currentCapRatio * _gCellSizeX / (curLayer.layerPitchY) ) :
        floor( 100.0 * currentCapRatio * _gCellSizeY / (curLayer.layerPitchX) ));

    PrintInfoString("RouteInit: Layer: " + curLayer.layerName + 
        " / numTracks: " + to_string( _trackCount[_trackCount.size()-1] ), 1);
  }
  // M1 have NO routability
//  if( _trackCount.size() > 1) {
//    _trackCount[0] = 0;
//  }
}


// update 
// _gridOriginX, _gridOriginY, _gridCountX, _gridCountY,
// _gridInnerCountX, _gridInnerCountY, 
// _bsReducedTrackStor
void RouteInstance::FillGridXY() {
  defiPoints points = _ckt->defDieArea.getPoint();
  float lx = FLT_MIN, ly = FLT_MIN, 
        ux = FLT_MIN, uy = FLT_MIN;
  if( points.numPoints >= 2) {
    lx = (points.x[0] + _offsetX) / _unitX;
    ly = (points.y[0] + _offsetY) / _unitY; 
    ux = (points.x[1] + _offsetX) / _unitX;
    uy = (points.y[1] + _offsetY) / _unitY;
    cout << lx << " " << ly << " " << ux << " " << uy << endl;
  }

  _gridOriginX = lx;
  _gridOriginY = ly;
  
  _gridCountX = ceil( (ux - lx+1) / _tileSizeX );
  _gridCountY = ceil( (uy - ly+1) / _tileSizeY );

  _gridInnerCountX = floor( (ux - lx) / _tileSizeX );
  _gridInnerCountY = floor( (uy - ly) / _tileSizeY );

}

void RouteInstance::FillForReducedTrackStor() {
  defiPoints points = _ckt->defDieArea.getPoint();
  float lx = FLT_MIN, ly = FLT_MIN, 
        ux = FLT_MIN, uy = FLT_MIN;
  if( points.numPoints >= 2) {
    lx = (points.x[0] + _offsetX) / _unitX;
    ly = (points.y[0] + _offsetY) / _unitY; 
    ux = (points.x[1] + _offsetX) / _unitX;
    uy = (points.y[1] + _offsetY) / _unitY;
    cout << lx << " " << ly << " " << ux << " " << uy << endl;
  }

  // normal cases
  if( _gridInnerCountX != _gridCountX && _gridInnerCountY != _gridCountY ) { 
    float diffX = (ux - lx) - _tileSizeX * _gridInnerCountX;
    float diffY = (uy - ly) - _tileSizeY * _gridInnerCountY;
    cout << "diffX: " << diffX << ", diffY: " << diffY << endl;

    // for Vertical Stripe in terms of horizontal coordinate
    for(auto& curLayer: _layerStor) {
      int layerIdx = &curLayer - &_layerStor[0];
      // only for Vertical Routing Layer
      if( curLayer.layerDirection == Horizontal ) {
        continue;
      }
      _bsReducedTrackStor.push_back( 
        ReducedTrack( 
          layerIdx, 
          _gridInnerCountX, 0, 
          _gridCountX, _gridInnerCountY, 
         INT_CONVERT( _trackCount[layerIdx] * diffX / _tileSizeX ) ) );
    }

    // for Horizontal Stripe in terms of vertical coordinate
    for(auto& curLayer: _layerStor) {
      int layerIdx = &curLayer - &_layerStor[0];
      // only for Horizontal Routing Layer
      if( curLayer.layerDirection == Vertical ) {
        continue;
      }
      _bsReducedTrackStor.push_back( 
        ReducedTrack( 
          layerIdx, 
          0, _gridInnerCountY, 
          _gridInnerCountX, _gridCountY, 
         INT_CONVERT( _trackCount[layerIdx] * diffY / _tileSizeY ) ) );
    }

    // for the Intersection of horizontal and vertical stripes.
    for(auto& curLayer: _layerStor) {
      int layerIdx = &curLayer - &_layerStor[0];
      _bsReducedTrackStor.push_back( 
        ReducedTrack( 
          layerIdx, 
          _gridInnerCountX, _gridInnerCountY,
          _gridCountX, _gridCountY,
          (curLayer.layerDirection == Horizontal)? 
            INT_CONVERT( _trackCount[layerIdx] * diffY / _tileSizeY ) :
            INT_CONVERT( _trackCount[layerIdx] * diffX / _tileSizeX )
          ) );
    }
  }


//  for(auto& rTrack: _bsReducedTrackStor) { 
//    rTrack.Dump();
//  }
}

void est_congest_global_router(char *dir, string plName) {
  run_global_router(dir, plName);
  
  string routeName = string(dir) + "/" + string(gbch) + ".est";
  read_routing_file(routeName);
}

void get_intermediate_pl_sol(char *dir, string plName) {
  output_tier_pl_global_router(plName.c_str(), 0, true);
  LinkConvertedBookshelf(dir);
}

void run_global_router(char *dir, string plName) {
  char cmd[BUFFERSIZE];

  sprintf(cmd, "%s ICCAD %s/%s.aux %s %s %s/%s.est",
          globalRouterPosition.c_str(), 
          dir, gbch, 
          plName.c_str(),  
          globalRouterSetPosition.c_str(), 
          dir, gbch); // est
  cout << cmd << endl;
  system(cmd);

  if(autoEvalRC_CMD)
    evaluate_RC_by_official_script(dir);
}

void evaluate_RC_by_official_script(char *dir) {
  /*

  char evalPos[PATH_MAX] = {0, };
  sprintf(evalPos, "%s/../router/iccad2012_evaluate_solution.pl", currentDir);

  char evalDirPos[PATH_MAX] = {0, };
  sprintf( evalDirPos, "%s/../router/");

  char fullEvalPos[PATH_MAX] = {0, };
  char* ptr = realpath( evalPos, fullEvalPos );
  
  char fullEvalDirPos[PATH_MAX] = {0, };
  ptr = realpath( evalDirPos, fullEvalDirPos );


//  sprintf(cmd, "ln -snf %s %s", fullEvalPos, fullDir); 
//  cout << cmd << endl;
//  system(cmd);
*/
  char cmd[BUFFERSIZE] = {0, };
  
  char realDirBnd[PATH_MAX] = {0, };
  char* ptr = realpath(dir_bnd, realDirBnd);
  
  char realCurDir[PATH_MAX] = {0, };
  ptr = realpath(currentDir, realCurDir);
  
  char fullDir[PATH_MAX] = {0, }; 
  ptr = realpath( dir, fullDir );

  sprintf( cmd, "ln -s %s/../router/*.pl %s/", realCurDir, fullDir); 
  cout << cmd << endl;
  system(cmd);
  
//  sprintf(cmd, "cd %s && perl iccad2012_evaluate_solution.pl -p %s.aux %s.pl %s.est",
  sprintf(cmd, "cd %s && perl iccad2012_evaluate_solution.pl %s.aux %s.lg.pl %s.est",
          fullDir, gbch, gbch, gbch);
  cout << cmd << endl;
  system(cmd);

  // draw plot  
//  sprintf(cmd, "cd %s && gnuplot *.plt",fullDir);
//  cout << cmd << endl;
//  system(cmd);
}

void clean_routing_tracks_in_net() {
  struct NET *net = NULL;
  for(int i = 0; i < netCNT; i++) {
    net = &netInstance[i];
    net->routing_tracks.clear();
    net->routing_tracks.shrink_to_fit();
  }
}

/*
void buildRSMT_FLUTE(struct FPOS *st) {
  struct  NET     *net = NULL;
  for (int i=0; i<netCNT; i++) {
    net = &netInstance[i];
    buildRSMT_FLUTE_PerNet(st, net);
  }
}
*/

/*
void buildRSMT_FLUTE_PerNet(struct FPOS *st, struct NET *net) {

  double   x[4096], y[4096];
  //prec   *x = malloc(4096*sizeof(prec  ));
  //prec   *y = malloc(4096*sizeof(prec  ));
  //int x1[4096], y1[4096];

  if (net->pinCNTinObject > 1500) {
    net->stn_cof = 10;
    return;
  }

  for (int i=0; i<net->pinCNTinObject; i++) {

      struct PIN *pin_start = net->pin[i];
      //struct MODULE *tempInst_start = &moduleInstance[pin_start->moduleID] ;
      struct FPOS center_start = pin_start->fp;
      struct FPOS pof_start = zeroFPoint ;
      if (!pin_start->term) {
        struct MODULE *tempInst_start = &moduleInstance[pin_start->moduleID] ;
        pof_start = tempInst_start->pof[pin_start->pinIDinModule] ;
        center_start = st[pin_start->moduleID] ;
        //cout <<net->name <<" " <<tempInst_start->name <<endl;
      } else {
        struct TERM *tempInst_start = &terminalInstance[pin_start->moduleID] ;
        pof_start = tempInst_start->pof[pin_start->pinIDinModule] ;
        //cout <<net->name <<" " <<tempInst_start->name <<endl;
      }
      x[i] = center_start.x + pof_start.x ;
      //x1[i] = (int)(x[i] * 100.0);
      y[i] = center_start.y + pof_start.y ;
      //y1[i] = (int)(y[i] * 100.0);
      //cout <<x[i] <<" " <<y[i] <<endl;
  }
  //cout <<net->name <<endl;
  //fflush(stdout);
  double   flutewl = flute_wl(net->pinCNTinObject, x, y, 3);
  //prec   flutewl = flute(net->pinCNTinObject, x, y, 3).length;
  //prec   flutewl = flute_wl(net->pinCNTinObject, x1, y1, 3)/100.0;
  //prec   flutewl = 0;
  prec   hpwl = fabs(net->max_x - net->min_x) + fabs(net->max_y - net->min_y);
  if (hpwl == 0) {
    net->stn_cof = 1;
  } else {
    net->stn_cof = max(1.0, flutewl / hpwl);
    //if (net->stn_cof > 8) {
    //  if (net->pinCNTinObject >= 2) {
    //    cout <<hpwl <<" " <<flutewl <<" " <<net->stn_cof <<" "
<<net->pinCNTinObject <<endl;
    //  }
    //  //for (int i=0; i<net->pinCNTinObject; i++) {

    //  //  struct PIN *pin_start = net->pin[i];
    //  //  //struct FPOS center_start = st[pin_start->moduleID] ;
    //  //  struct FPOS center_start = pin_start->fp;
    //  //  struct FPOS pof_start = zeroFPoint ;
    //  //  if (!pin_start->term) {
    //  //    struct MODULE *tempInst_start =
&moduleInstance[pin_start->moduleID] ;
    //  //    pof_start = tempInst_start->pof[pin_start->pinIDinModule] ;
    //  //    center_start = st[pin_start->moduleID] ;
    //  //  } else {
    //  //    struct TERM *tempInst_start =
&terminalInstance[pin_start->moduleID] ;
    //  //    pof_start = tempInst_start->pof[pin_start->pinIDinModule] ;
    //  //  }
    //  //  x[i] = center_start.x + pof_start.x ;
    //  //  y[i] = center_start.y + pof_start.y ;
    //  //  if (net->pinCNTinObject >= 2) {
    //  //    //cout <<tempInst_start->name <<" ";
    //  //    cout <<x[i] <<" " <<y[i] <<" " <<pin_start->term <<" "
<<pof_start.x <<" " <<pof_start.y <<endl;
    //  //  }
    //  //}
    //  if (net->pinCNTinObject >= 2) {
    //    cout <<net->name <<" " <<net->max_x <<" " <<net->min_x <<" "
    //         <<net->max_y <<" " <<net->min_y <<endl;
    //  }
    //}
    net->stn_cof = min(10.0, net->stn_cof);
  }
  //cout <<net->name <<" " <<net->pinCNTinObject <<" " <<hpwl <<" " <<flutewl
<<endl;
}
*/

void congEstimation(struct FPOS *st) {
  char dir[BUF_SZ];
  int inflation_index;

  /*
    if (conges_eval_methodCMD == prob_ripple_based) {
        cout << "INFO:  Your congestion est. method is based on probabilistic." << endl;
        for (inflation_index=0; inflation_index<100; inflation_index++){
            sprintf (dir, "%s/prob/tier%d/inflation_iter%d", dir_bnd, 0, inflation_index);
            struct  stat    infl;
            if (stat(dir, &infl) < 0) break;
        }
        get_intermediate_pl_sol (dir, 0);
        if (autoEvalRC_CMD) run_global_router (dir);
        //delete_input_files_in (dir);
        
        buildRMST2(st);
        calcCong (st, prob_ripple_based);
        clearTwoPinNets();
        calcCong_print();

    } else*/ 

  if(conges_eval_methodCMD == global_router_based) {
    cout << "INFO:  Your congestion est. method is based on global router "
            "(NCTUgr)."
         << endl;
    for(inflation_index = 0; inflation_index < 100; inflation_index++) {
      sprintf(dir, "%s/router/tier%d/inflation_iter%d", dir_bnd, 0,
              inflation_index);
      struct stat infl;
      if(stat(dir, &infl) < 0)
        break;
    }


    // directory create
    string command = "mkdir -p " + string(dir);
    system(command.c_str());
   
    string plName = string(dir) + "/" + string(gbch) + ".pl";
    get_intermediate_pl_sol(dir, plName);

    plName = string(gbch) + ".pl";
    string auxName = string(gbch) + ".aux";
    CallNtuPlacer4h(dir, auxName.c_str(), plName.c_str());
    
    string resultPlName = string(dir) + "/" + string(gbch) + ".lg.pl";
    ReadPl(resultPlName.c_str(), true);

    string defOut = string(dir) + "/" + string(gbch) + ".def";
    WriteDef( defOut.c_str() );
    
    // for NCTUgr
    est_congest_global_router(dir, resultPlName);
    
    // delete_input_files_in (dir);
    calcCong(st, global_router_based);
    CalcPinDensity(st);
    MergePinDen2Route();
    MergeBlkg2Route();
    calcCong_print();
    clean_routing_tracks_in_net();
  }
  else {
    cout << "ERROR:  Your congestion est. method is not applicable" << endl;
    exit(1);
  }
  calcInflationRatio_foreachTile();
}

void buildRMSTPerNet_genTwoPinNets(struct FPOS *st, struct NET *net) {
  net->two_pin_nets.clear();
  net->mUFPin.clear();

  for(int i = 0; i < net->pinCNTinObject; i++) {
    struct PIN *pin = net->pin[i];
    // if term -> odd moduleID
    // else -> even moduleID
    int moduleID = pin->moduleID * 2;
    if(pin->term) {
      moduleID++;
    }
    UFPin temp(moduleID);
    net->mUFPin[moduleID] = temp;
    // cout <<"abc " <<net->mUFPin[moduleID].parent <<" "
    // <<net->mUFPin[moduleID].modu <<endl;
  }

  for(int i = 0; i < net->pinCNTinObject - 1; i++) {
    for(int j = i + 1; j < net->pinCNTinObject; j++) {
      struct PIN *pin_start = net->pin[i];
      struct PIN *pin_end = net->pin[j];

      // if term -> odd moduleID
      // else -> even moduleID
      int moduleID_start = pin_start->moduleID * 2;
      int moduleID_end = pin_end->moduleID * 2;

      struct FPOS pof_start;
      struct FPOS pof_end;
      struct FPOS center_start = pin_start->fp;
      struct FPOS center_end = pin_end->fp;
      if(!pin_start->term) {
        struct MODULE *tempInst_start = &moduleInstance[pin_start->moduleID];
        pof_start = tempInst_start->pof[pin_start->pinIDinModule];
        center_start = st[pin_start->moduleID];
      }
      else {
        moduleID_start++;
        struct TERM *tempInst_start = &terminalInstance[pin_start->moduleID];
        pof_start = tempInst_start->pof[pin_start->pinIDinModule];
      }
      if(!pin_end->term) {
        struct MODULE *tempInst_end = &moduleInstance[pin_end->moduleID];
        pof_end = tempInst_end->pof[pin_end->pinIDinModule];
        center_end = st[pin_end->moduleID];
      }
      else {
        moduleID_end++;
        struct TERM *tempInst_end = &terminalInstance[pin_end->moduleID];
        pof_end = tempInst_end->pof[pin_end->pinIDinModule];
      }
      // cout <<"test" <<pof_start.x <<" " <<pof_start.y <<endl;
      // cout <<"test" <<pof_end.x <<" " <<pof_end.y <<endl;
      struct FPOS fp_start;
      struct FPOS fp_end;
      fp_start.x = center_start.x + pof_start.x;
      fp_start.y = center_start.y + pof_start.y;
      fp_end.x = center_end.x + pof_end.x;
      fp_end.y = center_end.y + pof_end.y;
      prec rect_dist =
          fabs(fp_start.x - fp_end.x) + fabs(fp_start.y - fp_end.y);
      // cout <<"a " <<pin_start->moduleID <<endl;
      TwoPinNets temp(moduleID_start, moduleID_end, rect_dist, i, j);
      net->two_pin_nets.push_back(temp);

      // cout << "def " <<
      // net->two_pin_nets[net->two_pin_nets.size()-1].start_modu <<endl;
      // cout << "X: " << net->pin[i]->fp.x << endl;
      // cout << "Y: " << net->pin[i]->fp.y << endl;
    }
  }
}

int UFFind(struct NET *net, int moduleID) {
  std::map< int, UFPin > &mUFPin = net->mUFPin;
  if(mUFPin[moduleID].parent != mUFPin[moduleID].modu) {
    mUFPin[moduleID].parent = UFFind(net, mUFPin[moduleID].parent);
  }
  // cout <<"parent " <<mUFPin[moduleID].parent <<endl;
  return mUFPin[moduleID].parent;
}

void UFUnion(struct NET *net, int idx, int idy) {
  std::map< int, UFPin > &mUFPin = net->mUFPin;
  int xRoot = UFFind(net, idx);
  int yRoot = UFFind(net, idy);
  if(xRoot == yRoot) {
    return;
  }
  if(mUFPin[xRoot].rank < mUFPin[yRoot].rank) {
    mUFPin[xRoot].parent = yRoot;
  }
  else if(mUFPin[xRoot].rank > mUFPin[yRoot].rank) {
    mUFPin[yRoot].parent = xRoot;
  }
  else {
    mUFPin[yRoot].parent = xRoot;
    mUFPin[xRoot].rank++;
  }
}

bool TwoPinNets_comp(TwoPinNets x, TwoPinNets y) {
  return x.rect_dist < y.rect_dist;
}

void buildRMSTPerNet_genRMST(struct NET *net) {
  int num_edges = net->pinCNTinObject - 1;
  std::vector< TwoPinNets > &two_pin_nets = net->two_pin_nets;
  std::sort(two_pin_nets.begin(), two_pin_nets.end(), TwoPinNets_comp);

  int num_curr_edges = 0;

  prec wl_rmst = 0;

  for(unsigned i = 0; i < two_pin_nets.size(); i++) {
    int moduleID_start = two_pin_nets[i].start_modu;
    int moduleID_end = two_pin_nets[i].end_modu;
    int start_root = UFFind(net, moduleID_start);
    int end_root = UFFind(net, moduleID_end);
    // cout <<moduleID_start <<" " <<start_root <<" " <<moduleID_end <<" "
    // <<end_root <<endl;
    if(start_root == end_root) {
      continue;
    }
    else {
      UFUnion(net, moduleID_start, moduleID_end);
      two_pin_nets[i].selected = true;
      wl_rmst += two_pin_nets[i].rect_dist;
      num_curr_edges++;
    }
    if(num_curr_edges >= num_edges) {
      break;
    }
  }

  ////net->wl_rsmt = wl_rmst * 0.88;
  // net->wl_rsmt = wl_rmst;
  // prec   hpwl = fabs(net->max_x - net->min_x) + fabs(net->max_y -
  // net->min_y);
  // if (hpwl == 0) {
  //  net->stn_cof = 1;
  //  //cout <<net->name <<" error: hpwl = 0" <<endl;
  //} else {
  //  net->stn_cof = max(1.0, net->wl_rsmt / hpwl);
  //  net->stn_cof = min(8.0, net->stn_cof);
  //}
  // cout <<net->name <<" degree = " <<net->pinCNTinObject <<" stn_cof = "
  // <<net->stn_cof <<endl;
}

void buildRMSTPerNet_printRMST(struct FPOS *st, struct NET *net) {
  cout << "net_name: " << net->Name() << endl;
  for(unsigned i = 0; i < net->two_pin_nets.size(); ++i) {
    if(net->two_pin_nets[i].selected == true) {
      cout << "Y: ";
    }
    else {
      cout << "N: ";
    }
    int start_modu = net->two_pin_nets[i].start_modu / 2;
    int end_modu = net->two_pin_nets[i].end_modu / 2;
    struct FPOS center1;
    struct FPOS center2;
    if(net->two_pin_nets[i].start_modu % 2 == 0) {
      struct MODULE *tempInst1 = &moduleInstance[start_modu];
      center1 = st[start_modu];
      cout << tempInst1->Name() << " (" << center1.x << ", " << center1.y << ") ";
    }
    else {
      struct TERM *tempInst1 = &terminalInstance[start_modu];
      center1 = net->pin[net->two_pin_nets[i].i]->fp;
      cout << tempInst1->Name() << " (" << center1.x << ", " << center1.y << ") ";
    }
    if(net->two_pin_nets[i].end_modu % 2 == 0) {
      struct MODULE *tempInst2 = &moduleInstance[end_modu];
      center2 = st[end_modu];
      cout << tempInst2->Name() << " (" << center2.x << ", " << center2.y << ") ";
    }
    else {
      struct TERM *tempInst2 = &terminalInstance[end_modu];
      center2 = net->pin[net->two_pin_nets[i].j]->fp;
      cout << tempInst2->Name() << " (" << center2.x << ", " << center2.y << ") ";
    }
    cout << fabs(center1.x - center2.x) + fabs(center1.y - center2.y) << " "
         << net->two_pin_nets[i].rect_dist << endl;
  }
  cout << endl;
}

/*
void buildRMSTPerNet(struct FPOS *st, struct NET *net) {
  buildRMSTPerNet_genTwoPinNets(st, net);
  buildRMSTPerNet_genRMST(net);
  //buildRMSTPerNet_printRMST(st, net);
}

void buildRMST(struct FPOS *st) {
    struct  NET     *net = NULL;

    for (int i=0; i<netCNT; i++) {
        net = &netInstance[i];
        //cout << "netName " << net->name << endl;
        if (net->pinCNTinObject > 65) {
          prec   hpwl = fabs(net->max_x - net->min_x) + fabs(net->max_y -
net->min_y);
          net->wl_rsmt = 0.76*sqrt(fabs(net->max_x-net->min_x)*
              fabs(net->max_y-net->min_y)*net->pinCNTinObject);
          if (hpwl == 0) {
            net->stn_cof = 1;
            //cout <<net->name <<" error: hpwl = 0" <<endl;
          } else {
            net->stn_cof = max(1.0, net->wl_rsmt / hpwl);
            net->stn_cof = min(8.0, net->stn_cof);
          }
        } else {
          buildRMSTPerNet(st, net);
        }
    }
}

void buildRMST2(struct FPOS *st) {
  struct  NET     *net = NULL;
  for (int i=0; i<netCNT; i++) {
    net = &netInstance[i];
    //cout << "netName " << net->name << endl;
    buildRMSTPerNet(st, net);
  }
}

void clearTwoPinNets() {
  struct  NET     *net = NULL;
  for (int i=0; i<netCNT; i++) {
    net = &netInstance[i];
    net->two_pin_nets.clear();          // clear() does not return mem.
    net->two_pin_nets.shrink_to_fit();  // shrink_to_fit helps to return mem.
    net->mUFPin.clear();  // delete stupid map.
  }
}
*/

void calcCongPerNet_grouter_based(struct NET *net) {
  // int             x, y = 0;
  int idx = 0;
  // int             jdx = 0;
  int metLayer = 0;
  bool isH_layer = false;
  bool isV_layer = false;
  prec x_min = 0;  // routing track's min and max
  prec y_min = 0;  // routing track's min and max
  prec x_max = 0;  // routing track's min and max
  prec y_max = 0;  // routing track's min and max
  struct POS b0;
  struct POS b1;
  // struct  POS bm1;
  struct TILE *bpx = nullptr;
  // struct  TILE    *bpy    = nullptr;
  struct TIER *tier = &tier_st[0];

  std::vector< ROUTRACK > &routing_tracks = net->routing_tracks;

  for(auto routrack = routing_tracks.begin(); routrack != routing_tracks.end();
      ++routrack) {
    metLayer = routrack->layer - 1;

    // Horizontal Metal Layer
    if(routrack->from.x == routrack->to.x) {
      if(routrack->from.y < routrack->to.y) {
        y_min = routrack->from.y;
        y_max = routrack->to.y;
      }
      else {
        y_min = routrack->to.y;
        y_max = routrack->from.y;
      }
      x_min = x_max = routrack->from.x;
      isV_layer = true;
      isH_layer = false;
    }
    // Vertical Metal Layer
    else {
      if(routrack->from.x < routrack->to.x) {
        x_min = routrack->from.x;
        x_max = routrack->to.x;
      }
      else {
        x_min = routrack->to.x;
        x_max = routrack->from.x;
      }
      y_min = y_max = routrack->from.y;
      isH_layer = true;
      isV_layer = false;
    }

    b0.x = (int)round((x_min - tier->tile_org.x) * tier->inv_tile_stp.x);
    b0.y = (int)round((y_min - tier->tile_org.y) * tier->inv_tile_stp.y);
    b1.x = (int)round((x_max - tier->tile_org.x) * tier->inv_tile_stp.x);
    b1.y = (int)round((y_max - tier->tile_org.y) * tier->inv_tile_stp.y);

    // if (isH_layer) {
    //    bm1.x = (int)((x_min - tier->tile_stp.x - tier->tile_org.x) *
    //    tier->inv_tile_stp.x);
    //    bm1.y = (int)((y_min - tier->tile_org.y) * tier->inv_tile_stp.y);
    //} else {
    //    bm1.x = (int)((x_min - tier->tile_org.x) * tier->inv_tile_stp.x);
    //    bm1.y = (int)((y_min - tier->tile_stp.y- tier->tile_org.y) *
    //    tier->inv_tile_stp.y);
    //}

    if(b0.x < 0)
      b0.x = 0;
    if(b0.x > tier->dim_tile.x - 1)
      b0.x = tier->dim_tile.x - 1;
    if(b0.y < 0)
      b0.y = 0;
    if(b0.y > tier->dim_tile.y - 1)
      b0.y = tier->dim_tile.y - 1;

    if(b1.x < 0)
      b1.x = 0;
    if(b1.x > tier->dim_tile.x - 1)
      b1.x = tier->dim_tile.x - 1;
    if(b1.y < 0)
      b1.y = 0;
    if(b1.y > tier->dim_tile.y - 1)
      b1.y = tier->dim_tile.y - 1;

    // bool flag = true;
    // if (bm1.x < 0)                   flag = false;
    // if (bm1.x > tier->dim_tile.x-1)  flag = false;
    // if (bm1.y < 0)                   flag = false;
    // if (bm1.y > tier->dim_tile.y-1)  flag = false;


    idx = b0.x * tier->dim_tile.y + b0.y;
//    cout << "idx: " << idx << endl;
//    cout << "tileCnt: " << tier->tot_tile_cnt << endl; 
    bpx = &tier->tile_mat[idx];
    if(isH_layer)
      bpx->h_gr_usage_per_layer_r[metLayer]++;
    if(isV_layer)
      bpx->v_gr_usage_per_layer_r[metLayer]++;

    bpx->route[metLayer] += minWireWidth[metLayer] + minWireSpacing[metLayer];

    idx = b1.x * tier->dim_tile.y + b1.y;
    bpx = &tier->tile_mat[idx];
    if(isH_layer)
      bpx->h_gr_usage_per_layer_l[metLayer]++;
    if(isV_layer)
      bpx->v_gr_usage_per_layer_l[metLayer]++;
    // jdx = bm1.x * tier->dim_tile.y + bm1.y;
    // if (flag) bpy=&tier->tile_mat[jdx];
    // if (isH_layer && flag) bpy->h_gr_usage_per_layer_r[metLayer]++;
    // if (isV_layer && flag) bpy->v_gr_usage_per_layer_r[metLayer]++;

    // for (x=b0.x, bpx=&tier->tile_mat[idx]; x<=b1.x; x++,
    // bpx+=tier->dim_tile.y) {
    //    for (y=b0.y, bpy=bpx; y<=b1.y; y++, bpy++) {
    //        if (isH_layer) bpy->h_gr_usage_per_layer[metLayer]++;
    //        if (isV_layer) bpy->v_gr_usage_per_layer[metLayer]++;
    //    }
    //}
  }
}

/*
void calcCongPerNet_prob_based (struct FPOS *st, struct NET *net) {
    int             x, y = 0;
    int             idx = 0;
    prec            area_share = 0;
    prec            min_x = 0;
    prec            min_y = 0;
    prec            max_x = 0;
    prec            max_y = 0;
    struct  TILE    *bpx = NULL;
    struct  TILE    *bpy = NULL;
    struct  POS     b0;
    struct  POS     b1;
    struct  TIER    *tier = &tier_st[0];

    struct POS ll;
    ll.x = INT_MAX;
    ll.y = INT_MAX;
    struct POS ur;

    std::vector<TwoPinNets> &two_pin_nets = net->two_pin_nets;

    //sanity check: passed
    //for (unsigned i = 0; i < tier->tot_tile_cnt; ++i) {
    //  if (tier->tile_mat[i].tmp_h_usage > 0) {
    //    cout <<"Error" <<endl;
    //  }
    //  if (tier->tile_mat[i].tmp_v_usage > 0) {
    //    cout <<"Error" <<endl;
    //  }
    //}


    for (unsigned m = 0; m < two_pin_nets.size(); ++m) {
        if (two_pin_nets[m].selected) {
            int i = two_pin_nets[m].i;
            int j = two_pin_nets[m].j;
            struct PIN *pin_start = net->pin[i];
            struct PIN *pin_end = net->pin[j];
            struct FPOS pof_start = zeroFPoint;
            struct FPOS pof_end = zeroFPoint;
            struct FPOS center_start = pin_start->fp;
            struct FPOS center_end = pin_end->fp;
            struct FPOS fp_start = zeroFPoint;
            struct FPOS fp_end = zeroFPoint;

            if (!pin_start->term) {
              struct MODULE *tempInst_start =
&moduleInstance[pin_start->moduleID] ;
              pof_start = tempInst_start->pof[pin_start->pinIDinModule] ;
              center_start = st[pin_start->moduleID] ;
            } else {
              struct TERM *tempInst_start =
&terminalInstance[pin_start->moduleID] ;
              pof_start = tempInst_start->pof[pin_start->pinIDinModule] ;
            }

            if (!pin_end->term) {
              struct MODULE *tempInst_end = &moduleInstance[pin_end->moduleID] ;
              pof_end = tempInst_end->pof[pin_end->pinIDinModule] ;
              center_end = st[pin_end->moduleID] ;
            } else {
              struct TERM *tempInst_end = &terminalInstance[pin_end->moduleID] ;
              pof_end = tempInst_end->pof[pin_end->pinIDinModule] ;
            }

            fp_start.x = center_start.x + pof_start.x ;
            fp_start.y = center_start.y + pof_start.y ;
            fp_end.x = center_end.x + pof_end.x ;
            fp_end.y = center_end.y + pof_end.y ;

            prec   x_min = (fp_start.x < fp_end.x)? fp_start.x : fp_end.x;
            prec   x_max = (fp_start.x > fp_end.x)? fp_start.x : fp_end.x;
            prec   y_min = (fp_start.y < fp_end.y)? fp_start.y : fp_end.y;
            prec   y_max = (fp_start.y > fp_end.y)? fp_start.y : fp_end.y;
            prec   bbox_area = (x_max - x_min) * (y_max - y_min);
            //prec   x_min = fp_start.x;
            //prec   y_min = fp_start.y;
            //prec   x_max = fp_end.x;
            //prec   y_max = fp_end.y;

            b0.x = (int)((x_min - tier->tile_org.x) * tier->inv_tile_stp.x);
            b0.y = (int)((y_min - tier->tile_org.y) * tier->inv_tile_stp.y);
            b1.x = (int)((x_max - tier->tile_org.x) * tier->inv_tile_stp.x);
            b1.y = (int)((y_max - tier->tile_org.y) * tier->inv_tile_stp.y);

            //cout <<net->name
            //  //<<", "
            //  //<<moduleInstance[pin_start->moduleID].name
            //  //<<", " <<moduleInstance[pin_end->moduleID].name
            //  <<", ll(" <<x_min <<", " <<y_min <<"), ur(" <<x_max <<", "
            //  <<y_max <<"), "
            //  //<<"cc(" <<center_start.x <<", " <<center_start.y <<"), "
            //  //<<"cc(" <<center_end.x <<", " <<center_end.y <<"); "
            //  <<"tilell("
            //  <<b0.x <<", " <<b0.y <<"), tileur(" <<b1.x <<", "
            //  <<b1.y <<")" <<endl;

            if (b0.x < 0)                   b0.x = 0;
            if (b0.x > tier->dim_tile.x-1)  b0.x = tier->dim_tile.x-1;
            if (b0.y < 0)                   b0.y = 0;
            if (b0.y > tier->dim_tile.y-1)  b0.y = tier->dim_tile.y-1;

            if (b1.x < 0)                   b1.x = 0;
            if (b1.x > tier->dim_tile.x-1)  b1.x = tier->dim_tile.x-1;
            if (b1.y < 0)                   b1.y = 0;
            if (b1.y > tier->dim_tile.y-1)  b1.y = tier->dim_tile.y-1;

            ll.x = min(ll.x, b0.x);
            ll.y = min(ll.y, b0.y);
            ur.x = max(ur.x, b1.x);
            ur.y = max(ur.y, b1.y);

            idx = b0.x * tier->dim_tile.y + b0.y;

            for (x=b0.x, bpx=&tier->tile_mat[idx]; x<=b1.x; x++,
bpx+=tier->dim_tile.y) {
                max_x = min (bpx->pmax.x, x_max);
                min_x = max (bpx->pmin.x, x_min);

                for (y=b0.y, bpy=bpx; y<=b1.y; y++, bpy++) {
                    max_y = min (bpy->pmax.y, y_max);
                    min_y = max (bpy->pmin.y, y_min);
                    area_share = (max_x - min_x) * (max_y - min_y);

                    //sanity check: passed
                    //if (bpx->pmin.x != bpy->pmin.x || bpx->pmax.x !=
bpy->pmax.x) {
                    //  cout <<"Error" <<endl;
                    //}

                    //sanity check: passed
                    //if (area_share > bpy->area) {
                    //  cout <<"area_share(" <<x <<", " <<y <<"): " <<area_share
<<endl;
                    //}

                    if (bbox_area != 0) {
                      prec   h_wirelength = area_share / bbox_area * (x_max -
x_min);
                      prec   v_wirelength = area_share / bbox_area * (y_max -
y_min);
                      bpy->tmp_h_usage = max(bpy->tmp_h_usage, h_wirelength);
                      bpy->tmp_v_usage = max(bpy->tmp_v_usage, v_wirelength);
                    }
                }
            }
        } else {
        }
    }

    //cout <<"tilell(" <<ll.x <<", " <<ll.y <<"), tileur(" <<ur.x <<", " <<ur.y
<<")" <<endl;
    idx = ll.x * tier->dim_tile.y + ll.y;

    //cout <<ll.x <<" " <<ll.y <<" " <<ur.x <<" " <<ur.y <<endl;

    for (x=ll.x, bpx=&tier->tile_mat[idx]; x<=ur.x; x++, bpx+=tier->dim_tile.y)
{
        for (y=ll.y, bpy=bpx; y<=ur.y; y++, bpy++) {
            bpy->h_usage += bpy->tmp_h_usage;
            bpy->v_usage += bpy->tmp_v_usage;
            bpy->tmp_h_usage = 0;
            bpy->tmp_v_usage = 0;
        }
    }

    // sanity check: passed
    //for (unsigned i = 0; i < tier->tot_tile_cnt; i++) {
    //  if (tier->tile_mat[i].tmp_h_usage != 0) {
    //    cout <<"ErrorH" <<endl;
    //  }
    //  if (tier->tile_mat[i].tmp_v_usage != 0) {
    //    cout <<"ErrorV" <<endl;
    //  }
    //}

}
*/

void calcCong_print_detail() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;

  /*
  cout <<"supplymap_h: " <<endl;
  for (int j = 0; j < tier->dim_tile.y; j++) {
    for (int i = 0 ; i < tier->dim_tile.x; i++) {
      bp = &tier->tile_mat[i * tier->dim_tile.y + j];
      if (i == tier->dim_tile.x - 1) {
        auto temp = bp->h_supply;
        cout <<temp <<"; ";
      } else {
        auto temp = bp->h_supply;
        cout <<temp <<", ";
      }
    }
  }
  cout <<endl <<"end supplymap_h" <<endl;
  cout <<"supplymap_v: " <<endl;
  for (int j = 0; j < tier->dim_tile.y; j++) {
    for (int i = 0 ; i < tier->dim_tile.x; i++) {
      bp = &tier->tile_mat[i * tier->dim_tile.y + j];
      if (i == tier->dim_tile.x - 1) {
        auto temp = bp->v_supply;
        cout <<temp <<"; ";
      } else {
        auto temp = bp->v_supply;
        cout <<temp <<", ";
      }
    }
  }
  cout <<endl <<"end supplymap_v" <<endl;
  */

  cout << "congmap_h: " << endl;
  for(int j = 0; j < tier->dim_tile.y; j++) {
    for(int i = 0; i < tier->dim_tile.x; i++) {
      bp = &tier->tile_mat[i * tier->dim_tile.y + j];
      if(i == tier->dim_tile.x - 1) {
        // cout <<bp->h_usage/ <<"; ";
        prec temp = bp->h_usage / bp->h_supply;
        if(bp->h_supply <= 0.01) {
          temp = 1;
        }
        cout << temp << "; ";
      }
      else {
        // cout <<bp->h_usage <<", ";
        prec temp = bp->h_usage / bp->h_supply;
        if(bp->h_supply <= 0.01) {
          temp = 1;
        }
        cout << temp << ", ";
      }
    }
    // cout <<endl;
  }
  cout << endl << "end congmap_h" << endl;

  cout << "congmap_v: " << endl;
  for(int j = 0; j < tier->dim_tile.y; j++) {
    for(int i = 0; i < tier->dim_tile.x; i++) {
      bp = &tier->tile_mat[i * tier->dim_tile.y + j];
      if(i == tier->dim_tile.x - 1) {
        // cout <<bp->v_usage/bp->v_supply <<"; ";
        prec temp = bp->v_usage / bp->v_supply;
        if(bp->v_supply <= 0.01) {
          temp = 1;
        }
        cout << temp << "; ";
      }
      else {
        // cout <<bp->v_usage/bp->v_supply <<", ";
        prec temp = bp->v_usage / bp->v_supply;
        if(bp->v_supply <= 0.01) {
          temp = 1;
        }
        cout << temp << ", ";
      }
    }
    // cout <<endl;
  }
  cout << endl << "end congmap_v" << endl;
}

void calcCong_print() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;
  double tot_route_h_ovfl = 0;
  double tot_route_v_ovfl = 0;
  int ovfl_tile_cnt = 0;
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    tot_route_h_ovfl += (prec)max(0.0, (double)bp->h_usage - bp->h_supply);
    tot_route_v_ovfl += (prec)max(0.0, (double)bp->v_usage - bp->v_supply);
    if(bp->h_usage > bp->h_supply || bp->v_usage > bp->v_supply) {
      ovfl_tile_cnt++;
      // cout <<"ovfl_tile_idx = " <<i <<endl;
    }
  }
  cout << "tot_route_h_ovfl = " << tot_route_h_ovfl << endl;
  cout << "tot_route_v_ovfl = " << tot_route_v_ovfl << endl;
  cout << "ovfl_tile_cnt = " << ovfl_tile_cnt << endl;

  double tot_route_h_ovfl2 = 0;
  double tot_route_v_ovfl2 = 0;
  int ovfl_tile_cnt2 = 0;
  // double ignoreEdgeRatio = 0.8;
  std::vector< double > horEdgeCongArray;
  std::vector< double > verEdgeCongArray;
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    for(int j = 0; j < nMetLayers; j++) {
      if(horizontalCapacity[j] != 0) {
        if(bp->blkg[j] > ignoreEdgeRatio * horizontalCapacity[j])
          continue;
        tot_route_h_ovfl2 +=
            (prec)max(0.0, -1 + bp->route[j] * 1.0 / horizontalCapacity[j]);
        horEdgeCongArray.push_back(bp->route[j] * 1.0 / horizontalCapacity[j]);
        if(bp->route[j] - horizontalCapacity[j] > 0) {
          ovfl_tile_cnt2++;
        }
      }
      else if(verticalCapacity[j] != 0) {
        if(bp->blkg[j] > ignoreEdgeRatio * verticalCapacity[j])
          continue;
        tot_route_v_ovfl2 +=
            (prec)max(0.0, -1 + bp->route[j] * 1.0 / verticalCapacity[j]);
        verEdgeCongArray.push_back(bp->route[j] * 1.0 / verticalCapacity[j]);
        if(bp->route[j] - verticalCapacity[j] > 0) {
          ovfl_tile_cnt2++;
        }
      }
    }
  }

  cout << "tot_route_h_ovfl2 = " << tot_route_h_ovfl2 << endl;
  cout << "tot_route_v_ovfl2 = " << tot_route_v_ovfl2 << endl;
  cout << "ovfl_tile_cnt2 = " << ovfl_tile_cnt2 << endl;

  int horArraySize = horEdgeCongArray.size();
  int verArraySize = verEdgeCongArray.size();

  std::sort(horEdgeCongArray.rbegin(), horEdgeCongArray.rend());
  std::sort(verEdgeCongArray.rbegin(), verEdgeCongArray.rend());

  double horAvg005RC = 0;
  double horAvg010RC = 0;
  double horAvg020RC = 0;
  double horAvg050RC = 0;
  for(int i = 0; i < horArraySize; ++i) {
    if(i < 0.005 * horArraySize) {
      horAvg005RC += horEdgeCongArray[i];
    }
    if(i < 0.01 * horArraySize) {
      horAvg010RC += horEdgeCongArray[i];
    }
    if(i < 0.02 * horArraySize) {
      horAvg020RC += horEdgeCongArray[i];
    }
    if(i < 0.05 * horArraySize) {
      horAvg050RC += horEdgeCongArray[i];
    }
  }
  horAvg005RC /= 1.0 * 0.005 * horArraySize;
  horAvg010RC /= 1.0 * 0.010 * horArraySize;
  horAvg020RC /= 1.0 * 0.020 * horArraySize;
  horAvg050RC /= 1.0 * 0.050 * horArraySize;

  double verAvg005RC = 0;
  double verAvg010RC = 0;
  double verAvg020RC = 0;
  double verAvg050RC = 0;
  for(int i = 0; i < verArraySize; ++i) {
    if(i < 0.005 * verArraySize) {
      verAvg005RC += verEdgeCongArray[i];
    }
    if(i < 0.01 * verArraySize) {
      verAvg010RC += verEdgeCongArray[i];
    }
    if(i < 0.02 * verArraySize) {
      verAvg020RC += verEdgeCongArray[i];
    }
    if(i < 0.05 * verArraySize) {
      verAvg050RC += verEdgeCongArray[i];
    }
  }
  verAvg005RC /= 1.0 * 0.005 * verArraySize;
  verAvg010RC /= 1.0 * 0.010 * verArraySize;
  verAvg020RC /= 1.0 * 0.020 * verArraySize;
  verAvg050RC /= 1.0 * 0.050 * verArraySize;

  cout << "topRC = (" << max(horAvg005RC, verAvg005RC) << ", "
       << max(horAvg010RC, verAvg010RC) << ", " << max(horAvg020RC, verAvg020RC)
       << ", " << max(horAvg050RC, verAvg050RC) << ")" << endl;

  auto finalRC = (1.0 * max(horAvg005RC, verAvg005RC) +
                  1.0 * max(horAvg010RC, verAvg010RC) +
                  1.0 * max(horAvg020RC, verAvg020RC) +
                  1.0 * max(horAvg050RC, verAvg050RC)) /
                 (1.0 + 1.0 + 1.0 + 1.0);

  cout << "finalRC = " << finalRC << endl;
  if(finalRC < 1.01)
    flg_noroute = true;
}

void calcCong(struct FPOS *st, int est_method) {
  struct NET *net = NULL;

  tile_clear();
  tile_reset_gr_usages();
  if(est_method == prob_ripple_based) {
    for(int i = 0; i < netCNT; i++) {
      net = &netInstance[i];
      auto aa = st;
      aa++;
      // calcCongPerNet_prob_based (st, net);
    }
  }
  else if(est_method == global_router_based) {
    for(int i = 0; i < netCNT; i++) {
      net = &netInstance[i];
      calcCongPerNet_grouter_based(net);
    }
    get_gr_usages_total();
  }
}

void CalcPinDensity(struct FPOS *st) {
  // struct  CELL     *cell = NULL;

  // for (int i=0; i<gcell_cnt; i++) {
  //  cell = &gcell_st[i];
  //  CalcPinDensityPerCell(cell);
  //}

  struct NET *net = NULL;
  struct TILE *bp = NULL;
  struct POS b0;
  struct TIER *tier = &tier_st[0];

  for(int i = 0; i < netCNT; i++) {
    net = &netInstance[i];
    for(int i = 0; i < net->pinCNTinObject; ++i) {
      struct PIN *pin_start = net->pin[i];
      // struct MODULE *tempInst_start = &moduleInstance[pin_start->moduleID] ;
      struct FPOS center_start = pin_start->fp;
      struct FPOS pof_start;
      if(!pin_start->term) {
        struct MODULE *tempInst_start = &moduleInstance[pin_start->moduleID];
        pof_start = tempInst_start->pof[pin_start->pinIDinModule];
        center_start = st[pin_start->moduleID];
        // cout <<net->name <<" " <<tempInst_start->name <<endl;
      }
      else {
        struct TERM *tempInst_start = &terminalInstance[pin_start->moduleID];
        pof_start = tempInst_start->pof[pin_start->pinIDinModule];
        // cout <<net->name <<" " <<tempInst_start->name <<endl;
      }
      struct FPOS p;
      p.x = center_start.x + pof_start.x;
      // x1[i] = (int)(x[i] * 100.0);
      p.y = center_start.y + pof_start.y;
      // y1[i] = (int)(y[i] * 100.0);
      // cout <<x[i] <<" " <<y[i] <<endl;
      b0.x = (int)((p.x - tier->tile_org.x) * tier->inv_tile_stp.x);
      b0.y = (int)((p.y - tier->tile_org.y) * tier->inv_tile_stp.y);

      if(b0.x < 0)
        b0.x = 0;
      if(b0.x > tier->dim_tile.x - 1)
        b0.x = tier->dim_tile.x - 1;
      if(b0.y < 0)
        b0.y = 0;
      if(b0.y > tier->dim_tile.y - 1)
        b0.y = tier->dim_tile.y - 1;

      auto idx = b0.x * tier->dim_tile.y + b0.y;
      bp = &tier->tile_mat[idx];
      (bp->pincnt)++;
    }
  }
}

void MergePinDen2Route() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;
  struct POS b0;
  int minPinLayer = 1;
  int maxPinLayer = 2;
  prec pinBlkFac = 0.05;
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    b0.x = i / tier->dim_tile.y;
    b0.y = i % tier->dim_tile.y;
    if(b0.x < 0)
      b0.x = 0;
    if(b0.x > tier->dim_tile.x - 1)
      b0.x = tier->dim_tile.x - 1;
    if(b0.y < 0)
      b0.y = 0;
    if(b0.y > tier->dim_tile.y - 1)
      b0.y = tier->dim_tile.y - 1;

    for(int j = minPinLayer; j <= maxPinLayer; j++) {
      if(horizontalCapacity[j] != 0) {
        bp->route[j] += ceil(pinBlkFac * bp->pincnt) *
                        (minWireWidth[j] + minWireSpacing[j]);
        if(b0.x + 1 < tier->dim_tile.x) {
          bp->route[j] +=
              ceil(
                  pinBlkFac *
                  tier->tile_mat[(b0.x + 1) * tier->dim_tile.y + b0.y].pincnt) *
              (minWireWidth[j] + minWireSpacing[j]);
        }
      }
      if(verticalCapacity[j] != 0) {
        bp->route[j] += ceil(pinBlkFac * bp->pincnt) *
                        (minWireWidth[j] + minWireSpacing[j]);
        if(b0.y + 1 < tier->dim_tile.y) {
          bp->route[j] += ceil(pinBlkFac * tier->tile_mat[i + 1].pincnt) *
                          (minWireWidth[j] + minWireSpacing[j]);
        }
      }
    }
  }
}

void MergeBlkg2Route() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    for(int j = 0; j < nMetLayers; j++) {
      bp->route[j] += bp->blkg[j];
    }
  }
}

void CalcPinDensityPerCell(struct CELL *cell) {
  struct TILE *bp = NULL;
  struct POS b0;
  struct TIER *tier = &tier_st[0];

  if(cell->flg == FillerCell)
    return;
  if(cell->flg == Macro)
    return;

  b0.x = (int)((cell->center.x - tier->tile_org.x) * tier->inv_tile_stp.x);
  b0.y = (int)((cell->center.y - tier->tile_org.y) * tier->inv_tile_stp.y);

  if(b0.x < 0)
    b0.x = 0;
  if(b0.x > tier->dim_tile.x - 1)
    b0.x = tier->dim_tile.x - 1;
  if(b0.y < 0)
    b0.y = 0;
  if(b0.y > tier->dim_tile.y - 1)
    b0.y = tier->dim_tile.y - 1;

  auto idx = b0.x * tier->dim_tile.y + b0.y;
  bp = &tier->tile_mat[idx];
  bp->pincnt += cell->pinCNTinObject;
}

void prepare_bloat_PIN_info() {
  ;
}

void restore_org_PIN_info() {
  ;
}

void prepare_bloat_MODULE_info() {
  ;
}

void restore_org_MODULE_info() {
  ;
}

void backup_org_CELL_info(struct CELL *cell) {
  cell->area_org_befo_bloating = cell->area;
  cell->den_scal_org_befo_bloating = cell->den_scal;
  cell->size_org_befo_bloating = cell->size;
  cell->half_size_org_befo_bloating = cell->half_size;
  cell->half_den_size_org_befo_bloating = cell->half_den_size;
}

void prepare_bloat_CELL_info() {
  ;
}

void restore_org_CELL_info(struct CELL *cell) {
  struct TIER *tier = &tier_st[0];

  cell->area = cell->area_org_befo_bloating;
  cell->den_scal = cell->den_scal_org_befo_bloating;
  cell->size = cell->size_org_befo_bloating;
  cell->half_size = cell->half_size_org_befo_bloating;
  cell->half_den_size = cell->half_den_size_org_befo_bloating;

  // Prevent cell X-coor going to outside of tier.
  if((cell->center.x - cell->half_size.x) < tier->pmin.x) {
    cell->pmax.x = cell->pmin.x + cell->size.x;
    cell->center.x = (cell->pmin.x + cell->pmax.x) * 0.5;
  }
  else if((cell->center.x + cell->half_size.x) > tier->pmax.x) {
    cell->pmin.x = cell->pmax.x - cell->size.x;
    cell->center.x = (cell->pmin.x + cell->pmax.x) * 0.5;
  }
  else {
    cell->pmin.x = cell->center.x - cell->half_size.x;
    cell->pmax.x = cell->center.x + cell->half_size.x;
  }

  // Prevent cell Y-coor going to outside of tier.
  if((cell->center.y - cell->half_size.y) < tier->pmin.y) {
    cell->pmax.y = cell->pmin.y + cell->size.y;
    cell->center.y = (cell->pmin.y + cell->pmax.y) * 0.5;
  }
  else if((cell->center.y + cell->half_size.y) > tier->pmax.y) {
    cell->pmin.y = cell->pmax.y - cell->size.y;
    cell->center.y = (cell->pmin.y + cell->pmax.y) * 0.5;
  }
  else {
    cell->pmin.y = cell->center.y - cell->half_size.y;
    cell->pmax.y = cell->center.y + cell->half_size.y;
  }
  cell->den_pmin.x = cell->center.x - cell->half_den_size.x;
  cell->den_pmin.y = cell->center.y - cell->half_den_size.y;
  cell->den_pmax.x = cell->center.x + cell->half_den_size.x;
  cell->den_pmax.y = cell->center.y + cell->half_den_size.y;
}

void prepare_bloat_TERM_info() {
  ;
}

void restore_org_TERM_info() {
}

void calcInflationRatio_foreachTile() {
  prec temp_h_max_inflation_ratio = 0;
  prec temp_v_max_inflation_ratio = 0;
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;
  struct TILE *bp_temp = NULL;
  struct POS bmax;
  struct POS b0;

  bmax.x = tier->dim_tile.x - 1;
  bmax.y = tier->dim_tile.y - 1;
  for(int i = 0; i <= bmax.x; i++) {
    for(int j = 0; j <= bmax.y; j++) {
      auto m_min = max(0, i - routepath_maxdist);
      auto m_max = min(bmax.x, i + routepath_maxdist);
      auto n_min = j;
      auto n_max = j;
      auto idx = i * tier->dim_tile.y + j;
      bp = &tier->tile_mat[idx];
      for(auto m = m_min; m <= m_max; ++m) {
        for(auto n = n_min; n <= n_max; ++n) {
          auto idx_temp = m * tier->dim_tile.y + n;
          bp_temp = &tier->tile_mat[idx_temp];
          // LW mod 11/26/16
          if(idx != idx_temp && bp_temp->is_macro_included == false)
            continue;
          bp->v_inflation_ratio = (prec)max(
              (double)bp->v_inflation_ratio,
              (double)pow((bp_temp->h_usage + pincnt_coef * bp_temp->pincnt) /
                              bp_temp->h_supply,
                          inflation_ratio_coef));
          bp->v_inflation_ratio = (prec)min((double)max_inflation_ratio,
                                            (double)bp->v_inflation_ratio);
          if(bp->v_inflation_ratio > temp_v_max_inflation_ratio) {
            temp_v_max_inflation_ratio = bp->v_inflation_ratio;
          }
        }
      }
    }
  }
  for(int i = 0; i <= bmax.x; i++) {
    for(int j = 0; j <= bmax.y; j++) {
      auto m_min = i;
      auto m_max = i;
      auto n_min = max(0, j - routepath_maxdist);
      auto n_max = min(bmax.y, j + routepath_maxdist);
      auto idx = i * tier->dim_tile.y + j;
      bp = &tier->tile_mat[idx];
      for(auto m = m_min; m <= m_max; ++m) {
        for(auto n = n_min; n <= n_max; ++n) {
          auto idx_temp = m * tier->dim_tile.y + n;
          bp_temp = &tier->tile_mat[idx_temp];
          // LW mod 11/26/16
          if(idx != idx_temp && bp_temp->is_macro_included == false)
            continue;
          bp->h_inflation_ratio = (prec)max(
              (double)bp->h_inflation_ratio,
              (double)pow((bp_temp->v_usage + pincnt_coef * bp_temp->pincnt) /
                              bp_temp->v_supply,
                          inflation_ratio_coef));
          bp->h_inflation_ratio = (prec)min((double)max_inflation_ratio,
                                            (double)bp->h_inflation_ratio);
          if(bp->h_inflation_ratio > temp_h_max_inflation_ratio) {
            temp_h_max_inflation_ratio = bp->h_inflation_ratio;
          }
        }
      }
    }
  }
  h_max_inflation_ratio = temp_h_max_inflation_ratio;
  v_max_inflation_ratio = temp_v_max_inflation_ratio;
  cout << "hv_inflation_ratio = " << h_max_inflation_ratio << ", "
       << v_max_inflation_ratio << endl;

  // new
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    b0.x = i / tier->dim_tile.y;
    b0.y = i % tier->dim_tile.y;
    for(int j = 0; j < nMetLayers; j++) {
      if(horizontalCapacity[j] != 0) {
        if(bp->blkg[j] > ignoreEdgeRatio * horizontalCapacity[j]) {
        }
        else {
          bp->infl_ratio = (prec)max(bp->infl_ratio, bp->route[j] * (prec)1.0 /
                                                         horizontalCapacity[j] *
                                                         gRoute_pitch_scal);
        }

        if(b0.x - 1 >= 0) {
          bp_temp = &tier->tile_mat[(b0.x - 1) * tier->dim_tile.y + b0.y];
          if(bp_temp->blkg[j] > ignoreEdgeRatio * horizontalCapacity[j]) {
          }
          else {
            bp->infl_ratio = (prec)max(
                bp->infl_ratio, bp_temp->route[j] * (prec)1.0 /
                                    horizontalCapacity[j] * gRoute_pitch_scal);
          }
        }
      }
      else if(verticalCapacity[j] != 0) {
        if(bp->blkg[j] > ignoreEdgeRatio * verticalCapacity[j]) {
        }
        else {
          bp->infl_ratio = (prec)max(bp->infl_ratio, bp->route[j] * (prec)1.0 /
                                                         verticalCapacity[j] *
                                                         gRoute_pitch_scal);
        }

        if(b0.y - 1 >= 0) {
          bp_temp = &tier->tile_mat[i - 1];
          if(bp_temp->blkg[j] > ignoreEdgeRatio * verticalCapacity[j]) {
          }
          else {
            bp->infl_ratio = (prec)max(
                bp->infl_ratio, bp_temp->route[j] * (prec)1.0 /
                                    verticalCapacity[j] * gRoute_pitch_scal);
          }
        }
      }
    }
    if(bp->infl_ratio > 1.0) {
      bp->infl_ratio = pow(bp->infl_ratio, inflation_ratio_coef);
    }
  }
}

/*
void calcInflationRatio_foreachTile () {
    prec            temp_h_max_inflation_ratio = 0;
    prec            temp_v_max_inflation_ratio = 0;
    struct  TIER    *tier   = &tier_st[0];
    struct  TILE    *bp     = NULL;

    //if (conges_eval_methodCMD == prob_ripple_based) {
        //cout << "INFO:  Your congestion est. method is based on
probabilistic." << endl;
        for (int i=0; i<tier->tot_tile_cnt; i++) {
            bp = &tier->tile_mat[i];
            // mod LW 10/30/16, add sub/super-linear bloating
            bp->v_inflation_ratio
                = min(max_inflation_ratio, pow((bp->h_usage + pincnt_coef *
bp->pincnt) / bp->h_supply, inflation_ratio_coef) );
            bp->h_inflation_ratio
                = min(max_inflation_ratio, pow((bp->v_usage + pincnt_coef *
bp->pincnt) / bp->v_supply, inflation_ratio_coef));
            //cout <<"idx = " <<i <<", v_ratio = " <<bp->v_inflation_ratio <<",
h_ratio = " <<bp->h_inflation_ratio <<endl;
            if (bp->h_inflation_ratio > temp_h_max_inflation_ratio) {
                temp_h_max_inflation_ratio = bp->h_inflation_ratio;
            }
            if (bp->v_inflation_ratio > temp_v_max_inflation_ratio) {
                temp_v_max_inflation_ratio = bp->v_inflation_ratio;
            }
        }
        h_max_inflation_ratio = temp_h_max_inflation_ratio;
        v_max_inflation_ratio = temp_v_max_inflation_ratio;
        cout <<"hv_inflation_ratio = "<<h_max_inflation_ratio <<", "
<<v_max_inflation_ratio <<endl;
    //} else if (conges_eval_methodCMD == global_router_based) {
    //    cout << "INFO:  Your congestion est. method is based on global router
(NCTUgr)." << endl;
    //    for (int i=0; i<tier->tot_tile_cnt; i++) {
    //        bp = &tier->tile_mat[i];
    //        // mod LW 10/30/16, add sub/super-linear bloating
    //        bp->v_inflation_ratio
    //            = min(max_inflation_ratio, );
    //        bp->h_inflation_ratio
    //            = min(max_inflation_ratio, );
    //        //cout <<"idx = " <<i <<", v_ratio = " <<bp->v_inflation_ratio
<<", h_ratio = " <<bp->h_inflation_ratio <<endl;
    //        if (bp->h_inflation_ratio > temp_h_max_inflation_ratio) {
    //            temp_h_max_inflation_ratio = bp->h_inflation_ratio;
    //        }
    //        if (bp->v_inflation_ratio > temp_v_max_inflation_ratio) {
    //            temp_v_max_inflation_ratio = bp->v_inflation_ratio;
    //        }
    //    }
    //    h_max_inflation_ratio = temp_h_max_inflation_ratio;
    //    v_max_inflation_ratio = temp_v_max_inflation_ratio;
    //    cout <<"hv_inflation_ratio = "<<h_max_inflation_ratio <<", "
<<v_max_inflation_ratio <<endl;
    //}
}
*/

void printInflationRatio() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;
  int last_x = 0;

  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    if(bp->p.x != last_x)
      cout << endl;
    cout << bp->h_inflation_ratio << " ";
    last_x = bp->p.x;
  }

  cout << endl << endl;
  last_x = 0;

  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    if(bp->p.x != last_x)
      cout << endl;
    cout << bp->v_inflation_ratio << " ";
    last_x = bp->p.x;
  }
}

void cell_calc_new_area_per_Cell(struct CELL *cell, struct TILE *bp) {
  struct FPOS cell_size;

  // if (bp->h_inflation_ratio > 1.0 && is_inflation_h == true) {
  //    cell_size.x = cell->size.x * bp->h_inflation_ratio;
  //} else {
  //    cell_size.x = cell->size.x;
  //    ;
  //}
  // if (bp->v_inflation_ratio > 1.0 && is_inflation_h == false) {
  //    cell_size.y = cell->size.y * bp->v_inflation_ratio;
  //} else {
  //    cell_size.y = cell->size.y;
  //}

  if(bp->infl_ratio > 1.0) {
    cell_size.x = cell->size.x * sqrt(bp->infl_ratio);
    cell_size.y = cell->size.y * sqrt(bp->infl_ratio);
  }
  else {
    cell_size.x = cell->size.x;
    cell_size.y = cell->size.y;
  }

  cell->inflatedNewArea = cell_size.x * cell_size.y;
  cell->inflatedNewAreaDelta = cell->inflatedNewArea - cell->area;
}

void cell_inflation_per_Cell(struct CELL *cell, struct TILE *bp) {
  struct TIER *tier = &tier_st[0];

  // if (bp->h_inflation_ratio > 0.90 && is_inflation_h == true) {
  //    // modify today!!!
  //    cell->size.x            *= bp->h_inflation_ratio;
  //    cell->half_size.x       = 0.5 * cell->size.x;
  //    //cout <<"test" <<endl;
  //} else if (bp->h_inflation_ratio <= 0.90 && is_inflation_h == true) {
  //    cell->size.x            *= 0.90;
  //    cell->half_size.x       = 0.5 * cell->size.x;
  //}
  // if (bp->v_inflation_ratio > 0.90 && is_inflation_h == false) {
  //    cell->size.y            *= bp->v_inflation_ratio;
  //    cell->half_size.y       = 0.5 * cell->size.y;
  //    //cout <<"test" <<endl;
  //} else if (bp->v_inflation_ratio <= 0.90 && is_inflation_h == false) {
  //    cell->size.y            *= 0.90;
  //    cell->half_size.y       = 0.5 * cell->size.y;
  //}

  // if (bp->h_inflation_ratio > 1.0 && is_inflation_h == true) {

  //    // modify today!!!
  //    cell->size.x            *= bp->h_inflation_ratio;
  //    cell->half_size.x       = 0.5 * cell->size.x;
  //    //cout <<"test" <<endl;
  //}
  // if (bp->v_inflation_ratio > 1.0 && is_inflation_h == false) {
  //    cell->size.y            *= bp->v_inflation_ratio;
  //    cell->half_size.y       = 0.5 * cell->size.y;
  //    //cout <<"test" <<endl;
  //}

  if(bp->infl_ratio > 1.0) {
    cell->size.x *= sqrt(bp->infl_ratio);
    cell->half_size.x = 0.5 * cell->size.x;
    cell->size.y *= sqrt(bp->infl_ratio);
    cell->half_size.y = 0.5 * cell->size.y;
  }

  // Prevent cell X-coor going to outside of tier.
  if((cell->center.x - cell->half_size.x) < tier->pmin.x) {
    cell->pmax.x = cell->pmin.x + cell->size.x;
    cell->center.x = (cell->pmin.x + cell->pmax.x) * 0.5;
  }
  else if((cell->center.x + cell->half_size.x) > tier->pmax.x) {
    cell->pmin.x = cell->pmax.x - cell->size.x;
    cell->center.x = (cell->pmin.x + cell->pmax.x) * 0.5;
  }
  else {
    // prec   temp1 = cell->center.x - cell->half_size.x;
    // struct FPOS temp = fp_add (cell->center, fp_scal (-0.5, cell->size));
    // if (abs(cell->pmin.x - temp.x) > 0.000001) {
    //    cout <<"xxxxxxxx" <<cell->center.x <<" "
    //         <<cell->half_size.x <<" " <<cell->pmin.x <<" " <<temp.x <<endl;
    //}
    cell->pmin.x = cell->center.x - cell->half_size.x;
    cell->pmax.x = cell->center.x + cell->half_size.x;
  }

  // Prevent cell Y-coor going to outside of tier.
  if((cell->center.y - cell->half_size.y) < tier->pmin.y) {
    cell->pmax.y = cell->pmin.y + cell->size.y;
    cell->center.y = (cell->pmin.y + cell->pmax.y) * 0.5;
  }
  else if((cell->center.y + cell->half_size.y) > tier->pmax.y) {
    cell->pmin.y = cell->pmax.y - cell->size.y;
    cell->center.y = (cell->pmin.y + cell->pmax.y) * 0.5;
  }
  else {
    cell->pmin.y = cell->center.y - cell->half_size.y;
    cell->pmax.y = cell->center.y + cell->half_size.y;
  }

  cell->inflatedNewArea = cell->size.x * cell->size.y;
  cell->inflatedNewAreaDelta = cell->inflatedNewArea - cell->area;
  cell->area = cell->size.x * cell->size.y;
}

void cell_den_scal_update_forNewGrad_inNSopt(struct CELL *cell) {
  struct FPOS scal;
  struct TIER *tier = &tier_st[0];

  if(DEN_ONLY_PRECON) {
    if(cell->size.x < tier->bin_stp.x) {
      scal.x = cell->size.x / (tier->bin_stp.x);
      cell->half_den_size.x = tier->half_bin_stp.x;
    }
    else {
      scal.x = 1.0;
      cell->half_den_size.x = cell->half_size.x;
    }

    if(cell->size.y < tier->bin_stp.y) {
      scal.y = cell->size.y / (tier->bin_stp.y);
      cell->half_den_size.y = tier->half_bin_stp.y;
    }
    else {
      scal.y = 1.0;
      cell->half_den_size.y = cell->half_size.y;
    }
  }
  else {
    if(cell->size.x < tier->bin_stp.x * SQRT2) {
      scal.x = cell->size.x / (tier->bin_stp.x * SQRT2);
      cell->half_den_size.x = tier->half_bin_stp.x * SQRT2;
    }
    else {
      scal.x = 1.0;
      cell->half_den_size.x = cell->half_size.x;
    }

    if(cell->size.y < tier->bin_stp.y * SQRT2) {
      scal.y = cell->size.y / (tier->bin_stp.y * SQRT2);
      cell->half_den_size.y = tier->half_bin_stp.y * SQRT2;
    }
    else {
      scal.y = 1.0;
      cell->half_den_size.y = cell->half_size.y;
    }
  }

  cell->den_scal = scal.x * scal.y;

  cell->den_pmin.x = cell->center.x - cell->half_den_size.x;
  cell->den_pmin.y = cell->center.y - cell->half_den_size.y;
  cell->den_pmax.x = cell->center.x + cell->half_den_size.x;
  cell->den_pmax.y = cell->center.y + cell->half_den_size.y;
}

void bloat_prep() {
  int idx = 0;
  struct TILE *bp = NULL;
  struct POS b0;
  struct TIER *tier = &tier_st[0];
  struct CELL *cell = NULL;

  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    bp->cell_area_befo_bloating_thisTile = 0;
    bp->inflation_area_thisTile = 0;
    bp->inflation_area_delta_thisTile = 0;
  }

  for(int i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];
    if(cell->flg == FillerCell)
      continue;
    // LW mod 10/18/16
    if(cell->flg == Macro)
      continue;

    b0.x = (int)((cell->center.x - tier->tile_org.x) * tier->inv_tile_stp.x);
    b0.y = (int)((cell->center.y - tier->tile_org.y) * tier->inv_tile_stp.y);

    if(b0.x < 0)
      b0.x = 0;
    if(b0.x > tier->dim_tile.x - 1)
      b0.x = tier->dim_tile.x - 1;
    if(b0.y < 0)
      b0.y = 0;
    if(b0.y > tier->dim_tile.y - 1)
      b0.y = tier->dim_tile.y - 1;

    idx = b0.x * tier->dim_tile.y + b0.y;
    bp = &tier->tile_mat[idx];

    cell->inflatedNewArea = 0;
    cell->inflatedNewAreaDelta = 0;

    // if (bp->h_inflation_ratio > 1.0 && is_inflation_h == true) {
    //    bp->cell_area_befo_bloating_thisTile += cell->area;
    //    cell_calc_new_area_per_Cell (cell, bp);
    //    bp->inflation_area_thisTile += cell->inflatedNewArea;
    //    bp->inflation_area_delta_thisTile += cell->inflatedNewAreaDelta;
    //}
    // if (bp->v_inflation_ratio > 1.0 && is_inflation_h == false) {
    //    bp->cell_area_befo_bloating_thisTile += cell->area;
    //    cell_calc_new_area_per_Cell (cell, bp);
    //    bp->inflation_area_thisTile += cell->inflatedNewArea;
    //    bp->inflation_area_delta_thisTile += cell->inflatedNewAreaDelta;
    //}

    bp->cell_area_befo_bloating_thisTile += cell->area;
    cell_calc_new_area_per_Cell(cell, bp);
    bp->inflation_area_thisTile += cell->inflatedNewArea;
    bp->inflation_area_delta_thisTile += cell->inflatedNewAreaDelta;
  }

  // for (int i=0; i<tier->tot_tile_cnt; i++) {
  //    bp = &tier->tile_mat[i];
  //    if (is_inflation_h == true) {
  //        if (bp->h_inflation_ratio > 1.0) {
  //            bp->inflatedRatio_thisTile = bp->h_inflation_ratio;
  //        } else {
  //            bp->inflatedRatio_thisTile = 1;
  //        }
  //    } else {
  //        if (bp->v_inflation_ratio > 1.0) {
  //            bp->inflatedRatio_thisTile = bp->v_inflation_ratio;
  //        } else {
  //            bp->inflatedRatio_thisTile = 1;
  //        }
  //    }
  //}
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    if(bp->infl_ratio > 1.0) {
      bp->inflatedRatio_thisTile = bp->infl_ratio;
    }
    else {
      bp->inflatedRatio_thisTile = 1;
    }
  }
}

void bloating() {
  int idx = 0;
  struct TILE *bp = NULL;
  struct POS b0;
  struct TIER *tier = &tier_st[0];
  struct CELL *cell = NULL;

  for(int i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];
    if(cell->flg == FillerCell)
      continue;

    b0.x = (int)((cell->center.x - tier->tile_org.x) * tier->inv_tile_stp.x);
    b0.y = (int)((cell->center.y - tier->tile_org.y) * tier->inv_tile_stp.y);

    if(b0.x < 0)
      b0.x = 0;
    if(b0.x > tier->dim_tile.x - 1)
      b0.x = tier->dim_tile.x - 1;
    if(b0.y < 0)
      b0.y = 0;
    if(b0.y > tier->dim_tile.y - 1)
      b0.y = tier->dim_tile.y - 1;

    idx = b0.x * tier->dim_tile.y + b0.y;
    bp = &tier->tile_mat[idx];

    // NEED TO FIX IF WE WANT TO SORT and APPLY THRESHOLD
    // if (bp->h_inflation_ratio > 1.0 || bp->v_inflation_ratio > 1.0) {
    ////if (bp->h_inflation_ratio > 0.90 || bp->v_inflation_ratio > 0.90) {
    //    cell_inflation_per_Cell (cell, bp);
    //    cell_den_scal_update_forNewGrad_inNSopt (cell);
    //}
    if(bp->infl_ratio > 1.0) {
      // if (bp->h_inflation_ratio > 0.90 || bp->v_inflation_ratio > 0.90) {
      cell_inflation_per_Cell(cell, bp);
      cell_den_scal_update_forNewGrad_inNSopt(cell);
    }
  }
}

void print_inflation_list() {
  int size = inflationList.size();

  unsigned top05percent = ceil(size * 0.005);
  if(top05percent == 0) {
    top05percent = 1;
  }
  unsigned top1percent = ceil(size * 0.01);
  if(top1percent == 0) {
    top1percent = 1;
  }
  unsigned top2percent = ceil(size * 0.02);
  if(top2percent == 0) {
    top2percent = 1;
  }
  unsigned top5percent = ceil(size * 0.05);
  if(top5percent == 0) {
    top5percent = 1;
  }

  prec top05ovfl = 0;
  prec top1ovfl = 0;
  prec top2ovfl = 0;
  prec top5ovfl = 0;

  if(size != 0) {
    for(unsigned i = 0; i < inflationList.size(); i++) {
      if(inflationList[size - 1 - i].second) {
        if(i < top05percent) {
          top05ovfl += inflationList[size - 1 - i].second;
        }
        if(i < top1percent) {
          top1ovfl += inflationList[size - 1 - i].second;
        }
        if(i < top2percent) {
          top2ovfl += inflationList[size - 1 - i].second;
        }
        if(i < top5percent) {
          top5ovfl += inflationList[size - 1 - i].second;
        }
      }
    }
    top05ovfl /= top05percent;
    top1ovfl /= top1percent;
    top2ovfl /= top2percent;
    top5ovfl /= top5percent;
    cout << "top05ovfl = " << top05ovfl << endl;
    cout << "top1ovfl = " << top1ovfl << endl;
    cout << "top2ovfl = " << top2ovfl << endl;
    cout << "top5ovfl = " << top5ovfl << endl;
    cout << "estRC = " << (top05ovfl + top1ovfl + top2ovfl + top5ovfl) / 4.0
         << endl;
  }

  // for (int i=0; i<tier->tot_tile_cnt; i++) {
  //    bp = &tier->tile_mat[i];
  //    if (bp->h_inflation_ratio > 1.0 && is_inflation_h == true) {
  //        cout <<bp->h_inflation_ratio <<endl;
  //    }

  //    if (bp->v_inflation_ratio > 1.0 && is_inflation_h == false) {
  //        cout <<bp->v_inflation_ratio <<endl;
  //    }
  //}
}

void gen_sort_InflationList() {
  struct TILE *bp = NULL;
  struct TIER *tier = &tier_st[0];

  inflationList.clear();
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    inflationList.push_back(
        std::pair< int, prec >(i, bp->inflatedRatio_thisTile));
    // if (bp->inflatedRatio_thisTile > 1) {
    //    cout <<"should be here" <<endl;
    //}
  }

  std::sort(inflationList.begin(), inflationList.end(), inflationList_comp);

  // for (unsigned i=0; i<inflationList.size(); i++) {
  //    if (inflationList[i].second > 1) {
  //        cout << "i          " << i << endl;
  //        cout << "vIndex     " << inflationList[i].first << endl;
  //        cout << "inRatio    " << inflationList[i].second << endl;
  //    }
  //}
}

bool inflationList_comp(std::pair< int, prec > a, std::pair< int, prec > b) {
  return a.second < b.second;
}

void dynamicInflationAdjustment() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;
  unsigned int firstTileLargerThan1 =
      max((int)0, (int)inflationList.size() - 1);

  for(unsigned i = 0; i < inflationList.size(); i++) {
    if(inflationList[i].second > 1) {
      firstTileLargerThan1 = i;
      // cout <<"testttttt: " <<i <<" " <<inflationList[i].second <<endl;
      break;
    }
    // cout <<inflationList[i].second <<endl;
  }

  for(unsigned i = firstTileLargerThan1; i < inflationList.size(); i++) {
    tier->tile_mat[inflationList[i].first].inflatedRatio_thisTile /=
        inflationList[firstTileLargerThan1].second;
  }
  // cout << "aa" << inflationList[firstTileLargerThan1].second << endl;
  // cout << "bb" << tier->tile_mat[1263].inflatedRatio_thisTile << endl;

  // for (int i=0; i<tier->tot_tile_cnt; i++) {
  //    bp = &tier->tile_mat[i];
  //    if (bp->h_inflation_ratio > 1.0 && is_inflation_h == true) {
  //        bp->h_inflation_ratio = bp->inflatedRatio_thisTile;
  //        //cout <<bp->h_inflation_ratio <<endl;
  //    }

  //    if (bp->v_inflation_ratio > 1.0 && is_inflation_h == false) {
  //        bp->v_inflation_ratio = bp->inflatedRatio_thisTile;
  //        //cout <<bp->v_inflation_ratio <<endl;
  //    }
  //}

  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    if(bp->infl_ratio > 1.0) {
      bp->infl_ratio = bp->inflatedRatio_thisTile;
    }
  }
}

void adjust_inflation() {
  calc_Total_inflated_cell_area();
  gen_sort_InflationList();

  currTotalInflation = total_inflatedNewAreaDelta / total_cell_area;

  if(currTotalInflation > inflation_area_over_whitespace) {
    dynamicInflationAdjustment();
  }

  // if (inflation_area_over_whitespace * curr_filler_area >
  // total_inflatedNewAreaDelta) {
  //    adjust_ratio = 1.0;
  //} else {
  //    adjust_ratio = inflation_area_over_whitespace *
  //                      curr_filler_area / total_inflatedNewAreaDelta;
  //}
  // cout <<"total_inflated_delta_area = " <<total_inflatedNewAreaDelta <<endl;
  // cout <<"total_filler_area = " <<total_filler_area <<endl;
  // cout <<"curr_filler_area = " <<curr_filler_area <<endl;
  // cout <<"adjust_ratio = " <<adjust_ratio <<endl;
  // total_inflation_ratio = sqrt((total_cell_area + total_inflatedNewAreaDelta)
  //                        / total_cell_area);
  ;
}

void calc_Total_inflate_ratio() {
  calc_Total_inflated_cell_area();

  total_inflation_ratio =
      sqrt((total_cell_area + total_inflatedNewAreaDelta) / total_cell_area);
  // cout << "total_inflation_ratio      " << total_inflation_ratio << endl;
  cout << "total_inflatedNewAreaDelta " << total_inflatedNewAreaDelta << endl;
  // cout << "total_cell_area            " << total_cell_area << endl;
}

void calc_Total_inflated_cell_area() {
  struct CELL *cell = NULL;
  prec temp_area = 0;

  total_inflatedNewAreaDelta = 0;

  for(int i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];
    if(cell->flg == FillerCell)
      continue;
    total_inflatedNewAreaDelta += cell->inflatedNewAreaDelta;
    temp_area += cell->area;
  }
  // cout <<"temp_area = " <<temp_area <<endl;
  currTotalInflation = total_inflatedNewAreaDelta;
}

void shrink_filler_cells(prec area_to_shrink) {
  struct CELL *cell = NULL;
  prec total_filler_inflation_ratio =
      (curr_filler_area - area_to_shrink) / curr_filler_area;

  prec temp_area_orig = 0;
  prec temp_area_new = 0;

  curr_filler_area -= area_to_shrink;

  for(int i = 0; i < gcell_cnt; i++) {
    cell = &gcell_st[i];
    if(cell->flg != FillerCell)
      continue;

    // if (is_inflation_h == true) {
    //    cell->size.x            *= total_filler_inflation_ratio;
    //    cell->half_size.x       = 0.5 * cell->size.x;
    //} else {
    //    cell->size.y            *= total_filler_inflation_ratio;
    //    cell->half_size.y       = 0.5 * cell->size.y;
    //}
    cell->size.x *= sqrt(total_filler_inflation_ratio);
    cell->half_size.x = 0.5 * cell->size.x;
    cell->size.y *= sqrt(total_filler_inflation_ratio);
    cell->half_size.y = 0.5 * cell->size.y;

    cell->pmin.x = cell->center.x - cell->half_size.x;
    cell->pmin.y = cell->center.y - cell->half_size.y;
    cell->pmax.x = cell->center.x + cell->half_size.x;
    cell->pmax.y = cell->center.y + cell->half_size.y;

    temp_area_orig += cell->area;
    cell->area = cell->size.x * cell->size.y;
    temp_area_new += cell->area;

    cell_den_scal_update_forNewGrad_inNSopt(cell);
  }

  cout << "filler_area_orig = " << temp_area_orig << endl;
  cout << "filler_area_new = " << temp_area_new << endl;
}

#ifndef __REPLACE_ROUTE__
#define __REPLACE_ROUTE__ 0
#include "replace_private.h"
#include "lefdefIO.h"


enum LayerDirection {
  Horizontal, Vertical
};

struct Layer {
  string layerName;
  float layerPitchX, layerPitchY;
  float layerWidth;
  LayerDirection layerDirection;
  Layer(string _layerName, float _layerPitchX, float _layerPitchY, 
      float _layerWidth, LayerDirection _layerDirection) : 
    layerName(_layerName), 
    layerPitchX(_layerPitchX), layerPitchY(_layerPitchY), 
    layerWidth(_layerWidth), 
    layerDirection(_layerDirection) {};


  void Dump() {
    cout << "name: " << layerName << endl;
    cout << "pitch: " << layerPitchX << " " << layerPitchY << endl;
    cout << "width: " << layerWidth << endl;
    cout << "direction: " << 
      ((layerDirection == LayerDirection::Horizontal)? "horizontal" : "vertical") << endl << endl;
  }
};

struct ReducedTrack {
  int layerIdx;
  int lx, ly, ux, uy;
  int trackCnt;
  ReducedTrack( int _layerIdx, int _lx, int _ly, 
      int _ux, int _uy, int _trackCnt ) : 
    layerIdx(_layerIdx), lx(_lx), ly(_ly), 
    ux(_ux), uy(_uy), trackCnt(_trackCnt) {};

  void Dump() {
    cout << "layerIdx: " << layerIdx << endl;
    cout << "tileXY: (" << lx << ", " << ly << ") - (" << ux << ", " << uy << ")" << endl;
    cout << "track: " << trackCnt << endl << endl;
  }
};

class RouteInstance {
  private:

    vector<Layer> _layerStor;
    // LayerIdx --> get corresponding track Count
    vector<int> _trackCount;


    Replace::Circuit* _ckt;

    float _defDbu; 

    float _unitX, _unitY;
    float _offsetX, _offsetY;

    // GCellSize in Micron
    float _gCellSizeX, _gCellSizeY;

    // to generate *.route files for bookshelf
    // GCellSize in Bookshelf's scale, i.e., tileSize
    float _tileSizeX, _tileSizeY;
    int _gridCountX, _gridCountY;

    int _gridInnerCountX, _gridInnerCountY;

    float _gridOriginX, _gridOriginY;

    HASH_MAP< string, int > _layerMap; 
  
    // Layer Name -> Metal Layer Resources control 
    HASH_MAP< string, float > _layerCapacityMap;

    // below is for bookshelf 
    vector< ReducedTrack > _bsReducedTrackStor;

    // located in routeOpt.cpp    
    // for netInstance, moduleInstance, terminalInstance, ...
    void SetReplaceStructure();

    void FillGCellXY();
    void FillTrack();
    void FillGridXY();

    void FillForReducedTrackStor();
    
    // located in lefdefIO.cpp
    void SetScaleFactor();
    void SetCircuitInst();


  public:
    RouteInstance() : _defDbu(FLT_MIN), _unitX(FLT_MIN), _unitY(FLT_MIN),
      _offsetX(FLT_MIN), _offsetY(FLT_MIN),
      _gCellSizeX(FLT_MIN), _gCellSizeY(FLT_MIN),
      _tileSizeX(INT_MIN), _tileSizeY(INT_MIN),
      _gridCountX(INT_MIN), _gridCountY(INT_MIN),
      _gridInnerCountX(INT_MIN), _gridInnerCountY(INT_MIN),
      _gridOriginX(FLT_MIN), _gridOriginY(FLT_MIN) {};

    // located in routeOpt.cpp
    void Init();
    void FillLayerStor();
    void FillLayerCapacityRatio(string fileName); 

    // helper function
    int GetLayerCount() { return _layerStor.size(); };
    Layer& GetLayer( int layerIdx ) { return _layerStor[layerIdx]; };
    vector<Layer>& GetLayerStor() {return _layerStor; };
    vector<int>& GetTrackCount() {return _trackCount; };
    int GetTrackCount(int layerIdx) { return _trackCount[layerIdx]; };
    float GetTileSizeX() { return _tileSizeX; };
    float GetTileSizeY() { return _tileSizeY; };
    int GetGridCountX() { return _gridCountX; };
    int GetGridCountY() { return _gridCountY; };
    float GetGridOriginX() {return _gridOriginX; };
    float GetGridOriginY() {return _gridOriginY; };
    Replace::Circuit* GetCircuitInst() { return _ckt; };
    HASH_MAP<string, int>& GetLayerMap() {return _layerMap; };
    
    int GetReducedTrackCount() { return _bsReducedTrackStor.size(); };
    ReducedTrack& GetReducedTrack( int idx ) { return _bsReducedTrackStor[idx]; };
    vector<ReducedTrack>& GetReducedTrackStor() { return _bsReducedTrackStor; }; 

    bool IsRoutingLayer( string input ) { return _layerMap.find(input) != _layerMap.end(); };
    int GetRoutingLayerIdx( string input ) { 
      auto lmPtr = _layerMap.find(input); 
      if( lmPtr == _layerMap.end() ) {
        exit(1);
      }
      return lmPtr->second;
    }; 

}; 

void get_intermediate_pl_sol(char *dir, int tier);
void evaluate_RC_by_official_script(char *dir);
void est_congest_global_router(char *dir);
void run_global_router(char *dir, string plName);

void congEstimation(struct FPOS *st);
extern RouteInstance routeInst;

#endif

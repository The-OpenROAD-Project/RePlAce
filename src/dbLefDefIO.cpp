#include "dbLefDefIO.h"
#include "replace_private.h"
#include "opt.h"
#include "defout.h"
#include <iostream>

using odb::dbDatabase;
using odb::dbTech;
using odb::dbChip;
using odb::dbBlock;
using odb::dbSet;
using odb::dbInst;
using odb::dbPlacementStatus;
using odb::dbITerm;
using odb::dbBTerm;
using odb::dbIoType;
using odb::dbRow;
using odb::dbNet;
using odb::dbBox;
using odb::dbRegion;
using odb::dbSigType;

using odb::adsRect;
using odb::defout;

using odb::error;
using odb::notice;


using std::string;
using std::pair;
using std::unordered_map;
using std::cout;
using std::endl;
using std::to_string;
using std::make_pair;
using std::vector;

static std::unordered_map<string, pair<bool, int>> moduleTermMap;

bool IsModule(dbPlacementStatus ps) {
  switch(ps) {
    case dbPlacementStatus::NONE:
    case dbPlacementStatus::UNPLACED:
    case dbPlacementStatus::SUGGESTED:
    case dbPlacementStatus::PLACED:
      return true;
      break;
    case dbPlacementStatus::LOCKED:
    case dbPlacementStatus::FIRM:
    case dbPlacementStatus::COVER:
      return false;
      break;
  }
}

void FillReplaceStructures(dbDatabase* db) {
  dbChip* chip = db->getChip();
  dbBlock* block = chip->getBlock();

  dbSet<dbInst> insts = block->getInsts();
  dbSet<dbInst>::iterator iitr;
 
  PrintProcBegin("Filling Replace Structure");

  // initialize module and terminal
  moduleCNT = terminalCNT = 0; 
  for( iitr = insts.begin(); iitr != insts.end(); ++iitr ) {
    dbInst* curInst = *iitr;
    if( IsModule(curInst->getPlacementStatus()) ) {
      moduleCNT++;
    }
    else {
      terminalCNT++;
    }
  }

  // PINS iterator
  dbSet<dbBTerm> bterms = block->getBTerms();
  terminalCNT += bterms.size();
//  dbSet<dbBTerm>::iterator btiter;
//  for(btiter = bterms.begin(); btiter != bterms.end(); ++btiter) {
//    dbBTerm* curBTerm = *btiter;
//    terminalCNT++;
//    cout << "Bterm: " << curBTerm -> getName() << endl;
//  }

  // Instance Term
//  dbSet<dbITerm> iterms = block->getITerms();
//  dbSet<dbITerm>::iterator ititer;
//  for(ititer = iterms.begin(); ititer != iterms.end(); ++ititer) {
//    dbITerm* curITerm = *ititer;
//    curITerm->print();
//    cout << "Iterm: " << curITerm -> getName() << endl;
//  }
  
  dbSet<dbNet> nets = block->getNets();
  dbSet<dbRow> rows = block->getRows();

  FillReplaceParameter(db);
  FillReplaceModule(insts);
  FillReplaceTerm(insts, bterms);
  FillReplaceNet(nets);
  FillReplaceRow(rows);  
  GenerateDummyCellDb(rows);  
  FillReplaceNewRow(rows);  

  FPOS tier_min, tier_max;
  int tier_row_cnt = 0;

  get_mms_3d_dim(&tier_min, &tier_max, &tier_row_cnt);
  transform_3d(&tier_min, &tier_max, tier_row_cnt);
  post_read_3d();

  PrintProcEnd("Filling Replace Structure");
}

// update
//
// rows
// unitX, unitY
// offsetX, offsetY
// 
// rowHeight
//
void FillReplaceParameter(dbDatabase* db) {
  dbChip* chip = db->getChip();
  dbBlock* block = chip->getBlock();
  dbSet<dbRow> rows = block->getRows();
  if( rows.size() == 0 ) {
    error(1, "ROW not exist!!");
  }

  dbTech* tech = db->getTech();
  SetDefDbu( tech->getDbUnitsPerMicron() );
  PrintInfoInt( "DEF DBU", GetDefDbu());


//  dbBox* bBox = block->getBBox();
//  cout << "Block: " << bBox->xMin() << " " << bBox->xMax() << endl;

//  dbSet<dbRegion> reg = block->getRegions();
//  dbSet<dbRegion>::iterator regiter;
//  for(regiter = reg.begin(); regiter != reg.begin(); ++regiter) {
//    dbRegion* curReg = *regiter;
//    cout << "Region: " << curReg->getName()<<endl;
//  } 

  // extract arbitrary row height
  dbSet<dbRow>::iterator riter;
  riter = rows.begin();
  dbRow* firstRow = *riter;

//  cout << firstRow->getName() << endl;
  adsRect rowRect;
  firstRow->getBBox( rowRect );
  PrintInfoPrec("RowHeight", rowRect.dy() );

  SetUnitY( 1.0 * rowRect.dy() / 9.0f );
  SetUnitX( GetUnitY() );

  PrintInfoPrec("ScaleDownUnit", GetUnitY());
  
  // Extract CoreArea from ROWS definition
  adsRect coreArea = GetCoreFromDb(rows);

  PrintInfoPrecPair("CoreAreaLxLy", coreArea.xMin(), coreArea.yMin());
  PrintInfoPrecPair("CoreAreaUxUy", coreArea.xMax(), coreArea.yMax());
  
  int coreLx = INT_CONVERT(coreArea.xMin());
  SetOffsetX( (coreLx % rowRect.dy() == 0)?
      0 : rowRect.dy() - (coreLx % rowRect.dy()) );

  int coreLy = INT_CONVERT(coreArea.yMin());
  SetOffsetY( (coreLy % rowRect.dy() == 0)?
      0 : rowRect.dy() - (coreLy % rowRect.dy()) );

  PrintInfoPrecPair("OffsetCoordi", GetOffsetX(), GetOffsetY());
  
  rowHeight =  GetScaleDownSize( rowRect.dy() ) ;
  PrintInfoPrec("ScaleDownRowHeight", rowHeight );
//  cout << rowRect.xMin() << " " << rowRect.yMin() << " - " << rowRect.xMax() << " " << rowRect.yMax() << endl;
//  cout << rowRect.dx() << " " << rowRect.dy() << endl;

}

// update 
// 
// moduleInstance
// moduleTermMap
//
void FillReplaceModule(dbSet<dbInst> &insts) {
  dbSet<dbInst>::iterator iitr;

  moduleInstance = 
    (MODULE*)malloc(sizeof(MODULE)*moduleCNT);

  int i=0;
  for( iitr = insts.begin(); iitr != insts.end(); ++iitr  ) {
    dbInst* curInst = *iitr;
    if( IsModule(curInst->getPlacementStatus()) == false ) {
      continue;
    }
    
    MODULE* curModule = &moduleInstance[i];
    new(curModule) MODULE();
    
    int placeX = 0, placeY = 0;
    curInst->getLocation(placeX, placeY);

    curModule->pmin.Set( 
        GetScaleDownPoint(placeX),
        GetScaleDownPoint(placeY));

    curModule->size.Set(
        GetScaleDownSize( curInst->getBBox()->getDX() ),
        GetScaleDownSize( curInst->getBBox()->getDY() ));

    moduleNameStor.push_back( curInst->getConstName() );

    curModule->half_size.Set(curModule->size.x / 2, curModule->size.y / 2);
    curModule->center.SetAdd(curModule->pmin, curModule->half_size);
    curModule->pmax.SetAdd(curModule->pmin, curModule->size);
    curModule->area = curModule->size.GetProduct();
    curModule->idx = i;

    moduleTermMap[curInst->getConstName()] = make_pair(true, i);

    i++; 
  }

  assert( i == moduleCNT );
  PrintInfoInt("Modules", moduleCNT);
}

// update
//
// terminalInstnace
// moduleTermMap
//
void FillReplaceTerm(dbSet<dbInst> &insts, dbSet<dbBTerm> &bterms) {
  dbSet<dbInst>::iterator iitr;

  terminalInstance = 
    (TERM* )malloc(sizeof(TERM)*terminalCNT);
  
  int i=0;
  for( iitr = insts.begin(); iitr != insts.end(); ++iitr  ) {
    dbInst* curInst = *iitr;
    if( IsModule(curInst->getPlacementStatus()) == true ) {
      continue;
    }

    TERM* curTerm = &terminalInstance[i];
    new(curTerm) TERM();

    int placeX = 0, placeY = 0;
    curInst->getLocation(placeX, placeY);

    curTerm->pmin.Set( 
        GetScaleDownPoint(placeX),
        GetScaleDownPoint(placeY));

    curTerm->size.Set(
        GetScaleDownSize( curInst->getBBox()->getDX() ),
        GetScaleDownSize( curInst->getBBox()->getDY() ));
    
    terminalNameStor.push_back( curInst->getConstName() );
    
    curTerm->center.Set(
        curTerm->pmin.x + curTerm->size.x / 2, 
        curTerm->pmin.y + curTerm->size.y / 2);
    curTerm->pmax.SetAdd(curTerm->pmin, curTerm->size);
    curTerm->area = curTerm->size.GetProduct();
    curTerm->idx = i;

    moduleTermMap[curInst->getConstName()] = make_pair(false, i);

    i++;
  }
 
  dbSet<dbBTerm>::iterator bIter;
  for(bIter = bterms.begin(); bIter != bterms.end(); ++bIter) {
    dbBTerm* bTerm = *bIter;
   
    TERM* curTerm = &terminalInstance[i];
    new(curTerm) TERM();

    curTerm->idx = i;
    curTerm->IO = (bTerm->getIoType() == dbIoType::INPUT)? 0 : 1;
    
    int placeX = 0, placeY = 0;
    bTerm->getFirstPinLocation(placeX, placeY);

    curTerm->pmin.Set( 
        GetScaleDownPoint(placeX),
        GetScaleDownPoint(placeY));
    curTerm->isTerminalNI = true;

    curTerm->pmax.Set(curTerm->pmin);
    curTerm->center.Set(curTerm->pmin);

    moduleTermMap[ bTerm->getConstName() ] = make_pair(false, i);
    i++;
  } 

  assert(i == terminalCNT);
  PrintInfoInt("Terminals", terminalCNT);
}

void FillReplaceNet(dbSet<dbNet> &nets) {
  PrintProcBegin("Generate Nets");
 
  pinCNT = 0;
  netCNT = nets.size();

  dbSet<dbNet>::iterator nIter;
  for(nIter = nets.begin(); nIter != nets.end(); ++nIter) {
    dbNet* curDnet = *nIter;
    pinCNT += curDnet->getITermCount() + curDnet->getBTermCount();
  }

  netInstance = (NET*)malloc(sizeof(NET)*netCNT);
  pinInstance = (PIN*)malloc(sizeof(PIN)*pinCNT);

  tPinName.resize(terminalCNT);
  mPinName.resize(moduleCNT);

  int netIdx = 0, pinIdx = 0;
  for(nIter = nets.begin(); nIter != nets.end(); ++nIter) {
    dbNet* curDnet = *nIter;
    
    dbSigType nType = curDnet->getSigType();
    if( nType == dbSigType::GROUND ||
        nType == dbSigType::POWER ||
        nType == dbSigType::CLOCK ||
        nType == dbSigType::RESET ) {
      continue;
    } 

    NET* curNet = &netInstance[netIdx];
    new(curNet) NET();

    netNameStor.push_back( curDnet->getConstName() );
    netNameMap[ curDnet->getConstName() ] = netIdx;

    curNet->idx = netIdx;
    curNet->timingWeight = 0;
    curNet->pinCNTinObject = curDnet->getITermCount() + curDnet->getBTermCount();

    // net connection counter
    int netConnCounter = 0;
    curNet->pin = (PIN**)malloc(sizeof(PIN*) * (curNet->pinCNTinObject));

    // PINS iterator
    dbSet<dbBTerm> bterms = curDnet->getBTerms();
    dbSet<dbBTerm>::iterator bIter;
    for(bIter = bterms.begin(); bIter != bterms.end(); ++bIter) {
      dbBTerm* curBTerm = *bIter;
      int io = INT_MAX;
      switch( curBTerm->getIoType() ) {
        case dbIoType::INPUT:
          io = 0;
          break;
        case dbIoType::OUTPUT:
          io = 1;
          break;
        default:
          io = 2;
          break;
      }
      PIN* curPin = &pinInstance[pinIdx];
      new(curPin) PIN(); 
      curNet->pin[netConnCounter] = curPin;

      auto mPtr = moduleTermMap.find(curBTerm->getConstName());
      if( mPtr == moduleTermMap.end()) {
        error( 1, "DEF's PIN name is mismatched in COMPONENT sections");
      }
      
      int termIdx = mPtr->second.second;
      TERM* curTerm = &terminalInstance[termIdx];

      tPinName[termIdx].push_back(curBTerm->getConstName());

      AddPinInfoForModuleAndTerminal(&curTerm->pin, &curTerm->pof, 
                                    curTerm->pinCNTinObject++, FPOS(0,0),
                                    termIdx, netIdx, netConnCounter++, pinIdx++, io, true);
    }

    // module/term pin iterator
    dbSet<dbITerm> iterms = curDnet->getITerms();
    dbSet<dbITerm>::iterator iIter;
    for(iIter = iterms.begin(); iIter != iterms.end(); ++iIter) {
      dbITerm* curITerm = *iIter;
      dbInst* curInst = curITerm->getInst();
      auto mtPtr = moduleTermMap.find( curInst->getConstName() );
  
      int io = INT_MAX;
      switch( curITerm->getIoType() ) {
        case dbIoType::INPUT:
          io = 0;
          break;
        case dbIoType::OUTPUT:
          io = 1;
          break;
        default:
          io = 2;
          break;
      }
      
      PIN* curPin = &pinInstance[pinIdx];
      new(curPin) PIN(); 
      curNet->pin[netConnCounter] = curPin;

      if( IsModule(curInst->getPlacementStatus()) ) {
        MODULE* curModule = &moduleInstance[mtPtr->second.second];
        
//        cout << curITerm->getId() << " " 
//          << curITerm->getMTerm()->getConstName() << " " 
//          << curITerm->getMTerm()->getMaster()->getConstName() << endl;

        // pinName
//        cout << "mPinInfo: " << curModule->Name() << " " << mtPtr->second.second 
//          << " " << mPinName[mtPtr->second.second].size() 
//          << " " << curITerm->getMTerm()->getConstName() << endl;


        mPinName[mtPtr->second.second].push_back( 
            curITerm->getMTerm()->getConstName() );

        // temporary
        FPOS curOffset;

        AddPinInfoForModuleAndTerminal( 
            &curModule->pin, &curModule->pof, 
            curModule->pinCNTinObject++, curOffset, 
            curModule->idx, netIdx, netConnCounter++, pinIdx++, io, false);

      }
      else {
        TERM* curTerm = &terminalInstance[mtPtr->second.second];
        
        // pinName
        tPinName[mtPtr->second.second].push_back(
            curITerm->getMTerm()->getConstName() ); 

        // temporary
        FPOS curOffset;

        AddPinInfoForModuleAndTerminal(
            &curTerm->pin, &curTerm->pof,
            curTerm->pinCNTinObject++, curOffset,
            curTerm->idx, netIdx, netConnCounter++, pinIdx++, io, true); 
      }
    }

    // ITerm : instances' terminal : instances' pin
    // BTerm : blocks' terminal : IO connection of blocks
//    if( curNet->getBTermCount() ) {
//      cout << curNet->getName() << " " 
//      << curNet->getITermCount() << " " 
//      << curNet->getBTermCount() << endl;
//    }
    netIdx++;
  }

  netCNT = netIdx;
  pinCNT = pinIdx;

  PrintInfoInt("NumNets", netCNT);
  PrintInfoInt("NumPins", pinCNT);
  PrintProcEnd("Generate Nets");
}

void FillReplaceRow(dbSet<dbRow> &rows) {
  row_cnt = rows.size();
  row_st = (ROW*)malloc(sizeof(ROW) * row_cnt);

  int i=0;
  for(dbRow* curDbRow : rows) {
    ROW* curRow = &row_st[i++];
    new(curRow) ROW();
   
    adsRect rowBox;
    curDbRow->getBBox( rowBox );

    float rowSizeX = GetScaleDownSize(rowBox.xMax()-rowBox.xMin());
    float rowSizeY = GetScaleDownSize(rowBox.yMax()-rowBox.yMin());
    float siteSizeX = GetScaleDownSize(curDbRow->getSite()->getWidth());

    // Just follow the openDB itself
    curRow->pmin.Set( GetScaleDownPoint(rowBox.xMin()), GetScaleDownPoint(rowBox.yMin()) );
    curRow->size.Set( rowSizeX, rowSizeY ); 
    curRow->pmax.Set( GetScaleDownPoint(rowBox.xMax()), GetScaleDownPoint(rowBox.yMax()) );
    
    curRow->x_cnt = rowSizeX / siteSizeX;
    curRow->site_wid = curRow->site_spa = SITE_SPA = siteSizeX; 
  }
}
// update rowHeight
void FillReplaceNewRow(dbSet<dbRow> &rows) {
  PrintProcBegin("Generate Rows");

  // get coreArea 
  adsRect coreArea = GetCoreFromDb(rows);
  
  float coreLx = GetScaleDownPoint( coreArea.xMin() );
  float coreLy = GetScaleDownPoint( coreArea.yMin() );

  dbSet<dbRow>::iterator riter;
  riter = rows.begin();
  dbRow* tmpRow = *riter;

  float siteX = GetScaleDownSize( tmpRow->getSite()->getWidth() );
  float siteY = GetScaleDownSize( tmpRow->getSite()->getHeight() );
  
  int rowCntX = (coreArea.xMax() - coreArea.xMin()) / tmpRow->getSite()->getWidth();
  int rowCntY = (coreArea.yMax() - coreArea.yMin()) / tmpRow->getSite()->getHeight();

  float rowSizeX = rowCntX * siteX;
  float rowSizeY = siteY;

  // free previous fragmented row DEF
  if( row_st ) {
    free(row_st);
  }

  row_cnt = rowCntY;
  row_st = (ROW*)malloc(sizeof(ROW) * row_cnt);
  
  for(int i=0; i<row_cnt; i++) {
    ROW* curRow = &row_st[i];
    new(curRow) ROW();

    curRow->pmin.Set(coreLx, coreLy + i * siteY );
    curRow->size.Set(rowSizeX, rowSizeY);
    curRow->pmax.Set(coreLx + rowSizeX, coreLy + i * siteY + rowSizeY);
    if( i == 0 ) {
      grow_pmin.Set(curRow->pmin);
    }
    else if( i == row_cnt - 1) {
      grow_pmax.Set(curRow->pmax);
    }
    curRow->x_cnt = rowCntX;
    curRow->site_wid = curRow->site_spa = SITE_SPA = siteX;
  }
  PrintInfoPrecPair( "RowSize", SITE_SPA, rowHeight);
  PrintInfoInt("NumRows", row_cnt);
  PrintProcEnd("Generate Rows");
}

void GenerateDummyCellDb(dbSet<dbRow> &rows) {
  dbRow* tmpRow = *(rows.begin());

  // Set Site Size
  float siteSizeX_ = GetScaleDownSize( tmpRow->getSite()->getWidth() );
  float siteSizeY_ = GetScaleDownSize( tmpRow->getSite()->getHeight() ) ;

  // Get Scale-Downed coreArea
  adsRect coreArea = GetCoreFromDb(rows);

  float coreLx = GetScaleDownPoint( coreArea.xMin() );
  float coreLy = GetScaleDownPoint( coreArea.yMin() );
//  float coreUx = GetScaleDownPoint( coreArea.xMax() );
//  float coreUy = GetScaleDownPoint( coreArea.yMax() );

  // Set Array Counts 
  int numX_ = (coreArea.xMax() - coreArea.xMin()) / tmpRow->getSite()->getWidth();
  int numY_ = (coreArea.yMax() - coreArea.yMin()) / tmpRow->getSite()->getHeight();
  // cout << "rowCnt: " << numX_ << " " << numY_ << endl;

  float rowSizeY_ = siteSizeY_;
  // cout << "rowSize: " << rowSizeX_ << " " << rowSizeY_ << endl;
 
  // Empty Array Fill 
  ArrayInfo::CellInfo* arr_ = new ArrayInfo::CellInfo[numX_ * numY_];
  for(int i = 0; i < numX_ * numY_; i++) {
    arr_[i] = ArrayInfo::CellInfo::Empty;
  }

  ArrayInfo ainfo(coreLx, coreLy, siteSizeX_, siteSizeY_); 

  // ROW array fill
  for(int i=0; i<row_cnt ; i++) {
    ROW* curRow = &row_st[i];

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
        curTerm.pmin.Set((prec)(coreLx + siteSizeX_ * startX),
                         (prec)(coreLy + j * siteSizeY_));
        curTerm.pmax.Set((prec)(coreLx + siteSizeX_ * endX),
                         (prec)(coreLy + j * siteSizeY_ + rowSizeY_));
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

adsRect GetDieFromDb(dbBox* bBox, bool isScaleDown) {
  return adsRect (bBox->xMin(), bBox->yMin(), 
      bBox->xMax(), bBox->yMax());
}

adsRect GetCoreFromDb(dbSet<odb::dbRow> &rows, bool isScaleDown) {
  float minX = FLT_MAX, minY = FLT_MAX;
  float maxX = FLT_MIN, maxY = FLT_MIN;

  dbSet<dbRow>::iterator riter;
  for(riter = rows.begin(); riter != rows.end(); ++riter) {
    dbRow* curRow = *riter;
    
    adsRect rowRect;
    curRow->getBBox( rowRect );

    minX = (minX > rowRect.xMin()) ? rowRect.xMin(): minX;
    minY = (minY > rowRect.yMin()) ? rowRect.yMin(): minY;
    maxX = (maxX < rowRect.xMax()) ? rowRect.xMax(): maxX;
    maxY = (maxY < rowRect.yMax()) ? rowRect.yMax(): maxY;
  }
  return (isScaleDown)? 
    adsRect( GetScaleDownPoint(minX), GetScaleDownPoint(minY),
            GetScaleDownPoint(maxX), GetScaleDownPoint(maxY))
    : adsRect(minX, minY, maxX, maxY); 
}

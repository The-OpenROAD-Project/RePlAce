#include "dbLefDefIO.h"
#include "replace_private.h"
#include "lefdefIO.h"
#include <iostream>

using ads::dbDatabase;
using ads::dbTech;
using ads::dbChip;
using ads::dbBlock;
using ads::dbSet;
using ads::dbInst;
using ads::dbPlacementStatus;
//using ads::dbITerm;
using ads::dbBTerm;
using ads::dbIoType;
using ads::dbRow;
using ads::dbNet;
using ads::dbBox;
using ads::dbRegion;

using ads::adsRect;

using ads::error;
using ads::notice;


using std::string;
using std::pair;
using std::unordered_map;
using std::cout;
using std::endl;
using std::to_string;
using std::make_pair;

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
  DieRect coreArea = GetCoreFromDb(rows);

  PrintInfoPrecPair("CoreAreaLxLy", coreArea.llx, coreArea.lly);
  PrintInfoPrecPair("CoreAreaUxUy", coreArea.urx, coreArea.ury);
  
  int coreLx = INT_CONVERT(coreArea.llx);
  SetOffsetX( (coreLx % rowRect.dy() == 0)?
      0 : rowRect.dy() - (coreLx % rowRect.dy()) );

  int coreLy = INT_CONVERT(coreArea.lly);
  SetOffsetY( (coreLy % rowRect.dy() == 0)?
      0 : rowRect.dy() - (coreLy % rowRect.dy()) );

  PrintInfoPrecPair("OffsetCoordi", GetOffsetX(), GetOffsetY());

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
//    curModule->Dump(moduleNameStor[i]);

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

//    curTerm->Dump();
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

}

void FillReplaceRow(dbSet<dbRow> &rows) {

}

DieRect GetDieFromDb(dbBox* bBox, bool isScaleDown) {
  return DieRect(bBox->xMin(), bBox->yMin(), 
      bBox->xMax(), bBox->yMax());
}

DieRect GetCoreFromDb(dbSet<ads::dbRow> &rows) {
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
  return DieRect(minX, minY, maxX, maxY); 
}

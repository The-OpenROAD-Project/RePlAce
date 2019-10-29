#ifndef __REPLACE_DB_LEFDEF__
#define __REPLACE_DB_LEFDEF__ 0

#include "db.h"
#include "lefdefIO.h"

void FillReplaceStructures(odb::dbDatabase* db);
void FillReplaceParameter(odb::dbDatabase* db);
void FillReplaceModule(odb::dbSet<odb::dbInst> &insts);
void FillReplaceTerm(odb::dbSet<odb::dbInst> &insts, odb::dbSet<odb::dbBTerm> &bterms);
void FillReplaceRow(odb::dbSet<odb::dbRow> &rows);
void FillReplaceNet(odb::dbSet<odb::dbNet> &nets);
void GenerateDummyCellDb(odb::dbSet<odb::dbRow> &rows);


DieRect GetDieFromDb(odb::dbBox &bBox, bool isScaleDown = false);
DieRect GetCoreFromDb(odb::dbSet<odb::dbRow> &rows, bool isScaleDown = false);

void WriteDefDb(odb::dbDatabase* db, const char* defName);

#endif

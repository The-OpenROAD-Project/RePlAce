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

#include "bin.h"
#include "bookShelfIO.h"
#include "lefdefIO.h"
#include "replace_private.h"
#include "initPlacement.h"
#include "macro.h"
#include "gcell.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::min;
using std::max;
using std::to_string;

// this is shared with lefdefIO.cpp
FPOS grow_pmin, grow_pmax;
FPOS terminal_pmin, terminal_pmax;

int numNonRectangularNodes;
int totalShapeCount;
BookshelfNameMap _bsMap;


static char error_text[BUFFERSIZE];
static FPOS terminal_size_max;
static FPOS module_size_max;
static POS max_mac_dim;

static HASH_MAP< string, NODES > nodesMap;


void ParseBookShelf() {
  // extract directory
  char dir[511] = {
      0,
  };
  extract_dir(bench_aux, dir);

  char fn[511] = {
      0,
  };
  char ffn[511] = {
      0,
  };
  char sfx[511] = {
      0,
  };
  char fn_st[511][511] = {
      0,
  };

  // parsing *.aux
  char buf[511] = {
      0,
  };
  int buf_size = 511;

  FILE *fp_aux = fopen(bench_aux, "r");

  if(fp_aux == NULL) {
    cout << "ERROR: Cannot open " << bench_aux << " file." << endl;
    exit(0);
  }
  fgets(buf, buf_size, fp_aux);

  char *token = NULL;
  token = strtok(buf, " \t\n");
  token = strtok(NULL, " \t\n");

  int cnt = 0;
  while((token = strtok(NULL, " \t\n")) != NULL) {
    strcpy(fn_st[cnt], token);
    cnt++;
  }

  // cout << "aux count: " << cnt << endl;
  for(int i = 0; i < cnt; i++) {
    strcpy(fn, fn_st[i]);
    extract_sfx(fn, sfx);
    strcpy(ffn, dir);
    strcat(ffn, fn);

    if(!strcmp(sfx, "nodes")) {
      read_nodes_3D(ffn);
    }
    else if(!strcmp(sfx, "shapes")) {
      read_shapes_3D(ffn);
    }
    else if(!strcmp(sfx, "route")) {
      read_routes_3D(ffn);
    }
    else if(!strcmp(sfx, "nets")) {
      read_nets_3D(ffn);
    }
    else if(!strcmp(sfx, "wts")) {
      // is currently do nothings..
    }
    else if(!strcmp(sfx, "pl")) {
      read_pl2(ffn);
    }
    else if(!strcmp(sfx, "scl")) {
      read_scl(ffn);
    }
  }

  /*
      if(flg_3dic) {
          if(INPUT_FLG == IBM) {
              // orig: get_ibm_3d_dim by LW
              get_mms_3d_dim(&tier_min, &tier_max, &tier_row_cnt);
          }
          else {
              get_mms_3d_dim(&tier_min, &tier_max, &tier_row_cnt);
          }
          transform_3d(&tier_min, &tier_max, tier_row_cnt);
          post_read_3d();
      }
      else {
          post_read_2d();
      }
  */
  FPOS tier_min, tier_max;
  int tier_row_cnt = 0;

  get_mms_3d_dim(&tier_min, &tier_max, &tier_row_cnt);
  transform_3d(&tier_min, &tier_max, tier_row_cnt);
  post_read_3d();
}

void transform_3d(FPOS *tier_min, FPOS *tier_max, int tier_row_cnt) {
  TERM *curTerminal = NULL;
  TIER *tier = NULL;
  MODULE *curModule = NULL;

  int tot_row_cnt = tier_row_cnt * numLayer;

  for(int i = 0; i < terminalCNT; i++) {
    curTerminal = &terminalInstance[i];

    // lutong
    // if (curTerminal->center.x > grow_pmin.x)
    // curTerminal->center.x
    // = grow_pmin.x + (curTerminal->center.x - grow_pmin.x) *
    // shrunk_ratio.x;
    // if (curTerminal->center.y > grow_pmin.y)
    // curTerminal->center.y
    // = grow_pmin.y + (curTerminal->center.y - grow_pmin.y) *
    // shrunk_ratio.y;

    curTerminal->pmin.x = curTerminal->center.x - 0.5 * curTerminal->size.x;
    curTerminal->pmin.y = curTerminal->center.y - 0.5 * curTerminal->size.y;
    curTerminal->pmax.x = curTerminal->center.x + 0.5 * curTerminal->size.x;
    curTerminal->pmax.y = curTerminal->center.y + 0.5 * curTerminal->size.y;
  }

  // mgwoo
  //
  assert(tot_row_cnt == row_cnt);

  //    row_st = (ROW *)realloc(row_st, sizeof(struct ROW) * tot_row_cnt);
  tier_st = (TIER *)malloc(sizeof(struct TIER) * numLayer);

  placementMacroCNT = 0;

  for(int i = 0; i < moduleCNT; i++) {
    curModule = &moduleInstance[i];

    if(curModule->size.y - rowHeight > Epsilon) {
      placementMacroCNT++;
      curModule->flg = Macro;
    }
    else if(curModule->size.y - rowHeight < -Epsilon) {
      printf("Cell %s   %d height ERROR: %f\n", curModule->Name(), i,
             curModule->size.y);
    }
    else {
      curModule->flg = StdCell;
    }
  }

  for(int i = 0; i < numLayer; i++) {
    tier = &tier_st[i];
    tier->row_st = &(row_st[tier_row_cnt * i]);
    tier->row_cnt = tier_row_cnt;
    tier->term_cnt = 0;

    if(terminalCNT > 0) {
      tier->term_st = (TERM **)malloc(sizeof(TERM *) * terminalCNT);
    }
    else {
      tier->term_st = NULL;
    }

    tier->modu_cnt = 0;
    tier->modu_st = NULL;
    tier->mac_cnt = 0;
    tier->mac_st = NULL;
    tier->term_area = 0;
    tier->virt_area = 0;
  }

  //    int curr_row_hei = tier_min->y;

  //    FPOS pmin;
  //    FPOS pmax;
  //    FPOS size;

  // site_wid= row_st[0].site_wid;
  // site_spa= row_st[0].site_spa;
  // ori     = row_st[0].ori;
  // sym     = row_st[0].sym;
  //    pmin = row_st[0].pmin;
  //    pmax = row_st[0].pmax;
  //    size = row_st[0].size;

  //    for(int i = 0; i < tier_row_cnt; i++) {
  // for (int j=0; j<numLayer; j++) {
  // row = &row_st[i + tier_row_cnt*j];
  // row->site_wid= site_wid; // lutong
  // row->site_spa= site_spa;
  // row->ori    = ori;
  // row->sym    = sym;
  // row->size.x = tier_max->x - tier_min->x;
  // row->size.y = rowHeight;
  // row->pmin.x = tier_min->x;
  // row->pmin.y = curr_row_hei;
  // row->pmax.x = row->pmin.x + row->size.x;
  // row->pmax.y = row->pmin.y + row->size.y;
  // row->x_cnt  = tier_max->x - tier_min->x;
  //}
  //        curr_row_hei += rowHeight;
  //    }

  // update global_variable
  row_cnt = tot_row_cnt;

  for(int i = 0; i < numLayer; i++) {
    tier = &tier_st[i];

    tier->pmin.x = tier_min->x;
    tier->pmin.y = tier_min->y;

    tier->pmax.x = tier_max->x;
    tier->pmax.y = tier_max->y;

    tier->size.x = tier->pmax.x - tier->pmin.x;
    tier->size.y = tier->pmax.y - tier->pmin.y;

    tier->center.x = (tier->pmin.x + tier->pmax.x) * 0.5;
    tier->center.y = (tier->pmin.y + tier->pmax.y) * 0.5;

    tier->area = tier->size.x * tier->size.y;
  }

  // TIER's term_area update!
  // mgwoo
  /*
  for(int i = 0; i < terminalCNT; i++) {
      curTerminal = &terminalInstance[i];
      tier = &tier_st[0];
      tier->term_st[tier->term_cnt++] = curTerminal;

      // skip for IO pin informations
//        if( curTerminal->isTerminalNI ) {
//            continue;
//        }

      // if there is no shape's information
      if( shapeMap.find( curTerminal->Name() ) == shapeMap.end() ) {
          cout << "not found: " << curTerminal->Name() << " " << place_st_cnt <<
endl;
          // tier->term_area += get_common_area(curTerminal->pmin,
          // curTerminal->pmax, tier->pmin, tier->pmax);
          for(int j=0; j<place_st_cnt; j++) {
              PLACE* curPlace = &place_st[j];
              tier->term_area += get_common_area(curTerminal->pmin,
                  curTerminal->pmax, curPlace->org, curPlace->end);
          }
      }
      // if there exist shape's information
      else {
          for(auto& curIdx : shapeMap[curTerminal->Name()]) {
              cout << "name: " << curTerminal->Name() << endl;
              prec llx = shapeStor[curIdx].llx,
                   lly = shapeStor[curIdx].lly,
                   width = shapeStor[curIdx].width,
                   height = shapeStor[curIdx].height;
              FPOS tmpMin(llx, lly, 0), tmpMax(llx + width, lly + height, 1);
              tmpMin.Dump("tmpMin");
              tmpMax.Dump("tmpMax");
              // prec commonArea = get_common_area(tmpMin, tmpMax,
              // tier->pmin, tier->pmax);
              for(int j=0; j<place_st_cnt; j++) {
                  PLACE* curPlace = &place_st[j];
                  tier->term_area += get_common_area(tmpMin, tmpMax,
                          curPlace->org, curPlace->end);
              }
          }
      }
  }
  */

  for(int i = 0; i < numLayer; i++) {
    tier = &tier_st[i];
    if(tier->term_cnt > 0) {
      tier->term_st = (TERM **)realloc(tier->term_st,
                                       sizeof(struct TERM *) * tier->term_cnt);
    }
    else if(tier->term_st) {
      free(tier->term_st);
    }
  }
}

bool SortRowByCoordinate(const ROW& lhs, const ROW& rhs) {
  if(lhs.pmin.x < rhs.pmin.x) {
    return true;
  }
  if(lhs.pmin.x > rhs.pmin.x) {
    return false;
  }
  return (lhs.pmin.y < rhs.pmin.y);
}

void GetSortedRowStor(ROW *origRowStor, int rowCnt) {
  // sort the Y-order and X-order
  vector< ROW > tmpRowStor;
  for(int i = 0; i < rowCnt; i++) {
    tmpRowStor.push_back(origRowStor[i]);
  }

  // sort function
  sort(tmpRowStor.begin(), tmpRowStor.end(), SortRowByCoordinate);

  // copy back
  for(auto &curRow : tmpRowStor) {
    origRowStor[&curRow - &tmpRowStor[0]] = curRow;
    //    cout << "SORTED!!" << endl;
    //    curRow.Dump(to_string(&curRow - &tmpRowStor[0]));
  }
}

//
// update: total_macro_area,
//          total_std_area,
//          total_PL_area,
//          gmov_macro_cnt,
//
//          placementStdcellCNT, placementMacroCNT
//
//          gmin, gmax // global minimum, global maximum
//          (plotting)
//
//  malloc : place_st, place_backup_st // place_st_cnt
//
void post_read_3d(void) {
  // int tmpMultiHeightCellCNT = 0;
  struct MODULE *curModule = NULL;
  struct TERM *curTerminal = NULL;

  placementStdcellCNT = 0;
  total_std_area = 0;
  placementMacroCNT = 0;
  total_macro_area = 0;

  for(int i = 0; i < moduleCNT; i++) {
    curModule = &moduleInstance[i];
    if(curModule->size.y - rowHeight > Epsilon) {
      // if (curModule->size.y - rowHeight > 24.0 + Epsilon) {
      // //lutong
      curModule->flg = Macro;
      placementMacroCNT++;
      total_macro_area += curModule->area;
    }
    else if(curModule->size.y - rowHeight < -Epsilon) {
      printf("*** ERROR:  Cell \"%d\" Height ERROR \n", i);
    }
    else {
      curModule->flg = StdCell;
      placementStdcellCNT++;
      total_std_area += curModule->area;
      continue;
    }
  }

  PrintInfoInt("NumPlaceStdCells", placementStdcellCNT);
  PrintInfoInt("NumPlaceMacros", placementMacroCNT);

  gmov_mac_cnt = placementMacroCNT;

  // initial malloc : row_cnt
  place_st_cnt = 0;
  place_st = (PLACE *)malloc(sizeof(struct PLACE) * row_cnt);

  ROW *last_row = NULL;
  PLACE *curPlace = NULL;

  GetSortedRowStor(row_st, row_cnt);

  //
  // Assume, height is equally distributed.
  //
  int place_hei = 0;
  for(int i = 0; i < row_cnt; i++) {
    ROW *row = &row_st[i];
    if(i == 0) {
      place_hei = row->size.y;
    }
    else if(row->pmin.x != last_row->pmin.x ||
            row->pmax.x != last_row->pmax.x ||

            row->pmin.y != last_row->pmax.y) {
      curPlace = &place_st[place_st_cnt++];

      curPlace->org.x = last_row->pmin.x;
      curPlace->org.y = last_row->pmax.y - place_hei;

      curPlace->end.x = last_row->pmax.x;
      curPlace->end.y = last_row->pmax.y;

      curPlace->stp.x = SITE_SPA;
      curPlace->stp.y = rowHeight;

      curPlace->cnt.x = curPlace->end.x - curPlace->org.x;
      curPlace->cnt.y = curPlace->end.y - curPlace->org.y;

      curPlace->area = curPlace->cnt.x * curPlace->cnt.y;

      curPlace->center.x = 0.5 * (curPlace->org.x + curPlace->end.x);
      curPlace->center.y = 0.5 * (curPlace->org.y + curPlace->end.y);

      place_hei = row->size.y;
    }
    else {
      place_hei += row->size.y;
    }
    last_row = row;
  }

  curPlace = &place_st[place_st_cnt++];

  curPlace->org.x = last_row->pmin.x;
  curPlace->org.y = last_row->pmax.y - place_hei;

  curPlace->end.x = last_row->pmax.x;
  curPlace->end.y = last_row->pmax.y;

  curPlace->stp.x = SITE_SPA;
  curPlace->stp.y = rowHeight;

  curPlace->cnt.x = curPlace->end.x - curPlace->org.x;
  curPlace->cnt.y = curPlace->end.y - curPlace->org.y;

  curPlace->area = curPlace->cnt.x * curPlace->cnt.y;

  curPlace->center.x = 0.5 * (curPlace->org.x + curPlace->end.x);
  curPlace->center.y = 0.5 * (curPlace->org.y + curPlace->end.y);

  //    for(int i=0; i<place_st_cnt; i++) {
  //        place_st[i].Dump(to_string(i));
  //    }

  // second re-malloc : upto place_st_cnt;
  place_st = (PLACE *)realloc(place_st, sizeof(struct PLACE) * place_st_cnt);
  place_backup_st =
      (PLACE *)realloc(place_backup_st, sizeof(struct PLACE) * place_st_cnt);

  for(int i = 0; i < place_st_cnt; i++) {
    place_backup_st[i] = place_st[i];
  }

  PrintInfoPrecPair( "RowSize", SITE_SPA, rowHeight );
  PrintInfoInt( "NumRows", row_cnt );

  // global variable 'place' update
  place.stp.x = SITE_SPA;
  place.stp.y = 1.0;  // rowHeight....................????

  place.org = place_st[0].org;
  place.end = place_st[0].end;

  total_PL_area = 0;
  for(int i = 0; i < place_st_cnt; i++) {
    curPlace = &place_st[i];

    place.org.x = min(place.org.x, curPlace->org.x);
    place.org.y = min(place.org.y, curPlace->org.y);

    place.end.x = max(place.end.x, curPlace->end.x);
    place.end.y = max(place.end.y, curPlace->end.y);

    total_PL_area += curPlace->area;
  }

  // global min & global max variable setting
  gmin = place.org;
  gmax = place.end;

  for(int i = 0; i < terminalCNT; i++) {
    curTerminal = &terminalInstance[i];

    gmin.x = min(gmin.x, curTerminal->pmin.x);
    gmin.y = min(gmin.y, curTerminal->pmin.y);

    gmax.x = max(gmax.x, curTerminal->pmax.x);
    gmax.y = max(gmax.y, curTerminal->pmax.y);
  }


  /*
  // setting additional margin
  // for plotting.. here It is not required

      gmin.Dump("gmin first");

      prec lx_mg = place.org.x - gmin.x,
           ly_mg = place.org.y - gmin.y,
           rx_mg = gmax.x - place.end.x,
           ry_mg = gmax.y - place.end.y;

  //    cout << "lx_mg: " << lx_mg << ", ly_mg: " << ly_mg << endl;
  //    cout << "rx_mg: " << rx_mg << ", ry_mg: " << ry_mg << endl;

      prec max_mg = PREC_MIN;
      max_mg = max(max_mg, lx_mg);
      max_mg = max(max_mg, ly_mg);
      max_mg = max(max_mg, rx_mg);
      max_mg = max(max_mg, ry_mg);

  //    cout << "max_mg: " << max_mg << endl;

      gmin.x = place.org.x - max_mg;
      gmin.y = place.org.y - max_mg;

      gmax.x = place.end.x + max_mg;
      gmax.y = place.end.y + max_mg;
  */

  PrintInfoPrecPair("GlobalAreaLxLy", gmin.x, gmin.y);
  PrintInfoPrecPair("GlobalAreaUxUy", gmax.x, gmax.y);

  PrintInfoPrecPair("PlaceAreaLxLy", place.org.x, place.org.y);
  PrintInfoPrecPair("PlaceAreaUxUy", place.end.x, place.end.y);

  place.cnt.x = place.end.x - place.org.x;
  place.cnt.y = place.end.y - place.org.y;

  place.center.x = 0.5 * (place.org.x + place.end.x);
  place.center.y = 0.5 * (place.org.y + place.end.y);

  place.area = place.cnt.x * place.cnt.y;
}

int read_shapes_3D(char *input) {
  FILE *fp = fopen(input, "r");
  if(fp == NULL) {
    cout << "** ERROR: Cannot open: " << input << " file (bookshelfIO)" << endl;
    exit(1);
  }

  char temp[BUFFERSIZE];
  char line[LINESIZE];

  bool flag = false;
  //    bool isFirstNode = true;

  int curShapeIdx = 0;
  totalShapeCount = 0;

#ifdef USE_GOOGLE_HASH
  shapeMap.set_empty_key(INIT_STR);
#endif

  char instName[255] = {
      0,
  };
  while(!feof(fp)) {
    *line = '\0';
    fgets(line, LINESIZE, fp);
    sscanf(line, "%s%*s", temp);

    if(strlen(line) < 5 || temp[0] == '#' || strcmp(temp, "shapes") == 0)
      continue;

    if(flag) {
      int curShapeCount;
      sscanf(line, "%*s %s", temp);

      if(strcmp(temp, ":") == 0) {
        sscanf(line, "%s", instName);
        //                cout << instName << endl;

        sscanf(line, "%*s %*s %d", &curShapeCount);
        totalShapeCount += curShapeCount;
        curShapeIdx = 0;
      }
      else {
        char name[255] = {
            0,
        };
        double llx, lly, width, height;

        sscanf(line, "%s %lf %lf %lf %lf", name, &llx, &lly, &width, &height);

        auto it = shapeMap.find(instName);
        if(it == shapeMap.end()) {
          vector< int > tmpStor;
          tmpStor.reserve(curShapeCount);
          tmpStor.push_back(shapeStor.size());

          shapeMap[instName] = tmpStor;
        }
        else {
          shapeMap[instName].push_back(shapeStor.size());
        }

        shapeStor.push_back(SHAPE(name, instName, curShapeIdx, (prec)llx,
                                  (prec)lly, (prec)width, (prec)height));

        curShapeIdx++;
      }
    }

    if(strcmp(temp, "NumNonRectangularNodes") == 0) {
      sscanf(line, "%*s %*s %d", &numNonRectangularNodes);
      cout << "INFO:  #NonRectangularNodes=" << numNonRectangularNodes << endl;
      flag = true;
    }
  }

  /*
   * Check whether SHAPE's are correctly parsed
      cout << "shapeMapSize: " << shapeMap.size() << endl;
      for(auto& curShape : shapeMap) {
          cout << "instName: " << curShape.first << endl;
          for(auto& curIdx : curShape.second) {
              shapeStor[curIdx].dump();
          }
          cout << endl;
      }
      */
  return 1;
}

int read_routes_3D(char *input) {
  char name[255];
  char *token = NULL;
  char temp[BUFFERSIZE];
  char line[LINESIZE];
  int numBlockedLayers = 0;
  bool blockageFlag = false;
  bool beolFlag = false;
  bool edgeFlag = false;
  int e1, e2, e3, e4, e5, e6, e7;
  double temp_gridLLx, temp_gridLLy;
  double temp_tileWidth, temp_tileHeight;
  double temp_blockagePorosity;
  vector< int > blockageLayers;

  FILE *fp;
  if((fp = fopen(input, "r")) == NULL) {
    sprintf(error_text, "bookshelf_IO: Cannot open: %s file", input);
    runtimeError(error_text);
  }

  while(!feof(fp)) {
    *line = '\0';
    fgets(line, LINESIZE, fp);
    sscanf(line, "%s%*s", temp);

    if(strlen(line) < 5 || temp[0] == '#' || strcmp(temp, "route") == 0)
      continue;
    if(strcmp(temp, "NumBlockageNodes") == 0) {
      blockageFlag = true;
      continue;
    }
    if(strcmp(temp, "NumEdgeCapacityAdjustments") == 0) {
      blockageFlag = false;
      edgeFlag = true;
      continue;
    }
    if(strcmp(temp, "Grid") == 0) {
      beolFlag = true;
    }
    if(strcmp(temp, "NumNiTerminals") == 0) {
      beolFlag = false;
      continue;
    }

    if(beolFlag) {
      sscanf(line, "%s :%*s", temp);
      if(strcmp(temp, "Grid") == 0) {
        sscanf(line, "%*s : %d %d %d", &nXgrids, &nYgrids, &nMetLayers);
      }
      else if(strcmp(temp, "VerticalCapacity") == 0) {
        verticalCapacity.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < nMetLayers; i++) {
          verticalCapacity.push_back(atoi(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "HorizontalCapacity") == 0) {
        horizontalCapacity.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < nMetLayers; i++) {
          horizontalCapacity.push_back(atoi(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "MinWireWidth") == 0) {
        minWireWidth.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < nMetLayers; i++) {
          minWireWidth.push_back(atof(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "MinWireSpacing") == 0) {
        minWireSpacing.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < nMetLayers; i++) {
          minWireSpacing.push_back(atof(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "ViaSpacing") == 0) {
        viaSpacing.clear();
        token = strtok(line, " \t\n");
        token = strtok(NULL, " \t\n");
        token = strtok(NULL, " \t\n");
        for(int i = 0; i < nMetLayers; i++) {
          viaSpacing.push_back(atof(token));
          token = strtok(NULL, " \t\n");
        }
      }
      else if(strcmp(temp, "GridOrigin") == 0) {
        sscanf(line, "%*s : %lf %lf", &temp_gridLLx, &temp_gridLLy);
        gridLLx = GetScaleDownPoint( temp_gridLLx );
        gridLLy = GetScaleDownPoint( temp_gridLLy );
      }
      else if(strcmp(temp, "TileSize") == 0) {
        sscanf(line, "%*s : %lf %lf", &temp_tileWidth, &temp_tileHeight);
        tileWidth = GetScaleDownSize( temp_tileWidth );
        tileHeight = GetScaleDownSize( temp_tileHeight );
      }
      else if(strcmp(temp, "BlockagePorosity") == 0) {
        sscanf(line, "%*s : %lf", &temp_blockagePorosity);
        blockagePorosity = (prec)temp_blockagePorosity;
      }
    }

    if(blockageFlag) {
      blockageLayers.clear();
      sscanf(line, "%s %d%*s", name, &numBlockedLayers);
      token = strtok(line, " \t\n");
      token = strtok(NULL, " \t\n");
      token = strtok(NULL, " \t\n");
      for(int i = 0; i < numBlockedLayers; i++) {
        blockageLayers.push_back(atoi(token));
        token = strtok(NULL, " \t\n");
      }
      routeBlockageNodes[name] = blockageLayers;
    }
    if(edgeFlag) {
      sscanf(line, "%d %d %d %d %d %d %d ", &e1, &e2, &e3, &e4, &e5, &e6, &e7);
      edgeCapAdj.push_back(std::make_tuple(e1, e2, e3, e4, e5, e6, e7));
    }
  }
  return 1;
}

// Input : .nodes
// output : moduleInstance, moduleCNT, module_size_max,
// terminalInstance, terminalCNT, terminal_size_max,
// max_mac_dim ( same as max_cell )
//
// note that module == cell, area == size
// mostly focusing moduleInstance, teminalInstance
//

int read_nodes_3D(char *input) {
#ifdef USE_GOOGLE_HASH
  nodesMap.set_empty_key(INIT_STR);
#endif

  FILE *fp = fopen(input, "r");
  char *token = NULL;
  char buf[255];

  MODULE *maxSizeCell = NULL;
  MODULE *minSizeCell = NULL;

  MODULE *curModule = NULL;
  TERM *curTerminal = NULL;

  int buf_size = 255;
  int idx = 0;
  int totalInstanceCNT = 0;
  // int isTerminal = 0;
  // int isTerminalNI = 0;
  int first_term = 1;
  int first_modu = 1;

  do {
    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
  } while(!token || token[0] == '#' || !strcmp(token, "UCLA"));
  token = strtok(NULL, " \t\n");
  token = strtok(NULL, " \t\n");
  totalInstanceCNT = moduleCNT = atoi(token);

  do {
    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
  } while(!token || token[0] == '#' || !strcmp(token, "UCLA"));
  token = strtok(NULL, " \t\n");
  token = strtok(NULL, " \t\n");
  terminalCNT = atoi(token);

  moduleCNT -= terminalCNT;
  moduleInstance =
      (struct MODULE *)malloc(sizeof(struct MODULE) * moduleCNT);
  terminalInstance =
      (struct TERM *)malloc(sizeof(struct TERM) * terminalCNT);

  // to find max size..
  prec maxSize = PREC_MIN;
  prec minSize = PREC_MAX;

  // for (int i=0; i<totalInstanceCNT; i++) {
  //

  // cursor for (module, terminal) Instance
  int moduleCur = 0, terminalCur = 0;
  int cnt = 0;

  // max & min initialize
  FPOS maxCell(PREC_MIN, PREC_MIN);
  FPOS minCell(PREC_MAX, PREC_MAX);
  FPOS maxTerm(PREC_MIN, PREC_MIN);
  FPOS minTerm(PREC_MAX, PREC_MAX);

  char nodeName[255], line[LINESIZE];

  FPOS avgCellSize, avgTerminalSize;

  while(!feof(fp)) {
    *line = '\0';
    fgets(line, LINESIZE, fp);
    sscanf(line, "%s\t%*s\n", nodeName);
    // cout << "current Line: " << line << endl;
    // cout << "current nodeName: " << nodeName << endl;

    if(strlen(line) < 5 || nodeName[0] == '#' || strcmp(nodeName, "UCLA") == 0)
      continue;

    if(strcmp(nodeName, "NumNodes") == 0 ||
       strcmp(nodeName, "NumTerminals") == 0)
      continue;

    cnt++;
    char node_type[BUFFERSIZE];
    *node_type = '\0';

    prec x, y;

#if PREC_MODE == IS_FLOAT
    sscanf(line, "%s%f%f%s\n", nodeName, &x, &y, node_type);
#elif PREC_MODE == IS_DOUBLE
    sscanf(line, "%s%lf%lf%s\n", nodeName, &x, &y, node_type);
#endif

    // cout << "current parsed: " << nodeName << ", " << x<< ", " <<
    // y << ", " << node_type << endl;
    // do {
    // fgets (buf, buf_size, fp);
    // token = strtok(buf, " \t\n");
    // }
    // while(!token || token[0]=='#' || !strcmp(token,"UCLA"));

    bool isTerminal = false, isTerminalNI = false;

    // get Index..
    if(strcmp(node_type, "") == 0) {
      // cout << "None!" << endl;
      idx = moduleCur++;
    }
    else if(strcmp(node_type, "terminal") == 0) {
      // cout << "terminal!!!" << endl;
      idx = terminalCur++;
      isTerminal = true;
    }
    else if(strcmp(node_type, "terminal_NI") == 0) {
      // cout << "terminalNI!!!" << endl;
      idx = terminalCur++;
      isTerminal = true;
      isTerminalNI = true;
    }

    // update NodesMap
    nodesMap[string(nodeName)] = NODES(idx, isTerminal, isTerminalNI);

    // moduleInstance update
    //
    // if normal basic nodes...( not terminal & not terminalNI )
    if(!isTerminal && !isTerminalNI) {
      curModule = &moduleInstance[idx];
      curModule->idx = idx;
//      strcpy(curModule->Name(), nodeName);
      moduleNameStor.push_back(nodeName);

      // width info
      // update size, half_size
      curModule->size.x = x;
      curModule->half_size.x = 0.5 * curModule->size.x;

      // height info
      // update size, half_size
      curModule->size.y = y;
      curModule->half_size.y = 0.5 * curModule->size.y;


      // update area
      curModule->area =
          curModule->size.x * curModule->size.y;

      // why this in here....
      curModule->pof = NULL;
      curModule->pin = NULL;
      curModule->netCNTinObject = 0;
      curModule->pinCNTinObject = 0;

      // max_min update for x.
      if(maxCell.x < curModule->size.x)
        maxCell.x = curModule->size.x;
      if(minCell.x > curModule->size.x)
        minCell.x = curModule->size.x;

      // max_min update for y
      if(maxCell.y < curModule->size.y)
        maxCell.y = curModule->size.y;
      if(minCell.y > curModule->size.y)
        minCell.y = curModule->size.y;

      // max update for area(size)
      if(maxSize < curModule->area) {
        maxSize = curModule->area;
        maxSizeCell = curModule;
      }

      // min update for area(size)
      if(minSize > curModule->area) {
        minSize = curModule->area;
        minSizeCell = curModule;
      }

      // update module_size_max (x,y)
      if(first_modu) {
        first_modu = 0;
        module_size_max = curModule->size;
      }
      else {
        module_size_max.x = max(module_size_max.x, curModule->size.x);
        module_size_max.y = max(module_size_max.y, curModule->size.y);
      }

      // for average calculation
      avgCellSize.x += curModule->size.x;
      avgCellSize.y += curModule->size.y;
    }
    // terminalInstance update
    // both of terminal & terminal_NI are saved in here!
    else {
      curTerminal = &terminalInstance[idx];
      curTerminal->idx = idx;
//      strcpy(curTerminal->Name(), nodeName);
      terminalNameStor.push_back( nodeName );

      curTerminal->size.x = x;
      curTerminal->size.y = y;

      // if Non-Image mode, ignore width & height
      if(isTerminalNI) {
        curTerminal->size.x = curTerminal->size.y = 0.0;
        curTerminal->isTerminalNI = true;
      }
      else {
        curTerminal->isTerminalNI = false;
      }

      curTerminal->area = curTerminal->size.x * curTerminal->size.y;

      // why this in here..
      curTerminal->pof = NULL;
      curTerminal->pin = NULL;
      curTerminal->netCNTinObject = 0;
      curTerminal->pinCNTinObject = 0;

      // max & min data update
      minTerm.x =
          (minTerm.x > curTerminal->size.x) ? curTerminal->size.x : minTerm.x;
      minTerm.y =
          (minTerm.y > curTerminal->size.y) ? curTerminal->size.y : minTerm.y;

      maxTerm.x =
          (maxTerm.x < curTerminal->size.x) ? curTerminal->size.x : maxTerm.x;
      maxTerm.y =
          (maxTerm.y < curTerminal->size.y) ? curTerminal->size.y : maxTerm.y;

      // update terminal_size_max (x,y)
      if(first_term) {
        first_term = 0;
        terminal_size_max = curTerminal->size;
      }
      else {
        terminal_size_max.x = max(curTerminal->size.x, terminal_size_max.x);
        terminal_size_max.y = max(curTerminal->size.y, terminal_size_max.y);
      }

      // average Terimanl Info update
      avgTerminalSize.x += curTerminal->size.x;
      avgTerminalSize.y += curTerminal->size.y;
    }
  }

  assert(cnt == totalInstanceCNT);

  // finally divide - module-cells
  avgCellSize.x /= (prec)moduleCNT;
  avgCellSize.y /= (prec)moduleCNT;

  // finally divide - terminal
  avgTerminalSize.x /= (prec)terminalCNT;
  avgTerminalSize.y /= (prec)terminalCNT;

  printf("INFO:  PLACEMENT INSTANCES INCLUDING STDCELL and MOVABLE MACRO\n");
  printf("INFO:    Instance   MinX=%.2lf, MaxX=%.2lf\n", minCell.x, maxCell.x);
  printf("INFO:               MinY=%.2lf, MaxY=%.2lf\n", minCell.y, maxCell.y);
  printf("INFO:               AvgX=%.2lf, AvgY=%.2lf\n", avgCellSize.x,
         avgCellSize.y);
  printf("INFO:      Smallest Instance  %10s  Size %.2f\n", minSizeCell->Name(),
         minSize);
  printf("INFO:      Largest  Instance  %10s  Size %.2f\n", maxSizeCell->Name(),
         maxSize);

  printf("INFO:  TERMINALS INCLUDING PAD and FIXED MACRO\n");
  printf("INFO:    Terminal   MinX=%.2lf, MaxX=%.2lf\n", minTerm.x, maxTerm.x);
  printf("INFO:               MinY=%.2lf, MaxY=%.2lf\n", minTerm.y, maxTerm.y);
  printf("INFO:               AvgX=%.2lf, AvgY=%.2lf\n", avgTerminalSize.x,
         avgTerminalSize.y);
  max_mac_dim.x = maxCell.x;
  max_mac_dim.y = maxCell.y;
  return 1;
}

int read_nets_3D(char *input) {
  FILE *fp = fopen(input, "r");
  char *token = NULL;
  char buf[255];
  char name[255];
  int buf_size = 255;
  int moduleID = 0;
  int io = 0;
  NET *curNet = NULL;
  MODULE *curModule = NULL;
  TERM *curTerminal = NULL;
  PIN *pin = NULL;
  FPOS offset;

  int isTerminal = 0;
  int max_net_deg = INT_MIN;

  do {
    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
  } while(!token || token[0] == '#' || !strcmp(token, "UCLA"));
  token = strtok(NULL, " \t\n");
  token = strtok(NULL, " \t\n");
  netCNT = atoi(token);

  do {
    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
  } while(!token || token[0] == '#' || !strcmp(token, "UCLA"));
  token = strtok(NULL, " \t\n");
  token = strtok(NULL, " \t\n");
  pinCNT = atoi(token);

  netInstance = (struct NET *)malloc(sizeof(struct NET) * netCNT);
  pinInstance = (struct PIN *)malloc(sizeof(struct PIN) * pinCNT);

  int pid = 0;
  for(int i = 0; i < netCNT; i++) {
    curNet = &netInstance[i];
    new(curNet) NET();
    curNet->idx = i;

    do {
      fgets(buf, buf_size, fp);
      token = strtok(buf, " \t\n");
    } while(!token || strcmp(token, "NetDegree"));

    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    curNet->pinCNTinObject = atoi(token);

    token = strtok(NULL, " \t\n");

    if(token) {
      netNameStor.push_back(token); 
    }
    // lutong 05132016
    // if (strcmp(curNet->Name(), "CLK") && (strcmp(curNet->Name(), "GND"))){

    curNet->pin = (struct PIN **)malloc(
        sizeof(struct PIN *) * curNet->pinCNTinObject);

    if(max_net_deg < curNet->pinCNTinObject) {
      max_net_deg = curNet->pinCNTinObject;
    }

    for(int j = 0; j < curNet->pinCNTinObject; j++) {
      fgets(buf, buf_size, fp);
      token = strtok(buf, " \t\n");
      strcpy(name, token);

      auto nodesMapPtr = nodesMap.find(string(name));
      if(nodesMapPtr == nodesMap.end()) {
        runtimeError(string(string(".net files' nodename is mismatched! : ") +
                            string(name))
                         .c_str());
      }
      else {
        isTerminal = nodesMapPtr->second.isTerminal;
        moduleID = nodesMapPtr->second.index;
        // cout << "currentname: " << name << ", index: "
        // << nodesMapPtr->second.index << endl;
      }

      token = strtok(NULL, " \t\n");
      if(token[0] == 'I')
        io = 0;  // 'I'
      else if(token[0] == 'O')
        io = 1;  // 'O'
      else
        io = 2;                       // 'B'
      token = strtok(NULL, " \t\n");  // ":"

      if(token != 0) {
        token = strtok(NULL, " \t\n");  // "offset_x"
        offset.x = atof(token);

        token = strtok(NULL, " \t\n");  // "offset_y"
        offset.y = atof(token);
      }
      else {
        offset.x = 0.0;
        offset.y = 0.0;
      }

      pin = &pinInstance[pid];
      curNet->pin[j] = pin;

      // because we don't know, in the same nets,
      // what component goes into moduleInstance's pin or
      // terminalInstance's pin,
      //
      // It always reallocate all net's informations

      if(isTerminal == false) {
        curModule = &moduleInstance[moduleID];
        AddPinInfoForModuleAndTerminal(&curModule->pin, &curModule->pof,
                                       curModule->pinCNTinObject++, offset,
                                       moduleID, i, j, pid++, io, isTerminal);
      }
      else {
        curTerminal = &terminalInstance[moduleID];
        AddPinInfoForModuleAndTerminal(&curTerminal->pin, &curTerminal->pof,
                                       curTerminal->pinCNTinObject++, offset,
                                       moduleID, i, j, pid++, io, isTerminal);
      }
    }
    //} else {
    // i      -= 1;
    // netCNT -= 1;
    // pinCNT -= curNet->pinCNTinObject;
    //}
  }

  printf("INFO:  #NET=%d\n", netCNT);
  printf("INFO:  #PIN=%d\n", pinCNT);
  printf("INFO:    Maximum Net Degree is %d\n", max_net_deg);
  return 1;
}

//
// update terminalInst & moduleInst's Info
int read_pl2(char *input) {
  FILE *fp = fopen(input, "r");
  char *token = NULL;
  char buf[255];
  int buf_size = 255;
  int moduleID = 0;
  char name[255];
  struct MODULE *curModule = NULL;
  struct TERM *curTerminal = NULL;

  bool isFirst = true;

  for(int i = 0; i < moduleCNT + terminalCNT; i++) {
    do {
      fgets(buf, buf_size, fp);
      token = strtok(buf, " \t\n");
    } while(!token || token[0] == '#' || !strcmp(token, "UCLA"));

    strcpy(name, token);
    // getCellIndex ( name , & moduleID , & isTerminal , &isTerminalNI)
    // ;

    bool isTerminal = false;
    auto nodesMapPtr = nodesMap.find(string(name));
    if(nodesMapPtr == nodesMap.end()) {
      runtimeError(
          string(string(".pl files' nodename is mismatched! : ") + string(name))
              .c_str());
    }
    else {
      isTerminal = nodesMapPtr->second.isTerminal;
      moduleID = nodesMapPtr->second.index;
      // cout << "currentname: " << name << ", index: " <<
      // nodesMapPtr->second.index << endl;
    }

    if(!isTerminal) {
      curModule = &moduleInstance[moduleID];
      token = strtok(NULL, " \t\n");
      curModule->pmin.x = atof(token);

      token = strtok(NULL, " \t\n");
      curModule->pmin.y = atof(token);

      curModule->center.x = curModule->pmin.x + curModule->half_size.x;
      curModule->center.y = curModule->pmin.y + curModule->half_size.y;

      curModule->pmax.x = curModule->pmin.x + curModule->size.x;
      curModule->pmax.y = curModule->pmin.y + curModule->size.y;

    }
    else {
      curTerminal = &terminalInstance[moduleID];

      token = strtok(NULL, " \t\n");
      curTerminal->pmin.x = atof(token);

      token = strtok(NULL, " \t\n");
      curTerminal->pmin.y = atof(token);

      curTerminal->center.x = curTerminal->pmin.x + 0.5 * curTerminal->size.x;
      curTerminal->center.y = curTerminal->pmin.y + 0.5 * curTerminal->size.y;

      curTerminal->pmax.x = curTerminal->pmin.x + curTerminal->size.x;
      curTerminal->pmax.y = curTerminal->pmin.y + curTerminal->size.y;

      if(isFirst) {
        terminal_pmin = curTerminal->pmin;
        terminal_pmax = curTerminal->pmax;

        isFirst = false;
      }
      else {
        terminal_pmin.x = min(terminal_pmin.x, curTerminal->pmin.x);
        terminal_pmin.y = min(terminal_pmin.y, curTerminal->pmin.y);

        terminal_pmax.x = max(terminal_pmax.x, curTerminal->pmax.x);
        terminal_pmax.y = max(terminal_pmax.y, curTerminal->pmax.y);
      }
    }
  }
  return 1;
}

//
// build the row_st(ROW)
//
int read_scl(char *input) {
  char *token = NULL;
  char buf[BUF_SZ];
  int buf_size = BUF_SZ;

  FILE *fp = fopen(input, "r");
  do {
    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
  } while(!token || token[0] == '#' || !strcmp(token, "UCLA"));

  token = strtok(NULL, " \t\n");
  token = strtok(NULL, " \t\n");

  row_cnt = atoi(token);
  //    row_st = (struct ROW *)malloc(sizeof(struct ROW) * row_cnt);
  // call constructor
  //    row_st = new ROW[row_cnt];
  row_st = (ROW *)malloc(sizeof(struct ROW) * row_cnt);
  fgets(buf, buf_size, fp);

  for(int i = 0; i < row_cnt; i++) {
    ROW *row = &row_st[i];
    new(row) ROW();

    fgets(buf, buf_size, fp);

    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    row->pmin.y = atoi(token);  // Coordinate

    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    row->size.y = atoi(token);  // Height
    row->pmax.y = row->pmin.y + row->size.y;

    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    row->site_wid = atoi(token);  // Sitewidth

    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    row->site_spa = atoi(token);  // Sitespacing

    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    //        row->ori = atoi(token);  // Siteorient
    row->ori = string(token);  // Siteorient

    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    //        row->sym = atoi(token);  // Sitesymmetry
    //        row->sym = string(token);  // Sitesymmetry

    if(string(token) == "Y") {
      row->isYSymmetry = true;
    }

    fgets(buf, buf_size, fp);
    token = strtok(buf, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    row->pmin.x = atoi(token);  // SubrowOrigin

    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    token = strtok(NULL, " \t\n");
    row->x_cnt = atoi(token);  // NumSites

    row->size.x = row->x_cnt * row->site_spa;
    row->pmax.x = row->pmin.x + row->size.x;

    if(i == 0) {
      grow_pmin.Set(row->pmin);
      grow_pmax.Set(row->pmax);
    }
    else {
      grow_pmin.x = min(grow_pmin.x, (prec)row->pmin.x);
      grow_pmin.y = min(grow_pmin.y, (prec)row->pmin.y);
      grow_pmax.x = max(grow_pmax.x, (prec)row->pmax.x);
      grow_pmax.y = max(grow_pmax.y, (prec)row->pmax.y);
    }

    // For Z handling
    fgets(buf, buf_size, fp);

    if(i == 0) {
      rowHeight = row->size.y;
      SITE_SPA = row->site_spa;
    }
    else if(rowHeight != row->size.y) {
      printf("Error: ROW HEIGHT INCONSISTENT!\n");
      exit(0);
    }
    else if(SITE_SPA != row->site_spa) {
      printf("Error: SITE SPACING INCONSISTENT!\n");
      exit(0);
    }
  }

  // check ROW instance
  //    for(int i=0; i<row_cnt; i++) {
  //        ROW* curRow = &row_st[i];
  //        curRow->Dump(to_string(i));
  //    }
  //
  return 1;
}

void output_pl(char *output) {
  FILE *fp = fopen(output, "w");
  struct MODULE *curModule = NULL;
  struct TERM *curTerminal = NULL;

  fputs("UCLA pl 1.0 \n", fp);
  fputs("# Created	:	Jan  6 2005\n", fp);
  fputs(
      "# User   	:	Gi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp);
  fputs("\n", fp);

  if(flg_3dic) {
    for(int i = 0; i < moduleCNT; i++) {
      curModule = &moduleInstance[i];
      fprintf(fp, "%s %.6lf\t%.6lf\t%.6lf\t: N\n", curModule->Name(),
              curModule->pmin.x, curModule->pmin.y, 0.0f);
    }
    for(int i = 0; i < terminalCNT; i++) {
      curTerminal = &terminalInstance[i];
      // if (!curTerminal->isTerminalNI) {
      fprintf(fp, "%s %d\t%d\t%d\t: N /FIXED\n", curTerminal->Name(),
              INT_CONVERT(curTerminal->pmin.x), INT_CONVERT(curTerminal->pmin.y),
              0);
      //} else {
      // fprintf (fp, "%s %d\t%d\t%d\t: N /FIXED_NI\n",
      // curTerminal->Name(),
      // INT_CONVERT (curTerminal->pmin.x),
      // INT_CONVERT (curTerminal->pmin.y),
      //}
    }
  }
  else {
    for(int i = 0; i < moduleCNT; i++) {
      curModule = &moduleInstance[i];
      fprintf(fp, "%s %.6lf\t%.6lf\t: N\n", curModule->Name(), curModule->pmin.x,
              curModule->pmin.y);
    }
    for(int i = 0; i < terminalCNT; i++) {
      curTerminal = &terminalInstance[i];
      // if (!curTerminal->isTerminalNI) {
      fprintf(fp, "%s %d\t%d\t: N /FIXED\n", curTerminal->Name(),
              INT_CONVERT(curTerminal->pmin.x), INT_CONVERT(curTerminal->pmin.y));
      //} else {
      // fprintf (fp, "%s %d\t%d\t: N /FIXED_NI\n",
      // curTerminal->Name(),
      // INT_CONVERT (curTerminal->pmin.x),
      // INT_CONVERT (curTerminal->pmin.y));
      //}
    }
  }
  fclose(fp);
}

// function is modified by mgwoo.
//
// this function will
// add pin's informations for moduleInstance & terminalInstance
// once it meets fixed or unfixed pins..
void AddPinInfoForModuleAndTerminal(PIN ***pin, FPOS **pof, int currentPinCount,
                                    FPOS offset, int curModuleIdx,
                                    int curNetIdx, int curPinIdxInNet,
                                    int curPinIdx, int curPinDirection,
                                    int isTerminal) {
  // pin : current Pin Object
  // pof : current Pin Offset
  //
  if(currentPinCount > 0) {
    currentPinCount++;
    *pin = (PIN **)realloc(*pin, sizeof(struct PIN *) * currentPinCount);
    *pof = (FPOS *)realloc(*pof, sizeof(struct FPOS) * currentPinCount);
    //*pin_tmp = (struct PIN**)malloc(sizeof(struct
    // PIN*)*currentPinCount, 64);
    //*pof_tmp = (struct FPOS*)malloc(sizeof(struct
    // FPOS)*currentPinCount, 64);
    // memcpy(*pin_tmp, *pin, currentPinCount*(sizeof(struct PIN*)));
    // memcpy(*pof_tmp, *pof, currentPinCount*(sizeof(struct FPOS)));
    // free(*pin);
    // free(*pof);
    //*pin = (struct PIN**)malloc(sizeof(struct PIN*)*currentPinCount,
    // 64);
    //*pof = (struct FPOS*)malloc(sizeof(struct FPOS)*currentPinCount,
    // 64);
    // memcpy(*pin, *pin_tmp, currentPinCount*(sizeof(struct PIN*)));
    // memcpy(*pof, *pof_tmp, currentPinCount*(sizeof(struct FPOS)));
    // free(*pin_tmp);
    // free(*pof_tmp);
  }
  else {
    currentPinCount++;
    *pin = (struct PIN **)malloc(sizeof(struct PIN *) * currentPinCount);
    *pof = (struct FPOS *)malloc(sizeof(struct FPOS) * currentPinCount);
  }

  (*pof)[currentPinCount - 1] = offset;

  (*pin)[currentPinCount - 1] = &pinInstance[curPinIdx];
  (*pin)[currentPinCount - 1]->term = isTerminal;
  (*pin)[currentPinCount - 1]->moduleID = curModuleIdx;
  (*pin)[currentPinCount - 1]->netID = curNetIdx;
  (*pin)[currentPinCount - 1]->pinIDinNet = curPinIdxInNet;
  (*pin)[currentPinCount - 1]->pinIDinModule = currentPinCount - 1;
  (*pin)[currentPinCount - 1]->gid = curPinIdx;
  (*pin)[currentPinCount - 1]->IO = curPinDirection;
}

void runtimeError(string error_text) {
  fprintf(stderr, "ERROR: %s \n", error_text.c_str());
  fprintf(stderr, "Aborting !! \n");
  fflush(stdout);
  fflush(stderr);
  exit(1);
}

void read_routing_file( string routeName ) {
  cout << "INFO:  READ BACK ROUTING FILE..." << endl;
  cout << "INFO:  netCNT: " << netCNT << endl;
  char route_file[BUFFERSIZE];
  char temp[BUFFERSIZE];
  char netName[BUFFERSIZE];
  int netIdx = 0;
  int fromX = 0, fromY = 0, fromL = 0, toX = 0, toY = 0, toL = 0, layer = 0;
  char line[LINESIZE] = {0, };
  bool flag = false;
  struct NET *curNet= NULL;
  struct FPOS from, to;
  from.SetZero();
  to.SetZero();

  strcpy(route_file, routeName.c_str());
  cout << "INFO:  ROUTING FILE LOCATION: " << route_file<< endl;
  FILE *fp = fopen(route_file, "r");

  if((fp = fopen(route_file, "r")) == NULL) {
    sprintf(error_text, "ERROR: Cannot open: %s file", route_file);
    runtimeError(error_text);
  }

  while(!feof(fp)) {
    *line = '\0';
    fgets(line, LINESIZE, fp);
    sscanf(line, "%s%*s", temp);

    if(strlen(line) < 3)
      continue;

    if(temp[0] == 'n') {
      sscanf(line, "%s %d%*s", netName, &netIdx);
      flag = true;
      continue;
    }

    if(temp[0] == '!') {
      flag = false;
      continue;
    }

    if(flag) {
      if(temp[0] == '(') {
        sscanf(line, "(%d,%d,%d)-(%d,%d,%d)%*s", &fromX, &fromY, &fromL, &toX,
               &toY, &toL);
        if(fromL != toL)
          continue;
        from.x = GetScaleDownPoint(fromX);
        from.y = GetScaleDownPoint(fromY);
//        from.z = fromL;
        to.x = GetScaleDownPoint(toX);
        to.y = GetScaleDownPoint(toY);
//        to.z = toL;
        layer = fromL;
        curNet= &netInstance[netIdx];
        curNet->routing_tracks.push_back(ROUTRACK(from, to, layer, netIdx));
//        curNet->routing_tracks[curNet->routing_tracks.size()-1].Dump();
      }
    }
  }
  cout << "INFO:  READ ESTIMATED ROUTING SUCCESS!" << endl;
}

void delete_input_files_in(char *dir) {
  char cmd[BUF_SZ];
  sprintf(cmd, "rm -rf %s/%s.aux", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s.nodes", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s.nets", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s.route", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s.shapes", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s.scl", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s.wts", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s_orig.pl", dir, gbch);
  system(cmd);
  sprintf(cmd, "rm -rf %s/%s.pl", dir, gbch);
  system(cmd);
}

void LinkConvertedBookshelf(char* newDir) {
  char cmd[BUF_SZ] = {0, };
  // Bookshelf Position
  char bsPosition[BUF_SZ] = {0, };

  // Assume only first layer
  sprintf(bsPosition, "%s/router_base", dir_bnd);
//  std::filesystem::path bsPath = bsPosition;
//  sprintf(bsPosition, "%s", std::filesystem::absolute(bsPath).c_str());
//  cout << bsPosition << endl;
//  exit(0);

  char fullBsPosition[PATH_MAX] = {0, };
  char* ptr= realpath( bsPosition, fullBsPosition );
  if( ptr == NULL) {
    cout << "ERROR: realpath function caused null return. Cannot find absolute path: " << bsPosition << endl;
    exit(0);
  }

  sprintf(cmd, "ln -snf %s/%s.aux %s/.", fullBsPosition, gbch, newDir);
  system(cmd);
  sprintf(cmd, "ln -snf %s/%s.nodes %s/.", fullBsPosition, gbch, newDir);
  system(cmd);
  sprintf(cmd, "ln -snf %s/%s.nets %s/.", fullBsPosition, gbch, newDir);
  system(cmd);
  sprintf(cmd, "ln -snf %s/%s.scl %s/.", fullBsPosition, gbch, newDir);
  system(cmd);
  sprintf(cmd, "ln -snf %s/%s.wts %s/.", fullBsPosition, gbch, newDir);
  system(cmd);
  sprintf(cmd, "ln -snf %s/%s.route %s/.", fullBsPosition, gbch, newDir);
  system(cmd);
  sprintf(cmd, "ln -snf %s/%s.shapes %s/.", fullBsPosition, gbch, newDir);
  system(cmd);
}

void link_original_SB_files_to_Dir(char *dir) {
  char cmd[BUF_SZ];
  char SBorigDirFile[BUF_SZ];

  sprintf(SBorigDirFile, "%s/SB_orig/%s/%s", currentDir, gbch, gbch);

  sprintf(cmd, "mkdir -p %s", dir);
  system(cmd);

  sprintf(cmd, "ln -snf %s.aux %s/.", SBorigDirFile, dir);
  system(cmd);
  sprintf(cmd, "ln -snf %s.nodes %s/.", SBorigDirFile, dir);
  system(cmd);
  sprintf(cmd, "ln -snf %s.nets %s/.", SBorigDirFile, dir);
  system(cmd);
//  sprintf(cmd, "cp -rf %s.pl %s/%s_orig.pl", SBorigDirFile, dir, gbch);
//  system(cmd);
  sprintf(cmd, "ln -snf %s.scl %s/.", SBorigDirFile, dir);
  system(cmd);
  sprintf(cmd, "ln -snf %s.wts %s/.", SBorigDirFile, dir);
  system(cmd);
  sprintf(cmd, "ln -snf %s.shapes %s/.", SBorigDirFile, dir);
  system(cmd);
  sprintf(cmd, "ln -snf %s.route %s/.", SBorigDirFile, dir);
  system(cmd);
}

///////////////////////////////////////////////////
//
//  bookshelf writing function.
//  (aux, nodes, shapes, net, pl, scl, wts)
//
///////////////////////////////////////////////////

///////////////////////////////////////////////////
// *.aux writing
void WriteAux(char *dir_tier, bool isShapeDrawing) {
  char fn_aux[BUF_SZ] = {
      0,
  };
  sprintf(fn_aux, "%s/%s.aux", dir_tier, gbch);
  FILE *fp_aux = fopen(fn_aux, "w");

  if(isShapeDrawing) {
    fprintf(fp_aux,
            "RowBasedPlacement : %s.nodes %s.nets %s.wts %s.pl %s.scl "
            "%s.shapes %s.route\n",
            gbch, gbch, gbch, gbch, gbch, gbch, gbch);
  }
  else {
    fprintf(fp_aux,
            "RowBasedPlacement : %s.nodes %s.nets %s.wts %s.pl %s.scl %s.route\n", gbch,
            gbch, gbch, gbch, gbch, gbch);
  }

  fclose(fp_aux);
}

// *.shape writing - mgwoo
void WriteShapes(char *dir_tier, bool isShapeDrawing, bool isNameConvert) {
  if(!isShapeDrawing) {
    return;
  }

  char fn_shapes[BUF_SZ] = {
      0,
  };
  sprintf(fn_shapes, "%s/%s.shapes", dir_tier, gbch);
  FILE *fp_shapes = fopen(fn_shapes, "w");

  // file header..
  fputs("shapes 1.0 \n", fp_shapes);
  fputs("# Created	:	Jan  6 2005\n", fp_shapes);
  fputs(
      "# User   	:	Gi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp_shapes);
  fputs("\n", fp_shapes);

  fprintf(fp_shapes, "NumNonRectangularNodes  :  %d\n\n",
          numNonRectangularNodes);

  for(auto &curShapeNode : shapeMap) {
    fprintf(fp_shapes, "%s  :  %ld\n", 
        (isNameConvert)? 
        _bsMap.GetBsTerminalName( curShapeNode.first.c_str() ) : curShapeNode.first.c_str(), 
            curShapeNode.second.size());
    for(auto &curIdx : curShapeNode.second) {
      fprintf(fp_shapes, "\t%s\t%d\t%d\t%d\t%d\n",
              shapeStor[curIdx].name.c_str(), 
              GetScaleUpPointX( shapeStor[curIdx].llx ),
              GetScaleUpPointY( shapeStor[curIdx].lly ), 
              GetScaleUpSize( shapeStor[curIdx].width ),
              GetScaleUpSize( shapeStor[curIdx].height ));
    }
  }

  fclose(fp_shapes);
}

// *.weignt writing - nothing to write..
void WriteWts(char *dir_tier) {
  char fn_wts[BUF_SZ] = {
      0,
  };
  sprintf(fn_wts, "%s/%s.wts", dir_tier, gbch);
  FILE *fp_wts = fopen(fn_wts, "w");

  fputs("UCLA wts 1.0\n", fp_wts);
  fputs("# Created\t:\tDec 27 2004\n", fp_wts);
  fputs(
      "# User   \t:\tGi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp_wts);

  fclose(fp_wts);
}

vector<int> GetBlockageLayers(Replace::Circuit* _ckt, 
    RouteInstance& routeInst, int macroIdx) {
  vector<int> retVec;
 
  // The M1 layer always assumes to be blocked.  
  retVec.push_back(0);

  for(auto& curObs : _ckt->lefObsStor[macroIdx]) {
    lefiGeometries* curGeom = curObs.geometries();

    bool isLayerFound = false;
    int foundLayerIdx = INT_MAX;
    for(int j = 0; j < curGeom->numItems(); j++) {
      // Only deals with GeomLayer
      if(curGeom->itemType(j) == lefiGeomLayerE) {
        string layerName = curGeom->getLayer(j);
        if( !routeInst.IsRoutingLayer(layerName) ) {
          continue;
        }
        isLayerFound = true;
        foundLayerIdx = routeInst.GetRoutingLayerIdx( layerName ); 
        continue;
      }
      else {
        if( !isLayerFound ) {
          continue;
        }

        // 
        isLayerFound = false;
        if( curGeom->itemType(j) != lefiGeomLayerMinSpacingE) {
          retVec.push_back( foundLayerIdx );
        }
        foundLayerIdx = INT_MAX;
      }
    }
  }
  retVec.erase( unique( retVec.begin(), retVec.end() ), retVec.end() );

  return retVec;
}

// *.route writing.
// this is assumed that LEF/DEF inputs were used
void WriteRoute(char *dir_tier, bool isNameConvert, RouteInstance& routeInst, 
    bool isMetal1Removed) {
  char fn_route[BUF_SZ] = {
      0,
  };
  sprintf(fn_route, "%s/%s.route", dir_tier, gbch);
  FILE *fp_route = fopen(fn_route, "w");

  fputs("route 1.0\n", fp_route);
  fputs("# Created\t:\tMay 25 2012\n", fp_route);
  fputs(
      "# User   \t:\tGi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp_route);

  fprintf( fp_route, "Grid               : %d %d %d\n", 
      routeInst.GetGridCountX(), routeInst.GetGridCountY(),
      routeInst.GetLayerCount());
  
  int space = 8; 
  fprintf( fp_route, "VerticalCapacity   : ");
  for(auto& curLayer: routeInst.GetLayerStor() ) {
    int layerIdx = &curLayer - &routeInst.GetLayerStor()[0];
    if( isMetal1Removed && layerIdx == 0 ){
      fprintf( fp_route, "%*d", space, 0 );
    }
    else {
      fprintf( fp_route, "%*d", space, 
          (curLayer.layerDirection == LayerDirection::Vertical)? 
          routeInst.GetTrackCount(layerIdx) : 0 );
    }
  } 
  fprintf(fp_route, "\n");
  
  fprintf( fp_route, "HorizontalCapacity : ");
  for(auto& curLayer: routeInst.GetLayerStor() ) {
    int layerIdx = &curLayer - &routeInst.GetLayerStor()[0];
    if( isMetal1Removed && layerIdx == 0 ){
      fprintf( fp_route, "%*d", space, 0 );
    }
    else {
      fprintf( fp_route, "%*d", space, 
          (curLayer.layerDirection == LayerDirection::Horizontal)? 
          routeInst.GetTrackCount(layerIdx) : 0 );
    }
  } 
  fprintf(fp_route, "\n");

  fprintf( fp_route, "MinWireWidth       : ");
  for(int i=0; i<routeInst.GetLayerCount(); i++) {
    fprintf( fp_route, "%*d", space, 100);
  }
  fprintf(fp_route, "\n");
  
  fprintf( fp_route, "MinWireSpacing     : ");  
  for(int i=0; i<routeInst.GetLayerCount(); i++) {
    fprintf( fp_route, "%*d", space, 0);
  }
  fprintf(fp_route, "\n");
  
  fprintf( fp_route, "ViaSpacing         : ");
  for(int i=0; i<routeInst.GetLayerCount(); i++) {
    fprintf( fp_route, "%*d", space, 0);
  }
  fprintf(fp_route, "\n");
  
  fprintf( fp_route, "GridOrigin         : %d %d\n", 
      INT_CONVERT( routeInst.GetGridOriginX() ),
      INT_CONVERT( routeInst.GetGridOriginY() ));
  fprintf( fp_route, "TileSize           : %d %d\n", 
      GetScaleUpSize(routeInst.GetTileSizeX()), 
      GetScaleUpSize(routeInst.GetTileSizeY()));
  fprintf( fp_route, "BlockagePorosity   : 0\n\n");
 
  
  fflush( fp_route );
 
  Replace::Circuit* _ckt = routeInst.GetCircuitInst();
  
  int pinCnt = 0;
  for(auto& curPin : _ckt->defPinStor) {
    // skip for GROUND/POWER pins
    if( curPin.hasUse() ) {
      if( strcmp(curPin.use(), "GROUND") == 0 ||
          strcmp(curPin.use(), "POWER") == 0 ) {
        continue;
      }
    }
    ++pinCnt;
  }

  fprintf( fp_route, "NumNiTerminals     : %d\n", pinCnt );
  
  for(auto& curPin : _ckt->defPinStor) {
    // skip for GROUND/POWER pins
    if( curPin.hasUse() ) {
      if( strcmp(curPin.use(), "GROUND") == 0 ||
          strcmp(curPin.use(), "POWER") == 0 ) {
        continue;
      }
    }
    if( !curPin.hasLayer() ) {
      cout << "ERROR: " << curPin.pinName() << " pin have no LAYER assignment!" << endl;
      exit(0);
    }
    if( curPin.numLayer() > 2) {
      cout << "WARNING: " << curPin.pinName() << " have multiple layer assignment: (" 
        << curPin.numLayer() 
        << "), but Global Router will use the first layer info only!" << endl;
    }
    auto lmPtr = routeInst.GetLayerMap().find( string(curPin.layer(0)) );
    if( lmPtr == routeInst.GetLayerMap().end()) {
      cout << "ERROR: " << curPin.pinName() << " have layer as " << curPin.layer(0)
        << ", but not exists in LEF" << endl;
      exit(0);
    }

    fprintf( fp_route, "  %s %d\n", 
        (isNameConvert)? _bsMap.GetBsTerminalName( curPin.pinName() ) : 
        curPin.pinName(), lmPtr->second+1);
  }

  int blockageCnt = 0;
  for(int i=0; i<terminalCNT; i++) {
    TERM* curTerminal = &terminalInstance[i];
    if( !curTerminal -> isTerminalNI ) {
      blockageCnt ++;
    }
  }

  fprintf( fp_route, "NumBlockageNodes   : %d\n", blockageCnt);
  HASH_MAP<int, vector<int>> macroBlockageMap;
#ifdef USE_GOOGLE_HASH
  macroBlockageMap.set_empty_key(INT_MAX);
#endif

  string blockagePrefix = "replace_blockage_";
  string dummyInstPrefix = "dummy_inst_";

  for(int i=0; i<terminalCNT; i++) {
    TERM* curTerminal = &terminalInstance[i];
    if( curTerminal -> isTerminalNI ) {
      continue;
    }

    string termNameStr = curTerminal->Name(); 
    vector<int>* layerIdx = NULL;

    bool isBlockage = false;

    // If termNameStr has a prefix as blockagePrefix, then use defBlockageStor's information
    if( termNameStr.compare( 0, blockagePrefix.length(), blockagePrefix) == 0 ) {
      isBlockage = true;
      layerIdx = new vector<int>;
      // HARDCODE: All of layers 
      for(size_t i=0; i<routeInst.GetLayerStor().size(); i++) {
        layerIdx->push_back(i);
      }
    }
    // for Dummy Instances, just block Metal 1.
    else if ( termNameStr.compare( 0, dummyInstPrefix.length(), dummyInstPrefix) == 0 ) {
      layerIdx = new vector<int>;
      layerIdx->push_back(0); 
    }
    // else, then It should reference from MACRO in LEF files
    else {
      int compIdx = GetDefComponentIdx( *_ckt, termNameStr );
      string macroNameStr = _ckt->defComponentStor[compIdx].name();
      int macroIdx = GetLefMacroIdx( *_ckt, macroNameStr );


      auto mbPtr = macroBlockageMap.find( macroIdx );
      if( mbPtr == macroBlockageMap.end() ) {
        macroBlockageMap[ macroIdx ] = 
          GetBlockageLayers(_ckt, routeInst, macroIdx);
      }
      layerIdx = &macroBlockageMap[ macroIdx ]; 
    }

    fprintf( fp_route, " %s ", 
        (isNameConvert)? 
        _bsMap.GetBsTerminalName( curTerminal->Name() ) : curTerminal->Name() );
    fprintf( fp_route, " %lu ", layerIdx->size());
    for(size_t i=0; i<layerIdx->size()-1; i++) {
      fprintf( fp_route, "%d ", layerIdx->at(i)+1);
    }
    fprintf( fp_route, "%d\n", layerIdx->at(layerIdx->size()-1)+1);
    
    if( isBlockage ) {
      delete layerIdx;  
    }
  }

  /*
  fprintf( fp_route, "NumEdgeCapacityAdjustments  : %d\n", routeInst.GetReducedTrackCount());
  for(auto& rTrack : routeInst.GetReducedTrackStor()) {
    fprintf( fp_route, "  %d %d %d %d %d %d %d\n", 
        rTrack.lx, rTrack.ly, rTrack.layerIdx+1, 
        rTrack.ux, rTrack.uy, rTrack.layerIdx+1, 
        rTrack.trackCnt );
  }*/
  fprintf( fp_route, "NumEdgeCapacityAdjustments  : 0\n");
  fclose(fp_route);
}

// *.nodes writing
void WriteNodes(char *dir_tier, int curLayer, int pin_term_cnt,
                bool isShapeDrawing, bool isNameConvert) {
  char fn_nodes[BUF_SZ] = {
      0,
  };
  sprintf(fn_nodes, "%s/%s.nodes", dir_tier, gbch);
  FILE *fp_nodes = fopen(fn_nodes, "w");

  fputs("UCLA nodes 1.0 \n", fp_nodes);
  fputs("# Created  :  Jan  6 2005\n", fp_nodes);
  fputs(
        "# User     :  Gi-Joon Nam & Mehmet Yildiz at IBM Austin "
        "Research({gnam, mcan}@us.ibm.com)\n",
      fp_nodes);
  fputs("\n", fp_nodes);

  int term_cnt = terminalCNT + pin_term_cnt;

  term_cnt = (!isShapeDrawing)
                 ? term_cnt + totalShapeCount - numNonRectangularNodes
                 : term_cnt;

  int node_cnt = moduleCNT + term_cnt;

  fprintf(fp_nodes, "NumNodes      :  %d\n", node_cnt);
  fprintf(fp_nodes, "NumTerminals  :  %d\n\n", term_cnt);

  for(int i = 0; i < moduleCNT; i++) {
    MODULE* curModule = &moduleInstance[i];
    fprintf(fp_nodes, "%*s %*d %*d\n", 15, 
        (isNameConvert)? _bsMap.GetBsModuleName( curModule->Name() ) : curModule->Name(), 
        14, GetScaleUpSize ( curModule->size.x ),
        14, GetScaleUpSize ( curModule->size.y ));
  }

  for(int i = 0; i < terminalCNT; i++) {
    TERM *curTerminal = &terminalInstance[i];
    string termString =
        isShapeDrawing
            ? ((curTerminal->isTerminalNI) ? "terminal_NI" : "terminal")
            : "terminal";

    if(isShapeDrawing) {
      fprintf(fp_nodes, "%*s %*d %*d %*s\n", 
          15, (isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(),
          14, GetScaleUpSize (curTerminal->size.x ),
          14, GetScaleUpSize (curTerminal->size.y ), 
          15, termString.c_str());
    }
    else {
      // considering shapeMaps - mgwoo
      auto shapeMapIter = shapeMap.find(curTerminal->Name());
      if(shapeMapIter == shapeMap.end()) {
        int sizeX =
            (curTerminal->isTerminalNI) ? 0 : GetScaleUpSize (curTerminal->size.x );
        int sizeY =
            (curTerminal->isTerminalNI) ? 0 : GetScaleUpSize (curTerminal->size.y );

        fprintf(fp_nodes, "%*s %*d %*d %*s\n", 
            15, (isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(), 
            14, sizeX, 14, sizeY,
            15, termString.c_str());
      }
      else {
        for(auto &curShapeIdx : shapeMap[curTerminal->Name()]) {
          int sizeX = (curTerminal->isTerminalNI)
                          ? 0
                          : GetScaleUpSize( shapeStor[curShapeIdx].width );
          int sizeY = (curTerminal->isTerminalNI)
                          ? 0
                          : GetScaleUpSize( shapeStor[curShapeIdx].height );

          fprintf(fp_nodes, "%*s %*d %*d %*s\n",
              15, string(string((isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name() ) 
                  : curTerminal->Name() + string("/") + string(shapeStor[curShapeIdx].name)))
                      .c_str(),
              14, sizeX, 
              14, sizeY, 
              15, termString.c_str());
        }
      }
    }
  }

  fclose(fp_nodes);
}

// for Net sorting
void BsNetInfo::Print( FILE* file ) {
  fprintf(file, "%*s%*s  :%*.4lf%*.4lf\n", 
      15, name.c_str(), 
      4, io.c_str(), 
      12, x, 
      12, y);
}

// net name sorting function
bool CompareBsNetInfo( const BsNetInfo& a, const BsNetInfo& b) {
  char prefixA = a.name.c_str()[0];
  char prefixB = b.name.c_str()[0];
  int idxA = atoi( a.name.substr(1, a.name.length()).c_str()); 
  int idxB = atoi( b.name.substr(1, b.name.length()).c_str());
 
//  if ( a.io < b.io ) {
//    return true; 
//  }
//  else if ( a.io > b.io ) {
//    return false;
//  }

  if( prefixA < prefixB ) {
    return true;
  }
  else if( prefixA > prefixB ) {
    return false;
  }
  
  return (idxA > idxB);
}

// *.nets writing
void WriteNet(char *dir_tier, int curLayer, int pin_cnt, int net_cnt,
              vector< int > &netChk, bool isShapeDrawing, bool isNameConvert ) {
  char fn_nets[BUF_SZ] = {
      0,
  };
  sprintf(fn_nets, "%s/%s.nets", dir_tier, gbch);
  FILE *fp_nets = fopen(fn_nets, "w");

  fputs("UCLA nets 1.0 \n", fp_nets);
  fputs("# Created  :  Jan  6 2005\n", fp_nets);
  fputs(
      "# User     :  Gi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp_nets);
  fputs("\n", fp_nets);

  fprintf(fp_nets, "NumNets  :  %d\n", net_cnt);
  fprintf(fp_nets, "NumPins  :  %d\n", pin_cnt);

  fputs("\n", fp_nets);

  int total_pinCNTinObject2 = 0;

  for(int i = 0; i < netCNT; i++) {
    NET *net = &netInstance[i];

    total_pinCNTinObject2 += net->pinCNTinObject2;

    if(netChk[i] != 1) {
      continue;
    }

    fprintf(fp_nets, "NetDegree  :  %d    %s\n", 
         net->pinCNTinObject_tier,
        (isNameConvert)?  _bsMap.GetBsNetName( net->Name() ) : net->Name());
 
    vector<BsNetInfo> bsNetInfoStor;

    for(int j = 0; j < net->pinCNTinObject2; j++) {
      PIN *pin = net->pin2[j];

      int moduleID = pin->moduleID;
      int pinIDinModule = pin->pinIDinModule;

      char io[BUF_SZ] = {
          0,
      };
      if(pin->IO == 0)
        strcpy(io, "I");
      else if(pin->IO == 1)
        strcpy(io, "O");
      else
        strcpy(io, "B");

      if(pin->term) {
        TERM *curTerminal = &terminalInstance[moduleID];
        if(isShapeDrawing) {
          //          fprintf(fp_nets, "\t%s\t%s : %.6lf\t%.6lf\n",
          bsNetInfoStor.push_back(
            BsNetInfo( 
              (isNameConvert)? 
              _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(), io,
              curTerminal->pof[pinIDinModule].x * GetUnitX(),
              curTerminal->pof[pinIDinModule].y * GetUnitY() )
            );
        }
        else {
          if(shapeMap.find(curTerminal->Name()) == shapeMap.end()) {
            bsNetInfoStor.push_back(
              BsNetInfo( 
                (isNameConvert)? 
                _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(), io,
                curTerminal->pof[pinIDinModule].x * GetUnitX(),
                curTerminal->pof[pinIDinModule].y * GetUnitY())
              );
          }
          else {
            // convert into "o506100/Shape_0"
            SHAPE *curShape = &shapeStor[shapeMap[curTerminal->Name()][0]];
            string concatedName = 
              string((isNameConvert)? 
                _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name()) 
              + string("/") + string(curShape->name);

            prec shapeCenterX = curShape->llx * GetUnitX() - GetOffsetX() 
              + (curShape->width * GetUnitX()) / 2;
            prec shapeCenterY = curShape->lly * GetUnitY() - GetOffsetY() 
              + (curShape->height * GetUnitY()) / 2;

            // covert as "o506100/shape_0"'s information
            bsNetInfoStor.push_back(
              BsNetInfo( 
                concatedName, io,
                curTerminal->pof[pinIDinModule].x * GetUnitX() 
                + (curTerminal->center.x - shapeCenterX) * GetUnitX(),
                curTerminal->pof[pinIDinModule].y * GetUnitY() 
                + (curTerminal->center.y - shapeCenterY) * GetUnitY())
              );
          }
        }
      }
      else {
        if(pin->term == 0) {
          MODULE *modu = &moduleInstance[moduleID];
          bsNetInfoStor.push_back(
            BsNetInfo( 
              (isNameConvert)? _bsMap.GetBsModuleName( modu->Name() ) : modu->Name(), io,
              modu->pof[pinIDinModule].x * GetUnitX(),
              modu->pof[pinIDinModule].y * GetUnitY())
            );
        }
        else {
          TERM *curTerminal = &terminalInstance[moduleID];
          bsNetInfoStor.push_back(
            BsNetInfo( 
              (isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(), io,
              curTerminal->pof[pinIDinModule].x * GetUnitX(),
              curTerminal->pof[pinIDinModule].y * GetUnitY() )
            );
        }
      }
    }
    sort( bsNetInfoStor.begin(), bsNetInfoStor.end(), CompareBsNetInfo);
    for(auto& curSubNet: bsNetInfoStor) {
      curSubNet.Print( fp_nets );
    }
  }

  fclose(fp_nets);
}

// *.pl writing
void WritePl(char *dir_tier, int curLayer, bool isShapeDrawing,
    bool isNameConvert) {
  char fn_pl[BUF_SZ] = {
      0,
  };
  sprintf(fn_pl, "%s/%s.pl", dir_tier, gbch);
  FILE *fp_pl = fopen(fn_pl, "w");

  fputs("UCLA pl 1.0\n", fp_pl);
  fputs("# Created\t:\tJan  6 2005\n", fp_pl);
  fputs(
      "# User   \t:\tGi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp_pl);

  fputs("\n", fp_pl);

  for(int i = 0; i < moduleCNT; i++) {
    MODULE *curModule = &moduleInstance[i];
    fprintf(fp_pl, "%s\t%.6lf\t%.6lf : N\n", 
        (isNameConvert)? _bsMap.GetBsModuleName( curModule->Name() ) : curModule->Name(), 
        GetScaleUpPointFloatX( curModule->pmin.x ), 
        GetScaleUpPointFloatY( curModule->pmin.y ));
  }

  for(int i = 0; i < terminalCNT; i++) {
    TERM *curTerminal = &terminalInstance[i];
    string fixedStr = (curTerminal->isTerminalNI) ? "FIXED_NI" : "FIXED";

    if(isShapeDrawing) {
      fprintf(fp_pl, "%s\t%d\t%d\t: N /%s\n", 
          (isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(),
              GetScaleUpPointX(curTerminal->pmin.x), 
              GetScaleUpPointY(curTerminal->pmin.y),
              fixedStr.c_str());
    }
    else {
      if(shapeMap.find(curTerminal->Name()) == shapeMap.end()) {
        fprintf(fp_pl, "%s\t%d\t%d\t: N /%s\n", 
            (isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(),
                GetScaleUpPointX(curTerminal->pmin.x), 
                GetScaleUpPointY(curTerminal->pmin.y),
                fixedStr.c_str());
      }
      else {
        for(auto &curShapeIdx : shapeMap[curTerminal->Name()]) {
          fprintf(fp_pl, "%s\t%d\t%d\t: N /%s\n",
                  string(string((isNameConvert)? 
                      _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name()) + string("/") +
                         string(shapeStor[curShapeIdx].name))
                      .c_str(),
                  GetScaleUpPointX(shapeStor[curShapeIdx].llx),
                  GetScaleUpPointY(shapeStor[curShapeIdx].lly), fixedStr.c_str());
        }
      }
    }
  }

  fclose(fp_pl);
}

// *.scl writing
void WriteScl(char *dir_tier, int curLayer) {
  char fn_scl[BUF_SZ];
  sprintf(fn_scl, "%s/%s.scl", dir_tier, gbch);
  FILE *fp_scl = fopen(fn_scl, "w");

  fputs("UCLA scl 1.0\n", fp_scl);
  fputs("# Created\t:\tJan  6 2005\n", fp_scl);
  fputs(
      "# User   \t:\tGi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp_scl);

  TIER *tier = &tier_st[curLayer];


  // sort the Y-Order due to limit of DP
  vector< ROW > tmpRowStor;
  for(int i = 0; i < tier->row_cnt; i++) {
    tmpRowStor.push_back(row_st[i]);
  }
  sort(tmpRowStor.begin(), tmpRowStor.end(), SortRowByCoordinate) ;
  
  fputs("\n", fp_scl);
  fprintf(fp_scl, "NumRows :  \t%lu\n", tmpRowStor.size() );
  fputs("\n", fp_scl);
  // iterate based on the sorted order
  for(auto &curRow : tmpRowStor) {
    fprintf(fp_scl, "CoreRow Horizontal\n");
    fprintf(fp_scl, "  Coordinate    :   %d\n", GetScaleUpPointY(curRow.pmin.y));
    fprintf(fp_scl, "  Height        :   %d\n", GetScaleUpSize(rowHeight));
    fprintf(fp_scl, "  Sitewidth     :    %d\n", GetScaleUpSize( curRow.site_wid ));
    fprintf(fp_scl, "  Sitespacing   :    %d\n", GetScaleUpSize( curRow.site_spa ));
    fprintf(fp_scl, "  Siteorient    :    %s\n",
            (curRow.isYSymmetry) ? "Y" : "1");
    fprintf(fp_scl, "  Sitesymmetry  :    %s\n", curRow.ori.c_str());
    fprintf(fp_scl, "  SubrowOrigin  :    %d\tNumSites  :  %d\n", GetScaleUpPointX(curRow.pmin.x),
            curRow.x_cnt);
    fprintf(fp_scl, "End\n");
  }

  fclose(fp_scl);
}

// write bookshelf's output
// z : current tier's index
void WriteBookshelfWithTier(char* dir_tier, int z, bool isShapeDrawing, 
    bool isNameConvert, bool isMetal1Removed) {
  PIN *pin = NULL;
  NET *net = NULL;

  // create directory for tier's result
//  char dir_tier[BUF_SZ] = {
//      0,
//  };
//  sprintf(dir_tier, "%s/tiers/%d", dir_bnd, z);
//
//  char cmd[BUF_SZ] = {
//      0,
//  };
//  sprintf(cmd, "mkdir -p %s", dir_tier);
//  system(cmd);

  /////////////////////////////////////////////////////////////////////////////
  // count for pin_term_cnt, net_cnt
  // (Required for *.nodes)
  int pin_term_cnt = 0;
  int net_cnt = 0;

  // to calculate net_cnt
  vector< int > netChk(netCNT, -1);

  for(int i = 0; i < pinCNT; i++) {
    pin = &pinInstance[i];
    if(!pin->term && netChk[pin->netID] == -1) {
      netChk[pin->netID] = 1;
      net_cnt++;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // count for pin_cnt
  // (Required for *.net)

  int pin_cnt = 0;
  for(int i = 0; i < netCNT; i++) {
    net = &netInstance[i];
    net->pinCNTinObject_tier = 0;

    if(netChk[i] != 1) {
      continue;
    }

    for(int j = 0; j < net->pinCNTinObject2; j++) {
      pin = net->pin[j];

      net->pinCNTinObject_tier++;
      pin_cnt++;
    }
  }


  WriteAux(dir_tier, isShapeDrawing);
  WriteWts(dir_tier);

  _bsMap.Init(); 
  WriteShapes(dir_tier, isShapeDrawing, isNameConvert);  
  WriteNet(dir_tier, z, pin_cnt, net_cnt, netChk, isShapeDrawing, isNameConvert);

  WriteScl(dir_tier, z);
  WriteNodes(dir_tier, z, pin_term_cnt, isShapeDrawing, isNameConvert);
  WritePl(dir_tier, z, isShapeDrawing, isNameConvert );

  WriteRoute( dir_tier, isNameConvert, routeInst, isMetal1Removed);


  // shapeSupport doesn't affect to below function
}

void BookshelfNameMap::Init() {
#ifdef USE_GOOGLE_HASH
  moduleToBsMap.set_empty_key(INIT_STR);
  bsToModuleMap.set_empty_key(INIT_STR);
  terminalToBsMap.set_empty_key(INIT_STR);
  bsToTerminalMap.set_empty_key(INIT_STR);
  netToBsMap.set_empty_key(INIT_STR);
  bsToNetMap.set_empty_key(INIT_STR); 
#endif

  bsModuleCnt = 0;
  for(int i=0; i<moduleCNT; i++) {
    MODULE* curModule = &moduleInstance[i];
    string bsModuleName = "o" + to_string(bsModuleCnt++);
    moduleToBsMap[ curModule->Name() ] = bsModuleName;
    bsToModuleMap[ bsModuleName ] = curModule->Name();
  }

  bsTerminalCnt = 0;
  for(int i=0; i<terminalCNT; i++) {
    TERM* curTerminal = &terminalInstance[i];
    if( curTerminal->isTerminalNI == false ) {
      string bsTerminalName = "o" + to_string(bsModuleCnt++);
      terminalToBsMap[ curTerminal ->Name() ] = bsTerminalName;
      bsToTerminalMap[ bsTerminalName ] = curTerminal->Name();
    }
    else { 
      string bsTerminalName = "p" + to_string(bsTerminalCnt++);
      terminalToBsMap[ curTerminal ->Name() ] = bsTerminalName;
      bsToTerminalMap[ bsTerminalName ] = curTerminal->Name();
    }
  }

  bsNetCnt = 0;
  for(int i=0; i<netCNT; i++) {
    NET* curNet = &netInstance[i]; 
    string bsNetName = "n" + to_string(bsNetCnt++);
    netToBsMap[ curNet->Name() ] = bsNetName;
    bsToNetMap[ bsNetName ] = curNet->Name();
  }
}

const char* BookshelfNameMap::GetOrigModuleName( const char* name ) {
  auto mtPtr = bsToModuleMap.find(string(name)); 
  if( mtPtr != bsToModuleMap.end()) {
    return mtPtr->second.c_str();
  }
  else {
    cout << "WARNING:  " << __FUNCTION__ << " Cannot find " << name << " in bookshelf name map!" << endl;
    return "";
  }
}

const char* BookshelfNameMap::GetOrigTerminalName( const char* name ) {
  auto mtPtr = bsToTerminalMap.find(string(name)); 
  if( mtPtr != bsToTerminalMap.end()) {
    return mtPtr->second.c_str();
  }
  else {
    cout << "ERROR:  " << __FUNCTION__ << " Cannot find " << name << " in bookshelf name map!" << endl;
    exit(1);
  }
}

const char* BookshelfNameMap::GetBsModuleName( const char* name ) {
  auto mtPtr = moduleToBsMap.find(string(name)); 
  if( mtPtr != moduleToBsMap.end()) {
    return mtPtr->second.c_str();
  }
  else {
    cout << "ERROR: Cannot find " << name << " in bookshelf name map!" << endl;
    exit(1);
  }
}
const char* BookshelfNameMap::GetBsTerminalName( const char* name ) {
  auto tPtr = terminalToBsMap.find(string(name)); 
  if( tPtr != terminalToBsMap.end()) {
    return tPtr->second.c_str();
  }
  else {
    cout << "ERROR: Cannot find " << name << " in bookshelf name map!" << endl;
    exit(1);
  }
}

const char* BookshelfNameMap::GetBsNetName( const char* name ){
  auto netPtr = netToBsMap.find(string(name)); 
  if( netPtr != netToBsMap.end()) {
    return netPtr->second.c_str();
  }
  else {
    cout << "ERROR: Cannot find " << name << " in bookshelf name map!" << endl;
    exit(1);
  }
}

//
//
// read *.pl back to the replace to do some post-processing -- nodesMap
//
// See also ReadPlLefDef(const char* fileName) in lefdefIO.cpp
//
void ReadPlBookshelf(const char *fileName) {
  cout << "READ BACK FROM " << fileName << endl;
  FPOS pof;
  FILE *fp = fopen(fileName, "r");
  if(!fp) {
    runtimeError(fileName + string(" is not Exist!"));
  }

  char buf[BUF_SZ], name[BUF_SZ];
  int moduleID = 0;
  while(fgets(buf, BUF_SZ, fp)) {
    char *token = strtok(buf, " \t\n");
    if(!token || token[0] == '#' || !strcmp(token, "UCLA"))
      continue;

    strcpy(name, token);

    if(name[0] == 'f' && name[1] == 'a' && name[2] == 'k' && name[3] == 'e')
      continue;

    // if(!getCellIndex ( name , & moduleID , & isTerminal,
    // &isTerminalNI))
    // continue;

    bool isTerminal = false;
    auto nodesMapPtr = nodesMap.find(string(name));
    if(nodesMapPtr == nodesMap.end()) {
      continue;
    }
    else {
      isTerminal = nodesMapPtr->second.isTerminal;
      moduleID = nodesMapPtr->second.index;
    }

    if(!isTerminal) {
      MODULE *curModule = &moduleInstance[moduleID];

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
        PIN *pin = curModule->pin[i];
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

void output_tier_pl_global_router(string plName, int z, bool isNameConvert) {
  FILE *fp_pl = NULL;

  struct TIER *tier = &tier_st[z];
  struct MODULE *modu = NULL;
  struct TERM *curTerminal = NULL;

  cout << "INFO:  pl file writing: " << plName << endl; 

  fp_pl = fopen(plName.c_str(), "w");

  fputs("UCLA pl 1.0\n", fp_pl);
  fputs("# Created\t:\tJan  6 2005\n", fp_pl);
  fputs(
      "# User   \t:\tGi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp_pl);
  fputs("\n", fp_pl);

  for(int i = 0; i < tier->modu_cnt; i++) {
    modu = tier->modu_st[i];
    fprintf(fp_pl, "%s\t%.6lf\t%.6lf : N\n", 
        (isNameConvert)? _bsMap.GetBsModuleName( modu->Name() ) : modu->Name(), 
        GetScaleUpPointFloatX( modu->pmin.x ), 
        GetScaleUpPointFloatY( modu->pmin.y ));
  }

  for(int i = 0; i < terminalCNT; i++) {
    curTerminal = &terminalInstance[i];
    if(!curTerminal->isTerminalNI) {
      fprintf(fp_pl, "%s\t%d\t%d\t: N /FIXED\n", 
          (isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(),
              GetScaleUpPointX(curTerminal->pmin.x), 
              GetScaleUpPointY(curTerminal->pmin.y));
    }
    else {
      fprintf(fp_pl, "%s\t%d\t%d\t: N /FIXED_NI\n", 
          (isNameConvert)? _bsMap.GetBsTerminalName( curTerminal->Name()) : curTerminal->Name(),
              GetScaleUpPointX(curTerminal->pmin.x), 
              GetScaleUpPointY(curTerminal->pmin.y));
    }
  }

  fclose(fp_pl);
  cout << "INFO:  Temp. placement solution for Global Router has been "
          "written at "
       << endl
       << "       " << plName << endl
       << endl;
}

//
// update tier_min, tier_max, and tier_row_cnt
//
void get_mms_3d_dim(FPOS *tier_min, FPOS *tier_max, int *tier_row_cnt) {
  prec xlen = grow_pmax.x - grow_pmin.x;
  prec ylen = grow_pmax.y - grow_pmin.y;
  prec aspect_ratio = (prec)ylen / (prec)xlen;

  PrintInfoPrec("AspectRatio", aspect_ratio);
  PrintInfoPrecPair("RowMinXY", grow_pmin.x, grow_pmin.y);
  PrintInfoPrecPair("RowMaxXY", grow_pmax.x, grow_pmax.y);
//  PrintInfoPrecPair("TerminalMinXY", terminal_pmin.x, terminal_pmin.y);
//  PrintInfoPrecPair("TerminalMaxXY", terminal_pmax.x, terminal_pmax.y);

  *tier_row_cnt = row_cnt;  

  tier_min->x = grow_pmin.x;
  tier_min->y = grow_pmin.y;
  
  tier_max->x = grow_pmax.x;
  tier_max->y = grow_pmax.y;
}

void output_gp_net_hpwl(char *fn) {
  int i = 0;
  FILE *fp = fopen(fn, "w");
  struct FPOS net_hpwl;
  struct NET *net = NULL;

  for(i = 0; i < netCNT; i++) {
    net = &netInstance[i];
    net_hpwl.x = net->max_x - net->min_x;
    net_hpwl.y = net->max_y - net->min_y;

    fprintf(fp, "%s %.6lf %.6lf\n", net->Name(), net_hpwl.x, net_hpwl.y);
  }
}

/*
void output_net_hpwl(char *fn) {
    int i = 0;
    FILE *fp = fopen(fn, "w");
    struct FPOS net_hpwl;
    struct NET *net = NULL;

    for(i = 0; i < netCNT; i++) {
        net = &netInstance[i];
        net_hpwl.x = net->max_x - net->min_x;
        net_hpwl.y = net->max_y - net->min_y;
        net_hpwl.z = net->max_z - net->min_z;

        fprintf(fp, "%s %.6lf %.6lf %.6lf\n", net->Name(), net_hpwl.x, net_hpwl.y,
                net_hpwl.z);
    }
}
*/

void output_final_pl(char *output) {
  int i = 0;
  FILE *fp = fopen(output, "w");
  struct MODULE *curModule = NULL;
  struct TERM *curTerminal = NULL;

  fputs("UCLA pl 1.0 \n", fp);
  fputs("# Created	:	Jan  6 2005\n", fp);
  fputs(
      "# User   	:	Gi-Joon Nam & Mehmet Yildiz at IBM Austin "
      "Research({gnam, mcan}@us.ibm.com)\n",
      fp);
  fputs("\n", fp);

  for(i = 0; i < moduleCNT; i++) {
    curModule = &moduleInstance[i];

    if(flg_3dic) {
      if(curModule->flg == StdCell) {
        fprintf(fp, "%s\t%d\t%d\t%d\t: N\n", curModule->Name(),
                (int)(curModule->pmin.x + 0.5), (int)(curModule->pmin.y + 0.5), 0);
      }
      else {
        fprintf(fp, "%s\t%d\t%d\t%d\t: N\n", curModule->Name(),
                curModule->pmin_lg.x, curModule->pmin_lg.y, 0);
      }
    }
    else {
      if(curModule->flg == StdCell) {
        fprintf(fp, "%s\t%d\t%d\t: N\n", curModule->Name(),
                (int)(curModule->pmin.x + 0.5), (int)(curModule->pmin.y + 0.5));
      }
      else {
        fprintf(fp, "%s\t%d\t%d\t: N\n", curModule->Name(), curModule->pmin_lg.x,
                curModule->pmin_lg.y);
      }
    }
  }

  for(i = 0; i < terminalCNT; i++) {
    if(flg_3dic) {
      curTerminal = &terminalInstance[i];
      if(!curTerminal->isTerminalNI) {
        fprintf(fp, "%s %d\t%d\t%d\t: N /FIXED\n", curTerminal->Name(),
                INT_CONVERT(curTerminal->pmin.x), INT_CONVERT(curTerminal->pmin.y), 0);
      }
      else {
        fprintf(fp, "%s %d\t%d\t%d\t: N /FIXED_NI\n", curTerminal->Name(),
                INT_CONVERT(curTerminal->pmin.x), INT_CONVERT(curTerminal->pmin.y), 0);
      }
    }
    else {
      curTerminal = &terminalInstance[i];
      if(!curTerminal->isTerminalNI) {
        fprintf(fp, "%s %d\t%d\t: N /FIXED\n", curTerminal->Name(),
                INT_CONVERT(curTerminal->pmin.x), INT_CONVERT(curTerminal->pmin.y));
      }
      else {
        fprintf(fp, "%s %d\t%d\t: N /FIXED_NI\n", curTerminal->Name(),
                INT_CONVERT(curTerminal->pmin.x), INT_CONVERT(curTerminal->pmin.y));
      }
    }
  }
  fclose(fp);
}

void extract_dir(char *f, char *d) {
  int i = 0, idx = 0;
  strcpy(d, f);

  int len = strlen(f);

  for(i = 0; i < len; i++) {
    if(f[i] == '/') {
      idx = i + 1;
      continue;
    }
  }
  d[idx] = '\0';
}

void extract_sfx(char *f, char *s) {
  int i = 0, last_dot = 0;
  int len = strlen(f);

  for(i = 0; i < len; i++) {
    if(f[i] == '.') {
      last_dot = i;
      continue;
    }
  }

  strcpy(s, &f[last_dot + 1]);
}

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

// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2017, Cadence Design Systems
//
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8.
//
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
//
//  $Author$
//  $Revision$
//  $Date$
//  $State:  $
// *****************************************************************************
// *****************************************************************************
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "defrReader.hpp"
#include "defiAlias.hpp"

#include <vector>
#include "lefdefIO.h"

using namespace std;
using Replace::Circuit;

// Global variables
static FILE* fout;
static void* userData;
static int numObjs;
static int isSumSet;    // to keep track if within SUM
static int isProp = 0;  // for PROPERTYDEFINITIONS
static int
    begOperand;  // to keep track for constraint, to print - as the 1st char
static double curVer = 0;
static int setSNetWireCbk = 0;
static int ignoreRowNames = 0;
static int ignoreViaNames = 0;
static int testDebugPrint = 0;  // test for ccr1488696

static string* defVersionPtr = 0;
static string* defDividerCharPtr = 0;
static string* defBusBitCharPtr = 0;
static string* defDesignNamePtr = 0;

static bool isVersionVisit = false;
static bool isDividerCharVisit = false;
static bool isBusBitCharVisit = false;
static bool isDesignNameVisit = false;

static vector< defiProp >* defPropStorPtr = 0;

static defiBox* defDieAreaPtr = 0;
static vector< defiRow >* defRowStorPtr = 0;
static vector< defiTrack >* defTrackStorPtr = 0;
static vector< defiGcellGrid >* defGcellGridStorPtr = 0;
static vector< defiVia >* defViaStorPtr = 0;

static defiComponentMaskShiftLayer* defComponentMaskShiftLayerPtr = 0;
static vector< defiComponent >* defComponentStorPtr = 0;
static vector< defiNet >* defNetStorPtr = 0;
static vector< defiBlockage >* defBlockageStorPtr = 0;

static vector< defiNet >* defSpecialNetStorPtr = 0;
// 0 for SpecialNet, 1 for SpecialPartialPath
#define DEF_SPECIALNET_ORIGINAL 0
#define DEF_SPECIALNET_PARTIAL_PATH 1
static vector< int > defSpecialNetType;

static vector< defiPin >* defPinStorPtr = 0;

static double* defUnitPtr = 0;

static HASH_MAP< string, int >* defComponentMapPtr = 0;
static HASH_MAP< string, int >* defPinMapPtr = 0;
static HASH_MAP< int, int >* defRowY2OrientMapPtr = 0;

static vector< HASH_MAP< string, int > >* defComponentPinToNetPtr = 0;
static HASH_MAP< string, int >* currentPinMap = 0;

// TX_DIR:TRANSLATION ON

// static void myLogFunction(const char* errMsg){
//    CIRCUIT_FPRINTF(fout, "ERROR: found error: %s\n", errMsg);
//}

// static void myWarningLogFunction(const char* errMsg){
//    CIRCUIT_FPRINTF(fout, "WARNING: found error: %s\n", errMsg);
//}

static void dataError() {
  CIRCUIT_FPRINTF(fout, "ERROR: returned user data is not correct!\n");
}

static void checkType(defrCallbackType_e c) {
  if(c >= 0 && c <= defrDesignEndCbkType) {
    // OK
  }
  else {
    CIRCUIT_FPRINTF(fout, "ERROR: callback type is out of bounds!\n");
  }
}

static int done(defrCallbackType_e c, void*, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "END DESIGN\n");
  return 0;
}

static int endfunc(defrCallbackType_e c, void*, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  return 0;
}

static const char* orientStr(int orient) {
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

int compMSL(defrCallbackType_e c, defiComponentMaskShiftLayer* co,
            defiUserData ud) {
  int i;

  checkType(c);
  if(ud != userData)
    dataError();

  if(co->numMaskShiftLayers()) {
    *defComponentMaskShiftLayerPtr = *co;
    CIRCUIT_FPRINTF(fout, "COMPONENTMASKSHIFT ");

    for(i = 0; i < co->numMaskShiftLayers(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", co->maskShiftLayer(i));
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }

  return 0;
}

int compf(defrCallbackType_e c, defiComponent* co, defiUserData ud) {
  currentPinMap = new HASH_MAP< string, int >;
#ifdef USE_GOOGLE_HASH
  currentPinMap->set_empty_key(INIT_STR);
#endif

  (*defComponentMapPtr)[string(co->id())] = defComponentStorPtr->size();
  defComponentStorPtr->push_back(*co);
  defComponentPinToNetPtr->push_back(*currentPinMap);

  delete currentPinMap;
  currentPinMap = NULL;

  if(testDebugPrint) {
    co->print(fout);
  }
  else {
    int i;

    checkType(c);
    if(ud != userData)
      dataError();
    //  missing GENERATE, FOREIGN
    CIRCUIT_FPRINTF(fout, "- %s %s ", co->id(), co->name());
    //    co->changeIdAndName("idName", "modelName");
    //    CIRCUIT_FPRINTF(fout, "%s %s ", co->id(),
    //            co->name());
    if(co->hasNets()) {
      for(i = 0; i < co->numNets(); i++)
        CIRCUIT_FPRINTF(fout, "%s ", co->net(i));
    }
    if(co->isFixed())
      CIRCUIT_FPRINTF(fout, "+ FIXED %d %d %s ", co->placementX(),
                      co->placementY(),
                      // orientStr(co->placementOrient()));
                      co->placementOrientStr());
    if(co->isCover())
      CIRCUIT_FPRINTF(fout, "+ COVER %d %d %s ", co->placementX(),
                      co->placementY(), orientStr(co->placementOrient()));
    if(co->isPlaced())
      CIRCUIT_FPRINTF(fout, "+ PLACED %d %d %s ", co->placementX(),
                      co->placementY(), orientStr(co->placementOrient()));
    if(co->isUnplaced()) {
      CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
      if((co->placementX() != -1) || (co->placementY() != -1))
        CIRCUIT_FPRINTF(fout, "%d %d %s ", co->placementX(), co->placementY(),
                        orientStr(co->placementOrient()));
    }
    if(co->hasSource())
      CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", co->source());
    if(co->hasGenerate()) {
      CIRCUIT_FPRINTF(fout, "+ GENERATE %s ", co->generateName());
      if(co->macroName() && *(co->macroName()))
        CIRCUIT_FPRINTF(fout, "%s ", co->macroName());
    }
    if(co->hasWeight())
      CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", co->weight());
    if(co->hasEEQ())
      CIRCUIT_FPRINTF(fout, "+ EEQMASTER %s ", co->EEQ());
    if(co->hasRegionName())
      CIRCUIT_FPRINTF(fout, "+ REGION %s ", co->regionName());
    if(co->hasRegionBounds()) {
      int *xl, *yl, *xh, *yh;
      int size;
      co->regionBounds(&size, &xl, &yl, &xh, &yh);
      for(i = 0; i < size; i++) {
        CIRCUIT_FPRINTF(fout, "+ REGION %d %d %d %d \n", xl[i], yl[i], xh[i],
                        yh[i]);
      }
    }
    if(co->maskShiftSize()) {
      CIRCUIT_FPRINTF(fout, "+ MASKSHIFT ");

      for(int i = co->maskShiftSize() - 1; i >= 0; i--) {
        CIRCUIT_FPRINTF(fout, "%d", co->maskShift(i));
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
    if(co->hasHalo()) {
      int left, bottom, right, top;
      (void)co->haloEdges(&left, &bottom, &right, &top);
      CIRCUIT_FPRINTF(fout, "+ HALO ");
      if(co->hasHaloSoft())
        CIRCUIT_FPRINTF(fout, "SOFT ");
      CIRCUIT_FPRINTF(fout, "%d %d %d %d\n", left, bottom, right, top);
    }
    if(co->hasRouteHalo()) {
      CIRCUIT_FPRINTF(fout, "+ ROUTEHALO %d %s %s\n", co->haloDist(),
                      co->minLayer(), co->maxLayer());
    }
    if(co->hasForeignName()) {
      CIRCUIT_FPRINTF(fout, "+ FOREIGN %s %d %d %s %d ", co->foreignName(),
                      co->foreignX(), co->foreignY(), co->foreignOri(),
                      co->foreignOrient());
    }
    if(co->numProps()) {
      for(i = 0; i < co->numProps(); i++) {
        CIRCUIT_FPRINTF(fout, "+ PROPERTY %s %s ", co->propName(i),
                        co->propValue(i));
        switch(co->propType(i)) {
          case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
            break;
          case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
            break;
          case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
            break;
          case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
            break;
          case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
            break;
        }
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
    --numObjs;
    if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END COMPONENTS\n");
  }

  return 0;
}

int netpath(defrCallbackType_e, defiNet*, defiUserData) {
  CIRCUIT_FPRINTF(fout, "\n");

  CIRCUIT_FPRINTF(fout, "Callback of partial path for net\n");

  return 0;
}

int netNamef(defrCallbackType_e c, const char* netName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "- %s ", netName);
  return 0;
}

int subnetNamef(defrCallbackType_e c, const char* subnetName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  if(curVer >= 5.6)
    CIRCUIT_FPRINTF(fout, "   + SUBNET CBK %s ", subnetName);
  return 0;
}

int nondefRulef(defrCallbackType_e c, const char* ruleName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  if(curVer >= 5.6)
    CIRCUIT_FPRINTF(fout, "   + NONDEFAULTRULE CBK %s ", ruleName);
  return 0;
}

int netf(defrCallbackType_e c, defiNet* net, defiUserData ud) {
  // For net and special net.
  int i, j, k, w, x, y, z, count, newLayer;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiVpin* vpin;
  // defiShield *noShield;
  defiWire* wire;

  checkType(c);
  if(ud != userData)
    dataError();
  if(c != defrNetCbkType)
    CIRCUIT_FPRINTF(fout, "BOGUS NET TYPE  ");

  if(net->pinIsMustJoin(0))
    CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
  // 5/6/2004 - don't need since I have a callback for the name
  //  else
  //      CIRCUIT_FPRINTF(fout, "- %s ", net->name());

  //  net->changeNetName("newNetName");
  //  CIRCUIT_FPRINTF(fout, "%s ", net->name());
  count = 0;
  // compName & pinName
  for(i = 0; i < net->numConnections(); i++) {
    // set the limit of only 5 items per line
    count++;
    if(count >= 5) {
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
    CIRCUIT_FPRINTF(fout, "( %s %s ) ", net->instance(i), net->pin(i));
    //      net->changeInstance("newInstance", i);
    //      net->changePin("newPin", i);
    //      CIRCUIT_FPRINTF(fout, "( %s %s ) ", net->instance(i),
    //              net->pin(i));

    // skip for PIN instance
    if(strcmp(net->instance(i), "PIN") != 0) {
      HASH_MAP< string, int >* currentPinMap =
          &(*defComponentPinToNetPtr)[(
              *defComponentMapPtr)[string(net->instance(i))]];

      (*currentPinMap)[string(net->pin(i))] = defNetStorPtr->size();
    }

    if(net->pinIsSynthesized(i))
      CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
  }

  if(net->hasNonDefaultRule())
    CIRCUIT_FPRINTF(fout, "+ NONDEFAULTRULE %s\n", net->nonDefaultRule());

  for(i = 0; i < net->numVpins(); i++) {
    vpin = net->vpin(i);
    CIRCUIT_FPRINTF(fout, "  + %s", vpin->name());
    if(vpin->layer())
      CIRCUIT_FPRINTF(fout, " %s", vpin->layer());
    CIRCUIT_FPRINTF(fout, " %d %d %d %d", vpin->xl(), vpin->yl(), vpin->xh(),
                    vpin->yh());
    if(vpin->status() != ' ') {
      CIRCUIT_FPRINTF(fout, " %c", vpin->status());
      CIRCUIT_FPRINTF(fout, " %d %d", vpin->xLoc(), vpin->yLoc());
      if(vpin->orient() != -1)
        CIRCUIT_FPRINTF(fout, " %s", orientStr(vpin->orient()));
    }
    CIRCUIT_FPRINTF(fout, "\n");
  }

  // regularWiring
  if(net->numWires()) {
    for(i = 0; i < net->numWires(); i++) {
      newLayer = 0;
      wire = net->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      count = 0;
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else {
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              }
              break;
            case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
              CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_RECT:
              p->getViaRect(&w, &x, &y, &z);
              CIRCUIT_FPRINTF(fout, "RECT ( %d %d %d %d ) ", w, x, y, z);
              break;
            case DEFIPATH_VIRTUALPOINT:
              p->getVirtualPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "VIRTUAL ( %d %d ) ", x, y);
              break;
            case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_TAPERRULE:
              CIRCUIT_FPRINTF(fout, "TAPERRULE %s ", p->getTaperRule());
              break;
            case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "STYLE %d ", p->getStyle());
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }

  // SHIELDNET
  if(net->numShieldNets()) {
    for(i = 0; i < net->numShieldNets(); i++)
      CIRCUIT_FPRINTF(fout, "\n  + SHIELDNET %s", net->shieldNet(i));
  }

  /* obsolete in 5.4
  if (net->numNoShields()) {
      for (i = 0; i < net->numNoShields(); i++) {
          noShield = net->noShield(i);
          CIRCUIT_FPRINTF(fout, "\n  + NOSHIELD ");
          newLayer = 0;
          for (j = 0; j < noShield->numPaths(); j++) {
              p = noShield->path(j);
              p->initTraverse();
              while ((path = (int)p->next()) != DEFIPATH_DONE) {
                  count++;
                  // Don't want the line to be too long
                  if (count >= 5) {
                      CIRCUIT_FPRINTF(fout, "\n");
                      count = 0;
                  }
                  switch (path) {
                      case DEFIPATH_LAYER:
                          if (newLayer == 0) {
                              CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                              newLayer = 1;
                          } else
                              CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                          break;
                      case DEFIPATH_VIA:
                          CIRCUIT_FPRINTF(fout, "%s ", p->getVia());
                          break;
                      case DEFIPATH_VIAROTATION:
                          CIRCUIT_FPRINTF(fout, "%s ",
                                  orientStr(p->getViaRotation()));
                          break;
                      case DEFIPATH_WIDTH:
                          CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                          break;
                      case DEFIPATH_POINT:
                          p->getPoint(&x, &y);
                          CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                          break;
                      case DEFIPATH_FLUSHPOINT:
                          p->getFlushPoint(&x, &y, &z);
                          CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                          break;
                      case DEFIPATH_TAPER:
                          CIRCUIT_FPRINTF(fout, "TAPER ");
                          break;
                      case DEFIPATH_TAPERRULE:
                          CIRCUIT_FPRINTF(fout, "TAPERRULE %s ",
                                  p->getTaperRule());
                          break;
                  }
              }
          }
      }
  }
  */

  if(net->hasSubnets()) {
    for(i = 0; i < net->numSubnets(); i++) {
      s = net->subnet(i);
      CIRCUIT_FPRINTF(fout, "\n");

      if(s->numConnections()) {
        if(s->pinIsMustJoin(0)) {
          CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "  + SUBNET %s ", s->name());
        }
        for(j = 0; j < s->numConnections(); j++)
          CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));

        // regularWiring
        if(s->numWires()) {
          for(k = 0; k < s->numWires(); k++) {
            newLayer = 0;
            wire = s->wire(k);
            CIRCUIT_FPRINTF(fout, "  %s ", wire->wireType());
            count = 0;
            for(j = 0; j < wire->numPaths(); j++) {
              p = wire->path(j);
              p->initTraverse();
              while((path = (int)p->next()) != DEFIPATH_DONE) {
                count++;
                // Don't want the line to be too long
                if(count >= 5) {
                  CIRCUIT_FPRINTF(fout, "\n");
                  count = 0;
                }
                switch(path) {
                  case DEFIPATH_LAYER:
                    if(newLayer == 0) {
                      CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                      newLayer = 1;
                    }
                    else
                      CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                    break;
                  case DEFIPATH_VIA:
                    CIRCUIT_FPRINTF(fout, "%s ",
                                    ignoreViaNames ? "XXX" : p->getVia());
                    break;
                  case DEFIPATH_VIAROTATION:
                    CIRCUIT_FPRINTF(fout, "%s ", p->getViaRotationStr());
                    break;
                  case DEFIPATH_WIDTH:
                    CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                    break;
                  case DEFIPATH_POINT:
                    p->getPoint(&x, &y);
                    CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                    break;
                  case DEFIPATH_FLUSHPOINT:
                    p->getFlushPoint(&x, &y, &z);
                    CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                    break;
                  case DEFIPATH_TAPER:
                    CIRCUIT_FPRINTF(fout, "TAPER ");
                    break;
                  case DEFIPATH_TAPERRULE:
                    CIRCUIT_FPRINTF(fout, "TAPERRULE  %s ", p->getTaperRule());
                    break;
                  case DEFIPATH_STYLE:
                    CIRCUIT_FPRINTF(fout, "STYLE  %d ", p->getStyle());
                    break;
                }
              }
            }
          }
        }
      }
    }
  }

  if(net->numProps()) {
    for(i = 0; i < net->numProps(); i++) {
      CIRCUIT_FPRINTF(fout, "  + PROPERTY %s ", net->propName(i));
      switch(net->propType(i)) {
        case 'R':
          CIRCUIT_FPRINTF(fout, "%g REAL ", net->propNumber(i));
          break;
        case 'I':
          CIRCUIT_FPRINTF(fout, "%g INTEGER ", net->propNumber(i));
          break;
        case 'S':
          CIRCUIT_FPRINTF(fout, "%s STRING ", net->propValue(i));
          break;
        case 'Q':
          CIRCUIT_FPRINTF(fout, "%s QUOTESTRING ", net->propValue(i));
          break;
        case 'N':
          CIRCUIT_FPRINTF(fout, "%g NUMBER ", net->propNumber(i));
          break;
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
  }

  if(net->hasWeight())
    CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", net->weight());
  if(net->hasCap())
    CIRCUIT_FPRINTF(fout, "+ ESTCAP %g ", net->cap());
  if(net->hasSource())
    CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", net->source());
  if(net->hasFixedbump())
    CIRCUIT_FPRINTF(fout, "+ FIXEDBUMP ");
  if(net->hasFrequency())
    CIRCUIT_FPRINTF(fout, "+ FREQUENCY %g ", net->frequency());
  if(net->hasPattern())
    CIRCUIT_FPRINTF(fout, "+ PATTERN %s ", net->pattern());
  if(net->hasOriginal())
    CIRCUIT_FPRINTF(fout, "+ ORIGINAL %s ", net->original());
  if(net->hasUse())
    CIRCUIT_FPRINTF(fout, "+ USE %s ", net->use());

  CIRCUIT_FPRINTF(fout, ";\n");
  --numObjs;
  if(numObjs <= 0)
    CIRCUIT_FPRINTF(fout, "END NETS\n");

  defNetStorPtr->push_back(*net);
  return 0;
}

int snetpath(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
  int i, j, x, y, z, count, newLayer;
  char* layerName;
  double dist, left, right;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiShield* shield;
  defiWire* wire;
  int numX, numY, stepX, stepY;

  if(c != defrSNetPartialPathCbkType)
    return 1;
  if(ud != userData)
    dataError();

  CIRCUIT_FPRINTF(fout, "SPECIALNET partial data\n");

  CIRCUIT_FPRINTF(fout, "- %s ", ppath->name());
  defSpecialNetStorPtr->push_back(*ppath);
  defSpecialNetType.push_back(DEF_SPECIALNET_PARTIAL_PATH);

  count = 0;
  // compName & pinName
  for(i = 0; i < ppath->numConnections(); i++) {
    // set the limit of only 5 items print out in one line
    count++;
    if(count >= 5) {
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
    CIRCUIT_FPRINTF(fout, "( %s %s ) ", ppath->instance(i), ppath->pin(i));
    if(ppath->pinIsSynthesized(i))
      CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
  }

  // specialWiring
  // POLYGON
  if(ppath->numPolygons()) {
    struct defiPoints points;
    for(i = 0; i < ppath->numPolygons(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", ppath->polygonName(i));
      points = ppath->getPolygon(i);
      for(j = 0; j < points.numPoints; j++)
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
    }
  }
  // RECT
  if(ppath->numRectangles()) {
    for(i = 0; i < ppath->numRectangles(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
                      ppath->xl(i), ppath->yl(i), ppath->xh(i), ppath->yh(i));
    }
  }

  // COVER, FIXED, ROUTED or SHIELD
  if(ppath->numWires()) {
    newLayer = 0;
    for(i = 0; i < ppath->numWires(); i++) {
      newLayer = 0;
      wire = ppath->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      if(strcmp(wire->wireType(), "SHIELD") == 0)
        CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
              CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_VIADATA:
              p->getViaData(&numX, &numY, &stepX, &stepY);
              CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                              stepX, stepY);
              break;
            case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
              CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }

  if(ppath->hasSubnets()) {
    for(i = 0; i < ppath->numSubnets(); i++) {
      s = ppath->subnet(i);
      if(s->numConnections()) {
        if(s->pinIsMustJoin(0)) {
          CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "- %s ", s->name());
        }
        for(j = 0; j < s->numConnections(); j++) {
          CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
        }
      }

      // regularWiring
      if(s->numWires()) {
        for(i = 0; i < s->numWires(); i++) {
          wire = s->wire(i);
          CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->print(fout);
          }
        }
      }
    }
  }

  if(ppath->numProps()) {
    for(i = 0; i < ppath->numProps(); i++) {
      if(ppath->propIsString(i))
        CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", ppath->propName(i),
                        ppath->propValue(i));
      if(ppath->propIsNumber(i))
        CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", ppath->propName(i),
                        ppath->propNumber(i));
      switch(ppath->propType(i)) {
        case 'R':
          CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
          CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
          CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
          CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
          CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
  }

  // SHIELD
  count = 0;
  // testing the SHIELD for 5.3, obsolete in 5.4
  if(ppath->numShields()) {
    for(i = 0; i < ppath->numShields(); i++) {
      shield = ppath->shield(i);
      CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
      newLayer = 0;
      for(j = 0; j < shield->numPaths(); j++) {
        p = shield->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
              if(newLayer) {
                CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              }
              else {
                CIRCUIT_FPRINTF(fout, "Str %s ", p->getViaRotationStr());
              }
              break;
            case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
              CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
          }
        }
      }
    }
  }

  // layerName width
  if(ppath->hasWidthRules()) {
    for(i = 0; i < ppath->numWidthRules(); i++) {
      ppath->widthRule(i, &layerName, &dist);
      CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
    }
  }

  // layerName spacing
  if(ppath->hasSpacingRules()) {
    for(i = 0; i < ppath->numSpacingRules(); i++) {
      ppath->spacingRule(i, &layerName, &dist, &left, &right);
      if(left == right) {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                        dist, left, right);
      }
    }
  }

  if(ppath->hasFixedbump())
    CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
  if(ppath->hasFrequency())
    CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", ppath->frequency());
  if(ppath->hasVoltage())
    CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", ppath->voltage());
  if(ppath->hasWeight())
    CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", ppath->weight());
  if(ppath->hasCap())
    CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", ppath->cap());
  if(ppath->hasSource())
    CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", ppath->source());
  if(ppath->hasPattern())
    CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", ppath->pattern());
  if(ppath->hasOriginal())
    CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", ppath->original());
  if(ppath->hasUse())
    CIRCUIT_FPRINTF(fout, "\n  + USE %s ", ppath->use());

  CIRCUIT_FPRINTF(fout, "\n");

  return 0;
}

int snetwire(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
  int i, j, x, y, z, count = 0, newLayer;
  defiPath* p;
  int path;
  defiWire* wire;
  defiShield* shield;
  int numX, numY, stepX, stepY;

  if(c != defrSNetWireCbkType)
    return 1;
  if(ud != userData)
    dataError();

  CIRCUIT_FPRINTF(fout, "SPECIALNET wire data\n");

  CIRCUIT_FPRINTF(fout, "- %s ", ppath->name());

  // POLYGON
  if(ppath->numPolygons()) {
    struct defiPoints points;
    for(i = 0; i < ppath->numPolygons(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", ppath->polygonName(i));

      points = ppath->getPolygon(i);

      for(j = 0; j < points.numPoints; j++) {
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      }
    }
    // RECT
  }
  if(ppath->numRectangles()) {
    for(i = 0; i < ppath->numRectangles(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
                      ppath->xl(i), ppath->yl(i), ppath->xh(i), ppath->yh(i));
    }
  }
  // VIA
  if(ppath->numViaSpecs()) {
    for(i = 0; i < ppath->numViaSpecs(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + VIA %s ", ppath->viaName(i));
      CIRCUIT_FPRINTF(fout, " %s", ppath->viaOrientStr(i));

      defiPoints points = ppath->getViaPts(i);

      for(int j = 0; j < points.numPoints; j++) {
        CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
      }
    }
  }

  // specialWiring
  if(ppath->numWires()) {
    newLayer = 0;
    for(i = 0; i < ppath->numWires(); i++) {
      newLayer = 0;
      wire = ppath->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      if(strcmp(wire->wireType(), "SHIELD") == 0)
        CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
              CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_VIADATA:
              p->getViaData(&numX, &numY, &stepX, &stepY);
              CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                              stepX, stepY);
              break;
            case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
              CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }
  else if(ppath->numShields()) {
    for(i = 0; i < ppath->numShields(); i++) {
      shield = ppath->shield(i);
      CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
      newLayer = 0;
      for(j = 0; j < shield->numPaths(); j++) {
        p = shield->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
              CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
              CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
    }
  }

  CIRCUIT_FPRINTF(fout, "\n");

  return 0;
}

int snetf(defrCallbackType_e c, defiNet* net, defiUserData ud) {
  // For net and special net.
  int i, j, x, y, z, count, newLayer;
  char* layerName;
  double dist, left, right;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiShield* shield;
  defiWire* wire;
  int numX, numY, stepX, stepY;

  checkType(c);
  if(ud != userData)
    dataError();
  if(c != defrSNetCbkType)
    CIRCUIT_FPRINTF(fout, "BOGUS NET TYPE  ");

  // 5/6/2004 - don't need since I have a callback for the name
  //  CIRCUIT_FPRINTF(fout, "- %s ", net->name());

  count = 0;
  // compName & pinName
  for(i = 0; i < net->numConnections(); i++) {
    // set the limit of only 5 items print out in one line
    count++;
    if(count >= 5) {
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
    CIRCUIT_FPRINTF(fout, "( %s %s ) ", net->instance(i), net->pin(i));
    if(net->pinIsSynthesized(i))
      CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
  }

  // specialWiring
  if(net->numWires()) {
    newLayer = 0;
    for(i = 0; i < net->numWires(); i++) {
      newLayer = 0;
      wire = net->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      if(strcmp(wire->wireType(), "SHIELD") == 0)
        CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        if(testDebugPrint) {
          p->print(fout);
        }
        else {
          while((path = (int)p->next()) != DEFIPATH_DONE) {
            count++;
            // Don't want the line to be too long
            if(count >= 5) {
              CIRCUIT_FPRINTF(fout, "\n");
              count = 0;
            }
            switch(path) {
              case DEFIPATH_LAYER:
                if(newLayer == 0) {
                  CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                  newLayer = 1;
                }
                else
                  CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                break;
              case DEFIPATH_VIA:
                CIRCUIT_FPRINTF(fout, "%s ",
                                ignoreViaNames ? "XXX" : p->getVia());
                break;
              case DEFIPATH_VIAROTATION:
                CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                break;
              case DEFIPATH_VIADATA:
                p->getViaData(&numX, &numY, &stepX, &stepY);
                CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                stepX, stepY);
                break;
              case DEFIPATH_WIDTH:
                CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                break;
              case DEFIPATH_MASK:
                CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                break;
              case DEFIPATH_VIAMASK:
                CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                p->getViaCutMask(), p->getViaBottomMask());
                break;
              case DEFIPATH_POINT:
                p->getPoint(&x, &y);
                CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                break;
              case DEFIPATH_FLUSHPOINT:
                p->getFlushPoint(&x, &y, &z);
                CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                break;
              case DEFIPATH_TAPER:
                CIRCUIT_FPRINTF(fout, "TAPER ");
                break;
              case DEFIPATH_SHAPE:
                CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                break;
              case DEFIPATH_STYLE:
                CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                break;
            }
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }

  // POLYGON
  if(net->numPolygons()) {
    struct defiPoints points;

    for(i = 0; i < net->numPolygons(); i++) {
      if(curVer >= 5.8) {
        if(strcmp(net->polyRouteStatus(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + %s ", net->polyRouteStatus(i));
          if(strcmp(net->polyRouteStatus(i), "SHIELD") == 0) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ",
                            net->polyRouteStatusShieldName(i));
          }
        }
        if(strcmp(net->polyShapeType(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", net->polyShapeType(i));
        }
      }
      if(net->polyMask(i)) {
        CIRCUIT_FPRINTF(fout, "\n  + MASK %d + POLYGON % s ", net->polyMask(i),
                        net->polygonName(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", net->polygonName(i));
      }
      points = net->getPolygon(i);
      for(j = 0; j < points.numPoints; j++)
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
    }
  }
  // RECT
  if(net->numRectangles()) {
    for(i = 0; i < net->numRectangles(); i++) {
      if(curVer >= 5.8) {
        if(strcmp(net->rectRouteStatus(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + %s ", net->rectRouteStatus(i));
          if(strcmp(net->rectRouteStatus(i), "SHIELD") == 0) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ",
                            net->rectRouteStatusShieldName(i));
          }
        }
        if(strcmp(net->rectShapeType(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", net->rectShapeType(i));
        }
      }
      if(net->rectMask(i)) {
        CIRCUIT_FPRINTF(fout, "\n  + MASK %d + RECT %s %d %d %d %d",
                        net->rectMask(i), net->rectName(i), net->xl(i),
                        net->yl(i), net->xh(i), net->yh(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", net->rectName(i),
                        net->xl(i), net->yl(i), net->xh(i), net->yh(i));
      }
    }
  }
  // VIA
  if(curVer >= 5.8 && net->numViaSpecs()) {
    for(i = 0; i < net->numViaSpecs(); i++) {
      if(strcmp(net->viaRouteStatus(i), "") != 0) {
        CIRCUIT_FPRINTF(fout, "\n  + %s ", net->viaRouteStatus(i));
        if(strcmp(net->viaRouteStatus(i), "SHIELD") == 0) {
          CIRCUIT_FPRINTF(fout, "\n  + %s ", net->viaRouteStatusShieldName(i));
        }
      }
      if(strcmp(net->viaShapeType(i), "") != 0) {
        CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", net->viaShapeType(i));
      }
      if(net->topMaskNum(i) || net->cutMaskNum(i) || net->bottomMaskNum(i)) {
        CIRCUIT_FPRINTF(fout, "\n  + MASK %d%d%d + VIA %s ", net->topMaskNum(i),
                        net->cutMaskNum(i), net->bottomMaskNum(i),
                        net->viaName(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + VIA %s ", net->viaName(i));
      }
      CIRCUIT_FPRINTF(fout, " %s", net->viaOrientStr(i));

      defiPoints points = net->getViaPts(i);

      for(int j = 0; j < points.numPoints; j++) {
        CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(net->hasSubnets()) {
    for(i = 0; i < net->numSubnets(); i++) {
      s = net->subnet(i);
      if(s->numConnections()) {
        if(s->pinIsMustJoin(0)) {
          CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "- %s ", s->name());
        }
        for(j = 0; j < s->numConnections(); j++) {
          CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
        }
      }

      // regularWiring
      if(s->numWires()) {
        for(i = 0; i < s->numWires(); i++) {
          wire = s->wire(i);
          CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->print(fout);
          }
        }
      }
    }
  }

  if(net->numProps()) {
    for(i = 0; i < net->numProps(); i++) {
      if(net->propIsString(i))
        CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", net->propName(i),
                        net->propValue(i));
      if(net->propIsNumber(i))
        CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", net->propName(i),
                        net->propNumber(i));
      switch(net->propType(i)) {
        case 'R':
          CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
          CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
          CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
          CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
          CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
  }

  // SHIELD
  count = 0;
  // testing the SHIELD for 5.3, obsolete in 5.4
  if(net->numShields()) {
    for(i = 0; i < net->numShields(); i++) {
      shield = net->shield(i);
      CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
      newLayer = 0;
      for(j = 0; j < shield->numPaths(); j++) {
        p = shield->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
              CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
              CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
    }
  }

  // layerName width
  if(net->hasWidthRules()) {
    for(i = 0; i < net->numWidthRules(); i++) {
      net->widthRule(i, &layerName, &dist);
      CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
    }
  }

  // layerName spacing
  if(net->hasSpacingRules()) {
    for(i = 0; i < net->numSpacingRules(); i++) {
      net->spacingRule(i, &layerName, &dist, &left, &right);
      if(left == right) {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                        dist, left, right);
      }
    }
  }

  if(net->hasFixedbump())
    CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
  if(net->hasFrequency())
    CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", net->frequency());
  if(net->hasVoltage())
    CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", net->voltage());
  if(net->hasWeight())
    CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", net->weight());
  if(net->hasCap())
    CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", net->cap());
  if(net->hasSource())
    CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", net->source());
  if(net->hasPattern())
    CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", net->pattern());
  if(net->hasOriginal())
    CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", net->original());
  if(net->hasUse())
    CIRCUIT_FPRINTF(fout, "\n  + USE %s ", net->use());

  CIRCUIT_FPRINTF(fout, ";\n");
  defSpecialNetStorPtr->push_back(*net);
  defSpecialNetType.push_back(DEF_SPECIALNET_ORIGINAL);
  --numObjs;
  if(numObjs <= 0)
    CIRCUIT_FPRINTF(fout, "END SPECIALNETS\n");
  return 0;
}

int ndr(defrCallbackType_e c, defiNonDefault* nd, defiUserData ud) {
  // For nondefaultrule
  int i;

  checkType(c);
  if(ud != userData)
    dataError();
  if(c != defrNonDefaultCbkType)
    CIRCUIT_FPRINTF(fout, "BOGUS NONDEFAULTRULE TYPE  ");
  CIRCUIT_FPRINTF(fout, "- %s\n", nd->name());
  if(nd->hasHardspacing())
    CIRCUIT_FPRINTF(fout, "   + HARDSPACING\n");
  for(i = 0; i < nd->numLayers(); i++) {
    CIRCUIT_FPRINTF(fout, "   + LAYER %s", nd->layerName(i));
    CIRCUIT_FPRINTF(fout, " WIDTH %d", nd->layerWidthVal(i));
    if(nd->hasLayerDiagWidth(i))
      CIRCUIT_FPRINTF(fout, " DIAGWIDTH %d", nd->layerDiagWidthVal(i));
    if(nd->hasLayerSpacing(i))
      CIRCUIT_FPRINTF(fout, " SPACING %d", nd->layerSpacingVal(i));
    if(nd->hasLayerWireExt(i))
      CIRCUIT_FPRINTF(fout, " WIREEXT %d", nd->layerWireExtVal(i));
    CIRCUIT_FPRINTF(fout, "\n");
  }
  for(i = 0; i < nd->numVias(); i++)
    CIRCUIT_FPRINTF(fout, "   + VIA %s\n", nd->viaName(i));
  for(i = 0; i < nd->numViaRules(); i++)
    CIRCUIT_FPRINTF(fout, "   + VIARULE %s\n",
                    ignoreViaNames ? "XXX" : nd->viaRuleName(i));
  for(i = 0; i < nd->numMinCuts(); i++)
    CIRCUIT_FPRINTF(fout, "   + MINCUTS %s %d\n", nd->cutLayerName(i),
                    nd->numCuts(i));
  for(i = 0; i < nd->numProps(); i++) {
    CIRCUIT_FPRINTF(fout, "   + PROPERTY %s %s ", nd->propName(i),
                    nd->propValue(i));
    switch(nd->propType(i)) {
      case 'R':
        CIRCUIT_FPRINTF(fout, "REAL\n");
        break;
      case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER\n");
        break;
      case 'S':
        CIRCUIT_FPRINTF(fout, "STRING\n");
        break;
      case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING\n");
        break;
      case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER\n");
        break;
    }
  }
  --numObjs;
  if(numObjs <= 0)
    CIRCUIT_FPRINTF(fout, "END NONDEFAULTRULES\n");
  return 0;
}

int tname(defrCallbackType_e c, const char* string, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "TECHNOLOGY %s ;\n", string);
  return 0;
}

int dname(defrCallbackType_e c, const char* str, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defDesignNamePtr) = string(str);
  isDesignNameVisit = true;

  CIRCUIT_FPRINTF(fout, "DESIGN %s ;\n", str);
  return 0;
}

char* address(const char* in) {
  return ((char*)in);
}

int cs(defrCallbackType_e c, int num, defiUserData ud) {
  char* name;

  checkType(c);

  if(ud != userData)
    dataError();

  switch(c) {
    case defrComponentStartCbkType:
      name = address("COMPONENTS");
      defComponentStorPtr->reserve(num);
      defComponentPinToNetPtr->reserve(num);
      break;
    case defrNetStartCbkType:
      name = address("NETS");
      defNetStorPtr->reserve(num);
      break;
    case defrStartPinsCbkType:
      name = address("PINS");
      defPinStorPtr->reserve(num);
      break;
    case defrViaStartCbkType:
      name = address("VIAS");
      break;
    case defrRegionStartCbkType:
      name = address("REGIONS");
      break;
    case defrSNetStartCbkType:
      name = address("SPECIALNETS");
      break;
    case defrGroupsStartCbkType:
      name = address("GROUPS");
      break;
    case defrScanchainsStartCbkType:
      name = address("SCANCHAINS");
      break;
    case defrIOTimingsStartCbkType:
      name = address("IOTIMINGS");
      break;
    case defrFPCStartCbkType:
      name = address("FLOORPLANCONSTRAINTS");
      break;
    case defrTimingDisablesStartCbkType:
      name = address("TIMING DISABLES");
      break;
    case defrPartitionsStartCbkType:
      name = address("PARTITIONS");
      break;
    case defrPinPropStartCbkType:
      name = address("PINPROPERTIES");
      break;
    case defrBlockageStartCbkType:
      name = address("BLOCKAGES");
      break;
    case defrSlotStartCbkType:
      name = address("SLOTS");
      break;
    case defrFillStartCbkType:
      name = address("FILLS");
      break;
    case defrNonDefaultStartCbkType:
      name = address("NONDEFAULTRULES");
      break;
    case defrStylesStartCbkType:
      name = address("STYLES");
      break;
    default:
      name = address("BOGUS");
      return 1;
  }
  CIRCUIT_FPRINTF(fout, "\n%s %d ;\n", name, num);
  numObjs = num;
  return 0;
}

int constraintst(defrCallbackType_e c, int num, defiUserData ud) {
  // Handles both constraints and assertions
  checkType(c);
  if(ud != userData)
    dataError();
  if(c == defrConstraintsStartCbkType) {
    CIRCUIT_FPRINTF(fout, "\nCONSTRAINTS %d ;\n\n", num);
  }
  else {
    CIRCUIT_FPRINTF(fout, "\nASSERTIONS %d ;\n\n", num);
  }
  numObjs = num;
  return 0;
}

void operand(defrCallbackType_e c, defiAssertion* a, int ind) {
  int i, first = 1;
  char* netName;
  char *fromInst, *fromPin, *toInst, *toPin;

  if(a->isSum()) {
    // Sum in operand, recursively call operand
    CIRCUIT_FPRINTF(fout, "- SUM ( ");
    a->unsetSum();
    isSumSet = 1;
    begOperand = 0;
    operand(c, a, ind);
    CIRCUIT_FPRINTF(fout, ") ");
  }
  else {
    // operand
    if(ind >= a->numItems()) {
      CIRCUIT_FPRINTF(fout, "ERROR: when writing out SUM in Constraints.\n");
      return;
    }
    if(begOperand) {
      CIRCUIT_FPRINTF(fout, "- ");
      begOperand = 0;
    }
    for(i = ind; i < a->numItems(); i++) {
      if(a->isNet(i)) {
        a->net(i, &netName);
        if(!first)
          CIRCUIT_FPRINTF(fout, ", ");  // print , as separator
        CIRCUIT_FPRINTF(fout, "NET %s ", netName);
      }
      else if(a->isPath(i)) {
        a->path(i, &fromInst, &fromPin, &toInst, &toPin);
        if(!first)
          CIRCUIT_FPRINTF(fout, ", ");
        CIRCUIT_FPRINTF(fout, "PATH %s %s %s %s ", fromInst, fromPin, toInst,
                        toPin);
      }
      else if(isSumSet) {
        // SUM within SUM, reset the flag
        a->setSum();
        operand(c, a, i);
      }
      first = 0;
    }
  }
}

int constraint(defrCallbackType_e c, defiAssertion* a, defiUserData ud) {
  // Handles both constraints and assertions

  checkType(c);
  if(ud != userData)
    dataError();
  if(a->isWiredlogic()) {
    // Wirelogic
    CIRCUIT_FPRINTF(fout, "- WIREDLOGIC %s + MAXDIST %g ;\n",
                    // Wiredlogic dist is also store in fallMax
                    //              a->netName(), a->distance());
                    a->netName(), a->fallMax());
  }
  else {
    // Call the operand function
    isSumSet = 0;  // reset the global variable
    begOperand = 1;
    operand(c, a, 0);
    // Get the Rise and Fall
    if(a->hasRiseMax())
      CIRCUIT_FPRINTF(fout, "+ RISEMAX %g ", a->riseMax());
    if(a->hasFallMax())
      CIRCUIT_FPRINTF(fout, "+ FALLMAX %g ", a->fallMax());
    if(a->hasRiseMin())
      CIRCUIT_FPRINTF(fout, "+ RISEMIN %g ", a->riseMin());
    if(a->hasFallMin())
      CIRCUIT_FPRINTF(fout, "+ FALLMIN %g ", a->fallMin());
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  --numObjs;
  if(numObjs <= 0) {
    if(c == defrConstraintCbkType) {
      CIRCUIT_FPRINTF(fout, "END CONSTRAINTS\n");
    }
    else {
      CIRCUIT_FPRINTF(fout, "END ASSERTIONS\n");
    }
  }
  return 0;
}

int propstart(defrCallbackType_e c, void*, defiUserData) {
  checkType(c);
  CIRCUIT_FPRINTF(fout, "\nPROPERTYDEFINITIONS\n");
  isProp = 1;

  return 0;
}

int prop(defrCallbackType_e c, defiProp* p, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  defPropStorPtr->push_back(*p);
  if(strcmp(p->propType(), "design") == 0) {
    CIRCUIT_FPRINTF(fout, "DESIGN %s ", p->propName());
  }
  else if(strcmp(p->propType(), "net") == 0) {
    CIRCUIT_FPRINTF(fout, "NET %s ", p->propName());
  }
  else if(strcmp(p->propType(), "component") == 0) {
    CIRCUIT_FPRINTF(fout, "COMPONENT %s ", p->propName());
  }
  else if(strcmp(p->propType(), "specialnet") == 0) {
    CIRCUIT_FPRINTF(fout, "SPECIALNET %s ", p->propName());
  }
  else if(strcmp(p->propType(), "group") == 0) {
    CIRCUIT_FPRINTF(fout, "GROUP %s ", p->propName());
  }
  else if(strcmp(p->propType(), "row") == 0) {
    CIRCUIT_FPRINTF(fout, "ROW %s ", p->propName());
  }
  else if(strcmp(p->propType(), "componentpin") == 0) {
    CIRCUIT_FPRINTF(fout, "COMPONENTPIN %s ", p->propName());
  }
  else if(strcmp(p->propType(), "region") == 0) {
    CIRCUIT_FPRINTF(fout, "REGION %s ", p->propName());
  }
  else if(strcmp(p->propType(), "nondefaultrule") == 0) {
    CIRCUIT_FPRINTF(fout, "NONDEFAULTRULE %s ", p->propName());
  }

  if(p->dataType() == 'I')
    CIRCUIT_FPRINTF(fout, "INTEGER ");
  if(p->dataType() == 'R')
    CIRCUIT_FPRINTF(fout, "REAL ");
  if(p->dataType() == 'S')
    CIRCUIT_FPRINTF(fout, "STRING ");
  if(p->dataType() == 'Q')
    CIRCUIT_FPRINTF(fout, "STRING ");
  if(p->hasRange()) {
    CIRCUIT_FPRINTF(fout, "RANGE %g %g ", p->left(), p->right());
  }
  if(p->hasNumber())
    CIRCUIT_FPRINTF(fout, "%g ", p->number());
  if(p->hasString())
    CIRCUIT_FPRINTF(fout, "\"%s\" ", p->string());
  CIRCUIT_FPRINTF(fout, ";\n");

  return 0;
}

int propend(defrCallbackType_e c, void*, defiUserData) {
  checkType(c);
  if(isProp) {
    CIRCUIT_FPRINTF(fout, "END PROPERTYDEFINITIONS\n\n");
    isProp = 0;
  }

  return 0;
}

int hist(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  defrSetCaseSensitivity(0);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "HISTORY %s ;\n", h);
  defrSetCaseSensitivity(1);
  return 0;
}

int an(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "ARRAY %s ;\n", h);
  return 0;
}

int fn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "FLOORPLAN %s ;\n", h);
  return 0;
}

int bbn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defBusBitCharPtr) = string(h);
  isBusBitCharVisit = true;

  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n", h);
  return 0;
}

int vers(defrCallbackType_e c, double d, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "VERSION %g ;\n", d);
  curVer = d;

  CIRCUIT_FPRINTF(fout, "ALIAS alias1 aliasValue1 1 ;\n");
  CIRCUIT_FPRINTF(fout, "ALIAS alias2 aliasValue2 0 ;\n");

  return 0;
}

int versStr(defrCallbackType_e c, const char* versionName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defVersionPtr) = string(versionName);
  isVersionVisit = true;

  CIRCUIT_FPRINTF(fout, "VERSION %s ;\n", versionName);
  return 0;
}

int units(defrCallbackType_e c, double d, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  *defUnitPtr = d;
  CIRCUIT_FPRINTF(fout, "UNITS DISTANCE MICRONS %g ;\n", d);
  return 0;
}

int casesens(defrCallbackType_e c, int d, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  if(d == 1) {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE ON ;\n", d);
  }
  else {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE OFF ;\n", d);
  }
  return 0;
}

static int cls(defrCallbackType_e c, void* cl, defiUserData ud) {
  defiSite* site;  // Site and Canplace and CannotOccupy
  defiBox* box;    // DieArea and
  defiPinCap* pc;
  defiPin* pin;
  int i, j, k;
  defiRow* row;
  defiTrack* track;
  defiGcellGrid* gcg;
  defiVia* via;
  defiRegion* re;
  defiGroup* group;
  defiComponentMaskShiftLayer* maskShiftLayer = NULL;
  defiScanchain* sc;
  defiIOTiming* iot;
  defiFPC* fpc;
  defiTimingDisable* td;
  defiPartition* part;
  defiPinProp* pprop;
  defiBlockage* block;
  defiSlot* slots;
  defiFill* fills;
  defiStyles* styles;
  int xl, yl, xh, yh;
  char *name, *a1, *b1;
  char **inst, **inPin, **outPin;
  int* bits;
  int size;
  int corner, typ;
  const char* itemT;
  char dir;
  defiPinAntennaModel* aModel;
  struct defiPoints points;

  checkType(c);
  if(ud != userData)
    dataError();
  switch(c) {
    case defrSiteCbkType:
      site = (defiSite*)cl;
      CIRCUIT_FPRINTF(fout, "SITE %s %g %g %s ", site->name(), site->x_orig(),
                      site->y_orig(), orientStr(site->orient()));
      CIRCUIT_FPRINTF(fout, "DO %g BY %g STEP %g %g ;\n", site->x_num(),
                      site->y_num(), site->x_step(), site->y_step());
      break;
    case defrCanplaceCbkType:
      site = (defiSite*)cl;
      CIRCUIT_FPRINTF(fout, "CANPLACE %s %g %g %s ", site->name(),
                      site->x_orig(), site->y_orig(),
                      orientStr(site->orient()));
      CIRCUIT_FPRINTF(fout, "DO %g BY %g STEP %g %g ;\n", site->x_num(),
                      site->y_num(), site->x_step(), site->y_step());
      break;
    case defrCannotOccupyCbkType:
      site = (defiSite*)cl;
      CIRCUIT_FPRINTF(fout, "CANNOTOCCUPY %s %g %g %s ", site->name(),
                      site->x_orig(), site->y_orig(),
                      orientStr(site->orient()));
      CIRCUIT_FPRINTF(fout, "DO %g BY %g STEP %g %g ;\n", site->x_num(),
                      site->y_num(), site->x_step(), site->y_step());
      break;
    case defrDieAreaCbkType:
      box = (defiBox*)cl;
      *defDieAreaPtr = *box;
      CIRCUIT_FPRINTF(fout, "DIEAREA %d %d %d %d ;\n", box->xl(), box->yl(),
                      box->xh(), box->yh());
      CIRCUIT_FPRINTF(fout, "DIEAREA ");
      points = box->getPoint();
      for(i = 0; i < points.numPoints; i++)
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[i], points.y[i]);
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrPinCapCbkType:
      pc = (defiPinCap*)cl;
      if(testDebugPrint) {
        pc->print(fout);
      }
      else {
        CIRCUIT_FPRINTF(fout, "MINPINS %d WIRECAP %g ;\n", pc->pin(),
                        pc->cap());
        --numObjs;
        if(numObjs <= 0)
          CIRCUIT_FPRINTF(fout, "END DEFAULTCAP\n");
      }
      break;
    case defrPinCbkType:
      pin = (defiPin*)cl;

      // update pin data
      (*defPinMapPtr)[string(pin->pinName())] = defPinStorPtr->size();
      defPinStorPtr->push_back(*pin);

      if(testDebugPrint) {
        pin->print(fout);
      }
      else {
        CIRCUIT_FPRINTF(fout, "- %s + NET %s ", pin->pinName(), pin->netName());
        //         pin->changePinName("pinName");
        //         CIRCUIT_FPRINTF(fout, "%s ", pin->pinName());
        if(pin->hasDirection())
          CIRCUIT_FPRINTF(fout, "+ DIRECTION %s ", pin->direction());
        if(pin->hasUse())
          CIRCUIT_FPRINTF(fout, "+ USE %s ", pin->use());
        if(pin->hasNetExpr())
          CIRCUIT_FPRINTF(fout, "+ NETEXPR \"%s\" ", pin->netExpr());
        if(pin->hasSupplySensitivity())
          CIRCUIT_FPRINTF(fout, "+ SUPPLYSENSITIVITY %s ",
                          pin->supplySensitivity());
        if(pin->hasGroundSensitivity())
          CIRCUIT_FPRINTF(fout, "+ GROUNDSENSITIVITY %s ",
                          pin->groundSensitivity());
        if(pin->hasLayer()) {
          struct defiPoints points;
          for(i = 0; i < pin->numLayer(); i++) {
            CIRCUIT_FPRINTF(fout, "\n  + LAYER %s ", pin->layer(i));
            if(pin->layerMask(i))
              CIRCUIT_FPRINTF(fout, "MASK %d ", pin->layerMask(i));
            if(pin->hasLayerSpacing(i))
              CIRCUIT_FPRINTF(fout, "SPACING %d ", pin->layerSpacing(i));
            if(pin->hasLayerDesignRuleWidth(i))
              CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                              pin->layerDesignRuleWidth(i));
            pin->bounds(i, &xl, &yl, &xh, &yh);
            CIRCUIT_FPRINTF(fout, "%d %d %d %d ", xl, yl, xh, yh);
          }
          for(i = 0; i < pin->numPolygons(); i++) {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", pin->polygonName(i));
            if(pin->polygonMask(i))
              CIRCUIT_FPRINTF(fout, "MASK %d ", pin->polygonMask(i));
            if(pin->hasPolygonSpacing(i))
              CIRCUIT_FPRINTF(fout, "SPACING %d ", pin->polygonSpacing(i));
            if(pin->hasPolygonDesignRuleWidth(i))
              CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                              pin->polygonDesignRuleWidth(i));
            points = pin->getPolygon(i);
            for(j = 0; j < points.numPoints; j++)
              CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
          }
          for(i = 0; i < pin->numVias(); i++) {
            if(pin->viaTopMask(i) || pin->viaCutMask(i) ||
               pin->viaBottomMask(i)) {
              CIRCUIT_FPRINTF(fout, "\n  + VIA %s MASK %d%d%d %d %d ",
                              pin->viaName(i), pin->viaTopMask(i),
                              pin->viaCutMask(i), pin->viaBottomMask(i),
                              pin->viaPtX(i), pin->viaPtY(i));
            }
            else {
              CIRCUIT_FPRINTF(fout, "\n  + VIA %s %d %d ", pin->viaName(i),
                              pin->viaPtX(i), pin->viaPtY(i));
            }
          }
        }
        if(pin->hasPort()) {
          struct defiPoints points;
          defiPinPort* port;
          for(j = 0; j < pin->numPorts(); j++) {
            port = pin->pinPort(j);
            CIRCUIT_FPRINTF(fout, "\n  + PORT");
            for(i = 0; i < port->numLayer(); i++) {
              CIRCUIT_FPRINTF(fout, "\n     + LAYER %s ", port->layer(i));
              if(port->layerMask(i))
                CIRCUIT_FPRINTF(fout, "MASK %d ", port->layerMask(i));
              if(port->hasLayerSpacing(i))
                CIRCUIT_FPRINTF(fout, "SPACING %d ", port->layerSpacing(i));
              if(port->hasLayerDesignRuleWidth(i))
                CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                                port->layerDesignRuleWidth(i));
              port->bounds(i, &xl, &yl, &xh, &yh);
              CIRCUIT_FPRINTF(fout, "%d %d %d %d ", xl, yl, xh, yh);
            }
            for(i = 0; i < port->numPolygons(); i++) {
              CIRCUIT_FPRINTF(fout, "\n     + POLYGON %s ",
                              port->polygonName(i));
              if(port->polygonMask(i))
                CIRCUIT_FPRINTF(fout, "MASK %d ", port->polygonMask(i));
              if(port->hasPolygonSpacing(i))
                CIRCUIT_FPRINTF(fout, "SPACING %d ", port->polygonSpacing(i));
              if(port->hasPolygonDesignRuleWidth(i))
                CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                                port->polygonDesignRuleWidth(i));
              points = port->getPolygon(i);
              for(k = 0; k < points.numPoints; k++)
                CIRCUIT_FPRINTF(fout, "( %d %d ) ", points.x[k], points.y[k]);
            }
            for(i = 0; i < port->numVias(); i++) {
              if(port->viaTopMask(i) || port->viaCutMask(i) ||
                 port->viaBottomMask(i)) {
                CIRCUIT_FPRINTF(fout, "\n     + VIA %s MASK %d%d%d ( %d %d ) ",
                                port->viaName(i), port->viaTopMask(i),
                                port->viaCutMask(i), port->viaBottomMask(i),
                                port->viaPtX(i), port->viaPtY(i));
              }
              else {
                CIRCUIT_FPRINTF(fout, "\n     + VIA %s ( %d %d ) ",
                                port->viaName(i), port->viaPtX(i),
                                port->viaPtY(i));
              }
            }
            if(port->hasPlacement()) {
              if(port->isPlaced()) {
                CIRCUIT_FPRINTF(fout, "\n     + PLACED ");
                CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                                port->placementY(), orientStr(port->orient()));
              }
              if(port->isCover()) {
                CIRCUIT_FPRINTF(fout, "\n     + COVER ");
                CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                                port->placementY(), orientStr(port->orient()));
              }
              if(port->isFixed()) {
                CIRCUIT_FPRINTF(fout, "\n     + FIXED ");
                CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                                port->placementY(), orientStr(port->orient()));
              }
            }
          }
        }
        if(pin->hasPlacement()) {
          if(pin->isPlaced()) {
            CIRCUIT_FPRINTF(fout, "+ PLACED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", pin->placementX(),
                            pin->placementY(), orientStr(pin->orient()));
          }
          if(pin->isCover()) {
            CIRCUIT_FPRINTF(fout, "+ COVER ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", pin->placementX(),
                            pin->placementY(), orientStr(pin->orient()));
          }
          if(pin->isFixed()) {
            CIRCUIT_FPRINTF(fout, "+ FIXED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", pin->placementX(),
                            pin->placementY(), orientStr(pin->orient()));
          }
          if(pin->isUnplaced())
            CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
        }
        if(pin->hasSpecial()) {
          CIRCUIT_FPRINTF(fout, "+ SPECIAL ");
        }
        if(pin->hasAPinPartialMetalArea()) {
          for(i = 0; i < pin->numAPinPartialMetalArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALAREA %d",
                            pin->APinPartialMetalArea(i));
            if(*(pin->APinPartialMetalAreaLayer(i)))
              CIRCUIT_FPRINTF(fout, " LAYER %s",
                              pin->APinPartialMetalAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
        if(pin->hasAPinPartialMetalSideArea()) {
          for(i = 0; i < pin->numAPinPartialMetalSideArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALSIDEAREA %d",
                            pin->APinPartialMetalSideArea(i));
            if(*(pin->APinPartialMetalSideAreaLayer(i)))
              CIRCUIT_FPRINTF(fout, " LAYER %s",
                              pin->APinPartialMetalSideAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
        if(pin->hasAPinDiffArea()) {
          for(i = 0; i < pin->numAPinDiffArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINDIFFAREA %d",
                            pin->APinDiffArea(i));
            if(*(pin->APinDiffAreaLayer(i)))
              CIRCUIT_FPRINTF(fout, " LAYER %s", pin->APinDiffAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
        if(pin->hasAPinPartialCutArea()) {
          for(i = 0; i < pin->numAPinPartialCutArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALCUTAREA %d",
                            pin->APinPartialCutArea(i));
            if(*(pin->APinPartialCutAreaLayer(i)))
              CIRCUIT_FPRINTF(fout, " LAYER %s",
                              pin->APinPartialCutAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }

        for(j = 0; j < pin->numAntennaModel(); j++) {
          aModel = pin->antennaModel(j);

          CIRCUIT_FPRINTF(fout, "ANTENNAMODEL %s\n", aModel->antennaOxide());

          if(aModel->hasAPinGateArea()) {
            for(i = 0; i < aModel->numAPinGateArea(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINGATEAREA %d",
                              aModel->APinGateArea(i));
              if(aModel->hasAPinGateAreaLayer(i))
                CIRCUIT_FPRINTF(fout, " LAYER %s",
                                aModel->APinGateAreaLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
          if(aModel->hasAPinMaxAreaCar()) {
            for(i = 0; i < aModel->numAPinMaxAreaCar(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXAREACAR %d",
                              aModel->APinMaxAreaCar(i));
              if(aModel->hasAPinMaxAreaCarLayer(i))
                CIRCUIT_FPRINTF(fout, " LAYER %s",
                                aModel->APinMaxAreaCarLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
          if(aModel->hasAPinMaxSideAreaCar()) {
            for(i = 0; i < aModel->numAPinMaxSideAreaCar(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXSIDEAREACAR %d",
                              aModel->APinMaxSideAreaCar(i));
              if(aModel->hasAPinMaxSideAreaCarLayer(i))
                CIRCUIT_FPRINTF(fout, " LAYER %s",
                                aModel->APinMaxSideAreaCarLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
          if(aModel->hasAPinMaxCutCar()) {
            for(i = 0; i < aModel->numAPinMaxCutCar(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXCUTCAR %d",
                              aModel->APinMaxCutCar(i));
              if(aModel->hasAPinMaxCutCarLayer(i))
                CIRCUIT_FPRINTF(fout, " LAYER %s",
                                aModel->APinMaxCutCarLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
        --numObjs;
        if(numObjs <= 0)
          CIRCUIT_FPRINTF(fout, "END PINS\n");
      }
      break;
    case defrDefaultCapCbkType:
      i = (long)cl;
      CIRCUIT_FPRINTF(fout, "DEFAULTCAP %d\n", i);
      numObjs = i;
      break;
    case defrRowCbkType:
      row = (defiRow*)cl;
      defRowStorPtr->push_back(*row);
      (*defRowY2OrientMapPtr)[row->y()] = row->orient();

      CIRCUIT_FPRINTF(fout, "ROW %s %s %g %.0f %s ",
                      ignoreRowNames ? "XXX" : row->name(), row->macro(),
                      row->x(), row->y(), orientStr(row->orient()));

      if(row->hasDo()) {
        CIRCUIT_FPRINTF(fout, "DO %g BY %g ", row->xNum(), row->yNum());
        if(row->hasDoStep()) {
          CIRCUIT_FPRINTF(fout, "STEP %g %g ;\n", row->xStep(), row->yStep());
        }
        else {
          CIRCUIT_FPRINTF(fout, ";\n");
        }
      }
      else
        CIRCUIT_FPRINTF(fout, ";\n");
      if(row->numProps() > 0) {
        for(i = 0; i < row->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", row->propName(i),
                          row->propValue(i));
          switch(row->propType(i)) {
            case 'R':
              CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
              CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
              CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
              CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
              CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      break;
    case defrTrackCbkType:
      track = (defiTrack*)cl;
      defTrackStorPtr->push_back(*track);
      if(track->firstTrackMask()) {
        if(track->sameMask()) {
          CIRCUIT_FPRINTF(fout,
                          "TRACKS %s %g DO %g STEP %g MASK %d SAMEMASK LAYER ",
                          track->macro(), track->x(), track->xNum(),
                          track->xStep(), track->firstTrackMask());
        }
        else {
          CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g MASK %d LAYER ",
                          track->macro(), track->x(), track->xNum(),
                          track->xStep(), track->firstTrackMask());
        }
      }
      else {
        CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g LAYER ",
                        track->macro(), track->x(), track->xNum(),
                        track->xStep());
      }
      for(i = 0; i < track->numLayers(); i++)
        CIRCUIT_FPRINTF(fout, "%s ", track->layer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrGcellGridCbkType:
      gcg = (defiGcellGrid*)cl;
      defGcellGridStorPtr->push_back(*gcg);
      CIRCUIT_FPRINTF(fout, "GCELLGRID %s %d DO %d STEP %g ;\n", gcg->macro(),
                      gcg->x(), gcg->xNum(), gcg->xStep());
      break;
    case defrViaCbkType:
      via = (defiVia*)cl;
      if(testDebugPrint) {
        via->print(fout);
      }
      else {
        CIRCUIT_FPRINTF(fout, "- %s ", via->name());
        if(via->hasPattern())
          CIRCUIT_FPRINTF(fout, "+ PATTERNNAME %s ", via->pattern());
        for(i = 0; i < via->numLayers(); i++) {
          via->layer(i, &name, &xl, &yl, &xh, &yh);
          int rectMask = via->rectMask(i);

          if(rectMask) {
            CIRCUIT_FPRINTF(fout, "+ RECT %s + MASK %d %d %d %d %d \n", name,
                            rectMask, xl, yl, xh, yh);
          }
          else {
            CIRCUIT_FPRINTF(fout, "+ RECT %s %d %d %d %d \n", name, xl, yl, xh,
                            yh);
          }
        }
        // POLYGON
        if(via->numPolygons()) {
          struct defiPoints points;
          for(i = 0; i < via->numPolygons(); i++) {
            int polyMask = via->polyMask(i);

            if(polyMask) {
              CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s + MASK %d ",
                              via->polygonName(i), polyMask);
            }
            else {
              CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", via->polygonName(i));
            }
            points = via->getPolygon(i);
            for(j = 0; j < points.numPoints; j++)
              CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
          }
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
        if(via->hasViaRule()) {
          char *vrn, *bl, *cl, *tl;
          int xs, ys, xcs, ycs, xbe, ybe, xte, yte;
          int cr, cc, xo, yo, xbo, ybo, xto, yto;
          (void)via->viaRule(&vrn, &xs, &ys, &bl, &cl, &tl, &xcs, &ycs, &xbe,
                             &ybe, &xte, &yte);
          CIRCUIT_FPRINTF(fout, "+ VIARULE '%s'\n",
                          ignoreViaNames ? "XXX" : vrn);
          CIRCUIT_FPRINTF(fout, "  + CUTSIZE %d %d\n", xs, ys);
          CIRCUIT_FPRINTF(fout, "  + LAYERS %s %s %s\n", bl, cl, tl);
          CIRCUIT_FPRINTF(fout, "  + CUTSPACING %d %d\n", xcs, ycs);
          CIRCUIT_FPRINTF(fout, "  + ENCLOSURE %d %d %d %d\n", xbe, ybe, xte,
                          yte);
          if(via->hasRowCol()) {
            (void)via->rowCol(&cr, &cc);
            CIRCUIT_FPRINTF(fout, "  + ROWCOL %d %d\n", cr, cc);
          }
          if(via->hasOrigin()) {
            (void)via->origin(&xo, &yo);
            CIRCUIT_FPRINTF(fout, "  + ORIGIN %d %d\n", xo, yo);
          }
          if(via->hasOffset()) {
            (void)via->offset(&xbo, &ybo, &xto, &yto);
            CIRCUIT_FPRINTF(fout, "  + OFFSET %d %d %d %d\n", xbo, ybo, xto,
                            yto);
          }
          if(via->hasCutPattern())
            CIRCUIT_FPRINTF(fout, "  + PATTERN '%s'\n", via->cutPattern());
        }
        --numObjs;
        if(numObjs <= 0)
          CIRCUIT_FPRINTF(fout, "END VIAS\n");
      }
      defViaStorPtr->push_back(*via);
      break;
    case defrRegionCbkType:
      re = (defiRegion*)cl;
      CIRCUIT_FPRINTF(fout, "- %s ", re->name());
      for(i = 0; i < re->numRectangles(); i++)
        CIRCUIT_FPRINTF(fout, "%d %d %d %d \n", re->xl(i), re->yl(i), re->xh(i),
                        re->yh(i));
      if(re->hasType())
        CIRCUIT_FPRINTF(fout, "+ TYPE %s\n", re->type());
      if(re->numProps()) {
        for(i = 0; i < re->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "+ PROPERTY %s %s ", re->propName(i),
                          re->propValue(i));
          switch(re->propType(i)) {
            case 'R':
              CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
              CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
              CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
              CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
              CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0) {
        CIRCUIT_FPRINTF(fout, "END REGIONS\n");
      }
      break;
    case defrGroupNameCbkType:
      if((char*)cl) {
        CIRCUIT_FPRINTF(fout, "- %s", (char*)cl);
      }
      break;
    case defrGroupMemberCbkType:
      if((char*)cl) {
        CIRCUIT_FPRINTF(fout, " %s", (char*)cl);
      }
      break;
    case defrComponentMaskShiftLayerCbkType:
      CIRCUIT_FPRINTF(fout, "COMPONENTMASKSHIFT ");

      for(i = 0; i < maskShiftLayer->numMaskShiftLayers(); i++) {
        CIRCUIT_FPRINTF(fout, "%s ", maskShiftLayer->maskShiftLayer(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrGroupCbkType:
      group = (defiGroup*)cl;
      if(group->hasMaxX() | group->hasMaxY() | group->hasPerim()) {
        CIRCUIT_FPRINTF(fout, "\n  + SOFT ");
        if(group->hasPerim())
          CIRCUIT_FPRINTF(fout, "MAXHALFPERIMETER %d ", group->perim());
        if(group->hasMaxX())
          CIRCUIT_FPRINTF(fout, "MAXX %d ", group->maxX());
        if(group->hasMaxY())
          CIRCUIT_FPRINTF(fout, "MAXY %d ", group->maxY());
      }
      if(group->hasRegionName())
        CIRCUIT_FPRINTF(fout, "\n  + REGION %s ", group->regionName());
      if(group->hasRegionBox()) {
        int *gxl, *gyl, *gxh, *gyh;
        int size;
        group->regionRects(&size, &gxl, &gyl, &gxh, &gyh);
        for(i = 0; i < size; i++)
          CIRCUIT_FPRINTF(fout, "REGION %d %d %d %d ", gxl[i], gyl[i], gxh[i],
                          gyh[i]);
      }
      if(group->numProps()) {
        for(i = 0; i < group->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "\n  + PROPERTY %s %s ", group->propName(i),
                          group->propValue(i));
          switch(group->propType(i)) {
            case 'R':
              CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
              CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
              CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
              CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
              CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, " ;\n");
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END GROUPS\n");
      break;
    case defrScanchainCbkType:
      sc = (defiScanchain*)cl;
      CIRCUIT_FPRINTF(fout, "- %s\n", sc->name());
      if(sc->hasStart()) {
        sc->start(&a1, &b1);
        CIRCUIT_FPRINTF(fout, "  + START %s %s\n", a1, b1);
      }
      if(sc->hasStop()) {
        sc->stop(&a1, &b1);
        CIRCUIT_FPRINTF(fout, "  + STOP %s %s\n", a1, b1);
      }
      if(sc->hasCommonInPin() || sc->hasCommonOutPin()) {
        CIRCUIT_FPRINTF(fout, "  + COMMONSCANPINS ");
        if(sc->hasCommonInPin())
          CIRCUIT_FPRINTF(fout, " ( IN %s ) ", sc->commonInPin());
        if(sc->hasCommonOutPin())
          CIRCUIT_FPRINTF(fout, " ( OUT %s ) ", sc->commonOutPin());
        CIRCUIT_FPRINTF(fout, "\n");
      }
      if(sc->hasFloating()) {
        sc->floating(&size, &inst, &inPin, &outPin, &bits);
        if(size > 0)
          CIRCUIT_FPRINTF(fout, "  + FLOATING\n");
        for(i = 0; i < size; i++) {
          CIRCUIT_FPRINTF(fout, "    %s ", inst[i]);
          if(inPin[i])
            CIRCUIT_FPRINTF(fout, "( IN %s ) ", inPin[i]);
          if(outPin[i])
            CIRCUIT_FPRINTF(fout, "( OUT %s ) ", outPin[i]);
          if(bits[i] != -1)
            CIRCUIT_FPRINTF(fout, "( BITS %d ) ", bits[i]);
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }

      if(sc->hasOrdered()) {
        for(i = 0; i < sc->numOrderedLists(); i++) {
          sc->ordered(i, &size, &inst, &inPin, &outPin, &bits);
          if(size > 0)
            CIRCUIT_FPRINTF(fout, "  + ORDERED\n");
          for(j = 0; j < size; j++) {
            CIRCUIT_FPRINTF(fout, "    %s ", inst[j]);
            if(inPin[j])
              CIRCUIT_FPRINTF(fout, "( IN %s ) ", inPin[j]);
            if(outPin[j])
              CIRCUIT_FPRINTF(fout, "( OUT %s ) ", outPin[j]);
            if(bits[j] != -1)
              CIRCUIT_FPRINTF(fout, "( BITS %d ) ", bits[j]);
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
      }

      if(sc->hasPartition()) {
        CIRCUIT_FPRINTF(fout, "  + PARTITION %s ", sc->partitionName());
        if(sc->hasPartitionMaxBits())
          CIRCUIT_FPRINTF(fout, "MAXBITS %d ", sc->partitionMaxBits());
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END SCANCHAINS\n");
      break;
    case defrIOTimingCbkType:
      iot = (defiIOTiming*)cl;
      CIRCUIT_FPRINTF(fout, "- ( %s %s )\n", iot->inst(), iot->pin());
      if(iot->hasSlewRise())
        CIRCUIT_FPRINTF(fout, "  + RISE SLEWRATE %g %g\n", iot->slewRiseMin(),
                        iot->slewRiseMax());
      if(iot->hasSlewFall())
        CIRCUIT_FPRINTF(fout, "  + FALL SLEWRATE %g %g\n", iot->slewFallMin(),
                        iot->slewFallMax());
      if(iot->hasVariableRise())
        CIRCUIT_FPRINTF(fout, "  + RISE VARIABLE %g %g\n",
                        iot->variableRiseMin(), iot->variableRiseMax());
      if(iot->hasVariableFall())
        CIRCUIT_FPRINTF(fout, "  + FALL VARIABLE %g %g\n",
                        iot->variableFallMin(), iot->variableFallMax());
      if(iot->hasCapacitance())
        CIRCUIT_FPRINTF(fout, "  + CAPACITANCE %g\n", iot->capacitance());
      if(iot->hasDriveCell()) {
        CIRCUIT_FPRINTF(fout, "  + DRIVECELL %s ", iot->driveCell());
        if(iot->hasFrom())
          CIRCUIT_FPRINTF(fout, "  FROMPIN %s ", iot->from());
        if(iot->hasTo())
          CIRCUIT_FPRINTF(fout, "  TOPIN %s ", iot->to());
        if(iot->hasParallel())
          CIRCUIT_FPRINTF(fout, "PARALLEL %g", iot->parallel());
        CIRCUIT_FPRINTF(fout, "\n");
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END IOTIMINGS\n");
      break;
    case defrFPCCbkType:
      fpc = (defiFPC*)cl;
      CIRCUIT_FPRINTF(fout, "- %s ", fpc->name());
      if(fpc->isVertical())
        CIRCUIT_FPRINTF(fout, "VERTICAL ");
      if(fpc->isHorizontal())
        CIRCUIT_FPRINTF(fout, "HORIZONTAL ");
      if(fpc->hasAlign())
        CIRCUIT_FPRINTF(fout, "ALIGN ");
      if(fpc->hasMax())
        CIRCUIT_FPRINTF(fout, "%g ", fpc->alignMax());
      if(fpc->hasMin())
        CIRCUIT_FPRINTF(fout, "%g ", fpc->alignMin());
      if(fpc->hasEqual())
        CIRCUIT_FPRINTF(fout, "%g ", fpc->equal());
      for(i = 0; i < fpc->numParts(); i++) {
        fpc->getPart(i, &corner, &typ, &name);
        if(corner == 'B') {
          CIRCUIT_FPRINTF(fout, "BOTTOMLEFT ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "TOPRIGHT ");
        }
        if(typ == 'R') {
          CIRCUIT_FPRINTF(fout, "ROWS %s ", name);
        }
        else {
          CIRCUIT_FPRINTF(fout, "COMPS %s ", name);
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END FLOORPLANCONSTRAINTS\n");
      break;
    case defrTimingDisableCbkType:
      td = (defiTimingDisable*)cl;
      if(td->hasFromTo())
        CIRCUIT_FPRINTF(fout, "- FROMPIN %s %s ", td->fromInst(), td->fromPin(),
                        td->toInst(), td->toPin());
      if(td->hasThru())
        CIRCUIT_FPRINTF(fout, "- THRUPIN %s %s ", td->thruInst(),
                        td->thruPin());
      if(td->hasMacroFromTo())
        CIRCUIT_FPRINTF(fout, "- MACRO %s FROMPIN %s %s ", td->macroName(),
                        td->fromPin(), td->toPin());
      if(td->hasMacroThru())
        CIRCUIT_FPRINTF(fout, "- MACRO %s THRUPIN %s %s ", td->macroName(),
                        td->fromPin());
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrPartitionCbkType:
      part = (defiPartition*)cl;
      CIRCUIT_FPRINTF(fout, "- %s ", part->name());
      if(part->isSetupRise() | part->isSetupFall() | part->isHoldRise() |
         part->isHoldFall()) {
        // has turnoff
        CIRCUIT_FPRINTF(fout, "TURNOFF ");
        if(part->isSetupRise())
          CIRCUIT_FPRINTF(fout, "SETUPRISE ");
        if(part->isSetupFall())
          CIRCUIT_FPRINTF(fout, "SETUPFALL ");
        if(part->isHoldRise())
          CIRCUIT_FPRINTF(fout, "HOLDRISE ");
        if(part->isHoldFall())
          CIRCUIT_FPRINTF(fout, "HOLDFALL ");
      }
      itemT = part->itemType();
      dir = part->direction();
      if(strcmp(itemT, "CLOCK") == 0) {
        if(dir == 'T')  // toclockpin
          CIRCUIT_FPRINTF(fout, "+ TOCLOCKPIN %s %s ", part->instName(),
                          part->pinName());
        if(dir == 'F')  // fromclockpin
          CIRCUIT_FPRINTF(fout, "+ FROMCLOCKPIN %s %s ", part->instName(),
                          part->pinName());
        if(part->hasMin())
          CIRCUIT_FPRINTF(fout, "MIN %g %g ", part->partitionMin(),
                          part->partitionMax());
        if(part->hasMax())
          CIRCUIT_FPRINTF(fout, "MAX %g %g ", part->partitionMin(),
                          part->partitionMax());
        CIRCUIT_FPRINTF(fout, "PINS ");
        for(i = 0; i < part->numPins(); i++)
          CIRCUIT_FPRINTF(fout, "%s ", part->pin(i));
      }
      else if(strcmp(itemT, "IO") == 0) {
        if(dir == 'T')  // toiopin
          CIRCUIT_FPRINTF(fout, "+ TOIOPIN %s %s ", part->instName(),
                          part->pinName());
        if(dir == 'F')  // fromiopin
          CIRCUIT_FPRINTF(fout, "+ FROMIOPIN %s %s ", part->instName(),
                          part->pinName());
      }
      else if(strcmp(itemT, "COMP") == 0) {
        if(dir == 'T')  // tocomppin
          CIRCUIT_FPRINTF(fout, "+ TOCOMPPIN %s %s ", part->instName(),
                          part->pinName());
        if(dir == 'F')  // fromcomppin
          CIRCUIT_FPRINTF(fout, "+ FROMCOMPPIN %s %s ", part->instName(),
                          part->pinName());
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END PARTITIONS\n");
      break;

    case defrPinPropCbkType:
      pprop = (defiPinProp*)cl;
      if(pprop->isPin()) {
        CIRCUIT_FPRINTF(fout, "- PIN %s ", pprop->pinName());
      }
      else {
        CIRCUIT_FPRINTF(fout, "- %s %s ", pprop->instName(), pprop->pinName());
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      if(pprop->numProps() > 0) {
        for(i = 0; i < pprop->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", pprop->propName(i),
                          pprop->propValue(i));
          switch(pprop->propType(i)) {
            case 'R':
              CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
              CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
              CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
              CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
              CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END PINPROPERTIES\n");
      break;
    case defrBlockageCbkType:
      block = (defiBlockage*)cl;
      defBlockageStorPtr->push_back(*block);
      if(testDebugPrint) {
        block->print(fout);
      }
      else {
        if(block->hasLayer()) {
          CIRCUIT_FPRINTF(fout, "- LAYER %s\n", block->layerName());
          if(block->hasComponent())
            CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                            block->layerComponentName());
          if(block->hasSlots())
            CIRCUIT_FPRINTF(fout, "   + SLOTS\n");
          if(block->hasFills())
            CIRCUIT_FPRINTF(fout, "   + FILLS\n");
          if(block->hasPushdown())
            CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
          if(block->hasExceptpgnet())
            CIRCUIT_FPRINTF(fout, "   + EXCEPTPGNET\n");
          if(block->hasMask())
            CIRCUIT_FPRINTF(fout, "   + MASK %d\n", block->mask());
          if(block->hasSpacing())
            CIRCUIT_FPRINTF(fout, "   + SPACING %d\n", block->minSpacing());
          if(block->hasDesignRuleWidth())
            CIRCUIT_FPRINTF(fout, "   + DESIGNRULEWIDTH %d\n",
                            block->designRuleWidth());
        }
        else if(block->hasPlacement()) {
          CIRCUIT_FPRINTF(fout, "- PLACEMENT\n");
          if(block->hasSoft())
            CIRCUIT_FPRINTF(fout, "   + SOFT\n");
          if(block->hasPartial())
            CIRCUIT_FPRINTF(fout, "   + PARTIAL %g\n",
                            block->placementMaxDensity());
          if(block->hasComponent())
            CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                            block->placementComponentName());
          if(block->hasPushdown())
            CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
        }

        for(i = 0; i < block->numRectangles(); i++) {
          CIRCUIT_FPRINTF(fout, "   RECT %d %d %d %d\n", block->xl(i),
                          block->yl(i), block->xh(i), block->yh(i));
        }

        for(i = 0; i < block->numPolygons(); i++) {
          CIRCUIT_FPRINTF(fout, "   POLYGON ");
          points = block->getPolygon(i);
          for(j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
          CIRCUIT_FPRINTF(fout, "\n");
        }
        CIRCUIT_FPRINTF(fout, ";\n");
        --numObjs;
        if(numObjs <= 0)
          CIRCUIT_FPRINTF(fout, "END BLOCKAGES\n");
      }
      break;
    case defrSlotCbkType:
      slots = (defiSlot*)cl;
      if(slots->hasLayer())
        CIRCUIT_FPRINTF(fout, "- LAYER %s\n", slots->layerName());

      for(i = 0; i < slots->numRectangles(); i++) {
        CIRCUIT_FPRINTF(fout, "   RECT %d %d %d %d\n", slots->xl(i),
                        slots->yl(i), slots->xh(i), slots->yh(i));
      }
      for(i = 0; i < slots->numPolygons(); i++) {
        CIRCUIT_FPRINTF(fout, "   POLYGON ");
        points = slots->getPolygon(i);
        for(j = 0; j < points.numPoints; j++)
          CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END SLOTS\n");
      break;
    case defrFillCbkType:
      fills = (defiFill*)cl;
      if(testDebugPrint) {
        fills->print(fout);
      }
      else {
        if(fills->hasLayer()) {
          CIRCUIT_FPRINTF(fout, "- LAYER %s", fills->layerName());
          if(fills->layerMask()) {
            CIRCUIT_FPRINTF(fout, " + MASK %d", fills->layerMask());
          }
          if(fills->hasLayerOpc())
            CIRCUIT_FPRINTF(fout, " + OPC");
          CIRCUIT_FPRINTF(fout, "\n");

          for(i = 0; i < fills->numRectangles(); i++) {
            CIRCUIT_FPRINTF(fout, "   RECT %d %d %d %d\n", fills->xl(i),
                            fills->yl(i), fills->xh(i), fills->yh(i));
          }
          for(i = 0; i < fills->numPolygons(); i++) {
            CIRCUIT_FPRINTF(fout, "   POLYGON ");
            points = fills->getPolygon(i);
            for(j = 0; j < points.numPoints; j++)
              CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
            CIRCUIT_FPRINTF(fout, ";\n");
          }
          CIRCUIT_FPRINTF(fout, ";\n");
        }
        --numObjs;
        if(fills->hasVia()) {
          CIRCUIT_FPRINTF(fout, "- VIA %s", fills->viaName());
          if(fills->viaTopMask() || fills->viaCutMask() ||
             fills->viaBottomMask()) {
            CIRCUIT_FPRINTF(fout, " + MASK %d%d%d", fills->viaTopMask(),
                            fills->viaCutMask(), fills->viaBottomMask());
          }
          if(fills->hasViaOpc())
            CIRCUIT_FPRINTF(fout, " + OPC");
          CIRCUIT_FPRINTF(fout, "\n");

          for(i = 0; i < fills->numViaPts(); i++) {
            points = fills->getViaPts(i);
            for(j = 0; j < points.numPoints; j++)
              CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
            CIRCUIT_FPRINTF(fout, ";\n");
          }
          CIRCUIT_FPRINTF(fout, ";\n");
        }
        if(numObjs <= 0)
          CIRCUIT_FPRINTF(fout, "END FILLS\n");
      }
      break;
    case defrStylesCbkType:
      //            struct defiPoints points;
      styles = (defiStyles*)cl;
      CIRCUIT_FPRINTF(fout, "- STYLE %d ", styles->style());
      points = styles->getPolygon();
      for(j = 0; j < points.numPoints; j++)
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END STYLES\n");
      break;

    default:
      CIRCUIT_FPRINTF(fout, "BOGUS callback to cls.\n");
      return 1;
  }
  return 0;
}

int dn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defDividerCharPtr) = string(h);
  isDividerCharVisit = true;

  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n", h);
  return 0;
}

int ext(defrCallbackType_e t, const char* c, defiUserData ud) {
  char* name;

  checkType(t);
  if(ud != userData)
    dataError();

  switch(t) {
    case defrNetExtCbkType:
      name = address("net");
      break;
    case defrComponentExtCbkType:
      name = address("component");
      break;
    case defrPinExtCbkType:
      name = address("pin");
      break;
    case defrViaExtCbkType:
      name = address("via");
      break;
    case defrNetConnectionExtCbkType:
      name = address("net connection");
      break;
    case defrGroupExtCbkType:
      name = address("group");
      break;
    case defrScanChainExtCbkType:
      name = address("scanchain");
      break;
    case defrIoTimingsExtCbkType:
      name = address("io timing");
      break;
    case defrPartitionsExtCbkType:
      name = address("partition");
      break;
    default:
      name = address("BOGUS");
      return 1;
  }
  CIRCUIT_FPRINTF(fout, "  %s extension %s\n", name, c);
  return 0;
}

int extension(defrCallbackType_e c, const char* extsn, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "BEGINEXT %s\n", extsn);
  return 0;
}

void* mallocCB(size_t size) {
  return malloc(size);
}

void* reallocCB(void* name, size_t size) {
  return realloc(name, size);
}

static void freeCB(void* name) {
  free(name);
  return;
}

BEGIN_LEFDEF_PARSER_NAMESPACE
extern long long nlines;
END_LEFDEF_PARSER_NAMESPACE
static int ccr1131444 = 0;

void lineNumberCB(long long lineNo) {
  // The CCR 1131444 tests ability of the DEF parser to count
  // input line numbers out of 32-bit int range. On the first callback
  // call 10G lines will be added to line counter. It should be
  // reflected in output.
  if(ccr1131444) {
    lineNo += 10000000000LL;
    defrSetNLines(lineNo);
    ccr1131444 = 0;
  }
  cout << "[DEF] Parsed " << lineNo / 1000 << "k number of lines!!" << endl;
}

int unUsedCB(defrCallbackType_e, void*, defiUserData) {
  CIRCUIT_FPRINTF(fout, "This callback is not used.\n");
  return 0;
}

static void printWarning(const char* str) {
  CIRCUIT_FPRINTF(stderr, "%s\n", str);
}

///////////////////////////////////////////////////////////////////////
//
// Print function for DEF writer
//
///////////////////////////////////////////////////////////////////////

void Replace::Circuit::DumpDefVersion() {
  CIRCUIT_FPRINTF(fout, "VERSION %s ;\n",
                  (isVersionVisit) ? defVersion.c_str() : "5.8");
}

void Replace::Circuit::DumpDefDividerChar() {
  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n",
                  (isDividerCharVisit) ? defDividerChar.c_str() : "/");
}

void Replace::Circuit::DumpDefBusBitChar() {
  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n",
                  (isBusBitCharVisit) ? defBusBitChar.c_str() : "[]");
}

void Replace::Circuit::DumpDefDesignName() {
  CIRCUIT_FPRINTF(fout, "DESIGN %s ;\n",
                  (isDesignNameVisit) ? defDesignName.c_str() : "noname");
}

void Replace::Circuit::DumpDefUnit() {
  CIRCUIT_FPRINTF(fout, "UNITS DISTANCE MICRONS %g ;\n", defUnit);
}

void Replace::Circuit::DumpDefProp() {
  if(defPropStor.size() == 0) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "\nPROPERTYDEFINITIONS\n");
  for(auto& p : defPropStor) {
    if(strcmp(p.propType(), "design") == 0) {
      CIRCUIT_FPRINTF(fout, "    DESIGN %s ", p.propName());
    }
    else if(strcmp(p.propType(), "net") == 0) {
      CIRCUIT_FPRINTF(fout, "    NET %s ", p.propName());
    }
    else if(strcmp(p.propType(), "component") == 0) {
      CIRCUIT_FPRINTF(fout, "    COMPONENT %s ", p.propName());
    }
    else if(strcmp(p.propType(), "specialnet") == 0) {
      CIRCUIT_FPRINTF(fout, "    SPECIALNET %s ", p.propName());
    }
    else if(strcmp(p.propType(), "group") == 0) {
      CIRCUIT_FPRINTF(fout, "    GROUP %s ", p.propName());
    }
    else if(strcmp(p.propType(), "row") == 0) {
      CIRCUIT_FPRINTF(fout, "    ROW %s ", p.propName());
    }
    else if(strcmp(p.propType(), "componentpin") == 0) {
      CIRCUIT_FPRINTF(fout, "    COMPONENTPIN %s ", p.propName());
    }
    else if(strcmp(p.propType(), "region") == 0) {
      CIRCUIT_FPRINTF(fout, "    REGION %s ", p.propName());
    }
    else if(strcmp(p.propType(), "nondefaultrule") == 0) {
      CIRCUIT_FPRINTF(fout, "    NONDEFAULTRULE %s ", p.propName());
    }

    if(p.dataType() == 'I')
      CIRCUIT_FPRINTF(fout, "INTEGER ");
    if(p.dataType() == 'R')
      CIRCUIT_FPRINTF(fout, "REAL ");
    if(p.dataType() == 'S')
      CIRCUIT_FPRINTF(fout, "STRING ");
    if(p.dataType() == 'Q')
      CIRCUIT_FPRINTF(fout, "STRING ");
    if(p.hasRange()) {
      CIRCUIT_FPRINTF(fout, "RANGE %g %g ", p.left(), p.right());
    }
    if(p.hasNumber())
      CIRCUIT_FPRINTF(fout, "%g ", p.number());
    if(p.hasString())
      CIRCUIT_FPRINTF(fout, "\"%s\" ", p.string());
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END PROPERTYDEFINITIONS\n\n");
}

void Replace::Circuit::DumpDefDieArea() {
  defiBox* box = &defDieArea;
  CIRCUIT_FPRINTF(fout, "DIEAREA ( %d %d ) ( %d %d ) ;\n\n", box->xl(),
                  box->yl(), box->xh(), box->yh());

  //    defiPoints points = defDieArea.getPoint();
  //    for (int i = 0; i < points.numPoints; i++)
  //        CIRCUIT_FPRINTF(fout, "( %d %d ) ", points.x[i], points.y[i]);
  //    CIRCUIT_FPRINTF(fout, ";\n");
}

void Replace::Circuit::DumpDefRow() {
  if(defRowStor.size() == 0) {
    return;
  }

  for(auto& curRow : defRowStor) {
    CIRCUIT_FPRINTF(fout, "ROW %s %s %.0f %.0f %s ",
                    ignoreRowNames ? "XXX" : curRow.name(), curRow.macro(),
                    curRow.x(), curRow.y(), orientStr(curRow.orient()));
    if(curRow.hasDo()) {
      CIRCUIT_FPRINTF(fout, "DO %g BY %g ", curRow.xNum(), curRow.yNum());
      if(curRow.hasDoStep()) {
        CIRCUIT_FPRINTF(fout, "STEP %g %g ;\n", curRow.xStep(), curRow.yStep());
      }
      else {
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    else
      CIRCUIT_FPRINTF(fout, ";\n");
    if(curRow.numProps() > 0) {
      for(int i = 0; i < curRow.numProps(); i++) {
        CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", curRow.propName(i),
                        curRow.propValue(i));
        switch(curRow.propType(i)) {
          case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
            break;
          case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
            break;
          case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
            break;
          case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
            break;
          case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
            break;
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  CIRCUIT_FPRINTF(fout, "\n");
}

void Replace::Circuit::DumpDefTrack() {
  if(defTrackStor.size() == 0) {
    return;
  }

  for(auto& curTrack : defTrackStor) {
    if(curTrack.firstTrackMask()) {
      if(curTrack.sameMask()) {
        CIRCUIT_FPRINTF(fout,
                        "TRACKS %s %g DO %g STEP %g MASK %d SAMEMASK LAYER ",
                        curTrack.macro(), curTrack.x(), curTrack.xNum(),
                        curTrack.xStep(), curTrack.firstTrackMask());
      }
      else {
        CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g MASK %d LAYER ",
                        curTrack.macro(), curTrack.x(), curTrack.xNum(),
                        curTrack.xStep(), curTrack.firstTrackMask());
      }
    }
    else {
      CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g LAYER ",
                      curTrack.macro(), curTrack.x(), curTrack.xNum(),
                      curTrack.xStep());
    }
    for(int i = 0; i < curTrack.numLayers(); i++)
      CIRCUIT_FPRINTF(fout, "%s ", curTrack.layer(i));
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "\n");
}

void Replace::Circuit::DumpDefGcellGrid() {
  if(defGcellGridStor.size() == 0) {
    return;
  }

  for(auto& curGcellGrid : defGcellGridStor) {
    CIRCUIT_FPRINTF(fout, "GCELLGRID %s %d DO %d STEP %g ;\n",
                    curGcellGrid.macro(), curGcellGrid.x(), curGcellGrid.xNum(),
                    curGcellGrid.xStep());
  }
  CIRCUIT_FPRINTF(fout, "\n");
}

void Replace::Circuit::DumpDefVia() {
  if(defViaStor.size() == 0) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "VIAS %d ; \n", defViaStor.size());
  for(auto& curVia : defViaStor) {
    if(testDebugPrint) {
      curVia.print(fout);
    }
    else {
      CIRCUIT_FPRINTF(fout, "- %s \n", curVia.name());
      if(curVia.hasPattern())
        CIRCUIT_FPRINTF(fout, "  + PATTERNNAME %s ", curVia.pattern());
      for(int i = 0; i < curVia.numLayers(); i++) {
        int xl, yl, xh, yh;
        char* name;

        curVia.layer(i, &name, &xl, &yl, &xh, &yh);
        int rectMask = curVia.rectMask(i);

        if(rectMask) {
          CIRCUIT_FPRINTF(fout, "  + RECT %s + MASK %d %d %d %d %d \n", name,
                          rectMask, xl, yl, xh, yh);
        }
        else {
          CIRCUIT_FPRINTF(fout, "  + RECT %s ( %d %d ) ( %d %d ) \n", name, xl,
                          yl, xh, yh);
        }
        //                free(name);
      }
      // POLYGON
      if(curVia.numPolygons()) {
        struct defiPoints points;
        for(int i = 0; i < curVia.numPolygons(); i++) {
          int polyMask = curVia.polyMask(i);

          if(polyMask) {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s + MASK %d ",
                            curVia.polygonName(i), polyMask);
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curVia.polygonName(i));
          }
          points = curVia.getPolygon(i);
          for(int j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        }
      }
      if(curVia.hasViaRule()) {
        char *vrn, *bl, *cl, *tl;
        int xs, ys, xcs, ycs, xbe, ybe, xte, yte;
        int cr, cc, xo, yo, xbo, ybo, xto, yto;
        (void)curVia.viaRule(&vrn, &xs, &ys, &bl, &cl, &tl, &xcs, &ycs, &xbe,
                             &ybe, &xte, &yte);

        CIRCUIT_FPRINTF(fout, "\n");
        CIRCUIT_FPRINTF(fout, "  + VIARULE '%s'\n",
                        ignoreViaNames ? "XXX" : vrn);
        CIRCUIT_FPRINTF(fout, "  + CUTSIZE %d %d\n", xs, ys);
        CIRCUIT_FPRINTF(fout, "  + LAYERS %s %s %s\n", bl, cl, tl);
        CIRCUIT_FPRINTF(fout, "  + CUTSPACING %d %d\n", xcs, ycs);
        CIRCUIT_FPRINTF(fout, "  + ENCLOSURE %d %d %d %d\n", xbe, ybe, xte,
                        yte);
        if(curVia.hasRowCol()) {
          (void)curVia.rowCol(&cr, &cc);
          CIRCUIT_FPRINTF(fout, "  + ROWCOL %d %d\n", cr, cc);
        }
        if(curVia.hasOrigin()) {
          (void)curVia.origin(&xo, &yo);
          CIRCUIT_FPRINTF(fout, "  + ORIGIN %d %d\n", xo, yo);
        }
        if(curVia.hasOffset()) {
          (void)curVia.offset(&xbo, &ybo, &xto, &yto);
          CIRCUIT_FPRINTF(fout, "  + OFFSET %d %d %d %d\n", xbo, ybo, xto, yto);
        }
        if(curVia.hasCutPattern())
          CIRCUIT_FPRINTF(fout, "  + PATTERN '%s'\n", curVia.cutPattern());
      }
    }
    CIRCUIT_FPRINTF(fout, " ;\n");
  }

  CIRCUIT_FPRINTF(fout, "END VIAS\n\n");
}

void Replace::Circuit::DumpDefComponentMaskShiftLayer() {
  if(!defComponentMaskShiftLayer.numMaskShiftLayers()) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "COMPONENTMASKSHIFT ");

  for(int i = 0; i < defComponentMaskShiftLayer.numMaskShiftLayers(); i++) {
    CIRCUIT_FPRINTF(fout, "%s ", defComponentMaskShiftLayer.maskShiftLayer(i));
  }
  CIRCUIT_FPRINTF(fout, ";\n\n");
}

void Replace::Circuit::DumpDefComponent() {
  if(defComponentStor.size() == 0) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "COMPONENTS %lu ;\n", defComponentStor.size());
  for(auto& curCo : defComponentStor) {
    if(testDebugPrint) {
      curCo.print(fout);
    }
    else {
      int i;

      //  missing GENERATE, FOREIGN
      CIRCUIT_FPRINTF(fout, "- %s %s ", curCo.id(), curCo.name());
      //    curCo.changeIdAndName("idName", "modelName");
      //    CIRCUIT_FPRINTF(fout, "%s %s ", curCo.id(),
      //            curCo.name());
      if(curCo.hasNets()) {
        for(i = 0; i < curCo.numNets(); i++)
          CIRCUIT_FPRINTF(fout, "%s ", curCo.net(i));
      }
      if(curCo.isFixed())
        CIRCUIT_FPRINTF(fout, "+ FIXED ( %d %d ) %s ", curCo.placementX(),
                        curCo.placementY(), orientStr(curCo.placementOrient()));
      if(curCo.isCover())
        CIRCUIT_FPRINTF(fout, "+ COVER ( %d %d ) %s ", curCo.placementX(),
                        curCo.placementY(), orientStr(curCo.placementOrient()));
      if(curCo.isPlaced())
        CIRCUIT_FPRINTF(fout, "+ PLACED ( %d %d ) %s ", curCo.placementX(),
                        curCo.placementY(), orientStr(curCo.placementOrient()));
      if(curCo.isUnplaced()) {
        CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
        if((curCo.placementX() != -1) || (curCo.placementY() != -1))
          CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curCo.placementX(),
                          curCo.placementY(),
                          orientStr(curCo.placementOrient()));
      }
      if(curCo.hasSource())
        CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", curCo.source());
      if(curCo.hasGenerate()) {
        CIRCUIT_FPRINTF(fout, "+ GENERATE %s ", curCo.generateName());
        if(curCo.macroName() && *(curCo.macroName()))
          CIRCUIT_FPRINTF(fout, "%s ", curCo.macroName());
      }
      if(curCo.hasWeight())
        CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", curCo.weight());
      if(curCo.hasEEQ())
        CIRCUIT_FPRINTF(fout, "+ EEQMASTER %s ", curCo.EEQ());
      if(curCo.hasRegionName())
        CIRCUIT_FPRINTF(fout, "+ REGION %s ", curCo.regionName());
      if(curCo.hasRegionBounds()) {
        int *xl, *yl, *xh, *yh;
        int size;
        curCo.regionBounds(&size, &xl, &yl, &xh, &yh);
        for(i = 0; i < size; i++) {
          CIRCUIT_FPRINTF(fout, "+ REGION %d %d %d %d \n", xl[i], yl[i], xh[i],
                          yh[i]);
        }
      }
      if(curCo.maskShiftSize()) {
        CIRCUIT_FPRINTF(fout, "+ MASKSHIFT ");

        for(int i = curCo.maskShiftSize() - 1; i >= 0; i--) {
          CIRCUIT_FPRINTF(fout, "%d", curCo.maskShift(i));
        }
        CIRCUIT_FPRINTF(fout, "\n");
      }
      if(curCo.hasHalo()) {
        int left, bottom, right, top;
        (void)curCo.haloEdges(&left, &bottom, &right, &top);
        CIRCUIT_FPRINTF(fout, "+ HALO ");
        if(curCo.hasHaloSoft())
          CIRCUIT_FPRINTF(fout, "SOFT ");
        CIRCUIT_FPRINTF(fout, "%d %d %d %d\n", left, bottom, right, top);
      }
      if(curCo.hasRouteHalo()) {
        CIRCUIT_FPRINTF(fout, "+ ROUTEHALO %d %s %s\n", curCo.haloDist(),
                        curCo.minLayer(), curCo.maxLayer());
      }
      if(curCo.hasForeignName()) {
        CIRCUIT_FPRINTF(fout, "+ FOREIGN %s %d %d %s %d ", curCo.foreignName(),
                        curCo.foreignX(), curCo.foreignY(), curCo.foreignOri(),
                        curCo.foreignOrient());
      }
      if(curCo.numProps()) {
        for(i = 0; i < curCo.numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "+ PROPERTY %s %s ", curCo.propName(i),
                          curCo.propValue(i));
          switch(curCo.propType(i)) {
            case 'R':
              CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
              CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
              CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
              CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
              CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  CIRCUIT_FPRINTF(fout, "END COMPONENTS\n\n");
}

void Replace::Circuit::DumpDefBlockage() {
  if(defBlockageStor.size() == 0) {
    return;
  }
  int i = 0, j = 0;
  defiPoints points;

  CIRCUIT_FPRINTF(fout, "BLOCKAGES %lu ;\n", defBlockageStor.size());

  for(auto& curBlockage : defBlockageStor) {
    if(curBlockage.hasLayer()) {
      CIRCUIT_FPRINTF(fout, "- LAYER %s\n", curBlockage.layerName());
      if(curBlockage.hasComponent())
        CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                        curBlockage.layerComponentName());
      if(curBlockage.hasSlots())
        CIRCUIT_FPRINTF(fout, "   + SLOTS\n");
      if(curBlockage.hasFills())
        CIRCUIT_FPRINTF(fout, "   + FILLS\n");
      if(curBlockage.hasPushdown())
        CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
      if(curBlockage.hasExceptpgnet())
        CIRCUIT_FPRINTF(fout, "   + EXCEPTPGNET\n");
      if(curBlockage.hasMask())
        CIRCUIT_FPRINTF(fout, "   + MASK %d\n", curBlockage.mask());
      if(curBlockage.hasSpacing())
        CIRCUIT_FPRINTF(fout, "   + SPACING %d\n", curBlockage.minSpacing());
      if(curBlockage.hasDesignRuleWidth())
        CIRCUIT_FPRINTF(fout, "   + DESIGNRULEWIDTH %d\n",
                        curBlockage.designRuleWidth());
    }
    else if(curBlockage.hasPlacement()) {
      CIRCUIT_FPRINTF(fout, "- PLACEMENT\n");
      if(curBlockage.hasSoft())
        CIRCUIT_FPRINTF(fout, "   + SOFT\n");
      if(curBlockage.hasPartial())
        CIRCUIT_FPRINTF(fout, "   + PARTIAL %g\n",
                        curBlockage.placementMaxDensity());
      if(curBlockage.hasComponent())
        CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                        curBlockage.placementComponentName());
      if(curBlockage.hasPushdown())
        CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
    }

    for(i = 0; i < curBlockage.numRectangles(); i++) {
      CIRCUIT_FPRINTF(fout, "   RECT ( %d %d ) ( %d %d ) \n", curBlockage.xl(i),
                      curBlockage.yl(i), curBlockage.xh(i), curBlockage.yh(i));
    }

    for(i = 0; i < curBlockage.numPolygons(); i++) {
      CIRCUIT_FPRINTF(fout, "   POLYGON ");
      points = curBlockage.getPolygon(i);
      for(j = 0; j < points.numPoints; j++)
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      CIRCUIT_FPRINTF(fout, "\n");
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END BLOCKAGES\n\n");
}

void Replace::Circuit::DumpDefPin() {
  if(defPinStor.size() == 0) {
    return;
  }

  int i = 0, j = 0, k = 0;
  int xl = 0, yl = 0, xh = 0, yh = 0;

  CIRCUIT_FPRINTF(fout, "PINS %lu ;\n", defPinStor.size());
  for(auto& curPin : defPinStor) {
    CIRCUIT_FPRINTF(fout, "- %s + NET %s ", curPin.pinName(), curPin.netName());
    //         curPin.changePinName("pinName");
    //         CIRCUIT_FPRINTF(fout, "%s ", curPin.pinName());
    if(curPin.hasDirection())
      CIRCUIT_FPRINTF(fout, "+ DIRECTION %s ", curPin.direction());
    if(curPin.hasUse())
      CIRCUIT_FPRINTF(fout, "+ USE %s ", curPin.use());
    if(curPin.hasNetExpr())
      CIRCUIT_FPRINTF(fout, "+ NETEXPR \"%s\" ", curPin.netExpr());
    if(curPin.hasSupplySensitivity())
      CIRCUIT_FPRINTF(fout, "+ SUPPLYSENSITIVITY %s ",
                      curPin.supplySensitivity());
    if(curPin.hasGroundSensitivity())
      CIRCUIT_FPRINTF(fout, "+ GROUNDSENSITIVITY %s ",
                      curPin.groundSensitivity());
    if(curPin.hasLayer()) {
      struct defiPoints points;
      for(i = 0; i < curPin.numLayer(); i++) {
        CIRCUIT_FPRINTF(fout, "\n  + LAYER %s ", curPin.layer(i));
        if(curPin.layerMask(i))
          CIRCUIT_FPRINTF(fout, "MASK %d ", curPin.layerMask(i));
        if(curPin.hasLayerSpacing(i))
          CIRCUIT_FPRINTF(fout, "SPACING %d ", curPin.layerSpacing(i));
        if(curPin.hasLayerDesignRuleWidth(i))
          CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                          curPin.layerDesignRuleWidth(i));
        curPin.bounds(i, &xl, &yl, &xh, &yh);
        CIRCUIT_FPRINTF(fout, "( %d %d ) ( %d %d ) ", xl, yl, xh, yh);
      }
      for(i = 0; i < curPin.numPolygons(); i++) {
        CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curPin.polygonName(i));
        if(curPin.polygonMask(i))
          CIRCUIT_FPRINTF(fout, "MASK %d ", curPin.polygonMask(i));
        if(curPin.hasPolygonSpacing(i))
          CIRCUIT_FPRINTF(fout, "SPACING %d ", curPin.polygonSpacing(i));
        if(curPin.hasPolygonDesignRuleWidth(i))
          CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                          curPin.polygonDesignRuleWidth(i));
        points = curPin.getPolygon(i);
        for(j = 0; j < points.numPoints; j++)
          CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      }
      for(i = 0; i < curPin.numVias(); i++) {
        if(curPin.viaTopMask(i) || curPin.viaCutMask(i) ||
           curPin.viaBottomMask(i)) {
          CIRCUIT_FPRINTF(fout, "\n  + VIA %s MASK %d%d%d %d %d ",
                          curPin.viaName(i), curPin.viaTopMask(i),
                          curPin.viaCutMask(i), curPin.viaBottomMask(i),
                          curPin.viaPtX(i), curPin.viaPtY(i));
        }
        else {
          CIRCUIT_FPRINTF(fout, "\n  + VIA %s %d %d ", curPin.viaName(i),
                          curPin.viaPtX(i), curPin.viaPtY(i));
        }
      }
    }
    if(curPin.hasPort()) {
      struct defiPoints points;
      defiPinPort* port;
      for(j = 0; j < curPin.numPorts(); j++) {
        port = curPin.pinPort(j);
        CIRCUIT_FPRINTF(fout, "\n  + PORT");
        for(i = 0; i < port->numLayer(); i++) {
          CIRCUIT_FPRINTF(fout, "\n     + LAYER %s ", port->layer(i));
          if(port->layerMask(i))
            CIRCUIT_FPRINTF(fout, "MASK %d ", port->layerMask(i));
          if(port->hasLayerSpacing(i))
            CIRCUIT_FPRINTF(fout, "SPACING %d ", port->layerSpacing(i));
          if(port->hasLayerDesignRuleWidth(i))
            CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                            port->layerDesignRuleWidth(i));
          port->bounds(i, &xl, &yl, &xh, &yh);
          CIRCUIT_FPRINTF(fout, "( %d %d ) ( %d %d ) ", xl, yl, xh, yh);
        }
        for(i = 0; i < port->numPolygons(); i++) {
          CIRCUIT_FPRINTF(fout, "\n     + POLYGON %s ", port->polygonName(i));
          if(port->polygonMask(i))
            CIRCUIT_FPRINTF(fout, "MASK %d ", port->polygonMask(i));
          if(port->hasPolygonSpacing(i))
            CIRCUIT_FPRINTF(fout, "SPACING %d ", port->polygonSpacing(i));
          if(port->hasPolygonDesignRuleWidth(i))
            CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                            port->polygonDesignRuleWidth(i));
          points = port->getPolygon(i);
          for(k = 0; k < points.numPoints; k++)
            CIRCUIT_FPRINTF(fout, "( %d %d ) ", points.x[k], points.y[k]);
        }
        for(i = 0; i < port->numVias(); i++) {
          if(port->viaTopMask(i) || port->viaCutMask(i) ||
             port->viaBottomMask(i)) {
            CIRCUIT_FPRINTF(fout, "\n     + VIA %s MASK %d%d%d ( %d %d ) ",
                            port->viaName(i), port->viaTopMask(i),
                            port->viaCutMask(i), port->viaBottomMask(i),
                            port->viaPtX(i), port->viaPtY(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n     + VIA %s ( %d %d ) ",
                            port->viaName(i), port->viaPtX(i), port->viaPtY(i));
          }
        }
        if(port->hasPlacement()) {
          if(port->isPlaced()) {
            CIRCUIT_FPRINTF(fout, "\n     + PLACED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                            port->placementY(), orientStr(port->orient()));
          }
          if(port->isCover()) {
            CIRCUIT_FPRINTF(fout, "\n     + COVER ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                            port->placementY(), orientStr(port->orient()));
          }
          if(port->isFixed()) {
            CIRCUIT_FPRINTF(fout, "\n     + FIXED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                            port->placementY(), orientStr(port->orient()));
          }
        }
      }
    }
    if(curPin.hasPlacement()) {
      if(curPin.isPlaced()) {
        CIRCUIT_FPRINTF(fout, "+ PLACED ");
        CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curPin.placementX(),
                        curPin.placementY(), orientStr(curPin.orient()));
      }
      if(curPin.isCover()) {
        CIRCUIT_FPRINTF(fout, "+ COVER ");
        CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curPin.placementX(),
                        curPin.placementY(), orientStr(curPin.orient()));
      }
      if(curPin.isFixed()) {
        CIRCUIT_FPRINTF(fout, "+ FIXED ");
        CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curPin.placementX(),
                        curPin.placementY(), orientStr(curPin.orient()));
      }
      if(curPin.isUnplaced())
        CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
    }
    if(curPin.hasSpecial()) {
      CIRCUIT_FPRINTF(fout, "+ SPECIAL ");
    }
    if(curPin.hasAPinPartialMetalArea()) {
      for(i = 0; i < curPin.numAPinPartialMetalArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALAREA %d",
                        curPin.APinPartialMetalArea(i));
        if(*(curPin.APinPartialMetalAreaLayer(i)))
          CIRCUIT_FPRINTF(fout, " LAYER %s",
                          curPin.APinPartialMetalAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }
    if(curPin.hasAPinPartialMetalSideArea()) {
      for(i = 0; i < curPin.numAPinPartialMetalSideArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALSIDEAREA %d",
                        curPin.APinPartialMetalSideArea(i));
        if(*(curPin.APinPartialMetalSideAreaLayer(i)))
          CIRCUIT_FPRINTF(fout, " LAYER %s",
                          curPin.APinPartialMetalSideAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }
    if(curPin.hasAPinDiffArea()) {
      for(i = 0; i < curPin.numAPinDiffArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINDIFFAREA %d", curPin.APinDiffArea(i));
        if(*(curPin.APinDiffAreaLayer(i)))
          CIRCUIT_FPRINTF(fout, " LAYER %s", curPin.APinDiffAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }
    if(curPin.hasAPinPartialCutArea()) {
      for(i = 0; i < curPin.numAPinPartialCutArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALCUTAREA %d",
                        curPin.APinPartialCutArea(i));
        if(*(curPin.APinPartialCutAreaLayer(i)))
          CIRCUIT_FPRINTF(fout, " LAYER %s", curPin.APinPartialCutAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }

    for(j = 0; j < curPin.numAntennaModel(); j++) {
      defiPinAntennaModel* aModel;
      aModel = curPin.antennaModel(j);

      CIRCUIT_FPRINTF(fout, "ANTENNAMODEL %s\n", aModel->antennaOxide());

      if(aModel->hasAPinGateArea()) {
        for(i = 0; i < aModel->numAPinGateArea(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINGATEAREA %d",
                          aModel->APinGateArea(i));
          if(aModel->hasAPinGateAreaLayer(i))
            CIRCUIT_FPRINTF(fout, " LAYER %s", aModel->APinGateAreaLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
      if(aModel->hasAPinMaxAreaCar()) {
        for(i = 0; i < aModel->numAPinMaxAreaCar(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXAREACAR %d",
                          aModel->APinMaxAreaCar(i));
          if(aModel->hasAPinMaxAreaCarLayer(i))
            CIRCUIT_FPRINTF(fout, " LAYER %s", aModel->APinMaxAreaCarLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
      if(aModel->hasAPinMaxSideAreaCar()) {
        for(i = 0; i < aModel->numAPinMaxSideAreaCar(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXSIDEAREACAR %d",
                          aModel->APinMaxSideAreaCar(i));
          if(aModel->hasAPinMaxSideAreaCarLayer(i))
            CIRCUIT_FPRINTF(fout, " LAYER %s",
                            aModel->APinMaxSideAreaCarLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
      if(aModel->hasAPinMaxCutCar()) {
        for(i = 0; i < aModel->numAPinMaxCutCar(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXCUTCAR %d",
                          aModel->APinMaxCutCar(i));
          if(aModel->hasAPinMaxCutCarLayer(i))
            CIRCUIT_FPRINTF(fout, " LAYER %s", aModel->APinMaxCutCarLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END PINS\n\n");
}

void Replace::Circuit::DumpDefSpecialNet() {
  if(defSpecialNetStor.size() == 0) {
    return;
  }

  // Check SpecialNetCnt
  int sNetCnt = 0;
  for(auto& curNetType : defSpecialNetType) {
    if(curNetType == DEF_SPECIALNET_ORIGINAL) {
      sNetCnt++;
    }
  }
  CIRCUIT_FPRINTF(fout, "SPECIALNETS %d ;\n", sNetCnt);

  bool objectChange = true;
  for(auto& curNet : defSpecialNetStor) {
    int netType = defSpecialNetType[&curNet - &defSpecialNetStor[0]];

    if(netType == DEF_SPECIALNET_ORIGINAL) {
      // For net and special net.
      int i, j, x, y, z, count = 0, newLayer;
      char* layerName;
      double dist, left, right;
      defiPath* p;
      defiSubnet* s;
      int path;
      defiShield* shield;
      defiWire* wire;
      int numX, numY, stepX, stepY;

      // 5/6/2004 - don't need since I have a callback for the name
      if(objectChange) {
        CIRCUIT_FPRINTF(fout, "- %s ", curNet.name());

        count = 0;
        // compName & pinName
        for(i = 0; i < curNet.numConnections(); i++) {
          // set the limit of only 5 items print out in one line
          count++;
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i),
                          curNet.pin(i));
          if(curNet.pinIsSynthesized(i))
            CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
        }
      }

      // specialWiring
      if(curNet.numWires()) {
        for(i = 0; i < curNet.numWires(); i++) {
          newLayer = (objectChange) ? 0 : 1;
          wire = curNet.wire(i);
          if(objectChange) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
            if(strcmp(wire->wireType(), "SHIELD") == 0)
              CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
          }
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->initTraverse();
            if(testDebugPrint) {
              p->print(fout);
            }
            else {
              while((path = (int)p->next()) != DEFIPATH_DONE) {
                count++;
                // Don't want the line to be too long
                if(count >= 5) {
                  CIRCUIT_FPRINTF(fout, "\n");
                  count = 0;
                }
                switch(path) {
                  case DEFIPATH_LAYER:
                    if(newLayer == 0) {
                      CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                      newLayer = 1;
                    }
                    else
                      CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                    break;
                  case DEFIPATH_VIA:
                    CIRCUIT_FPRINTF(fout, "%s ",
                                    ignoreViaNames ? "XXX" : p->getVia());
                    break;
                  case DEFIPATH_VIAROTATION:
                    CIRCUIT_FPRINTF(fout, "%s ",
                                    orientStr(p->getViaRotation()));
                    break;
                  case DEFIPATH_VIADATA:
                    p->getViaData(&numX, &numY, &stepX, &stepY);
                    CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                    stepX, stepY);
                    break;
                  case DEFIPATH_WIDTH:
                    CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                    break;
                  case DEFIPATH_MASK:
                    CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                    break;
                  case DEFIPATH_VIAMASK:
                    CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                    p->getViaCutMask(), p->getViaBottomMask());
                    break;
                  case DEFIPATH_POINT:
                    p->getPoint(&x, &y);
                    CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                    break;
                  case DEFIPATH_FLUSHPOINT:
                    p->getFlushPoint(&x, &y, &z);
                    CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                    break;
                  case DEFIPATH_TAPER:
                    CIRCUIT_FPRINTF(fout, "TAPER ");
                    break;
                  case DEFIPATH_SHAPE:
                    CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                    break;
                  case DEFIPATH_STYLE:
                    CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                    break;
                }
              }
            }
          }
          CIRCUIT_FPRINTF(fout, "\n");
          count = 0;
        }
      }

      // POLYGON
      if(curNet.numPolygons()) {
        struct defiPoints points;

        for(i = 0; i < curNet.numPolygons(); i++) {
          if(curVer >= 5.8) {
            if(strcmp(curNet.polyRouteStatus(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + %s ", curNet.polyRouteStatus(i));
              if(strcmp(curNet.polyRouteStatus(i), "SHIELD") == 0) {
                CIRCUIT_FPRINTF(fout, "\n  + %s ",
                                curNet.polyRouteStatusShieldName(i));
              }
            }
            if(strcmp(curNet.polyShapeType(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", curNet.polyShapeType(i));
            }
          }
          if(curNet.polyMask(i)) {
            CIRCUIT_FPRINTF(fout, "\n  + MASK %d + POLYGON % s ",
                            curNet.polyMask(i), curNet.polygonName(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curNet.polygonName(i));
          }
          points = curNet.getPolygon(i);
          for(j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        }
      }
      // RECT
      if(curNet.numRectangles()) {
        for(i = 0; i < curNet.numRectangles(); i++) {
          if(curVer >= 5.8) {
            if(strcmp(curNet.rectRouteStatus(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + %s ", curNet.rectRouteStatus(i));
              if(strcmp(curNet.rectRouteStatus(i), "SHIELD") == 0) {
                CIRCUIT_FPRINTF(fout, "\n  + %s ",
                                curNet.rectRouteStatusShieldName(i));
              }
            }
            if(strcmp(curNet.rectShapeType(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", curNet.rectShapeType(i));
            }
          }
          if(curNet.rectMask(i)) {
            CIRCUIT_FPRINTF(fout, "\n  + MASK %d + RECT %s %d %d %d %d",
                            curNet.rectMask(i), curNet.rectName(i),
                            curNet.xl(i), curNet.yl(i), curNet.xh(i),
                            curNet.yh(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d",
                            curNet.rectName(i), curNet.xl(i), curNet.yl(i),
                            curNet.xh(i), curNet.yh(i));
          }
        }
      }
      // VIA
      if(curVer >= 5.8 && curNet.numViaSpecs()) {
        for(i = 0; i < curNet.numViaSpecs(); i++) {
          if(strcmp(curNet.viaRouteStatus(i), "") != 0) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ", curNet.viaRouteStatus(i));
            if(strcmp(curNet.viaRouteStatus(i), "SHIELD") == 0) {
              CIRCUIT_FPRINTF(fout, "\n  + %s ",
                              curNet.viaRouteStatusShieldName(i));
            }
          }
          if(strcmp(curNet.viaShapeType(i), "") != 0) {
            CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", curNet.viaShapeType(i));
          }
          if(curNet.topMaskNum(i) || curNet.cutMaskNum(i) ||
             curNet.bottomMaskNum(i)) {
            CIRCUIT_FPRINTF(fout, "\n  + MASK %d%d%d + VIA %s ",
                            curNet.topMaskNum(i), curNet.cutMaskNum(i),
                            curNet.bottomMaskNum(i), curNet.viaName(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + VIA %s ", curNet.viaName(i));
          }
          CIRCUIT_FPRINTF(fout, " %s", curNet.viaOrientStr(i));

          defiPoints points = curNet.getViaPts(i);

          for(int j = 0; j < points.numPoints; j++) {
            CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
          }
          CIRCUIT_FPRINTF(fout, ";\n");
        }
      }

      if(curNet.hasSubnets()) {
        for(i = 0; i < curNet.numSubnets(); i++) {
          s = curNet.subnet(i);
          if(s->numConnections()) {
            if(s->pinIsMustJoin(0)) {
              CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
            }
            else {
              CIRCUIT_FPRINTF(fout, "- %s ", s->name());
            }
            for(j = 0; j < s->numConnections(); j++) {
              CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
            }
          }

          // regularWiring
          if(s->numWires()) {
            for(i = 0; i < s->numWires(); i++) {
              wire = s->wire(i);
              CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
              for(j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->print(fout);
              }
            }
          }
        }
      }

      if(curNet.numProps()) {
        for(i = 0; i < curNet.numProps(); i++) {
          if(curNet.propIsString(i))
            CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", curNet.propName(i),
                            curNet.propValue(i));
          if(curNet.propIsNumber(i))
            CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", curNet.propName(i),
                            curNet.propNumber(i));
          switch(curNet.propType(i)) {
            case 'R':
              CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
              CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
              CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
              CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
              CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }

      // SHIELD
      count = 0;
      // testing the SHIELD for 5.3, obsolete in 5.4
      if(curNet.numShields()) {
        for(i = 0; i < curNet.numShields(); i++) {
          shield = curNet.shield(i);
          CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
          newLayer = 0;
          for(j = 0; j < shield->numPaths(); j++) {
            p = shield->path(j);
            p->initTraverse();
            while((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if(count >= 5) {
                CIRCUIT_FPRINTF(fout, "\n");
                count = 0;
              }
              switch(path) {
                case DEFIPATH_LAYER:
                  if(newLayer == 0) {
                    CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                    newLayer = 1;
                  }
                  else
                    CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                  break;
                case DEFIPATH_VIA:
                  CIRCUIT_FPRINTF(fout, "%s ",
                                  ignoreViaNames ? "XXX" : p->getVia());
                  break;
                case DEFIPATH_VIAROTATION:
                  CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                  break;
                case DEFIPATH_WIDTH:
                  CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                  break;
                case DEFIPATH_MASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                  p->getViaCutMask(), p->getViaBottomMask());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                  break;
                case DEFIPATH_FLUSHPOINT:
                  p->getFlushPoint(&x, &y, &z);
                  CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                  break;
                case DEFIPATH_TAPER:
                  CIRCUIT_FPRINTF(fout, "TAPER ");
                  break;
                case DEFIPATH_SHAPE:
                  CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                  break;
                case DEFIPATH_STYLE:
                  CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                  break;
              }
            }
          }
        }
      }

      // layerName width
      if(curNet.hasWidthRules()) {
        for(i = 0; i < curNet.numWidthRules(); i++) {
          curNet.widthRule(i, &layerName, &dist);
          CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
        }
      }

      // layerName spacing
      if(curNet.hasSpacingRules()) {
        for(i = 0; i < curNet.numSpacingRules(); i++) {
          curNet.spacingRule(i, &layerName, &dist, &left, &right);
          if(left == right) {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                            dist, left, right);
          }
        }
      }

      if(curNet.hasFixedbump())
        CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
      if(curNet.hasFrequency())
        CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", curNet.frequency());
      if(curNet.hasVoltage())
        CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", curNet.voltage());
      if(curNet.hasWeight())
        CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", curNet.weight());
      if(curNet.hasCap())
        CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", curNet.cap());
      if(curNet.hasSource())
        CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", curNet.source());
      if(curNet.hasPattern())
        CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", curNet.pattern());
      if(curNet.hasOriginal())
        CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", curNet.original());
      if(curNet.hasUse())
        CIRCUIT_FPRINTF(fout, "\n  + USE %s ", curNet.use());

      CIRCUIT_FPRINTF(fout, ";\n");

      objectChange = true;
    }
    else if(netType == DEF_SPECIALNET_PARTIAL_PATH) {
      int i, j, x, y, z, count, newLayer;
      char* layerName;
      double dist, left, right;
      defiPath* p;
      defiSubnet* s;
      int path;
      defiShield* shield;
      defiWire* wire;
      int numX, numY, stepX, stepY;

      //      if (c != defrSNetPartialPathCbkType)
      //        return 1;
      //      if (ud != userData) dataError();

      //      CIRCUIT_FPRINTF (fout, "SPECIALNET partial data\n");

      count = 0;
      if(objectChange) {
        CIRCUIT_FPRINTF(fout, "- %s ", curNet.name());

        // compName & pinName
        for(i = 0; i < curNet.numConnections(); i++) {
          // set the limit of only 5 items print out in one line
          count++;
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i),
                          curNet.pin(i));
          if(curNet.pinIsSynthesized(i))
            CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
        }
      }

      // specialWiring
      // POLYGON
      if(curNet.numPolygons()) {
        struct defiPoints points;
        for(i = 0; i < curNet.numPolygons(); i++) {
          CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curNet.polygonName(i));
          points = curNet.getPolygon(i);
          for(j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        }
      }
      // RECT
      if(curNet.numRectangles()) {
        for(i = 0; i < curNet.numRectangles(); i++) {
          CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", curNet.rectName(i),
                          curNet.xl(i), curNet.yl(i), curNet.xh(i),
                          curNet.yh(i));
        }
      }

      // COVER, FIXED, ROUTED or SHIELD
      if(curNet.numWires()) {
        for(i = 0; i < curNet.numWires(); i++) {
          newLayer = (objectChange) ? 0 : 1;
          wire = curNet.wire(i);
          if(objectChange) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
            if(strcmp(wire->wireType(), "SHIELD") == 0)
              CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
          }
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->initTraverse();
            while((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if(count >= 5) {
                CIRCUIT_FPRINTF(fout, "\n");
                count = 0;
              }
              switch(path) {
                case DEFIPATH_LAYER:
                  if(newLayer == 0) {
                    CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                    newLayer = 1;
                  }
                  else
                    CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                  break;
                case DEFIPATH_VIA:
                  CIRCUIT_FPRINTF(fout, "%s ",
                                  ignoreViaNames ? "XXX" : p->getVia());
                  break;
                case DEFIPATH_VIAROTATION:
                  CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                  break;
                case DEFIPATH_VIADATA:
                  p->getViaData(&numX, &numY, &stepX, &stepY);
                  CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                  stepX, stepY);
                  break;
                case DEFIPATH_WIDTH:
                  CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                  break;
                case DEFIPATH_MASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                  p->getViaCutMask(), p->getViaBottomMask());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                  break;
                case DEFIPATH_FLUSHPOINT:
                  p->getFlushPoint(&x, &y, &z);
                  CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                  break;
                case DEFIPATH_TAPER:
                  CIRCUIT_FPRINTF(fout, "TAPER ");
                  break;
                case DEFIPATH_SHAPE:
                  CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                  break;
                case DEFIPATH_STYLE:
                  CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                  break;
              }
            }
          }
          CIRCUIT_FPRINTF(fout, "\n");
          count = 0;
        }
      }

      if(curNet.hasSubnets()) {
        for(i = 0; i < curNet.numSubnets(); i++) {
          s = curNet.subnet(i);
          if(s->numConnections()) {
            if(s->pinIsMustJoin(0)) {
              CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
            }
            else {
              CIRCUIT_FPRINTF(fout, "- %s ", s->name());
            }
            for(j = 0; j < s->numConnections(); j++) {
              CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
            }
          }

          // regularWiring
          if(s->numWires()) {
            for(i = 0; i < s->numWires(); i++) {
              wire = s->wire(i);
              CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
              for(j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->print(fout);
              }
            }
          }
        }
      }

      if(curNet.numProps()) {
        for(i = 0; i < curNet.numProps(); i++) {
          if(curNet.propIsString(i))
            CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", curNet.propName(i),
                            curNet.propValue(i));
          if(curNet.propIsNumber(i))
            CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", curNet.propName(i),
                            curNet.propNumber(i));
          switch(curNet.propType(i)) {
            case 'R':
              CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
              CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
              CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
              CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
              CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }

      // SHIELD
      count = 0;
      // testing the SHIELD for 5.3, obsolete in 5.4
      if(curNet.numShields()) {
        for(i = 0; i < curNet.numShields(); i++) {
          shield = curNet.shield(i);
          CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
          newLayer = 0;
          for(j = 0; j < shield->numPaths(); j++) {
            p = shield->path(j);
            p->initTraverse();
            while((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if(count >= 5) {
                CIRCUIT_FPRINTF(fout, "\n");
                count = 0;
              }
              switch(path) {
                case DEFIPATH_LAYER:
                  if(newLayer == 0) {
                    CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                    newLayer = 1;
                  }
                  else
                    CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                  break;
                case DEFIPATH_VIA:
                  CIRCUIT_FPRINTF(fout, "%s ",
                                  ignoreViaNames ? "XXX" : p->getVia());
                  break;
                case DEFIPATH_VIAROTATION:
                  if(newLayer) {
                    CIRCUIT_FPRINTF(fout, "%s ",
                                    orientStr(p->getViaRotation()));
                  }
                  else {
                    CIRCUIT_FPRINTF(fout, "Str %s ", p->getViaRotationStr());
                  }
                  break;
                case DEFIPATH_WIDTH:
                  CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                  break;
                case DEFIPATH_MASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                  p->getViaCutMask(), p->getViaBottomMask());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                  break;
                case DEFIPATH_FLUSHPOINT:
                  p->getFlushPoint(&x, &y, &z);
                  CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                  break;
                case DEFIPATH_TAPER:
                  CIRCUIT_FPRINTF(fout, "TAPER ");
                  break;
                case DEFIPATH_SHAPE:
                  CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                  break;
                case DEFIPATH_STYLE:
                  CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              }
            }
          }
        }
      }

      // layerName width
      if(curNet.hasWidthRules()) {
        for(i = 0; i < curNet.numWidthRules(); i++) {
          curNet.widthRule(i, &layerName, &dist);
          CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
        }
      }

      // layerName spacing
      if(curNet.hasSpacingRules()) {
        for(i = 0; i < curNet.numSpacingRules(); i++) {
          curNet.spacingRule(i, &layerName, &dist, &left, &right);
          if(left == right) {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                            dist, left, right);
          }
        }
      }

      if(curNet.hasFixedbump())
        CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
      if(curNet.hasFrequency())
        CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", curNet.frequency());
      if(curNet.hasVoltage())
        CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", curNet.voltage());
      if(curNet.hasWeight())
        CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", curNet.weight());
      if(curNet.hasCap())
        CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", curNet.cap());
      if(curNet.hasSource())
        CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", curNet.source());
      if(curNet.hasPattern())
        CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", curNet.pattern());
      if(curNet.hasOriginal())
        CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", curNet.original());
      if(curNet.hasUse())
        CIRCUIT_FPRINTF(fout, "\n  + USE %s ", curNet.use());

      CIRCUIT_FPRINTF(fout, "\n");

      if(objectChange) {
        objectChange = false;
      }
    }
  }

  CIRCUIT_FPRINTF(fout, "END SPECIALNETS\n\n");
}

void Replace::Circuit::DumpDefNet() {
  if(defNetStor.size() == 0) {
    return;
  }

  // For net and special net.
  int i, j, k, w, x, y, z, count, newLayer;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiVpin* vpin;
  // defiShield *noShield;
  defiWire* wire;

  CIRCUIT_FPRINTF(fout, "NETS %lu ;\n", defNetStor.size());

  for(auto& curNet : defNetStor) {
    CIRCUIT_FPRINTF(fout, "- %s \n", curNet.name());

    if(curNet.pinIsMustJoin(0))
      CIRCUIT_FPRINTF(fout, "- MUSTJOIN \n");

    count = 0;
    // compName & pinName
    for(i = 0; i < curNet.numConnections(); i++) {
      // set the limit of only 5 items per line
      if(count >= 4) {
        CIRCUIT_FPRINTF(fout, "\n");
        count = 0;
      }

      if(count == 0) {
        CIRCUIT_FPRINTF(fout, "  ");
      }

      count++;

      CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i), curNet.pin(i));
      //      curNet.changeInstance("newInstance", i);
      //      curNet.changePin("newPin", i);
      //      CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i),
      //              curNet.pin(i));
      if(curNet.pinIsSynthesized(i))
        CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
    }

    if(curNet.hasNonDefaultRule())
      CIRCUIT_FPRINTF(fout, "+ NONDEFAULTRULE %s\n", curNet.nonDefaultRule());

    for(i = 0; i < curNet.numVpins(); i++) {
      vpin = curNet.vpin(i);
      CIRCUIT_FPRINTF(fout, "  + %s", vpin->name());
      if(vpin->layer())
        CIRCUIT_FPRINTF(fout, " %s", vpin->layer());
      CIRCUIT_FPRINTF(fout, " %d %d %d %d", vpin->xl(), vpin->yl(), vpin->xh(),
                      vpin->yh());
      if(vpin->status() != ' ') {
        CIRCUIT_FPRINTF(fout, " %c", vpin->status());
        CIRCUIT_FPRINTF(fout, " %d %d", vpin->xLoc(), vpin->yLoc());
        if(vpin->orient() != -1)
          CIRCUIT_FPRINTF(fout, " %s", orientStr(vpin->orient()));
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }

    fflush(stdout);
    // regularWiring
    if(curNet.numWires()) {
      for(i = 0; i < curNet.numWires(); i++) {
        newLayer = 0;
        wire = curNet.wire(i);
        CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
        count = 0;
        for(j = 0; j < wire->numPaths(); j++) {
          p = wire->path(j);
          p->initTraverse();
          while((path = (int)p->next()) != DEFIPATH_DONE) {
            count++;
            // Don't want the line to be too long
            if(count >= 5) {
              CIRCUIT_FPRINTF(fout, "\n");
              count = 0;
            }
            switch(path) {
              case DEFIPATH_LAYER:
                if(newLayer == 0) {
                  CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                  newLayer = 1;
                }
                else
                  CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                break;
              case DEFIPATH_MASK:
                CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                break;
              case DEFIPATH_VIAMASK:
                CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                p->getViaCutMask(), p->getViaBottomMask());
                break;
              case DEFIPATH_VIA:
                CIRCUIT_FPRINTF(fout, "%s ",
                                ignoreViaNames ? "XXX" : p->getVia());
                break;
              case DEFIPATH_VIAROTATION:
                CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                break;
              case DEFIPATH_RECT:
                p->getViaRect(&w, &x, &y, &z);
                CIRCUIT_FPRINTF(fout, "RECT ( %d %d %d %d ) ", w, x, y, z);
                break;
              case DEFIPATH_VIRTUALPOINT:
                p->getVirtualPoint(&x, &y);
                CIRCUIT_FPRINTF(fout, "VIRTUAL ( %d %d ) ", x, y);
                break;
              case DEFIPATH_WIDTH:
                CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                break;
              case DEFIPATH_POINT:
                p->getPoint(&x, &y);
                CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                break;
              case DEFIPATH_FLUSHPOINT:
                p->getFlushPoint(&x, &y, &z);
                CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                break;
              case DEFIPATH_TAPER:
                CIRCUIT_FPRINTF(fout, "TAPER ");
                break;
              case DEFIPATH_TAPERRULE:
                CIRCUIT_FPRINTF(fout, "TAPERRULE %s ", p->getTaperRule());
                break;
              case DEFIPATH_STYLE:
                CIRCUIT_FPRINTF(fout, "STYLE %d ", p->getStyle());
                break;
            }
          }
        }
        CIRCUIT_FPRINTF(fout, "\n");
        count = 0;
      }
    }

    // SHIELDNET
    if(curNet.numShieldNets()) {
      for(i = 0; i < curNet.numShieldNets(); i++)
        CIRCUIT_FPRINTF(fout, "\n  + SHIELDNET %s", curNet.shieldNet(i));
    }

    if(curNet.hasSubnets()) {
      for(i = 0; i < curNet.numSubnets(); i++) {
        s = curNet.subnet(i);
        CIRCUIT_FPRINTF(fout, "\n");

        if(s->numConnections()) {
          if(s->pinIsMustJoin(0)) {
            CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
          }
          else {
            CIRCUIT_FPRINTF(fout, "  + SUBNET %s ", s->name());
          }
          for(j = 0; j < s->numConnections(); j++) {
            CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
          }

          // regularWiring
          if(s->numWires()) {
            for(k = 0; k < s->numWires(); k++) {
              newLayer = 0;
              wire = s->wire(k);
              CIRCUIT_FPRINTF(fout, "  %s ", wire->wireType());
              count = 0;
              for(j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->initTraverse();
                while((path = (int)p->next()) != DEFIPATH_DONE) {
                  count++;
                  // Don't want the line to be too long
                  if(count >= 5) {
                    CIRCUIT_FPRINTF(fout, "\n");
                    count = 0;
                  }
                  switch(path) {
                    case DEFIPATH_LAYER:
                      if(newLayer == 0) {
                        CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                        newLayer = 1;
                      }
                      else
                        CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                      break;
                    case DEFIPATH_VIA:
                      CIRCUIT_FPRINTF(fout, "%s ",
                                      ignoreViaNames ? "XXX" : p->getVia());
                      break;
                    case DEFIPATH_VIAROTATION:
                      CIRCUIT_FPRINTF(fout, "%s ", p->getViaRotationStr());
                      break;
                    case DEFIPATH_WIDTH:
                      CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                      break;
                    case DEFIPATH_POINT:
                      p->getPoint(&x, &y);
                      CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                      break;
                    case DEFIPATH_FLUSHPOINT:
                      p->getFlushPoint(&x, &y, &z);
                      CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                      break;
                    case DEFIPATH_TAPER:
                      CIRCUIT_FPRINTF(fout, "TAPER ");
                      break;
                    case DEFIPATH_TAPERRULE:
                      CIRCUIT_FPRINTF(fout, "TAPERRULE  %s ",
                                      p->getTaperRule());
                      break;
                    case DEFIPATH_STYLE:
                      CIRCUIT_FPRINTF(fout, "STYLE  %d ", p->getStyle());
                      break;
                  }
                }
              }
            }
          }
        }
      }
    }

    if(curNet.numProps()) {
      CIRCUIT_FPRINTF(fout, "\n  + PROPERTY ");
      for(i = 0; i < curNet.numProps(); i++) {
        CIRCUIT_FPRINTF(fout, "%s ", curNet.propName(i));
        switch(curNet.propType(i)) {
          case 'R':
            CIRCUIT_FPRINTF(fout, "%.6f ", curNet.propNumber(i));
            break;
          case 'I':
            CIRCUIT_FPRINTF(fout, "%g INTEGER ", curNet.propNumber(i));
            break;
          case 'S':
            CIRCUIT_FPRINTF(fout, "%s STRING ", curNet.propValue(i));
            break;
          case 'Q':
            CIRCUIT_FPRINTF(fout, "%s QUOTESTRING ", curNet.propValue(i));
            break;
          case 'N':
            CIRCUIT_FPRINTF(fout, "%g NUMBER ", curNet.propNumber(i));
            break;
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }

    if(curNet.hasWeight())
      CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", curNet.weight());
    if(curNet.hasCap())
      CIRCUIT_FPRINTF(fout, "+ ESTCAP %.6f ", curNet.cap());
    if(curNet.hasSource())
      CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", curNet.source());
    if(curNet.hasFixedbump())
      CIRCUIT_FPRINTF(fout, "+ FIXEDBUMP ");
    if(curNet.hasFrequency())
      CIRCUIT_FPRINTF(fout, "+ FREQUENCY %.6f ", curNet.frequency());
    if(curNet.hasPattern())
      CIRCUIT_FPRINTF(fout, "+ PATTERN %s ", curNet.pattern());
    if(curNet.hasOriginal())
      CIRCUIT_FPRINTF(fout, "+ ORIGINAL %s ", curNet.original());
    if(curNet.hasUse())
      CIRCUIT_FPRINTF(fout, "+ USE %s ", curNet.use());

    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END NETS\n\n");
}

void Replace::Circuit::DumpDefDone() {
  CIRCUIT_FPRINTF(fout, "END DESIGN\n");
}

// below is for additional Check
void Replace::Circuit::DumpDefComponentPinToNet() {
  for(auto& curComponent : defComponentPinToNet) {
    int idx = &curComponent - &defComponentPinToNet[0];
    cout << "Comp: " << defComponentStor[idx].id() << endl;
    for(auto& curPin : curComponent) {
      cout << defNetStor[curPin.second].name() << " " << curPin.first << ", ";
    }
    cout << endl;
  }
}

//
// Print Function End
//
/////////////////////////////////////////////////////////////////////

int Replace::Circuit::ParseDef(string fileName, bool isVerbose) {
  //    int num = 99;
  char* inFile[6];
  FILE* f;
  int res;
  //    int noCalls = 0;
  //  long start_mem;
  //    int numInFile = 0;
  int fileCt = 0;
  int noNetCb = 0;

  //  start_mem = (long)sbrk(0);

  inFile[0] = strdup(fileName.c_str());
  fout = stdout;
  userData = (void*)0x01020304;

  defVersionPtr = &(this->defVersion);
  defDividerCharPtr = &(this->defDividerChar);
  defBusBitCharPtr = &(this->defBusBitChar);
  defDesignNamePtr = &(this->defDesignName);

  defPropStorPtr = &(this->defPropStor);
  defUnitPtr = &(this->defUnit);

  defDieAreaPtr = &(this->defDieArea);
  defRowStorPtr = &(this->defRowStor);
  defTrackStorPtr = &(this->defTrackStor);
  defGcellGridStorPtr = &(this->defGcellGridStor);
  defViaStorPtr = &(this->defViaStor);

  defComponentMaskShiftLayerPtr = &(this->defComponentMaskShiftLayer);
  defComponentStorPtr = &(this->defComponentStor);
  defNetStorPtr = &(this->defNetStor);
  defSpecialNetStorPtr = &(this->defSpecialNetStor);
  defPinStorPtr = &(this->defPinStor);
  defBlockageStorPtr = &(this->defBlockageStor);

  defComponentMapPtr = &(this->defComponentMap);
  defPinMapPtr = &(this->defPinMap);
  defRowY2OrientMapPtr = &(this->defRowY2OrientMap);

  defComponentPinToNetPtr = &(this->defComponentPinToNet);

  defrInitSession(0);

  defrSetWarningLogFunction(printWarning);

  defrSetUserData((void*)3);
  defrSetDesignCbk(dname);
  defrSetTechnologyCbk(tname);
  defrSetExtensionCbk(extension);
  defrSetDesignEndCbk(done);
  defrSetPropDefStartCbk(propstart);
  defrSetPropCbk(prop);
  defrSetPropDefEndCbk(propend);

  /* Test for CCR 766289*/
  if(!noNetCb)
    defrSetNetCbk(netf);

  defrSetNetNameCbk(netNamef);
  defrSetNetNonDefaultRuleCbk(nondefRulef);
  defrSetNetSubnetNameCbk(subnetNamef);
  defrSetNetPartialPathCbk(netpath);
  defrSetSNetCbk(snetf);
  defrSetSNetPartialPathCbk(snetpath);
  if(setSNetWireCbk)
    defrSetSNetWireCbk(snetwire);
  defrSetComponentMaskShiftLayerCbk(compMSL);
  defrSetComponentCbk(compf);
  defrSetAddPathToNet();
  defrSetHistoryCbk(hist);
  defrSetConstraintCbk(constraint);
  defrSetAssertionCbk(constraint);
  defrSetArrayNameCbk(an);
  defrSetFloorPlanNameCbk(fn);
  defrSetDividerCbk(dn);
  defrSetBusBitCbk(bbn);
  defrSetNonDefaultCbk(ndr);

  defrSetAssertionsStartCbk(constraintst);
  defrSetConstraintsStartCbk(constraintst);
  defrSetComponentStartCbk(cs);
  defrSetPinPropStartCbk(cs);
  defrSetNetStartCbk(cs);
  defrSetStartPinsCbk(cs);
  defrSetViaStartCbk(cs);
  defrSetRegionStartCbk(cs);
  defrSetSNetStartCbk(cs);
  defrSetGroupsStartCbk(cs);
  defrSetScanchainsStartCbk(cs);
  defrSetIOTimingsStartCbk(cs);
  defrSetFPCStartCbk(cs);
  defrSetTimingDisablesStartCbk(cs);
  defrSetPartitionsStartCbk(cs);
  defrSetBlockageStartCbk(cs);
  defrSetSlotStartCbk(cs);
  defrSetFillStartCbk(cs);
  defrSetNonDefaultStartCbk(cs);
  defrSetStylesStartCbk(cs);

  // All of the extensions point to the same function.
  defrSetNetExtCbk(ext);
  defrSetComponentExtCbk(ext);
  defrSetPinExtCbk(ext);
  defrSetViaExtCbk(ext);
  defrSetNetConnectionExtCbk(ext);
  defrSetGroupExtCbk(ext);
  defrSetScanChainExtCbk(ext);
  defrSetIoTimingsExtCbk(ext);
  defrSetPartitionsExtCbk(ext);

  defrSetUnitsCbk(units);
  defrSetVersionStrCbk(versStr);
  defrSetCaseSensitiveCbk(casesens);

  // The following calls are an example of using one function "cls"
  // to be the callback for many DIFFERENT types of constructs.
  // We have to cast the function type to meet the requirements
  // of each different set function.
  defrSetSiteCbk((defrSiteCbkFnType)cls);
  defrSetCanplaceCbk((defrSiteCbkFnType)cls);
  defrSetCannotOccupyCbk((defrSiteCbkFnType)cls);
  defrSetDieAreaCbk((defrBoxCbkFnType)cls);
  defrSetPinCapCbk((defrPinCapCbkFnType)cls);
  defrSetPinCbk((defrPinCbkFnType)cls);

  defrSetPinPropCbk((defrPinPropCbkFnType)cls);
  defrSetDefaultCapCbk((defrIntegerCbkFnType)cls);
  defrSetRowCbk((defrRowCbkFnType)cls);
  defrSetTrackCbk((defrTrackCbkFnType)cls);
  defrSetGcellGridCbk((defrGcellGridCbkFnType)cls);
  defrSetViaCbk((defrViaCbkFnType)cls);
  defrSetRegionCbk((defrRegionCbkFnType)cls);
  defrSetGroupNameCbk((defrStringCbkFnType)cls);
  defrSetGroupMemberCbk((defrStringCbkFnType)cls);
  defrSetGroupCbk((defrGroupCbkFnType)cls);
  defrSetScanchainCbk((defrScanchainCbkFnType)cls);
  defrSetIOTimingCbk((defrIOTimingCbkFnType)cls);
  defrSetFPCCbk((defrFPCCbkFnType)cls);
  defrSetTimingDisableCbk((defrTimingDisableCbkFnType)cls);
  defrSetPartitionCbk((defrPartitionCbkFnType)cls);
  defrSetBlockageCbk((defrBlockageCbkFnType)cls);
  defrSetSlotCbk((defrSlotCbkFnType)cls);
  defrSetFillCbk((defrFillCbkFnType)cls);
  defrSetStylesCbk((defrStylesCbkFnType)cls);

  defrSetAssertionsEndCbk(endfunc);
  defrSetComponentEndCbk(endfunc);
  defrSetConstraintsEndCbk(endfunc);
  defrSetNetEndCbk(endfunc);
  defrSetFPCEndCbk(endfunc);
  defrSetFPCEndCbk(endfunc);
  defrSetGroupsEndCbk(endfunc);
  defrSetIOTimingsEndCbk(endfunc);
  defrSetNetEndCbk(endfunc);
  defrSetPartitionsEndCbk(endfunc);
  defrSetRegionEndCbk(endfunc);
  defrSetSNetEndCbk(endfunc);
  defrSetScanchainsEndCbk(endfunc);
  defrSetPinEndCbk(endfunc);
  defrSetTimingDisablesEndCbk(endfunc);
  defrSetViaEndCbk(endfunc);
  defrSetPinPropEndCbk(endfunc);
  defrSetBlockageEndCbk(endfunc);
  defrSetSlotEndCbk(endfunc);
  defrSetFillEndCbk(endfunc);
  defrSetNonDefaultEndCbk(endfunc);
  defrSetStylesEndCbk(endfunc);

  defrSetMallocFunction(mallocCB);
  defrSetReallocFunction(reallocCB);
  defrSetFreeFunction(freeCB);

  // defrSetRegisterUnusedCallbacks();

  // Testing to set the number of warnings
  defrSetAssertionWarnings(3);
  defrSetBlockageWarnings(3);
  defrSetCaseSensitiveWarnings(3);
  defrSetComponentWarnings(3);
  defrSetConstraintWarnings(0);
  defrSetDefaultCapWarnings(3);
  defrSetGcellGridWarnings(3);
  defrSetIOTimingWarnings(3);
  defrSetNetWarnings(3);
  defrSetNonDefaultWarnings(3);
  defrSetPinExtWarnings(3);
  defrSetPinWarnings(3);
  defrSetRegionWarnings(3);
  defrSetRowWarnings(3);
  defrSetScanchainWarnings(3);
  defrSetSNetWarnings(3);
  defrSetStylesWarnings(3);
  defrSetTrackWarnings(3);
  defrSetUnitsWarnings(3);
  defrSetVersionWarnings(3);
  defrSetViaWarnings(3);

  if(isVerbose) {
    defrSetLongLineNumberFunction(lineNumberCB);
    defrSetDeltaNumberLines(10000);
  }

  (void)defrSetOpenLogFileAppend();

  if((f = fopen(inFile[fileCt], "r")) == 0) {
    fprintf(stderr, "**\nERROR: Couldn't open input file '%s'\n",
            inFile[fileCt]);
    exit(1);
  }
  // Set case sensitive to 0 to start with, in History & PropertyDefinition
  // reset it to 1.

  fout = NULL;
  res = defrRead(f, inFile[fileCt], userData, 1);

  if(res) {
    CIRCUIT_FPRINTF(stderr, "Reader returns bad status.\n", inFile[fileCt]);
    return res;
  }

  // Testing the aliases API.
  defrAddAlias("alias1", "aliasValue1", 1);

  defiAlias_itr aliasStore;
  const char* alias1Value = NULL;

  while(aliasStore.Next()) {
    if(strcmp(aliasStore.Key(), "alias1") == 0) {
      alias1Value = aliasStore.Data();
    }
  }

  if(!alias1Value || strcmp(alias1Value, "aliasValue1")) {
    CIRCUIT_FPRINTF(stderr, "**\nERROR: Aliases don't work\n");
  }

  (void)defrPrintUnusedCallbacks(fout);
  (void)defrReleaseNResetMemory();

  (void)defrUnsetCallbacks();
  (void)defrSetUnusedCallbacks(unUsedCB);

  fflush(stdout);
  //    printf("Component Dump Check\n");

  //    DumpDefComponentPinToNet();
  //    int testInput = 0;
  //    scanf("%d", &testInput);

  // Unset all the callbacks
  defrUnsetArrayNameCbk();
  defrUnsetAssertionCbk();
  defrUnsetAssertionsStartCbk();
  defrUnsetAssertionsEndCbk();
  defrUnsetBlockageCbk();
  defrUnsetBlockageStartCbk();
  defrUnsetBlockageEndCbk();
  defrUnsetBusBitCbk();
  defrUnsetCannotOccupyCbk();
  defrUnsetCanplaceCbk();
  defrUnsetCaseSensitiveCbk();
  defrUnsetComponentCbk();
  defrUnsetComponentExtCbk();
  defrUnsetComponentStartCbk();
  defrUnsetComponentEndCbk();
  defrUnsetConstraintCbk();
  defrUnsetConstraintsStartCbk();
  defrUnsetConstraintsEndCbk();
  defrUnsetDefaultCapCbk();
  defrUnsetDesignCbk();
  defrUnsetDesignEndCbk();
  defrUnsetDieAreaCbk();
  defrUnsetDividerCbk();
  defrUnsetExtensionCbk();
  defrUnsetFillCbk();
  defrUnsetFillStartCbk();
  defrUnsetFillEndCbk();
  defrUnsetFPCCbk();
  defrUnsetFPCStartCbk();
  defrUnsetFPCEndCbk();
  defrUnsetFloorPlanNameCbk();
  defrUnsetGcellGridCbk();
  defrUnsetGroupCbk();
  defrUnsetGroupExtCbk();
  defrUnsetGroupMemberCbk();
  defrUnsetComponentMaskShiftLayerCbk();
  defrUnsetGroupNameCbk();
  defrUnsetGroupsStartCbk();
  defrUnsetGroupsEndCbk();
  defrUnsetHistoryCbk();
  defrUnsetIOTimingCbk();
  defrUnsetIOTimingsStartCbk();
  defrUnsetIOTimingsEndCbk();
  defrUnsetIOTimingsExtCbk();
  defrUnsetNetCbk();
  defrUnsetNetNameCbk();
  defrUnsetNetNonDefaultRuleCbk();
  defrUnsetNetConnectionExtCbk();
  defrUnsetNetExtCbk();
  defrUnsetNetPartialPathCbk();
  defrUnsetNetSubnetNameCbk();
  defrUnsetNetStartCbk();
  defrUnsetNetEndCbk();
  defrUnsetNonDefaultCbk();
  defrUnsetNonDefaultStartCbk();
  defrUnsetNonDefaultEndCbk();
  defrUnsetPartitionCbk();
  defrUnsetPartitionsExtCbk();
  defrUnsetPartitionsStartCbk();
  defrUnsetPartitionsEndCbk();
  defrUnsetPathCbk();
  defrUnsetPinCapCbk();
  defrUnsetPinCbk();
  defrUnsetPinEndCbk();
  defrUnsetPinExtCbk();
  defrUnsetPinPropCbk();
  defrUnsetPinPropStartCbk();
  defrUnsetPinPropEndCbk();
  defrUnsetPropCbk();
  defrUnsetPropDefEndCbk();
  defrUnsetPropDefStartCbk();
  defrUnsetRegionCbk();
  defrUnsetRegionStartCbk();
  defrUnsetRegionEndCbk();
  defrUnsetRowCbk();
  defrUnsetScanChainExtCbk();
  defrUnsetScanchainCbk();
  defrUnsetScanchainsStartCbk();
  defrUnsetScanchainsEndCbk();
  defrUnsetSiteCbk();
  defrUnsetSlotCbk();
  defrUnsetSlotStartCbk();
  defrUnsetSlotEndCbk();
  defrUnsetSNetWireCbk();
  defrUnsetSNetCbk();
  defrUnsetSNetStartCbk();
  defrUnsetSNetEndCbk();
  defrUnsetSNetPartialPathCbk();
  defrUnsetStartPinsCbk();
  defrUnsetStylesCbk();
  defrUnsetStylesStartCbk();
  defrUnsetStylesEndCbk();
  defrUnsetTechnologyCbk();
  defrUnsetTimingDisableCbk();
  defrUnsetTimingDisablesStartCbk();
  defrUnsetTimingDisablesEndCbk();
  defrUnsetTrackCbk();
  defrUnsetUnitsCbk();
  defrUnsetVersionCbk();
  defrUnsetVersionStrCbk();
  defrUnsetViaCbk();
  defrUnsetViaExtCbk();
  defrUnsetViaStartCbk();
  defrUnsetViaEndCbk();

  //    fclose(fout);

  // Release allocated singleton data.
  defrClear();

  free(inFile[0]);
  return res;
}

void Replace::Circuit::WriteDef(FILE* _fout) {
  fout = _fout;

  //    fout = stdout;
  DumpDefVersion();
  DumpDefDividerChar();
  DumpDefBusBitChar();
  DumpDefDesignName();
  DumpDefUnit();
  fflush(fout);

  DumpDefProp();
  fflush(fout);

  DumpDefDieArea();
  DumpDefRow();
  DumpDefTrack();
  DumpDefGcellGrid();
  DumpDefVia();
  fflush(fout);

  DumpDefComponentMaskShiftLayer();
  DumpDefComponent();
  fflush(fout);

  DumpDefBlockage();
  DumpDefPin();
  DumpDefSpecialNet();
  fflush(fout);

  DumpDefNet();
  fflush(fout);

  DumpDefDone();
  fflush(fout);
  //    fflush(stdout);
}

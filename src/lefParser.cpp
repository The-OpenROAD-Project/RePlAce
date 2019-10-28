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
// Copyright 2014 - 2017, Cadence Design Systems
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

//#ifdef WIN32
//#pragma warning (disable : 4786)
//#endif

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string>

#include <unistd.h>

#include "lefrReader.hpp"
#include "lefwWriter.hpp"
#include "lefiDebug.hpp"
#include "lefiEncryptInt.hpp"
#include "lefiUtil.hpp"

#include "lefdefIO.h"

using namespace std;
using Replace::Circuit;

// static char defaultOut[128];
// static int printing = 0;     // Printing the output.
// static int isSessionles = 0;

static FILE* fout;
static int parse65nm = 0;
static int parseLef58Type = 0;

static double* lefVersionPtr = 0;
static string* lefDividerPtr = 0;
static string* lefBusBitCharPtr = 0;

static lefiUnits* lefUnitPtr = 0;
static double* lefManufacturingGridPtr = 0;

static vector< lefiLayer >* lefLayerStorPtr = 0;
static vector< lefiSite >* lefSiteStorPtr = 0;
static vector< lefiMacro >* lefMacroStorPtr = 0;
static vector< lefiVia >* lefViaStorPtr = 0;

// cursor of PIN & OBS
static vector< lefiPin >* curPinStorPtr = 0;
static HASH_MAP< string, int >* curPinMapPtr = 0;
static vector< lefiObstruction >* curObsStorPtr = 0;

// PIN & OBS info
static vector< vector< lefiPin > >* lefPinStorPtr = 0;
static vector< HASH_MAP< string, int > >* lefPinMapStorPtr = 0;
static vector< vector< lefiObstruction > >* lefObsStorPtr = 0;

static HASH_MAP< string, int >* lefMacroMapPtr = 0;
static HASH_MAP< string, int >* lefViaMapPtr = 0;
static HASH_MAP< string, int >* lefLayerMapPtr = 0;
static HASH_MAP< string, int >* lefSiteMapPtr = 0;

static void dataError() {
  CIRCUIT_FPRINTF(fout, "ERROR: returned user data is not correct!\n");
}

static void checkType(lefrCallbackType_e c) {
  if(c >= 0 && c <= lefrLibraryEndCbkType) {
    // OK
  }
  else {
    CIRCUIT_FPRINTF(fout, "ERROR: callback type is out of bounds!\n");
  }
}

static char* orientStr(int orient) {
  switch(orient) {
    case 0:
      return ((char*)"N");
    case 1:
      return ((char*)"W");
    case 2:
      return ((char*)"S");
    case 3:
      return ((char*)"E");
    case 4:
      return ((char*)"FN");
    case 5:
      return ((char*)"FW");
    case 6:
      return ((char*)"FS");
    case 7:
      return ((char*)"FE");
  };
  return ((char*)"BOGUS");
}

void lefVia(lefiVia* via) {
  int i, j;

  lefrSetCaseSensitivity(1);
  CIRCUIT_FPRINTF(fout, "VIA %s ", via->lefiVia::name());
  if(via->lefiVia::hasDefault()) {
    CIRCUIT_FPRINTF(fout, "DEFAULT");
  }
  else if(via->lefiVia::hasGenerated()) {
    CIRCUIT_FPRINTF(fout, "GENERATED");
  }
  CIRCUIT_FPRINTF(fout, "\n");
  if(via->lefiVia::hasTopOfStack())
    CIRCUIT_FPRINTF(fout, "  TOPOFSTACKONLY\n");
  if(via->lefiVia::hasForeign()) {
    CIRCUIT_FPRINTF(fout, "  FOREIGN %s ", via->lefiVia::foreign());
    if(via->lefiVia::hasForeignPnt()) {
      CIRCUIT_FPRINTF(fout, "( %g %g ) ", via->lefiVia::foreignX(),
                      via->lefiVia::foreignY());
      if(via->lefiVia::hasForeignOrient())
        CIRCUIT_FPRINTF(fout, "%s ", orientStr(via->lefiVia::foreignOrient()));
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  if(via->lefiVia::hasProperties()) {
    CIRCUIT_FPRINTF(fout, "  PROPERTY ");
    for(i = 0; i < via->lefiVia::numProperties(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", via->lefiVia::propName(i));
      if(via->lefiVia::propIsNumber(i))
        CIRCUIT_FPRINTF(fout, "%g ", via->lefiVia::propNumber(i));
      if(via->lefiVia::propIsString(i))
        CIRCUIT_FPRINTF(fout, "%s ", via->lefiVia::propValue(i));
      /*
         if (i+1 == via->lefiVia::numProperties())  // end of properties
         CIRCUIT_FPRINTF(fout, ";\n");
         else      // just add new line
         CIRCUIT_FPRINTF(fout, "\n");
         */
      switch(via->lefiVia::propType(i)) {
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
  if(via->lefiVia::hasResistance())
    CIRCUIT_FPRINTF(fout, "  RESISTANCE %g ;\n", via->lefiVia::resistance());
  if(via->lefiVia::numLayers() > 0) {
    for(i = 0; i < via->lefiVia::numLayers(); i++) {
      CIRCUIT_FPRINTF(fout, "  LAYER %s\n", via->lefiVia::layerName(i));
      for(j = 0; j < via->lefiVia::numRects(i); j++)
        if(via->lefiVia::rectColorMask(i, j)) {
          CIRCUIT_FPRINTF(fout, "    RECT MASK %d ( %f %f ) ( %f %f ) ;\n",
                          via->lefiVia::rectColorMask(i, j),
                          via->lefiVia::xl(i, j), via->lefiVia::yl(i, j),
                          via->lefiVia::xh(i, j), via->lefiVia::yh(i, j));
        }
        else {
          CIRCUIT_FPRINTF(fout, "    RECT ( %f %f ) ( %f %f ) ;\n",
                          via->lefiVia::xl(i, j), via->lefiVia::yl(i, j),
                          via->lefiVia::xh(i, j), via->lefiVia::yh(i, j));
        }
      for(j = 0; j < via->lefiVia::numPolygons(i); j++) {
        struct lefiGeomPolygon poly;
        poly = via->lefiVia::getPolygon(i, j);
        if(via->lefiVia::polyColorMask(i, j)) {
          CIRCUIT_FPRINTF(fout, "    POLYGON MASK %d",
                          via->lefiVia::polyColorMask(i, j));
        }
        else {
          CIRCUIT_FPRINTF(fout, "    POLYGON ");
        }
        for(int k = 0; k < poly.numPoints; k++)
          CIRCUIT_FPRINTF(fout, " %g %g ", poly.x[k], poly.y[k]);
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }
  if(via->lefiVia::hasViaRule()) {
    CIRCUIT_FPRINTF(fout, "  VIARULE %s ;\n", via->lefiVia::viaRuleName());
    CIRCUIT_FPRINTF(fout, "    CUTSIZE %g %g ;\n", via->lefiVia::xCutSize(),
                    via->lefiVia::yCutSize());
    CIRCUIT_FPRINTF(fout, "    LAYERS %s %s %s ;\n",
                    via->lefiVia::botMetalLayer(), via->lefiVia::cutLayer(),
                    via->lefiVia::topMetalLayer());
    CIRCUIT_FPRINTF(fout, "    CUTSPACING %g %g ;\n",
                    via->lefiVia::xCutSpacing(), via->lefiVia::yCutSpacing());
    CIRCUIT_FPRINTF(fout, "    ENCLOSURE %g %g %g %g ;\n",
                    via->lefiVia::xBotEnc(), via->lefiVia::yBotEnc(),
                    via->lefiVia::xTopEnc(), via->lefiVia::yTopEnc());
    if(via->lefiVia::hasRowCol())
      CIRCUIT_FPRINTF(fout, "    ROWCOL %d %d ;\n", via->lefiVia::numCutRows(),
                      via->lefiVia::numCutCols());
    if(via->lefiVia::hasOrigin())
      CIRCUIT_FPRINTF(fout, "    ORIGIN %g %g ;\n", via->lefiVia::xOffset(),
                      via->lefiVia::yOffset());
    if(via->lefiVia::hasOffset())
      CIRCUIT_FPRINTF(fout, "    OFFSET %g %g %g %g ;\n",
                      via->lefiVia::xBotOffset(), via->lefiVia::yBotOffset(),
                      via->lefiVia::xTopOffset(), via->lefiVia::yTopOffset());
    if(via->lefiVia::hasCutPattern())
      CIRCUIT_FPRINTF(fout, "    PATTERN %s ;\n", via->lefiVia::cutPattern());
  }
  CIRCUIT_FPRINTF(fout, "END %s\n\n", via->lefiVia::name());

  return;
}

void lefSpacing(lefiSpacing* spacing) {
  CIRCUIT_FPRINTF(fout, "  SAMENET %s %s %g ", spacing->lefiSpacing::name1(),
                  spacing->lefiSpacing::name2(),
                  spacing->lefiSpacing::distance());
  if(spacing->lefiSpacing::hasStack())
    CIRCUIT_FPRINTF(fout, "STACK ");
  CIRCUIT_FPRINTF(fout, ";\n");
  return;
}

void lefViaRuleLayer(lefiViaRuleLayer* vLayer) {
  CIRCUIT_FPRINTF(fout, "  LAYER %s ;\n", vLayer->lefiViaRuleLayer::name());
  if(vLayer->lefiViaRuleLayer::hasDirection()) {
    if(vLayer->lefiViaRuleLayer::isHorizontal())
      CIRCUIT_FPRINTF(fout, "    DIRECTION HORIZONTAL ;\n");
    if(vLayer->lefiViaRuleLayer::isVertical())
      CIRCUIT_FPRINTF(fout, "    DIRECTION VERTICAL ;\n");
  }
  if(vLayer->lefiViaRuleLayer::hasEnclosure()) {
    CIRCUIT_FPRINTF(fout, "    ENCLOSURE %g %g ;\n",
                    vLayer->lefiViaRuleLayer::enclosureOverhang1(),
                    vLayer->lefiViaRuleLayer::enclosureOverhang2());
  }
  if(vLayer->lefiViaRuleLayer::hasWidth())
    CIRCUIT_FPRINTF(fout, "    WIDTH %g TO %g ;\n",
                    vLayer->lefiViaRuleLayer::widthMin(),
                    vLayer->lefiViaRuleLayer::widthMax());
  if(vLayer->lefiViaRuleLayer::hasResistance())
    CIRCUIT_FPRINTF(fout, "    RESISTANCE %g ;\n",
                    vLayer->lefiViaRuleLayer::resistance());
  if(vLayer->lefiViaRuleLayer::hasOverhang())
    CIRCUIT_FPRINTF(fout, "    OVERHANG %g ;\n",
                    vLayer->lefiViaRuleLayer::overhang());
  if(vLayer->lefiViaRuleLayer::hasMetalOverhang())
    CIRCUIT_FPRINTF(fout, "    METALOVERHANG %g ;\n",
                    vLayer->lefiViaRuleLayer::metalOverhang());
  if(vLayer->lefiViaRuleLayer::hasSpacing())
    CIRCUIT_FPRINTF(fout, "    SPACING %g BY %g ;\n",
                    vLayer->lefiViaRuleLayer::spacingStepX(),
                    vLayer->lefiViaRuleLayer::spacingStepY());
  if(vLayer->lefiViaRuleLayer::hasRect())
    CIRCUIT_FPRINTF(
        fout, "    RECT ( %f %f ) ( %f %f ) ;\n",
        vLayer->lefiViaRuleLayer::xl(), vLayer->lefiViaRuleLayer::yl(),
        vLayer->lefiViaRuleLayer::xh(), vLayer->lefiViaRuleLayer::yh());
  return;
}

void prtGeometry(lefiGeometries* geometry) {
  int numItems = geometry->lefiGeometries::numItems();
  int i, j;
  lefiGeomPath* path;
  lefiGeomPathIter* pathIter;
  lefiGeomRect* rect;
  lefiGeomRectIter* rectIter;
  lefiGeomPolygon* polygon;
  lefiGeomPolygonIter* polygonIter;
  lefiGeomVia* via;
  lefiGeomViaIter* viaIter;

  for(i = 0; i < numItems; i++) {
    switch(geometry->lefiGeometries::itemType(i)) {
      case lefiGeomClassE:
        CIRCUIT_FPRINTF(fout, "CLASS %s ",
                        geometry->lefiGeometries::getClass(i));
        break;
      case lefiGeomLayerE:
        CIRCUIT_FPRINTF(fout, "      LAYER %s ;\n",
                        geometry->lefiGeometries::getLayer(i));
        break;
      case lefiGeomLayerExceptPgNetE:
        CIRCUIT_FPRINTF(fout, "      EXCEPTPGNET ;\n");
        break;
      case lefiGeomLayerMinSpacingE:
        CIRCUIT_FPRINTF(fout, "      SPACING %g ;\n",
                        geometry->lefiGeometries::getLayerMinSpacing(i));
        break;
      case lefiGeomLayerRuleWidthE:
        CIRCUIT_FPRINTF(fout, "      DESIGNRULEWIDTH %g ;\n",
                        geometry->lefiGeometries::getLayerRuleWidth(i));
        break;
      case lefiGeomWidthE:
        CIRCUIT_FPRINTF(fout, "      WIDTH %g ;\n",
                        geometry->lefiGeometries::getWidth(i));
        break;
      case lefiGeomPathE:
        path = geometry->lefiGeometries::getPath(i);
        if(path->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      PATH MASK %d ", path->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      PATH ");
        }
        for(j = 0; j < path->numPoints; j++) {
          if(j + 1 == path->numPoints) {  // last one on the list
            CIRCUIT_FPRINTF(fout, "      ( %g %g ) ;\n", path->x[j],
                            path->y[j]);
          }
          else {
            CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", path->x[j], path->y[j]);
          }
        }
        break;
      case lefiGeomPathIterE:
        pathIter = geometry->lefiGeometries::getPathIter(i);
        if(pathIter->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      PATH MASK %d ITERATED ",
                          pathIter->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      PATH ITERATED ");
        }
        for(j = 0; j < pathIter->numPoints; j++)
          CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", pathIter->x[j],
                          pathIter->y[j]);
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        pathIter->xStart, pathIter->yStart, pathIter->xStep,
                        pathIter->yStep);
        break;
      case lefiGeomRectE:
        rect = geometry->lefiGeometries::getRect(i);
        if(rect->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      RECT MASK %d ( %f %f ) ( %f %f ) ;\n",
                          rect->colorMask, rect->xl, rect->yl, rect->xh,
                          rect->yh);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      RECT ( %f %f ) ( %f %f ) ;\n", rect->xl,
                          rect->yl, rect->xh, rect->yh);
        }
        break;
      case lefiGeomRectIterE:
        rectIter = geometry->lefiGeometries::getRectIter(i);
        if(rectIter->colorMask != 0) {
          CIRCUIT_FPRINTF(fout,
                          "      RECT MASK %d ITERATE ( %f %f ) ( %f %f )\n",
                          rectIter->colorMask, rectIter->xl, rectIter->yl,
                          rectIter->xh, rectIter->yh);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      RECT ITERATE ( %f %f ) ( %f %f )\n",
                          rectIter->xl, rectIter->yl, rectIter->xh,
                          rectIter->yh);
        }
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        rectIter->xStart, rectIter->yStart, rectIter->xStep,
                        rectIter->yStep);
        break;
      case lefiGeomPolygonE:
        polygon = geometry->lefiGeometries::getPolygon(i);
        if(polygon->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      POLYGON MASK %d ", polygon->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      POLYGON ");
        }
        for(j = 0; j < polygon->numPoints; j++) {
          if(j + 1 == polygon->numPoints) {  // last one on the list
            CIRCUIT_FPRINTF(fout, "      ( %g %g ) ;\n", polygon->x[j],
                            polygon->y[j]);
          }
          else {
            CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", polygon->x[j],
                            polygon->y[j]);
          }
        }
        break;
      case lefiGeomPolygonIterE:
        polygonIter = geometry->lefiGeometries::getPolygonIter(i);
        if(polygonIter->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "       POLYGON MASK %d ITERATE ",
                          polygonIter->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      POLYGON ITERATE");
        }
        for(j = 0; j < polygonIter->numPoints; j++)
          CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", polygonIter->x[j],
                          polygonIter->y[j]);
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        polygonIter->xStart, polygonIter->yStart,
                        polygonIter->xStep, polygonIter->yStep);
        break;
      case lefiGeomViaE:
        via = geometry->lefiGeometries::getVia(i);
        if(via->topMaskNum != 0 || via->bottomMaskNum != 0 ||
           via->cutMaskNum != 0) {
          CIRCUIT_FPRINTF(fout, "      VIA MASK %d%d%d ( %g %g ) %s ;\n",
                          via->topMaskNum, via->cutMaskNum, via->bottomMaskNum,
                          via->x, via->y, via->name);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      VIA ( %g %g ) %s ;\n", via->x, via->y,
                          via->name);
        }
        break;
      case lefiGeomViaIterE:
        viaIter = geometry->lefiGeometries::getViaIter(i);
        if(viaIter->topMaskNum != 0 || viaIter->cutMaskNum != 0 ||
           viaIter->bottomMaskNum != 0) {
          CIRCUIT_FPRINTF(fout, "      VIA ITERATE MASK %d%d%d ( %g %g ) %s\n",
                          viaIter->topMaskNum, viaIter->cutMaskNum,
                          viaIter->bottomMaskNum, viaIter->x, viaIter->y,
                          viaIter->name);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      VIA ITERATE ( %g %g ) %s\n", viaIter->x,
                          viaIter->y, viaIter->name);
        }
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        viaIter->xStart, viaIter->yStart, viaIter->xStep,
                        viaIter->yStep);
        break;
      default:
        CIRCUIT_FPRINTF(fout, "BOGUS geometries type.\n");
        break;
    }
  }
}

int antennaCB(lefrCallbackType_e c, double value, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  switch(c) {
    case lefrAntennaInputCbkType:
      CIRCUIT_FPRINTF(fout, "ANTENNAINPUTGATEAREA %g ;\n", value);
      break;
    case lefrAntennaInoutCbkType:
      CIRCUIT_FPRINTF(fout, "ANTENNAINOUTDIFFAREA %g ;\n", value);
      break;
    case lefrAntennaOutputCbkType:
      CIRCUIT_FPRINTF(fout, "ANTENNAOUTPUTDIFFAREA %g ;\n", value);
      break;
    case lefrInputAntennaCbkType:
      CIRCUIT_FPRINTF(fout, "INPUTPINANTENNASIZE %g ;\n", value);
      break;
    case lefrOutputAntennaCbkType:
      CIRCUIT_FPRINTF(fout, "OUTPUTPINANTENNASIZE %g ;\n", value);
      break;
    case lefrInoutAntennaCbkType:
      CIRCUIT_FPRINTF(fout, "INOUTPINANTENNASIZE %g ;\n", value);
      break;
    default:
      CIRCUIT_FPRINTF(fout, "BOGUS antenna type.\n");
      break;
  }
  return 0;
}

int arrayBeginCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  int status;

  checkType(c);
  // if ((long)ud != userData) dataError();
  // use the lef writer to write the data out
  status = lefwStartArray(name);
  if(status != LEFW_OK)
    return status;
  return 0;
}

int arrayCB(lefrCallbackType_e c, lefiArray* a, lefiUserData) {
  int status, i, j, defCaps;
  lefiSitePattern* pattern;
  lefiTrackPattern* track;
  lefiGcellPattern* gcell;

  checkType(c);
  // if ((long)ud != userData) dataError();

  if(a->lefiArray::numSitePattern() > 0) {
    for(i = 0; i < a->lefiArray::numSitePattern(); i++) {
      pattern = a->lefiArray::sitePattern(i);
      status = lefwArraySite(
          pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
          pattern->lefiSitePattern::y(), pattern->lefiSitePattern::orient(),
          pattern->lefiSitePattern::xStart(),
          pattern->lefiSitePattern::yStart(), pattern->lefiSitePattern::xStep(),
          pattern->lefiSitePattern::yStep());
      if(status != LEFW_OK)
        dataError();
    }
  }
  if(a->lefiArray::numCanPlace() > 0) {
    for(i = 0; i < a->lefiArray::numCanPlace(); i++) {
      pattern = a->lefiArray::canPlace(i);
      status = lefwArrayCanplace(
          pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
          pattern->lefiSitePattern::y(), pattern->lefiSitePattern::orient(),
          pattern->lefiSitePattern::xStart(),
          pattern->lefiSitePattern::yStart(), pattern->lefiSitePattern::xStep(),
          pattern->lefiSitePattern::yStep());
      if(status != LEFW_OK)
        dataError();
    }
  }
  if(a->lefiArray::numCannotOccupy() > 0) {
    for(i = 0; i < a->lefiArray::numCannotOccupy(); i++) {
      pattern = a->lefiArray::cannotOccupy(i);
      status = lefwArrayCannotoccupy(
          pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
          pattern->lefiSitePattern::y(), pattern->lefiSitePattern::orient(),
          pattern->lefiSitePattern::xStart(),
          pattern->lefiSitePattern::yStart(), pattern->lefiSitePattern::xStep(),
          pattern->lefiSitePattern::yStep());
      if(status != LEFW_OK)
        dataError();
    }
  }

  if(a->lefiArray::numTrack() > 0) {
    for(i = 0; i < a->lefiArray::numTrack(); i++) {
      track = a->lefiArray::track(i);
      CIRCUIT_FPRINTF(fout, "  TRACKS %s, %g DO %d STEP %g\n",
                      track->lefiTrackPattern::name(),
                      track->lefiTrackPattern::start(),
                      track->lefiTrackPattern::numTracks(),
                      track->lefiTrackPattern::space());
      if(track->lefiTrackPattern::numLayers() > 0) {
        CIRCUIT_FPRINTF(fout, "  LAYER ");
        for(j = 0; j < track->lefiTrackPattern::numLayers(); j++)
          CIRCUIT_FPRINTF(fout, "%s ", track->lefiTrackPattern::layerName(j));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }

  if(a->lefiArray::numGcell() > 0) {
    for(i = 0; i < a->lefiArray::numGcell(); i++) {
      gcell = a->lefiArray::gcell(i);
      CIRCUIT_FPRINTF(
          fout, "  GCELLGRID %s, %g DO %d STEP %g\n",
          gcell->lefiGcellPattern::name(), gcell->lefiGcellPattern::start(),
          gcell->lefiGcellPattern::numCRs(), gcell->lefiGcellPattern::space());
    }
  }

  if(a->lefiArray::numFloorPlans() > 0) {
    for(i = 0; i < a->lefiArray::numFloorPlans(); i++) {
      status = lefwStartArrayFloorplan(a->lefiArray::floorPlanName(i));
      if(status != LEFW_OK)
        dataError();
      for(j = 0; j < a->lefiArray::numSites(i); j++) {
        pattern = a->lefiArray::site(i, j);
        status = lefwArrayFloorplan(
            a->lefiArray::siteType(i, j), pattern->lefiSitePattern::name(),
            pattern->lefiSitePattern::x(), pattern->lefiSitePattern::y(),
            pattern->lefiSitePattern::orient(),
            (int)pattern->lefiSitePattern::xStart(),
            (int)pattern->lefiSitePattern::yStart(),
            pattern->lefiSitePattern::xStep(),
            pattern->lefiSitePattern::yStep());
        if(status != LEFW_OK)
          dataError();
      }
      status = lefwEndArrayFloorplan(a->lefiArray::floorPlanName(i));
      if(status != LEFW_OK)
        dataError();
    }
  }

  defCaps = a->lefiArray::numDefaultCaps();
  if(defCaps > 0) {
    status = lefwStartArrayDefaultCap(defCaps);
    if(status != LEFW_OK)
      dataError();
    for(i = 0; i < defCaps; i++) {
      status = lefwArrayDefaultCap(a->lefiArray::defaultCapMinPins(i),
                                   a->lefiArray::defaultCap(i));
      if(status != LEFW_OK)
        dataError();
    }
    status = lefwEndArrayDefaultCap();
    if(status != LEFW_OK)
      dataError();
  }
  return 0;
}

int arrayEndCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  int status;

  checkType(c);
  // if ((long)ud != userData) dataError();
  // use the lef writer to write the data out
  status = lefwEndArray(name);
  if(status != LEFW_OK)
    return status;
  return 0;
}

int busBitCharsCB(lefrCallbackType_e c, const char* busBit, lefiUserData) {
  checkType(c);

  // use the lef writer to write out the data
  //    int status = lefwBusBitChars(busBit);
  //    if (status != LEFW_OK)
  //        dataError();

  (*lefBusBitCharPtr) = string(busBit);
  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n", busBit);
  return 0;
}

int caseSensCB(lefrCallbackType_e c, int caseSense, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  if(caseSense == TRUE) {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE ON ;\n");
  }
  else {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE OFF ;\n");
  }
  return 0;
}

int fixedMaskCB(lefrCallbackType_e c, int fixedMask, lefiUserData) {
  checkType(c);

  if(fixedMask == 1)
    CIRCUIT_FPRINTF(fout, "FIXEDMASK ;\n");
  return 0;
}

int clearanceCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "CLEARANCEMEASURE %s ;\n", name);
  return 0;
}

int dividerCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  checkType(c);
  (*lefDividerPtr) = string(name);

  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n", name);
  return 0;
}

int noWireExtCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "NOWIREEXTENSION %s ;\n", name);
  return 0;
}

int noiseMarCB(lefrCallbackType_e c, lefiNoiseMargin*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  return 0;
}

int edge1CB(lefrCallbackType_e c, double name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "EDGERATETHRESHOLD1 %g ;\n", name);
  return 0;
}

int edge2CB(lefrCallbackType_e c, double name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "EDGERATETHRESHOLD2 %g ;\n", name);
  return 0;
}

int edgeScaleCB(lefrCallbackType_e c, double name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "EDGERATESCALEFACTORE %g ;\n", name);
  return 0;
}

int noiseTableCB(lefrCallbackType_e c, lefiNoiseTable*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  return 0;
}

int correctionCB(lefrCallbackType_e c, lefiCorrectionTable*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  return 0;
}

int dielectricCB(lefrCallbackType_e c, double dielectric, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "DIELECTRIC %g ;\n", dielectric);
  return 0;
}

int irdropBeginCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "IRDROP\n");
  return 0;
}

int irdropCB(lefrCallbackType_e c, lefiIRDrop* irdrop, lefiUserData) {
  int i;
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  TABLE %s ", irdrop->lefiIRDrop::name());
  for(i = 0; i < irdrop->lefiIRDrop::numValues(); i++)
    CIRCUIT_FPRINTF(fout, "%g %g ", irdrop->lefiIRDrop::value1(i),
                    irdrop->lefiIRDrop::value2(i));
  CIRCUIT_FPRINTF(fout, ";\n");
  return 0;
}

int irdropEndCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END IRDROP\n");
  return 0;
}

int layerCB(lefrCallbackType_e c, lefiLayer* layer, lefiUserData) {
  //    int i, j, k;
  //    int numPoints, propNum;
  //    double *widths, *current;
  //    lefiLayerDensity* density;
  //    lefiAntennaPWL* pwl;
  //    lefiSpacingTable* spTable;
  //    lefiInfluence* influence;
  //    lefiParallel* parallel;
  //    lefiTwoWidths* twoWidths;
  //    char pType;
  //    int numMinCut, numMinenclosed;
  //    lefiAntennaModel* aModel;
  //    lefiOrthogonal*   ortho;

  checkType(c);
  // if ((long)ud != userData) dataError();

  lefrSetCaseSensitivity(0);

  // Call parse65nmRules for 5.7 syntax in 5.6
  if(parse65nm)
    layer->lefiLayer::parse65nmRules();

  // Call parseLef58Type for 5.8 syntax in 5.7
  if(parseLef58Type)
    layer->lefiLayer::parseLEF58Layer();

  // layerName -> lefLayerStor's index.
  (*lefLayerMapPtr)[string(layer->name())] = lefLayerStorPtr->size();
  lefLayerStorPtr->push_back(*layer);

  // Set it to case sensitive from here on
  lefrSetCaseSensitivity(1);

  return 0;
}

int macroBeginCB(lefrCallbackType_e c, const char* macroName, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  (*lefMacroMapPtr)[string(macroName)] = lefMacroStorPtr->size();

  curPinStorPtr = new vector< lefiPin >;
  curObsStorPtr = new vector< lefiObstruction >;
  curPinMapPtr = new HASH_MAP< string, int >;
#ifdef USE_GOOGLE_HASH
  curPinMapPtr->set_empty_key(INIT_STR);
#endif

  CIRCUIT_FPRINTF(fout, "MACRO %s\n", macroName);
  return 0;
}

int macroFixedMaskCB(lefrCallbackType_e c, int, lefiUserData) {
  checkType(c);

  return 0;
}

int macroClassTypeCB(lefrCallbackType_e c, const char* macroClassType,
                     lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "MACRO CLASS %s\n", macroClassType);
  return 0;
}

int macroOriginCB(lefrCallbackType_e c, lefiNum, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  // CIRCUIT_FPRINTF(fout, "  ORIGIN ( %g %g ) ;\n", macroNum.x, macroNum.y);
  return 0;
}

int macroSizeCB(lefrCallbackType_e c, lefiNum, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  // CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", macroNum.x, macroNum.y);
  return 0;
}

int macroCB(lefrCallbackType_e c, lefiMacro* macro, lefiUserData) {
  lefiSitePattern* pattern;
  int propNum, i, hasPrtSym = 0;

  lefMacroStorPtr->push_back(*macro);
  checkType(c);
  // if ((long)ud != userData) dataError();
  if(macro->lefiMacro::hasClass())
    CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", macro->lefiMacro::macroClass());
  if(macro->lefiMacro::isFixedMask())
    CIRCUIT_FPRINTF(fout, "  FIXEDMASK ;\n");
  if(macro->lefiMacro::hasEEQ())
    CIRCUIT_FPRINTF(fout, "  EEQ %s ;\n", macro->lefiMacro::EEQ());
  if(macro->lefiMacro::hasLEQ())
    CIRCUIT_FPRINTF(fout, "  LEQ %s ;\n", macro->lefiMacro::LEQ());
  if(macro->lefiMacro::hasSource())
    CIRCUIT_FPRINTF(fout, "  SOURCE %s ;\n", macro->lefiMacro::source());
  if(macro->lefiMacro::hasXSymmetry()) {
    CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
    hasPrtSym = 1;
  }
  if(macro->lefiMacro::hasYSymmetry()) {  // print X Y & R90 in one line
    if(!hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
      hasPrtSym = 1;
    }
    else
      CIRCUIT_FPRINTF(fout, "Y ");
  }
  if(macro->lefiMacro::has90Symmetry()) {
    if(!hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
      hasPrtSym = 1;
    }
    else
      CIRCUIT_FPRINTF(fout, "R90 ");
  }
  if(hasPrtSym) {
    CIRCUIT_FPRINTF(fout, ";\n");
    hasPrtSym = 0;
  }
  if(macro->lefiMacro::hasSiteName())
    CIRCUIT_FPRINTF(fout, "  SITE %s ;\n", macro->lefiMacro::siteName());
  if(macro->lefiMacro::hasSitePattern()) {
    for(i = 0; i < macro->lefiMacro::numSitePattern(); i++) {
      pattern = macro->lefiMacro::sitePattern(i);
      if(pattern->lefiSitePattern::hasStepPattern()) {
        CIRCUIT_FPRINTF(fout, "  SITE %s %g %g %s DO %g BY %g STEP %g %g ;\n",
                        pattern->lefiSitePattern::name(),
                        pattern->lefiSitePattern::x(),
                        pattern->lefiSitePattern::y(),
                        orientStr(pattern->lefiSitePattern::orient()),
                        pattern->lefiSitePattern::xStart(),
                        pattern->lefiSitePattern::yStart(),
                        pattern->lefiSitePattern::xStep(),
                        pattern->lefiSitePattern::yStep());
      }
      else {
        CIRCUIT_FPRINTF(
            fout, "  SITE %s %g %g %s ;\n", pattern->lefiSitePattern::name(),
            pattern->lefiSitePattern::x(), pattern->lefiSitePattern::y(),
            orientStr(pattern->lefiSitePattern::orient()));
      }
    }
  }
  if(macro->lefiMacro::hasSize())
    CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", macro->lefiMacro::sizeX(),
                    macro->lefiMacro::sizeY());

  if(macro->lefiMacro::hasForeign()) {
    for(i = 0; i < macro->lefiMacro::numForeigns(); i++) {
      CIRCUIT_FPRINTF(fout, "  FOREIGN %s ", macro->lefiMacro::foreignName(i));
      if(macro->lefiMacro::hasForeignPoint(i)) {
        CIRCUIT_FPRINTF(fout, "( %g %g ) ", macro->lefiMacro::foreignX(i),
                        macro->lefiMacro::foreignY(i));
        if(macro->lefiMacro::hasForeignOrient(i))
          CIRCUIT_FPRINTF(fout, "%s ", macro->lefiMacro::foreignOrientStr(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(macro->lefiMacro::hasOrigin())
    CIRCUIT_FPRINTF(fout, "  ORIGIN ( %g %g ) ;\n", macro->lefiMacro::originX(),
                    macro->lefiMacro::originY());
  if(macro->lefiMacro::hasPower())
    CIRCUIT_FPRINTF(fout, "  POWER %g ;\n", macro->lefiMacro::power());
  propNum = macro->lefiMacro::numProperties();
  if(propNum > 0) {
    CIRCUIT_FPRINTF(fout, "  PROPERTY ");
    for(i = 0; i < propNum; i++) {
      // value can either be a string or number
      if(macro->lefiMacro::propValue(i)) {
        CIRCUIT_FPRINTF(fout, "%s %s ", macro->lefiMacro::propName(i),
                        macro->lefiMacro::propValue(i));
      }
      else
        CIRCUIT_FPRINTF(fout, "%s %g ", macro->lefiMacro::propName(i),
                        macro->lefiMacro::propNum(i));

      switch(macro->lefiMacro::propType(i)) {
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
  // CIRCUIT_FPRINTF(fout, "END %s\n", macro->lefiMacro::name());
  return 0;
}

int macroEndCB(lefrCallbackType_e c, const char* macroName, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  lefPinStorPtr->push_back(*curPinStorPtr);
  lefObsStorPtr->push_back(*curObsStorPtr);
  lefPinMapStorPtr->push_back(*curPinMapPtr);

  delete curPinStorPtr;
  delete curObsStorPtr;
  delete curPinMapPtr;
  curPinStorPtr = NULL;
  curObsStorPtr = NULL;
  curPinMapPtr = NULL;

  CIRCUIT_FPRINTF(fout, "END %s\n", macroName);
  return 0;
}

int manufacturingCB(lefrCallbackType_e c, double num, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "MANUFACTURINGGRID %g ;\n", num);
  *lefManufacturingGridPtr = num;
  return 0;
}

int maxStackViaCB(lefrCallbackType_e c, lefiMaxStackVia* maxStack,
                  lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "MAXVIASTACK %d ",
                  maxStack->lefiMaxStackVia::maxStackVia());
  if(maxStack->lefiMaxStackVia::hasMaxStackViaRange())
    CIRCUIT_FPRINTF(fout, "RANGE %s %s ",
                    maxStack->lefiMaxStackVia::maxStackViaBottomLayer(),
                    maxStack->lefiMaxStackVia::maxStackViaTopLayer());
  CIRCUIT_FPRINTF(fout, ";\n");
  return 0;
}

int minFeatureCB(lefrCallbackType_e c, lefiMinFeature* min, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "MINFEATURE %g %g ;\n", min->lefiMinFeature::one(),
                  min->lefiMinFeature::two());
  return 0;
}

int nonDefaultCB(lefrCallbackType_e c, lefiNonDefault* def, lefiUserData) {
  int i;
  lefiVia* via;
  lefiSpacing* spacing;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "NONDEFAULTRULE %s\n", def->lefiNonDefault::name());
  if(def->lefiNonDefault::hasHardspacing())
    CIRCUIT_FPRINTF(fout, "  HARDSPACING ;\n");
  for(i = 0; i < def->lefiNonDefault::numLayers(); i++) {
    CIRCUIT_FPRINTF(fout, "  LAYER %s\n", def->lefiNonDefault::layerName(i));
    if(def->lefiNonDefault::hasLayerWidth(i))
      CIRCUIT_FPRINTF(fout, "    WIDTH %g ;\n",
                      def->lefiNonDefault::layerWidth(i));
    if(def->lefiNonDefault::hasLayerSpacing(i))
      CIRCUIT_FPRINTF(fout, "    SPACING %g ;\n",
                      def->lefiNonDefault::layerSpacing(i));
    if(def->lefiNonDefault::hasLayerDiagWidth(i))
      CIRCUIT_FPRINTF(fout, "    DIAGWIDTH %g ;\n",
                      def->lefiNonDefault::layerDiagWidth(i));
    if(def->lefiNonDefault::hasLayerWireExtension(i))
      CIRCUIT_FPRINTF(fout, "    WIREEXTENSION %g ;\n",
                      def->lefiNonDefault::layerWireExtension(i));
    if(def->lefiNonDefault::hasLayerResistance(i))
      CIRCUIT_FPRINTF(fout, "    RESISTANCE RPERSQ %g ;\n",
                      def->lefiNonDefault::layerResistance(i));
    if(def->lefiNonDefault::hasLayerCapacitance(i))
      CIRCUIT_FPRINTF(fout, "    CAPACITANCE CPERSQDIST %g ;\n",
                      def->lefiNonDefault::layerCapacitance(i));
    if(def->lefiNonDefault::hasLayerEdgeCap(i))
      CIRCUIT_FPRINTF(fout, "    EDGECAPACITANCE %g ;\n",
                      def->lefiNonDefault::layerEdgeCap(i));
    CIRCUIT_FPRINTF(fout, "  END %s\n", def->lefiNonDefault::layerName(i));
  }

  // handle via in nondefaultrule
  for(i = 0; i < def->lefiNonDefault::numVias(); i++) {
    via = def->lefiNonDefault::viaRule(i);
    lefVia(via);
  }

  // handle spacing in nondefaultrule
  for(i = 0; i < def->lefiNonDefault::numSpacingRules(); i++) {
    spacing = def->lefiNonDefault::spacingRule(i);
    lefSpacing(spacing);
  }

  // handle usevia
  for(i = 0; i < def->lefiNonDefault::numUseVia(); i++)
    CIRCUIT_FPRINTF(fout, "    USEVIA %s ;\n", def->lefiNonDefault::viaName(i));

  // handle useviarule
  for(i = 0; i < def->lefiNonDefault::numUseViaRule(); i++)
    CIRCUIT_FPRINTF(fout, "    USEVIARULE %s ;\n",
                    def->lefiNonDefault::viaRuleName(i));

  // handle mincuts
  for(i = 0; i < def->lefiNonDefault::numMinCuts(); i++) {
    CIRCUIT_FPRINTF(fout, "   MINCUTS %s %d ;\n",
                    def->lefiNonDefault::cutLayerName(i),
                    def->lefiNonDefault::numCuts(i));
  }

  // handle property in nondefaultrule
  if(def->lefiNonDefault::numProps() > 0) {
    CIRCUIT_FPRINTF(fout, "   PROPERTY ");
    for(i = 0; i < def->lefiNonDefault::numProps(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", def->lefiNonDefault::propName(i));
      if(def->lefiNonDefault::propIsNumber(i))
        CIRCUIT_FPRINTF(fout, "%g ", def->lefiNonDefault::propNumber(i));
      if(def->lefiNonDefault::propIsString(i))
        CIRCUIT_FPRINTF(fout, "%s ", def->lefiNonDefault::propValue(i));
      switch(def->lefiNonDefault::propType(i)) {
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
  CIRCUIT_FPRINTF(fout, "END %s ;\n", def->lefiNonDefault::name());

  return 0;
}

int obstructionCB(lefrCallbackType_e c, lefiObstruction* obs, lefiUserData) {
  lefiGeometries* geometry;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  OBS\n");
  geometry = obs->lefiObstruction::geometries();
  prtGeometry(geometry);
  CIRCUIT_FPRINTF(fout, "  END\n");
  curObsStorPtr->push_back(*obs);
  return 0;
}

int pinCB(lefrCallbackType_e c, lefiPin* pin, lefiUserData) {
  int numPorts, i, j;
  lefiGeometries* geometry;
  lefiPinAntennaModel* aModel;

  (*curPinMapPtr)[string(pin->name())] = curPinStorPtr->size();
  curPinStorPtr->push_back(*pin);

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  PIN %s\n", pin->lefiPin::name());
  if(pin->lefiPin::hasForeign()) {
    for(i = 0; i < pin->lefiPin::numForeigns(); i++) {
      if(pin->lefiPin::hasForeignOrient(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) %s ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i),
                        pin->lefiPin::foreignOrientStr(i));
      }
      else if(pin->lefiPin::hasForeignPoint(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s ;\n",
                        pin->lefiPin::foreignName(i));
      }
    }
  }
  if(pin->lefiPin::hasLEQ())
    CIRCUIT_FPRINTF(fout, "    LEQ %s ;\n", pin->lefiPin::LEQ());
  if(pin->lefiPin::hasDirection())
    CIRCUIT_FPRINTF(fout, "    DIRECTION %s ;\n", pin->lefiPin::direction());
  if(pin->lefiPin::hasUse())
    CIRCUIT_FPRINTF(fout, "    USE %s ;\n", pin->lefiPin::use());
  if(pin->lefiPin::hasShape())
    CIRCUIT_FPRINTF(fout, "    SHAPE %s ;\n", pin->lefiPin::shape());
  if(pin->lefiPin::hasMustjoin())
    CIRCUIT_FPRINTF(fout, "    MUSTJOIN %s ;\n", pin->lefiPin::mustjoin());
  if(pin->lefiPin::hasOutMargin())
    CIRCUIT_FPRINTF(fout, "    OUTPUTNOISEMARGIN %g %g ;\n",
                    pin->lefiPin::outMarginHigh(),
                    pin->lefiPin::outMarginLow());
  if(pin->lefiPin::hasOutResistance())
    CIRCUIT_FPRINTF(fout, "    OUTPUTRESISTANCE %g %g ;\n",
                    pin->lefiPin::outResistanceHigh(),
                    pin->lefiPin::outResistanceLow());
  if(pin->lefiPin::hasInMargin())
    CIRCUIT_FPRINTF(fout, "    INPUTNOISEMARGIN %g %g ;\n",
                    pin->lefiPin::inMarginHigh(), pin->lefiPin::inMarginLow());
  if(pin->lefiPin::hasPower())
    CIRCUIT_FPRINTF(fout, "    POWER %g ;\n", pin->lefiPin::power());
  if(pin->lefiPin::hasLeakage())
    CIRCUIT_FPRINTF(fout, "    LEAKAGE %g ;\n", pin->lefiPin::leakage());
  if(pin->lefiPin::hasMaxload())
    CIRCUIT_FPRINTF(fout, "    MAXLOAD %g ;\n", pin->lefiPin::maxload());
  if(pin->lefiPin::hasCapacitance())
    CIRCUIT_FPRINTF(fout, "    CAPACITANCE %g ;\n",
                    pin->lefiPin::capacitance());
  if(pin->lefiPin::hasResistance())
    CIRCUIT_FPRINTF(fout, "    RESISTANCE %g ;\n", pin->lefiPin::resistance());
  if(pin->lefiPin::hasPulldownres())
    CIRCUIT_FPRINTF(fout, "    PULLDOWNRES %g ;\n",
                    pin->lefiPin::pulldownres());
  if(pin->lefiPin::hasTieoffr())
    CIRCUIT_FPRINTF(fout, "    TIEOFFR %g ;\n", pin->lefiPin::tieoffr());
  if(pin->lefiPin::hasVHI())
    CIRCUIT_FPRINTF(fout, "    VHI %g ;\n", pin->lefiPin::VHI());
  if(pin->lefiPin::hasVLO())
    CIRCUIT_FPRINTF(fout, "    VLO %g ;\n", pin->lefiPin::VLO());
  if(pin->lefiPin::hasRiseVoltage())
    CIRCUIT_FPRINTF(fout, "    RISEVOLTAGETHRESHOLD %g ;\n",
                    pin->lefiPin::riseVoltage());
  if(pin->lefiPin::hasFallVoltage())
    CIRCUIT_FPRINTF(fout, "    FALLVOLTAGETHRESHOLD %g ;\n",
                    pin->lefiPin::fallVoltage());
  if(pin->lefiPin::hasRiseThresh())
    CIRCUIT_FPRINTF(fout, "    RISETHRESH %g ;\n", pin->lefiPin::riseThresh());
  if(pin->lefiPin::hasFallThresh())
    CIRCUIT_FPRINTF(fout, "    FALLTHRESH %g ;\n", pin->lefiPin::fallThresh());
  if(pin->lefiPin::hasRiseSatcur())
    CIRCUIT_FPRINTF(fout, "    RISESATCUR %g ;\n", pin->lefiPin::riseSatcur());
  if(pin->lefiPin::hasFallSatcur())
    CIRCUIT_FPRINTF(fout, "    FALLSATCUR %g ;\n", pin->lefiPin::fallSatcur());
  if(pin->lefiPin::hasRiseSlewLimit())
    CIRCUIT_FPRINTF(fout, "    RISESLEWLIMIT %g ;\n",
                    pin->lefiPin::riseSlewLimit());
  if(pin->lefiPin::hasFallSlewLimit())
    CIRCUIT_FPRINTF(fout, "    FALLSLEWLIMIT %g ;\n",
                    pin->lefiPin::fallSlewLimit());
  if(pin->lefiPin::hasCurrentSource())
    CIRCUIT_FPRINTF(fout, "    CURRENTSOURCE %s ;\n",
                    pin->lefiPin::currentSource());
  if(pin->lefiPin::hasTables())
    CIRCUIT_FPRINTF(fout, "    IV_TABLES %s %s ;\n",
                    pin->lefiPin::tableHighName(),
                    pin->lefiPin::tableLowName());
  if(pin->lefiPin::hasTaperRule())
    CIRCUIT_FPRINTF(fout, "    TAPERRULE %s ;\n", pin->lefiPin::taperRule());
  if(pin->lefiPin::hasNetExpr())
    CIRCUIT_FPRINTF(fout, "    NETEXPR \"%s\" ;\n", pin->lefiPin::netExpr());
  if(pin->lefiPin::hasSupplySensitivity())
    CIRCUIT_FPRINTF(fout, "    SUPPLYSENSITIVITY %s ;\n",
                    pin->lefiPin::supplySensitivity());
  if(pin->lefiPin::hasGroundSensitivity())
    CIRCUIT_FPRINTF(fout, "    GROUNDSENSITIVITY %s ;\n",
                    pin->lefiPin::groundSensitivity());
  if(pin->lefiPin::hasAntennaSize()) {
    for(i = 0; i < pin->lefiPin::numAntennaSize(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNASIZE %g ",
                      pin->lefiPin::antennaSize(i));
      if(pin->lefiPin::antennaSizeLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ", pin->lefiPin::antennaSizeLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALAREA %g ",
                      pin->lefiPin::antennaMetalArea(i));
      if(pin->lefiPin::antennaMetalAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalLength()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalLength(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALLENGTH %g ",
                      pin->lefiPin::antennaMetalLength(i));
      if(pin->lefiPin::antennaMetalLengthLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaMetalLengthLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALAREA %g ",
                      pin->lefiPin::antennaPartialMetalArea(i));
      if(pin->lefiPin::antennaPartialMetalAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaPartialMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalSideArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalSideArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALSIDEAREA %g ",
                      pin->lefiPin::antennaPartialMetalSideArea(i));
      if(pin->lefiPin::antennaPartialMetalSideAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaPartialMetalSideAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialCutArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialCutArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALCUTAREA %g ",
                      pin->lefiPin::antennaPartialCutArea(i));
      if(pin->lefiPin::antennaPartialCutAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaPartialCutAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaDiffArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaDiffArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNADIFFAREA %g ",
                      pin->lefiPin::antennaDiffArea(i));
      if(pin->lefiPin::antennaDiffAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaDiffAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  for(j = 0; j < pin->lefiPin::numAntennaModel(); j++) {
    aModel = pin->lefiPin::antennaModel(j);

    CIRCUIT_FPRINTF(fout, "    ANTENNAMODEL %s ;\n",
                    aModel->lefiPinAntennaModel::antennaOxide());

    if(aModel->lefiPinAntennaModel::hasAntennaGateArea()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaGateArea(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAGATEAREA %g ",
                        aModel->lefiPinAntennaModel::antennaGateArea(i));
        if(aModel->lefiPinAntennaModel::antennaGateAreaLayer(i))
          CIRCUIT_FPRINTF(fout, "LAYER %s ",
                          aModel->lefiPinAntennaModel::antennaGateAreaLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxAreaCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i))
          CIRCUIT_FPRINTF(
              fout, "LAYER %s ",
              aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxSideAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxSideAreaCar();
          i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXSIDEAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxSideAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i))
          CIRCUIT_FPRINTF(
              fout, "LAYER %s ",
              aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxCutCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxCutCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXCUTCAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxCutCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i))
          CIRCUIT_FPRINTF(
              fout, "LAYER %s ",
              aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }

  if(pin->lefiPin::numProperties() > 0) {
    CIRCUIT_FPRINTF(fout, "    PROPERTY ");
    for(i = 0; i < pin->lefiPin::numProperties(); i++) {
      // value can either be a string or number
      if(pin->lefiPin::propValue(i)) {
        CIRCUIT_FPRINTF(fout, "%s %s ", pin->lefiPin::propName(i),
                        pin->lefiPin::propValue(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "%s %g ", pin->lefiPin::propName(i),
                        pin->lefiPin::propNum(i));
      }
      switch(pin->lefiPin::propType(i)) {
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

  numPorts = pin->lefiPin::numPorts();
  for(i = 0; i < numPorts; i++) {
    CIRCUIT_FPRINTF(fout, "    PORT\n");
    fflush(stdout);
    geometry = pin->lefiPin::port(i);
    prtGeometry(geometry);
    CIRCUIT_FPRINTF(fout, "    END\n");
  }
  CIRCUIT_FPRINTF(fout, "  END %s\n", pin->lefiPin::name());
  return 0;
}

int densityCB(lefrCallbackType_e c, lefiDensity* density, lefiUserData) {
  struct lefiGeomRect rect;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  DENSITY\n");
  for(int i = 0; i < density->lefiDensity::numLayer(); i++) {
    CIRCUIT_FPRINTF(fout, "    LAYER %s ;\n",
                    density->lefiDensity::layerName(i));
    for(int j = 0; j < density->lefiDensity::numRects(i); j++) {
      rect = density->lefiDensity::getRect(i, j);
      CIRCUIT_FPRINTF(fout, "      RECT %g %g %g %g ", rect.xl, rect.yl,
                      rect.xh, rect.yh);
      CIRCUIT_FPRINTF(fout, "%g ;\n", density->lefiDensity::densityValue(i, j));
    }
  }
  CIRCUIT_FPRINTF(fout, "  END\n");
  return 0;
}

int propDefBeginCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "PROPERTYDEFINITIONS\n");
  return 0;
}

int propDefCB(lefrCallbackType_e c, lefiProp* prop, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, " %s %s", prop->lefiProp::propType(),
                  prop->lefiProp::propName());
  switch(prop->lefiProp::dataType()) {
    case 'I':
      CIRCUIT_FPRINTF(fout, " INTEGER");
      break;
    case 'R':
      CIRCUIT_FPRINTF(fout, " REAL");
      break;
    case 'S':
      CIRCUIT_FPRINTF(fout, " STRING");
      break;
  }
  if(prop->lefiProp::hasNumber())
    CIRCUIT_FPRINTF(fout, " %g", prop->lefiProp::number());
  if(prop->lefiProp::hasRange())
    CIRCUIT_FPRINTF(fout, " RANGE %g %g", prop->lefiProp::left(),
                    prop->lefiProp::right());
  if(prop->lefiProp::hasString())
    CIRCUIT_FPRINTF(fout, " %s", prop->lefiProp::string());
  CIRCUIT_FPRINTF(fout, "\n");
  return 0;
}

int propDefEndCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END PROPERTYDEFINITIONS\n");
  return 0;
}

int siteCB(lefrCallbackType_e c, lefiSite* site, lefiUserData) {
  int hasPrtSym = 0;
  int i;

  checkType(c);
  (*lefSiteMapPtr)[string(site->name())] = lefSiteStorPtr->size();
  lefSiteStorPtr->push_back(*site);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "SITE %s\n", site->lefiSite::name());
  if(site->lefiSite::hasClass())
    CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", site->lefiSite::siteClass());
  if(site->lefiSite::hasXSymmetry()) {
    CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
    hasPrtSym = 1;
  }
  if(site->lefiSite::hasYSymmetry()) {
    if(hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "Y ");
    }
    else {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
      hasPrtSym = 1;
    }
  }
  if(site->lefiSite::has90Symmetry()) {
    if(hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "R90 ");
    }
    else {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
      hasPrtSym = 1;
    }
  }
  if(hasPrtSym)
    CIRCUIT_FPRINTF(fout, ";\n");
  if(site->lefiSite::hasSize())
    CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", site->lefiSite::sizeX(),
                    site->lefiSite::sizeY());

  if(site->hasRowPattern()) {
    CIRCUIT_FPRINTF(fout, "  ROWPATTERN ");
    for(i = 0; i < site->lefiSite::numSites(); i++)
      CIRCUIT_FPRINTF(fout, "  %s %s ", site->lefiSite::siteName(i),
                      site->lefiSite::siteOrientStr(i));
    CIRCUIT_FPRINTF(fout, ";\n");
  }

  CIRCUIT_FPRINTF(fout, "END %s\n", site->lefiSite::name());
  return 0;
}

int spacingBeginCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "SPACING\n");
  return 0;
}

int spacingCB(lefrCallbackType_e c, lefiSpacing* spacing, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  lefSpacing(spacing);
  return 0;
}

int spacingEndCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END SPACING\n");
  return 0;
}

int timingCB(lefrCallbackType_e c, lefiTiming* timing, lefiUserData) {
  int i;
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "TIMING\n");
  for(i = 0; i < timing->numFromPins(); i++)
    CIRCUIT_FPRINTF(fout, " FROMPIN %s ;\n", timing->fromPin(i));
  for(i = 0; i < timing->numToPins(); i++)
    CIRCUIT_FPRINTF(fout, " TOPIN %s ;\n", timing->toPin(i));
  CIRCUIT_FPRINTF(fout, " RISE SLEW1 %g %g %g %g ;\n", timing->riseSlewOne(),
                  timing->riseSlewTwo(), timing->riseSlewThree(),
                  timing->riseSlewFour());
  if(timing->hasRiseSlew2())
    CIRCUIT_FPRINTF(fout, " RISE SLEW2 %g %g %g ;\n", timing->riseSlewFive(),
                    timing->riseSlewSix(), timing->riseSlewSeven());
  if(timing->hasFallSlew())
    CIRCUIT_FPRINTF(fout, " FALL SLEW1 %g %g %g %g ;\n", timing->fallSlewOne(),
                    timing->fallSlewTwo(), timing->fallSlewThree(),
                    timing->fallSlewFour());
  if(timing->hasFallSlew2())
    CIRCUIT_FPRINTF(fout, " FALL SLEW2 %g %g %g ;\n", timing->fallSlewFive(),
                    timing->fallSlewSix(), timing->riseSlewSeven());
  if(timing->hasRiseIntrinsic()) {
    CIRCUIT_FPRINTF(fout, "TIMING RISE INTRINSIC %g %g ;\n",
                    timing->riseIntrinsicOne(), timing->riseIntrinsicTwo());
    CIRCUIT_FPRINTF(fout, "TIMING RISE VARIABLE %g %g ;\n",
                    timing->riseIntrinsicThree(), timing->riseIntrinsicFour());
  }
  if(timing->hasFallIntrinsic()) {
    CIRCUIT_FPRINTF(fout, "TIMING FALL INTRINSIC %g %g ;\n",
                    timing->fallIntrinsicOne(), timing->fallIntrinsicTwo());
    CIRCUIT_FPRINTF(fout, "TIMING RISE VARIABLE %g %g ;\n",
                    timing->fallIntrinsicThree(), timing->fallIntrinsicFour());
  }
  if(timing->hasRiseRS())
    CIRCUIT_FPRINTF(fout, "TIMING RISERS %g %g ;\n", timing->riseRSOne(),
                    timing->riseRSTwo());
  if(timing->hasRiseCS())
    CIRCUIT_FPRINTF(fout, "TIMING RISECS %g %g ;\n", timing->riseCSOne(),
                    timing->riseCSTwo());
  if(timing->hasFallRS())
    CIRCUIT_FPRINTF(fout, "TIMING FALLRS %g %g ;\n", timing->fallRSOne(),
                    timing->fallRSTwo());
  if(timing->hasFallCS())
    CIRCUIT_FPRINTF(fout, "TIMING FALLCS %g %g ;\n", timing->fallCSOne(),
                    timing->fallCSTwo());
  if(timing->hasUnateness())
    CIRCUIT_FPRINTF(fout, "TIMING UNATENESS %s ;\n", timing->unateness());
  if(timing->hasRiseAtt1())
    CIRCUIT_FPRINTF(fout, "TIMING RISESATT1 %g %g ;\n", timing->riseAtt1One(),
                    timing->riseAtt1Two());
  if(timing->hasFallAtt1())
    CIRCUIT_FPRINTF(fout, "TIMING FALLSATT1 %g %g ;\n", timing->fallAtt1One(),
                    timing->fallAtt1Two());
  if(timing->hasRiseTo())
    CIRCUIT_FPRINTF(fout, "TIMING RISET0 %g %g ;\n", timing->riseToOne(),
                    timing->riseToTwo());
  if(timing->hasFallTo())
    CIRCUIT_FPRINTF(fout, "TIMING FALLT0 %g %g ;\n", timing->fallToOne(),
                    timing->fallToTwo());
  if(timing->hasSDFonePinTrigger())
    CIRCUIT_FPRINTF(fout, " %s TABLEDIMENSION %g %g %g ;\n",
                    timing->SDFonePinTriggerType(), timing->SDFtriggerOne(),
                    timing->SDFtriggerTwo(), timing->SDFtriggerThree());
  if(timing->hasSDFtwoPinTrigger())
    CIRCUIT_FPRINTF(fout, " %s %s %s TABLEDIMENSION %g %g %g ;\n",
                    timing->SDFtwoPinTriggerType(), timing->SDFfromTrigger(),
                    timing->SDFtoTrigger(), timing->SDFtriggerOne(),
                    timing->SDFtriggerTwo(), timing->SDFtriggerThree());
  CIRCUIT_FPRINTF(fout, "END TIMING\n");
  return 0;
}

int unitsCB(lefrCallbackType_e c, lefiUnits* unit, lefiUserData) {
  checkType(c);
  *lefUnitPtr = (*unit);
  // if ((long)ud != userData) dataError();

  *lefUnitPtr = *unit;
  CIRCUIT_FPRINTF(fout, "UNITS\n");
  if(unit->lefiUnits::hasDatabase())
    CIRCUIT_FPRINTF(fout, "  DATABASE %s %g ;\n",
                    unit->lefiUnits::databaseName(),
                    unit->lefiUnits::databaseNumber());
  if(unit->lefiUnits::hasCapacitance())
    CIRCUIT_FPRINTF(fout, "  CAPACITANCE PICOFARADS %g ;\n",
                    unit->lefiUnits::capacitance());
  if(unit->lefiUnits::hasResistance())
    CIRCUIT_FPRINTF(fout, "  RESISTANCE OHMS %g ;\n",
                    unit->lefiUnits::resistance());
  if(unit->lefiUnits::hasPower())
    CIRCUIT_FPRINTF(fout, "  POWER MILLIWATTS %g ;\n",
                    unit->lefiUnits::power());
  if(unit->lefiUnits::hasCurrent())
    CIRCUIT_FPRINTF(fout, "  CURRENT MILLIAMPS %g ;\n",
                    unit->lefiUnits::current());
  if(unit->lefiUnits::hasVoltage())
    CIRCUIT_FPRINTF(fout, "  VOLTAGE VOLTS %g ;\n", unit->lefiUnits::voltage());
  if(unit->lefiUnits::hasFrequency())
    CIRCUIT_FPRINTF(fout, "  FREQUENCY MEGAHERTZ %g ;\n",
                    unit->lefiUnits::frequency());
  CIRCUIT_FPRINTF(fout, "END UNITS\n");
  return 0;
}

int useMinSpacingCB(lefrCallbackType_e c, lefiUseMinSpacing* spacing,
                    lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "USEMINSPACING %s ",
                  spacing->lefiUseMinSpacing::name());
  if(spacing->lefiUseMinSpacing::value()) {
    CIRCUIT_FPRINTF(fout, "ON ;\n");
  }
  else {
    CIRCUIT_FPRINTF(fout, "OFF ;\n");
  }
  return 0;
}

int versionCB(lefrCallbackType_e c, double num, lefiUserData) {
  checkType(c);

  *lefVersionPtr = num;
  CIRCUIT_FPRINTF(fout, "VERSION %g ;\n", num);
  return 0;
}

int versionStrCB(lefrCallbackType_e c, const char* versionName, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "VERSION %s ;\n", versionName);
  return 0;
}

int viaCB(lefrCallbackType_e c, lefiVia* via, lefiUserData) {
  checkType(c);

  (*lefViaMapPtr)[string(via->name())] = lefViaStorPtr->size();
  lefViaStorPtr->push_back(*via);
  // if ((long)ud != userData) dataError();
  lefVia(via);
  return 0;
}

int viaRuleCB(lefrCallbackType_e c, lefiViaRule* viaRule, lefiUserData) {
  int numLayers, numVias, i;
  lefiViaRuleLayer* vLayer;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "VIARULE %s", viaRule->lefiViaRule::name());
  if(viaRule->lefiViaRule::hasGenerate())
    CIRCUIT_FPRINTF(fout, " GENERATE");
  if(viaRule->lefiViaRule::hasDefault())
    CIRCUIT_FPRINTF(fout, " DEFAULT");
  CIRCUIT_FPRINTF(fout, "\n");

  numLayers = viaRule->lefiViaRule::numLayers();
  // if numLayers == 2, it is VIARULE without GENERATE and has via name
  // if numLayers == 3, it is VIARULE with GENERATE, and the 3rd layer is cut
  for(i = 0; i < numLayers; i++) {
    vLayer = viaRule->lefiViaRule::layer(i);
    lefViaRuleLayer(vLayer);
  }

  if(numLayers == 2 && !(viaRule->lefiViaRule::hasGenerate())) {
    // should have vianames
    numVias = viaRule->lefiViaRule::numVias();
    if(numVias == 0) {
      CIRCUIT_FPRINTF(fout, "Should have via names in VIARULE.\n");
    }
    else {
      for(i = 0; i < numVias; i++)
        CIRCUIT_FPRINTF(fout, "  VIA %s ;\n", viaRule->lefiViaRule::viaName(i));
    }
  }
  if(viaRule->lefiViaRule::numProps() > 0) {
    CIRCUIT_FPRINTF(fout, "  PROPERTY ");
    for(i = 0; i < viaRule->lefiViaRule::numProps(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", viaRule->lefiViaRule::propName(i));
      if(viaRule->lefiViaRule::propValue(i))
        CIRCUIT_FPRINTF(fout, "%s ", viaRule->lefiViaRule::propValue(i));
      switch(viaRule->lefiViaRule::propType(i)) {
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
  CIRCUIT_FPRINTF(fout, "END %s\n", viaRule->lefiViaRule::name());
  return 0;
}

int extensionCB(lefrCallbackType_e c, const char* extsn, lefiUserData) {
  checkType(c);
  // lefrSetCaseSensitivity(0);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "BEGINEXT %s ;\n", extsn);
  // lefrSetCaseSensitivity(1);
  return 0;
}

int doneCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END LIBRARY\n");
  return 0;
}

void errorCB(const char* msg) {
  printf("%s : %s\n", lefrGetUserData(), (char*)msg);
}

void warningCB(const char* msg) {
  printf("%s : %s\n", lefrGetUserData(), (char*)msg);
}

void* mallocCB(int size) {
  return malloc(size);
}

void* reallocCB(void* name, int size) {
  return realloc(name, size);
}

void freeCB(void* name) {
  free(name);
  return;
}

void lineNumberCB(int lineNo) {
  cout << "[LEF] Parsed " << lineNo << " number of lines!!" << endl;
}

void printWarning(const char* str) {
  CIRCUIT_FPRINTF(stderr, "%s\n", str);
}

//////////
// Print Function
//

void Replace::Circuit::DumpLefVersion() {
  CIRCUIT_FPRINTF(fout, "VERSION %g ;\n", lefVersion);
}

void Replace::Circuit::DumpLefBusBitChar() {
  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n", lefBusBitChar.c_str());
}

void Replace::Circuit::DumpLefDivider() {
  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n\n", lefDivider.c_str());
}

void Replace::Circuit::DumpLefUnit() {
  //    lefiUnits* unit = &lefUnit;
  CIRCUIT_FPRINTF(fout, "UNITS\n");
  if(lefUnit.lefiUnits::hasDatabase())
    CIRCUIT_FPRINTF(fout, "  DATABASE %s %g ;\n",
                    lefUnit.lefiUnits::databaseName(),
                    lefUnit.lefiUnits::databaseNumber());
  if(lefUnit.lefiUnits::hasCapacitance())
    CIRCUIT_FPRINTF(fout, "  CAPACITANCE PICOFARADS %g ;\n",
                    lefUnit.lefiUnits::capacitance());
  if(lefUnit.lefiUnits::hasResistance())
    CIRCUIT_FPRINTF(fout, "  RESISTANCE OHMS %g ;\n",
                    lefUnit.lefiUnits::resistance());
  if(lefUnit.lefiUnits::hasPower())
    CIRCUIT_FPRINTF(fout, "  POWER MILLIWATTS %g ;\n",
                    lefUnit.lefiUnits::power());
  if(lefUnit.lefiUnits::hasCurrent())
    CIRCUIT_FPRINTF(fout, "  CURRENT MILLIAMPS %g ;\n",
                    lefUnit.lefiUnits::current());
  if(lefUnit.lefiUnits::hasVoltage())
    CIRCUIT_FPRINTF(fout, "  VOLTAGE VOLTS %g ;\n",
                    lefUnit.lefiUnits::voltage());
  if(lefUnit.lefiUnits::hasFrequency())
    CIRCUIT_FPRINTF(fout, "  FREQUENCY MEGAHERTZ %g ;\n",
                    lefUnit.lefiUnits::frequency());
  CIRCUIT_FPRINTF(fout, "END UNITS\n\n");
}

void Replace::Circuit::DumpLefManufacturingGrid() {
  if(lefManufacturingGrid != DBL_MIN) {
    CIRCUIT_FPRINTF(fout, "MANUFACTURINGGRID %g ;\n\n", lefManufacturingGrid);
  }
}

void Replace::Circuit::DumpLefLayer() {
  if(lefLayerStor.size() == 0) {
    return;
  }

  int i, j, k;
  int numPoints, propNum;
  double *widths, *current;
  lefiLayerDensity* density;
  lefiAntennaPWL* pwl;
  lefiSpacingTable* spTable;
  lefiInfluence* influence;
  lefiParallel* parallel;
  lefiTwoWidths* twoWidths;
  char pType;
  int numMinCut, numMinenclosed;
  lefiAntennaModel* aModel;
  lefiOrthogonal* ortho;

  lefrSetCaseSensitivity(0);
  for(auto& curLayer : lefLayerStor) {
    // Call parse65nmRules for 5.7 syntax in 5.6
    if(parse65nm)
      curLayer.lefiLayer::parse65nmRules();

    // Call parseLef58Type for 5.8 syntax in 5.7
    if(parseLef58Type)
      curLayer.lefiLayer::parseLEF58Layer();

    CIRCUIT_FPRINTF(fout, "LAYER %s\n", curLayer.lefiLayer::name());
    if(curLayer.lefiLayer::hasType())
      CIRCUIT_FPRINTF(fout, "  TYPE %s ;\n", curLayer.lefiLayer::type());
    if(curLayer.lefiLayer::hasLayerType())
      CIRCUIT_FPRINTF(fout, "  LAYER TYPE %s ;\n",
                      curLayer.lefiLayer::layerType());
    if(curLayer.lefiLayer::hasMask())
      CIRCUIT_FPRINTF(fout, "  MASK %d ;\n", curLayer.lefiLayer::mask());
    if(curLayer.lefiLayer::hasPitch()) {
      CIRCUIT_FPRINTF(fout, "  PITCH %g ;\n", curLayer.lefiLayer::pitch());
    }
    else if(curLayer.lefiLayer::hasXYPitch()) {
      CIRCUIT_FPRINTF(fout, "  PITCH %g %g ;\n", curLayer.lefiLayer::pitchX(),
                      curLayer.lefiLayer::pitchY());
    }
    if(curLayer.lefiLayer::hasOffset()) {
      CIRCUIT_FPRINTF(fout, "  OFFSET %g ;\n", curLayer.lefiLayer::offset());
    }
    else if(curLayer.lefiLayer::hasXYOffset()) {
      CIRCUIT_FPRINTF(fout, "  OFFSET %g %g ;\n", curLayer.lefiLayer::offsetX(),
                      curLayer.lefiLayer::offsetY());
    }
    if(curLayer.lefiLayer::hasDiagPitch()) {
      CIRCUIT_FPRINTF(fout, "  DIAGPITCH %g ;\n",
                      curLayer.lefiLayer::diagPitch());
    }
    else if(curLayer.lefiLayer::hasXYDiagPitch()) {
      CIRCUIT_FPRINTF(fout, "  DIAGPITCH %g %g ;\n",
                      curLayer.lefiLayer::diagPitchX(),
                      curLayer.lefiLayer::diagPitchY());
    }
    if(curLayer.lefiLayer::hasDiagWidth())
      CIRCUIT_FPRINTF(fout, "  DIAGWIDTH %g ;\n",
                      curLayer.lefiLayer::diagWidth());
    if(curLayer.lefiLayer::hasDiagSpacing())
      CIRCUIT_FPRINTF(fout, "  DIAGSPACING %g ;\n",
                      curLayer.lefiLayer::diagSpacing());
    if(curLayer.lefiLayer::hasWidth())
      CIRCUIT_FPRINTF(fout, "  WIDTH %g ;\n", curLayer.lefiLayer::width());
    if(curLayer.lefiLayer::hasArea())
      CIRCUIT_FPRINTF(fout, "  AREA %g ;\n", curLayer.lefiLayer::area());
    if(curLayer.lefiLayer::hasSlotWireWidth())
      CIRCUIT_FPRINTF(fout, "  SLOTWIREWIDTH %g ;\n",
                      curLayer.lefiLayer::slotWireWidth());
    if(curLayer.lefiLayer::hasSlotWireLength())
      CIRCUIT_FPRINTF(fout, "  SLOTWIRELENGTH %g ;\n",
                      curLayer.lefiLayer::slotWireLength());
    if(curLayer.lefiLayer::hasSlotWidth())
      CIRCUIT_FPRINTF(fout, "  SLOTWIDTH %g ;\n",
                      curLayer.lefiLayer::slotWidth());
    if(curLayer.lefiLayer::hasSlotLength())
      CIRCUIT_FPRINTF(fout, "  SLOTLENGTH %g ;\n",
                      curLayer.lefiLayer::slotLength());
    if(curLayer.lefiLayer::hasMaxAdjacentSlotSpacing())
      CIRCUIT_FPRINTF(fout, "  MAXADJACENTSLOTSPACING %g ;\n",
                      curLayer.lefiLayer::maxAdjacentSlotSpacing());
    if(curLayer.lefiLayer::hasMaxCoaxialSlotSpacing())
      CIRCUIT_FPRINTF(fout, "  MAXCOAXIALSLOTSPACING %g ;\n",
                      curLayer.lefiLayer::maxCoaxialSlotSpacing());
    if(curLayer.lefiLayer::hasMaxEdgeSlotSpacing())
      CIRCUIT_FPRINTF(fout, "  MAXEDGESLOTSPACING %g ;\n",
                      curLayer.lefiLayer::maxEdgeSlotSpacing());
    if(curLayer.lefiLayer::hasMaxFloatingArea())  // 5.7
      CIRCUIT_FPRINTF(fout, "  MAXFLOATINGAREA %g ;\n",
                      curLayer.lefiLayer::maxFloatingArea());
    if(curLayer.lefiLayer::hasArraySpacing()) {  // 5.7
      CIRCUIT_FPRINTF(fout, "  ARRAYSPACING ");
      if(curLayer.lefiLayer::hasLongArray())
        CIRCUIT_FPRINTF(fout, "LONGARRAY ");
      if(curLayer.lefiLayer::hasViaWidth())
        CIRCUIT_FPRINTF(fout, "WIDTH %g ", curLayer.lefiLayer::viaWidth());
      CIRCUIT_FPRINTF(fout, "CUTSPACING %g", curLayer.lefiLayer::cutSpacing());
      for(i = 0; i < curLayer.lefiLayer::numArrayCuts(); i++)
        CIRCUIT_FPRINTF(fout, "\n\tARRAYCUTS %d SPACING %g",
                        curLayer.lefiLayer::arrayCuts(i),
                        curLayer.lefiLayer::arraySpacing(i));
      CIRCUIT_FPRINTF(fout, " ;\n");
    }
    if(curLayer.lefiLayer::hasSplitWireWidth())
      CIRCUIT_FPRINTF(fout, "  SPLITWIREWIDTH %g ;\n",
                      curLayer.lefiLayer::splitWireWidth());
    if(curLayer.lefiLayer::hasMinimumDensity())
      CIRCUIT_FPRINTF(fout, "  MINIMUMDENSITY %g ;\n",
                      curLayer.lefiLayer::minimumDensity());
    if(curLayer.lefiLayer::hasMaximumDensity())
      CIRCUIT_FPRINTF(fout, "  MAXIMUMDENSITY %g ;\n",
                      curLayer.lefiLayer::maximumDensity());
    if(curLayer.lefiLayer::hasDensityCheckWindow())
      CIRCUIT_FPRINTF(fout, "  DENSITYCHECKWINDOW %g %g ;\n",
                      curLayer.lefiLayer::densityCheckWindowLength(),
                      curLayer.lefiLayer::densityCheckWindowWidth());
    if(curLayer.lefiLayer::hasDensityCheckStep())
      CIRCUIT_FPRINTF(fout, "  DENSITYCHECKSTEP %g ;\n",
                      curLayer.lefiLayer::densityCheckStep());
    if(curLayer.lefiLayer::hasFillActiveSpacing())
      CIRCUIT_FPRINTF(fout, "  FILLACTIVESPACING %g ;\n",
                      curLayer.lefiLayer::fillActiveSpacing());
    // 5.4.1
    numMinCut = curLayer.lefiLayer::numMinimumcut();
    if(numMinCut > 0) {
      for(i = 0; i < numMinCut; i++) {
        CIRCUIT_FPRINTF(fout, "  MINIMUMCUT %d WIDTH %g ",
                        curLayer.lefiLayer::minimumcut(i),
                        curLayer.lefiLayer::minimumcutWidth(i));
        if(curLayer.lefiLayer::hasMinimumcutWithin(i))
          CIRCUIT_FPRINTF(fout, "WITHIN %g ",
                          curLayer.lefiLayer::minimumcutWithin(i));
        if(curLayer.lefiLayer::hasMinimumcutConnection(i))
          CIRCUIT_FPRINTF(fout, "%s ",
                          curLayer.lefiLayer::minimumcutConnection(i));
        if(curLayer.lefiLayer::hasMinimumcutNumCuts(i))
          CIRCUIT_FPRINTF(fout, "LENGTH %g WITHIN %g ",
                          curLayer.lefiLayer::minimumcutLength(i),
                          curLayer.lefiLayer::minimumcutDistance(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    // 5.4.1
    if(curLayer.lefiLayer::hasMaxwidth()) {
      CIRCUIT_FPRINTF(fout, "  MAXWIDTH %g ;\n",
                      curLayer.lefiLayer::maxwidth());
    }
    // 5.5
    if(curLayer.lefiLayer::hasMinwidth()) {
      CIRCUIT_FPRINTF(fout, "  MINWIDTH %g ;\n",
                      curLayer.lefiLayer::minwidth());
    }
    // 5.5
    numMinenclosed = curLayer.lefiLayer::numMinenclosedarea();
    if(numMinenclosed > 0) {
      for(i = 0; i < numMinenclosed; i++) {
        CIRCUIT_FPRINTF(fout, "  MINENCLOSEDAREA %g ",
                        curLayer.lefiLayer::minenclosedarea(i));
        if(curLayer.lefiLayer::hasMinenclosedareaWidth(i))
          CIRCUIT_FPRINTF(fout, "MINENCLOSEDAREAWIDTH %g ",
                          curLayer.lefiLayer::minenclosedareaWidth(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    // 5.4.1 & 5.6
    if(curLayer.lefiLayer::hasMinstep()) {
      for(i = 0; i < curLayer.lefiLayer::numMinstep(); i++) {
        CIRCUIT_FPRINTF(fout, "  MINSTEP %g ", curLayer.lefiLayer::minstep(i));
        if(curLayer.lefiLayer::hasMinstepType(i))
          CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::minstepType(i));
        if(curLayer.lefiLayer::hasMinstepLengthsum(i))
          CIRCUIT_FPRINTF(fout, "LENGTHSUM %g ",
                          curLayer.lefiLayer::minstepLengthsum(i));
        if(curLayer.lefiLayer::hasMinstepMaxedges(i))
          CIRCUIT_FPRINTF(fout, "MAXEDGES %d ",
                          curLayer.lefiLayer::minstepMaxedges(i));
        if(curLayer.lefiLayer::hasMinstepMinAdjLength(i))
          CIRCUIT_FPRINTF(fout, "MINADJLENGTH %g ",
                          curLayer.lefiLayer::minstepMinAdjLength(i));
        if(curLayer.lefiLayer::hasMinstepMinBetLength(i))
          CIRCUIT_FPRINTF(fout, "MINBETLENGTH %g ",
                          curLayer.lefiLayer::minstepMinBetLength(i));
        if(curLayer.lefiLayer::hasMinstepXSameCorners(i))
          CIRCUIT_FPRINTF(fout, "XSAMECORNERS");
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    // 5.4.1
    if(curLayer.lefiLayer::hasProtrusion()) {
      CIRCUIT_FPRINTF(fout, "  PROTRUSIONWIDTH %g LENGTH %g WIDTH %g ;\n",
                      curLayer.lefiLayer::protrusionWidth1(),
                      curLayer.lefiLayer::protrusionLength(),
                      curLayer.lefiLayer::protrusionWidth2());
    }
    if(curLayer.lefiLayer::hasSpacingNumber()) {
      for(i = 0; i < curLayer.lefiLayer::numSpacing(); i++) {
        CIRCUIT_FPRINTF(fout, "  SPACING %g ", curLayer.lefiLayer::spacing(i));
        if(curLayer.lefiLayer::hasSpacingName(i))
          CIRCUIT_FPRINTF(fout, "LAYER %s ",
                          curLayer.lefiLayer::spacingName(i));
        if(curLayer.lefiLayer::hasSpacingLayerStack(i))
          CIRCUIT_FPRINTF(fout, "STACK ");  // 5.7
        if(curLayer.lefiLayer::hasSpacingAdjacent(i))
          CIRCUIT_FPRINTF(fout, "ADJACENTCUTS %d WITHIN %g ",
                          curLayer.lefiLayer::spacingAdjacentCuts(i),
                          curLayer.lefiLayer::spacingAdjacentWithin(i));
        if(curLayer.lefiLayer::hasSpacingAdjacentExcept(i))  // 5.7
          CIRCUIT_FPRINTF(fout, "EXCEPTSAMEPGNET ");
        if(curLayer.lefiLayer::hasSpacingCenterToCenter(i))
          CIRCUIT_FPRINTF(fout, "CENTERTOCENTER ");
        if(curLayer.lefiLayer::hasSpacingSamenet(i))  // 5.7
          CIRCUIT_FPRINTF(fout, "SAMENET ");
        if(curLayer.lefiLayer::hasSpacingSamenetPGonly(i))  // 5.7
          CIRCUIT_FPRINTF(fout, "PGONLY ");
        if(curLayer.lefiLayer::hasSpacingArea(i))  // 5.7
          CIRCUIT_FPRINTF(fout, "AREA %g ", curLayer.lefiLayer::spacingArea(i));
        if(curLayer.lefiLayer::hasSpacingRange(i)) {
          CIRCUIT_FPRINTF(fout, "RANGE %g %g ",
                          curLayer.lefiLayer::spacingRangeMin(i),
                          curLayer.lefiLayer::spacingRangeMax(i));
          if(curLayer.lefiLayer::hasSpacingRangeUseLengthThreshold(i)) {
            CIRCUIT_FPRINTF(fout, "USELENGTHTHRESHOLD ");
          }
          else if(curLayer.lefiLayer::hasSpacingRangeInfluence(i)) {
            CIRCUIT_FPRINTF(fout, "INFLUENCE %g ",
                            curLayer.lefiLayer::spacingRangeInfluence(i));
            if(curLayer.lefiLayer::hasSpacingRangeInfluenceRange(i)) {
              CIRCUIT_FPRINTF(fout, "RANGE %g %g ",
                              curLayer.lefiLayer::spacingRangeInfluenceMin(i),
                              curLayer.lefiLayer::spacingRangeInfluenceMax(i));
            }
          }
          else if(curLayer.lefiLayer::hasSpacingRangeRange(i)) {
            CIRCUIT_FPRINTF(fout, "RANGE %g %g ",
                            curLayer.lefiLayer::spacingRangeRangeMin(i),
                            curLayer.lefiLayer::spacingRangeRangeMax(i));
          }
        }
        else if(curLayer.lefiLayer::hasSpacingLengthThreshold(i)) {
          CIRCUIT_FPRINTF(fout, "LENGTHTHRESHOLD %g ",
                          curLayer.lefiLayer::spacingLengthThreshold(i));
          if(curLayer.lefiLayer::hasSpacingLengthThresholdRange(i))
            CIRCUIT_FPRINTF(
                fout, "RANGE %g %g",
                curLayer.lefiLayer::spacingLengthThresholdRangeMin(i),
                curLayer.lefiLayer::spacingLengthThresholdRangeMax(i));
        }
        else if(curLayer.lefiLayer::hasSpacingNotchLength(i)) {  // 5.7
          CIRCUIT_FPRINTF(fout, "NOTCHLENGTH %g",
                          curLayer.lefiLayer::spacingNotchLength(i));
        }
        else if(curLayer.lefiLayer::hasSpacingEndOfNotchWidth(i))  // 5.7
          CIRCUIT_FPRINTF(fout,
                          "ENDOFNOTCHWIDTH %g NOTCHSPACING %g, NOTCHLENGTH %g",
                          curLayer.lefiLayer::spacingEndOfNotchWidth(i),
                          curLayer.lefiLayer::spacingEndOfNotchSpacing(i),
                          curLayer.lefiLayer::spacingEndOfNotchLength(i));

        if(curLayer.lefiLayer::hasSpacingParallelOverlap(i))  // 5.7
          CIRCUIT_FPRINTF(fout, "PARALLELOVERLAP ");
        if(curLayer.lefiLayer::hasSpacingEndOfLine(i)) {  // 5.7
          CIRCUIT_FPRINTF(fout, "ENDOFLINE %g WITHIN %g ",
                          curLayer.lefiLayer::spacingEolWidth(i),
                          curLayer.lefiLayer::spacingEolWithin(i));
          if(curLayer.lefiLayer::hasSpacingParellelEdge(i)) {
            CIRCUIT_FPRINTF(fout, "PARALLELEDGE %g WITHIN %g ",
                            curLayer.lefiLayer::spacingParSpace(i),
                            curLayer.lefiLayer::spacingParWithin(i));
            if(curLayer.lefiLayer::hasSpacingTwoEdges(i)) {
              CIRCUIT_FPRINTF(fout, "TWOEDGES ");
            }
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    if(curLayer.lefiLayer::hasSpacingTableOrtho()) {  // 5.7
      CIRCUIT_FPRINTF(fout, "SPACINGTABLE ORTHOGONAL");
      ortho = curLayer.lefiLayer::orthogonal();
      for(i = 0; i < ortho->lefiOrthogonal::numOrthogonal(); i++) {
        CIRCUIT_FPRINTF(fout, "\n   WITHIN %g SPACING %g",
                        ortho->lefiOrthogonal::cutWithin(i),
                        ortho->lefiOrthogonal::orthoSpacing(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    for(i = 0; i < curLayer.lefiLayer::numEnclosure(); i++) {
      CIRCUIT_FPRINTF(fout, "ENCLOSURE ");
      if(curLayer.lefiLayer::hasEnclosureRule(i))
        CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::enclosureRule(i));
      CIRCUIT_FPRINTF(fout, "%g %g ", curLayer.lefiLayer::enclosureOverhang1(i),
                      curLayer.lefiLayer::enclosureOverhang2(i));
      if(curLayer.lefiLayer::hasEnclosureWidth(i))
        CIRCUIT_FPRINTF(fout, "WIDTH %g ",
                        curLayer.lefiLayer::enclosureMinWidth(i));
      if(curLayer.lefiLayer::hasEnclosureExceptExtraCut(i))
        CIRCUIT_FPRINTF(fout, "EXCEPTEXTRACUT %g ",
                        curLayer.lefiLayer::enclosureExceptExtraCut(i));
      if(curLayer.lefiLayer::hasEnclosureMinLength(i))
        CIRCUIT_FPRINTF(fout, "LENGTH %g ",
                        curLayer.lefiLayer::enclosureMinLength(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    for(i = 0; i < curLayer.lefiLayer::numPreferEnclosure(); i++) {
      CIRCUIT_FPRINTF(fout, "PREFERENCLOSURE ");
      if(curLayer.lefiLayer::hasPreferEnclosureRule(i))
        CIRCUIT_FPRINTF(fout, "%s ",
                        curLayer.lefiLayer::preferEnclosureRule(i));
      CIRCUIT_FPRINTF(fout, "%g %g ",
                      curLayer.lefiLayer::preferEnclosureOverhang1(i),
                      curLayer.lefiLayer::preferEnclosureOverhang2(i));
      if(curLayer.lefiLayer::hasPreferEnclosureWidth(i))
        CIRCUIT_FPRINTF(fout, "WIDTH %g ",
                        curLayer.lefiLayer::preferEnclosureMinWidth(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    if(curLayer.lefiLayer::hasResistancePerCut())
      CIRCUIT_FPRINTF(fout, "  RESISTANCE %g ;\n",
                      curLayer.lefiLayer::resistancePerCut());
    if(curLayer.lefiLayer::hasCurrentDensityPoint())
      CIRCUIT_FPRINTF(fout, "  CURRENTDEN %g ;\n",
                      curLayer.lefiLayer::currentDensityPoint());
    if(curLayer.lefiLayer::hasCurrentDensityArray()) {
      curLayer.lefiLayer::currentDensityArray(&numPoints, &widths, &current);
      for(i = 0; i < numPoints; i++)
        CIRCUIT_FPRINTF(fout, "  CURRENTDEN ( %g %g ) ;\n", widths[i],
                        current[i]);
    }
    if(curLayer.lefiLayer::hasDirection())
      CIRCUIT_FPRINTF(fout, "  DIRECTION %s ;\n",
                      curLayer.lefiLayer::direction());
    if(curLayer.lefiLayer::hasResistance())
      CIRCUIT_FPRINTF(fout, "  RESISTANCE RPERSQ %g ;\n",
                      curLayer.lefiLayer::resistance());
    if(curLayer.lefiLayer::hasCapacitance())
      CIRCUIT_FPRINTF(fout, "  CAPACITANCE CPERSQDIST %g ;\n",
                      curLayer.lefiLayer::capacitance());
    if(curLayer.lefiLayer::hasEdgeCap())
      CIRCUIT_FPRINTF(fout, "  EDGECAPACITANCE %g ;\n",
                      curLayer.lefiLayer::edgeCap());
    if(curLayer.lefiLayer::hasHeight())
      CIRCUIT_FPRINTF(fout, "  TYPE %g ;\n", curLayer.lefiLayer::height());
    if(curLayer.lefiLayer::hasThickness())
      CIRCUIT_FPRINTF(fout, "  THICKNESS %g ;\n",
                      curLayer.lefiLayer::thickness());
    if(curLayer.lefiLayer::hasWireExtension())
      CIRCUIT_FPRINTF(fout, "  WIREEXTENSION %g ;\n",
                      curLayer.lefiLayer::wireExtension());
    if(curLayer.lefiLayer::hasShrinkage())
      CIRCUIT_FPRINTF(fout, "  SHRINKAGE %g ;\n",
                      curLayer.lefiLayer::shrinkage());
    if(curLayer.lefiLayer::hasCapMultiplier())
      CIRCUIT_FPRINTF(fout, "  CAPMULTIPLIER %g ;\n",
                      curLayer.lefiLayer::capMultiplier());
    if(curLayer.lefiLayer::hasAntennaArea())
      CIRCUIT_FPRINTF(fout, "  ANTENNAAREAFACTOR %g ;\n",
                      curLayer.lefiLayer::antennaArea());
    if(curLayer.lefiLayer::hasAntennaLength())
      CIRCUIT_FPRINTF(fout, "  ANTENNALENGTHFACTOR %g ;\n",
                      curLayer.lefiLayer::antennaLength());

    // 5.5 AntennaModel
    for(i = 0; i < curLayer.lefiLayer::numAntennaModel(); i++) {
      aModel = curLayer.lefiLayer::antennaModel(i);

      CIRCUIT_FPRINTF(fout, "  ANTENNAMODEL %s ;\n",
                      aModel->lefiAntennaModel::antennaOxide());

      if(aModel->lefiAntennaModel::hasAntennaAreaRatio())
        CIRCUIT_FPRINTF(fout, "  ANTENNAAREARATIO %g ;\n",
                        aModel->lefiAntennaModel::antennaAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaDiffAreaRatio()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFAREARATIO %g ;\n",
                        aModel->lefiAntennaModel::antennaDiffAreaRatio());
      }
      else if(aModel->lefiAntennaModel::hasAntennaDiffAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaDiffAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++) {
          CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                          pwl->lefiAntennaPWL::PWLdiffusion(j),
                          pwl->lefiAntennaPWL::PWLratio(j));
        }
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaCumAreaRatio())
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMAREARATIO %g ;\n",
                        aModel->lefiAntennaModel::antennaCumAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatio())
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMDIFFAREARATIO %g\n",
                        aModel->lefiAntennaModel::antennaCumDiffAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaCumDiffAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMDIFFAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
          CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                          pwl->lefiAntennaPWL::PWLdiffusion(j),
                          pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaAreaFactor()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNAAREAFACTOR %g ",
                        aModel->lefiAntennaModel::antennaAreaFactor());
        if(aModel->lefiAntennaModel::hasAntennaAreaFactorDUO())
          CIRCUIT_FPRINTF(fout, "  DIFFUSEONLY ");
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaSideAreaRatio())
        CIRCUIT_FPRINTF(fout, "  ANTENNASIDEAREARATIO %g ;\n",
                        aModel->lefiAntennaModel::antennaSideAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatio()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFSIDEAREARATIO %g\n",
                        aModel->lefiAntennaModel::antennaDiffSideAreaRatio());
      }
      else if(aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaDiffSideAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFSIDEAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
          CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                          pwl->lefiAntennaPWL::PWLdiffusion(j),
                          pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaCumSideAreaRatio())
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMSIDEAREARATIO %g ;\n",
                        aModel->lefiAntennaModel::antennaCumSideAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatio()) {
        CIRCUIT_FPRINTF(
            fout, "  ANTENNACUMDIFFSIDEAREARATIO %g\n",
            aModel->lefiAntennaModel::antennaCumDiffSideAreaRatio());
      }
      else if(aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaCumDiffSideAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMDIFFSIDEAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
          CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                          pwl->lefiAntennaPWL::PWLdiffusion(j),
                          pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaSideAreaFactor()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNASIDEAREAFACTOR %g ",
                        aModel->lefiAntennaModel::antennaSideAreaFactor());
        if(aModel->lefiAntennaModel::hasAntennaSideAreaFactorDUO())
          CIRCUIT_FPRINTF(fout, "  DIFFUSEONLY ");
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaCumRoutingPlusCut())
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMROUTINGPLUSCUT ;\n");
      if(aModel->lefiAntennaModel::hasAntennaGatePlusDiff())
        CIRCUIT_FPRINTF(fout, "  ANTENNAGATEPLUSDIFF %g ;\n",
                        aModel->lefiAntennaModel::antennaGatePlusDiff());
      if(aModel->lefiAntennaModel::hasAntennaAreaMinusDiff())
        CIRCUIT_FPRINTF(fout, "  ANTENNAAREAMINUSDIFF %g ;\n",
                        aModel->lefiAntennaModel::antennaAreaMinusDiff());
      if(aModel->lefiAntennaModel::hasAntennaAreaDiffReducePWL()) {
        pwl = aModel->lefiAntennaModel::antennaAreaDiffReducePWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNAAREADIFFREDUCEPWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
          CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                          pwl->lefiAntennaPWL::PWLdiffusion(j),
                          pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
    }

    if(curLayer.lefiLayer::numAccurrentDensity()) {
      for(i = 0; i < curLayer.lefiLayer::numAccurrentDensity(); i++) {
        density = curLayer.lefiLayer::accurrent(i);
        CIRCUIT_FPRINTF(fout, "  ACCURRENTDENSITY %s", density->type());
        if(density->hasOneEntry()) {
          CIRCUIT_FPRINTF(fout, " %g ;\n", density->oneEntry());
        }
        else {
          CIRCUIT_FPRINTF(fout, "\n");
          if(density->numFrequency()) {
            CIRCUIT_FPRINTF(fout, "    FREQUENCY");
            for(j = 0; j < density->numFrequency(); j++)
              CIRCUIT_FPRINTF(fout, " %g", density->frequency(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numCutareas()) {
            CIRCUIT_FPRINTF(fout, "    CUTAREA");
            for(j = 0; j < density->numCutareas(); j++)
              CIRCUIT_FPRINTF(fout, " %g", density->cutArea(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numWidths()) {
            CIRCUIT_FPRINTF(fout, "    WIDTH");
            for(j = 0; j < density->numWidths(); j++)
              CIRCUIT_FPRINTF(fout, " %g", density->width(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numTableEntries()) {
            k = 5;
            CIRCUIT_FPRINTF(fout, "    TABLEENTRIES");
            for(j = 0; j < density->numTableEntries(); j++)
              if(k > 4) {
                CIRCUIT_FPRINTF(fout, "\n     %g", density->tableEntry(j));
                k = 1;
              }
              else {
                CIRCUIT_FPRINTF(fout, " %g", density->tableEntry(j));
                k++;
              }
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
        }
      }
    }
    if(curLayer.lefiLayer::numDccurrentDensity()) {
      for(i = 0; i < curLayer.lefiLayer::numDccurrentDensity(); i++) {
        density = curLayer.lefiLayer::dccurrent(i);
        CIRCUIT_FPRINTF(fout, "  DCCURRENTDENSITY %s", density->type());
        if(density->hasOneEntry()) {
          CIRCUIT_FPRINTF(fout, " %g ;\n", density->oneEntry());
        }
        else {
          CIRCUIT_FPRINTF(fout, "\n");
          if(density->numCutareas()) {
            CIRCUIT_FPRINTF(fout, "    CUTAREA");
            for(j = 0; j < density->numCutareas(); j++)
              CIRCUIT_FPRINTF(fout, " %g", density->cutArea(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numWidths()) {
            CIRCUIT_FPRINTF(fout, "    WIDTH");
            for(j = 0; j < density->numWidths(); j++)
              CIRCUIT_FPRINTF(fout, " %g", density->width(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numTableEntries()) {
            CIRCUIT_FPRINTF(fout, "    TABLEENTRIES");
            for(j = 0; j < density->numTableEntries(); j++)
              CIRCUIT_FPRINTF(fout, " %g", density->tableEntry(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
        }
      }
    }

    for(i = 0; i < curLayer.lefiLayer::numSpacingTable(); i++) {
      spTable = curLayer.lefiLayer::spacingTable(i);
      CIRCUIT_FPRINTF(fout, "   SPACINGTABLE\n");
      if(spTable->lefiSpacingTable::isInfluence()) {
        influence = spTable->lefiSpacingTable::influence();
        CIRCUIT_FPRINTF(fout, "      INFLUENCE");
        for(j = 0; j < influence->lefiInfluence::numInfluenceEntry(); j++) {
          CIRCUIT_FPRINTF(fout, "\n          WIDTH %g WITHIN %g SPACING %g",
                          influence->lefiInfluence::width(j),
                          influence->lefiInfluence::distance(j),
                          influence->lefiInfluence::spacing(j));
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
      }
      else if(spTable->lefiSpacingTable::isParallel()) {
        parallel = spTable->lefiSpacingTable::parallel();
        CIRCUIT_FPRINTF(fout, "      PARALLELRUNLENGTH");
        for(j = 0; j < parallel->lefiParallel::numLength(); j++) {
          CIRCUIT_FPRINTF(fout, " %g", parallel->lefiParallel::length(j));
        }
        for(j = 0; j < parallel->lefiParallel::numWidth(); j++) {
          CIRCUIT_FPRINTF(fout, "\n          WIDTH %g",
                          parallel->lefiParallel::width(j));
          for(k = 0; k < parallel->lefiParallel::numLength(); k++) {
            CIRCUIT_FPRINTF(fout, " %g",
                            parallel->lefiParallel::widthSpacing(j, k));
          }
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
      }
      else {  // 5.7 TWOWIDTHS
        twoWidths = spTable->lefiSpacingTable::twoWidths();
        CIRCUIT_FPRINTF(fout, "      TWOWIDTHS");
        for(j = 0; j < twoWidths->lefiTwoWidths::numWidth(); j++) {
          CIRCUIT_FPRINTF(fout, "\n          WIDTH %g ",
                          twoWidths->lefiTwoWidths::width(j));
          if(twoWidths->lefiTwoWidths::hasWidthPRL(j))
            CIRCUIT_FPRINTF(fout, "PRL %g ",
                            twoWidths->lefiTwoWidths::widthPRL(j));
          for(k = 0; k < twoWidths->lefiTwoWidths::numWidthSpacing(j); k++)
            CIRCUIT_FPRINTF(fout, "%g ",
                            twoWidths->lefiTwoWidths::widthSpacing(j, k));
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
      }
    }

    propNum = curLayer.lefiLayer::numProps();
    if(propNum > 0) {
      CIRCUIT_FPRINTF(fout, "  PROPERTY ");
      for(i = 0; i < propNum; i++) {
        // value can either be a string or number
        CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::propName(i));
        if(curLayer.lefiLayer::propIsNumber(i))
          CIRCUIT_FPRINTF(fout, "%g ", curLayer.lefiLayer::propNumber(i));
        if(curLayer.lefiLayer::propIsString(i))
          CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::propValue(i));
        pType = curLayer.lefiLayer::propType(i);
        switch(pType) {
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
    if(curLayer.lefiLayer::hasDiagMinEdgeLength())
      CIRCUIT_FPRINTF(fout, "  DIAGMINEDGELENGTH %g ;\n",
                      curLayer.lefiLayer::diagMinEdgeLength());
    if(curLayer.lefiLayer::numMinSize()) {
      CIRCUIT_FPRINTF(fout, "  MINSIZE ");
      for(i = 0; i < curLayer.lefiLayer::numMinSize(); i++) {
        CIRCUIT_FPRINTF(fout, "%g %g ", curLayer.lefiLayer::minSizeWidth(i),
                        curLayer.lefiLayer::minSizeLength(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }

    CIRCUIT_FPRINTF(fout, "END %s\n\n", curLayer.lefiLayer::name());
  }
  // Set it to case sensitive from here on
  lefrSetCaseSensitivity(1);
}

void Replace::Circuit::DumpLefSite() {
  if(lefSiteStor.size() == 0) {
    return;
  }

  int hasPrtSym = 0;
  for(auto& curSite : lefSiteStor) {
    CIRCUIT_FPRINTF(fout, "SITE %s\n", curSite.lefiSite::name());
    if(curSite.lefiSite::hasClass())
      CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", curSite.lefiSite::siteClass());
    if(curSite.lefiSite::hasXSymmetry()) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
      hasPrtSym = 1;
    }
    if(curSite.lefiSite::hasYSymmetry()) {
      if(hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "Y ");
      }
      else {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
        hasPrtSym = 1;
      }
    }
    if(curSite.lefiSite::has90Symmetry()) {
      if(hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "R90 ");
      }
      else {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
        hasPrtSym = 1;
      }
    }
    if(hasPrtSym)
      CIRCUIT_FPRINTF(fout, ";\n");
    if(curSite.lefiSite::hasSize())
      CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", curSite.lefiSite::sizeX(),
                      curSite.lefiSite::sizeY());

    if(curSite.hasRowPattern()) {
      CIRCUIT_FPRINTF(fout, "  ROWPATTERN ");
      for(int i = 0; i < curSite.lefiSite::numSites(); i++)
        CIRCUIT_FPRINTF(fout, "  %s %s ", curSite.lefiSite::siteName(i),
                        curSite.lefiSite::siteOrientStr(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }

    CIRCUIT_FPRINTF(fout, "END %s\n\n", curSite.lefiSite::name());
  }
}

void Replace::Circuit::DumpLefVia() {
  if(lefViaStor.size() == 0) {
    return;
  }

  for(auto& curVia : lefViaStor) {
    lefVia(&curVia);
  }
}

void Replace::Circuit::DumpLefMacro() {
  lefiSitePattern* pattern;
  int propNum, i, hasPrtSym = 0;

  for(auto& curMacro : lefMacroStor) {
    //        cout << "MACRO " << curMacro.lefiMacro::name() << endl;
    CIRCUIT_FPRINTF(fout, "MACRO %s\n", curMacro.lefiMacro::name());
    // if ((long)ud != userData) dataError();
    if(curMacro.lefiMacro::hasClass())
      CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", curMacro.lefiMacro::macroClass());
    if(curMacro.lefiMacro::isFixedMask())
      CIRCUIT_FPRINTF(fout, "  FIXEDMASK ;\n");
    if(curMacro.lefiMacro::hasEEQ())
      CIRCUIT_FPRINTF(fout, "  EEQ %s ;\n", curMacro.lefiMacro::EEQ());
    if(curMacro.lefiMacro::hasLEQ())
      CIRCUIT_FPRINTF(fout, "  LEQ %s ;\n", curMacro.lefiMacro::LEQ());
    if(curMacro.lefiMacro::hasSource())
      CIRCUIT_FPRINTF(fout, "  SOURCE %s ;\n", curMacro.lefiMacro::source());
    if(curMacro.lefiMacro::hasXSymmetry()) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
      hasPrtSym = 1;
    }
    if(curMacro.lefiMacro::hasYSymmetry()) {  // print X Y & R90 in one line
      if(!hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
        hasPrtSym = 1;
      }
      else
        CIRCUIT_FPRINTF(fout, "Y ");
    }
    if(curMacro.lefiMacro::has90Symmetry()) {
      if(!hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
        hasPrtSym = 1;
      }
      else
        CIRCUIT_FPRINTF(fout, "R90 ");
    }
    if(hasPrtSym) {
      CIRCUIT_FPRINTF(fout, ";\n");
      hasPrtSym = 0;
    }
    if(curMacro.lefiMacro::hasSiteName())
      CIRCUIT_FPRINTF(fout, "  SITE %s ;\n", curMacro.lefiMacro::siteName());
    if(curMacro.lefiMacro::hasSitePattern()) {
      for(i = 0; i < curMacro.lefiMacro::numSitePattern(); i++) {
        pattern = curMacro.lefiMacro::sitePattern(i);
        if(pattern->lefiSitePattern::hasStepPattern()) {
          CIRCUIT_FPRINTF(fout, "  SITE %s %g %g %s DO %g BY %g STEP %g %g ;\n",
                          pattern->lefiSitePattern::name(),
                          pattern->lefiSitePattern::x(),
                          pattern->lefiSitePattern::y(),
                          orientStr(pattern->lefiSitePattern::orient()),
                          pattern->lefiSitePattern::xStart(),
                          pattern->lefiSitePattern::yStart(),
                          pattern->lefiSitePattern::xStep(),
                          pattern->lefiSitePattern::yStep());
        }
        else {
          CIRCUIT_FPRINTF(
              fout, "  SITE %s %g %g %s ;\n", pattern->lefiSitePattern::name(),
              pattern->lefiSitePattern::x(), pattern->lefiSitePattern::y(),
              orientStr(pattern->lefiSitePattern::orient()));
        }
      }
    }
    if(curMacro.lefiMacro::hasSize())
      CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", curMacro.lefiMacro::sizeX(),
                      curMacro.lefiMacro::sizeY());

    if(curMacro.lefiMacro::hasForeign()) {
      for(i = 0; i < curMacro.lefiMacro::numForeigns(); i++) {
        CIRCUIT_FPRINTF(fout, "  FOREIGN %s ",
                        curMacro.lefiMacro::foreignName(i));
        if(curMacro.lefiMacro::hasForeignPoint(i)) {
          CIRCUIT_FPRINTF(fout, "( %g %g ) ", curMacro.lefiMacro::foreignX(i),
                          curMacro.lefiMacro::foreignY(i));
          if(curMacro.lefiMacro::hasForeignOrient(i))
            CIRCUIT_FPRINTF(fout, "%s ",
                            curMacro.lefiMacro::foreignOrientStr(i));
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    if(curMacro.lefiMacro::hasOrigin())
      CIRCUIT_FPRINTF(fout, "  ORIGIN ( %g %g ) ;\n",
                      curMacro.lefiMacro::originX(),
                      curMacro.lefiMacro::originY());
    if(curMacro.lefiMacro::hasPower())
      CIRCUIT_FPRINTF(fout, "  POWER %g ;\n", curMacro.lefiMacro::power());
    propNum = curMacro.lefiMacro::numProperties();
    if(propNum > 0) {
      CIRCUIT_FPRINTF(fout, "  PROPERTY ");
      for(i = 0; i < propNum; i++) {
        // value can either be a string or number
        if(curMacro.lefiMacro::propValue(i)) {
          CIRCUIT_FPRINTF(fout, "%s %s ", curMacro.lefiMacro::propName(i),
                          curMacro.lefiMacro::propValue(i));
        }
        else
          CIRCUIT_FPRINTF(fout, "%s %g ", curMacro.lefiMacro::propName(i),
                          curMacro.lefiMacro::propNum(i));

        switch(curMacro.lefiMacro::propType(i)) {
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
    fflush(stdout);

    // curPin.first : pinName
    // curPin.second : lefPinStor's index
    for(auto& curPin : lefPinStor[&curMacro - &lefMacroStor[0]]) {
      DumpLefPin(&curPin);
    }

    for(auto& curObs : lefObsStor[&curMacro - &lefMacroStor[0]]) {
      DumpLefObs(&curObs);
    }

    CIRCUIT_FPRINTF(fout, "END %s\n\n", curMacro.lefiMacro::name());
  }
}

void Replace::Circuit::DumpLefDone() {
  CIRCUIT_FPRINTF(fout, "END LIBRARY\n");
}

// below is helper function
void Replace::Circuit::DumpLefObs(lefiObstruction* obs) {
  CIRCUIT_FPRINTF(fout, "  OBS\n");
  lefiGeometries* geometry = obs->lefiObstruction::geometries();
  prtGeometry(geometry);
  CIRCUIT_FPRINTF(fout, "  END\n");
}

void Replace::Circuit::DumpLefPin(lefiPin* pin) {
  int numPorts, i, j;
  lefiGeometries* geometry;
  lefiPinAntennaModel* aModel;

  CIRCUIT_FPRINTF(fout, "  PIN %s\n", pin->lefiPin::name());
  if(pin->lefiPin::hasForeign()) {
    for(i = 0; i < pin->lefiPin::numForeigns(); i++) {
      if(pin->lefiPin::hasForeignOrient(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) %s ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i),
                        pin->lefiPin::foreignOrientStr(i));
      }
      else if(pin->lefiPin::hasForeignPoint(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s ;\n",
                        pin->lefiPin::foreignName(i));
      }
    }
  }
  if(pin->lefiPin::hasLEQ())
    CIRCUIT_FPRINTF(fout, "    LEQ %s ;\n", pin->lefiPin::LEQ());
  if(pin->lefiPin::hasDirection())
    CIRCUIT_FPRINTF(fout, "    DIRECTION %s ;\n", pin->lefiPin::direction());
  if(pin->lefiPin::hasUse())
    CIRCUIT_FPRINTF(fout, "    USE %s ;\n", pin->lefiPin::use());
  if(pin->lefiPin::hasShape())
    CIRCUIT_FPRINTF(fout, "    SHAPE %s ;\n", pin->lefiPin::shape());
  if(pin->lefiPin::hasMustjoin())
    CIRCUIT_FPRINTF(fout, "    MUSTJOIN %s ;\n", pin->lefiPin::mustjoin());
  if(pin->lefiPin::hasOutMargin())
    CIRCUIT_FPRINTF(fout, "    OUTPUTNOISEMARGIN %g %g ;\n",
                    pin->lefiPin::outMarginHigh(),
                    pin->lefiPin::outMarginLow());
  if(pin->lefiPin::hasOutResistance())
    CIRCUIT_FPRINTF(fout, "    OUTPUTRESISTANCE %g %g ;\n",
                    pin->lefiPin::outResistanceHigh(),
                    pin->lefiPin::outResistanceLow());
  if(pin->lefiPin::hasInMargin())
    CIRCUIT_FPRINTF(fout, "    INPUTNOISEMARGIN %g %g ;\n",
                    pin->lefiPin::inMarginHigh(), pin->lefiPin::inMarginLow());
  if(pin->lefiPin::hasPower())
    CIRCUIT_FPRINTF(fout, "    POWER %g ;\n", pin->lefiPin::power());
  if(pin->lefiPin::hasLeakage())
    CIRCUIT_FPRINTF(fout, "    LEAKAGE %g ;\n", pin->lefiPin::leakage());
  if(pin->lefiPin::hasMaxload())
    CIRCUIT_FPRINTF(fout, "    MAXLOAD %g ;\n", pin->lefiPin::maxload());
  if(pin->lefiPin::hasCapacitance())
    CIRCUIT_FPRINTF(fout, "    CAPACITANCE %g ;\n",
                    pin->lefiPin::capacitance());
  if(pin->lefiPin::hasResistance())
    CIRCUIT_FPRINTF(fout, "    RESISTANCE %g ;\n", pin->lefiPin::resistance());
  if(pin->lefiPin::hasPulldownres())
    CIRCUIT_FPRINTF(fout, "    PULLDOWNRES %g ;\n",
                    pin->lefiPin::pulldownres());
  if(pin->lefiPin::hasTieoffr())
    CIRCUIT_FPRINTF(fout, "    TIEOFFR %g ;\n", pin->lefiPin::tieoffr());
  if(pin->lefiPin::hasVHI())
    CIRCUIT_FPRINTF(fout, "    VHI %g ;\n", pin->lefiPin::VHI());
  if(pin->lefiPin::hasVLO())
    CIRCUIT_FPRINTF(fout, "    VLO %g ;\n", pin->lefiPin::VLO());
  if(pin->lefiPin::hasRiseVoltage())
    CIRCUIT_FPRINTF(fout, "    RISEVOLTAGETHRESHOLD %g ;\n",
                    pin->lefiPin::riseVoltage());
  if(pin->lefiPin::hasFallVoltage())
    CIRCUIT_FPRINTF(fout, "    FALLVOLTAGETHRESHOLD %g ;\n",
                    pin->lefiPin::fallVoltage());
  if(pin->lefiPin::hasRiseThresh())
    CIRCUIT_FPRINTF(fout, "    RISETHRESH %g ;\n", pin->lefiPin::riseThresh());
  if(pin->lefiPin::hasFallThresh())
    CIRCUIT_FPRINTF(fout, "    FALLTHRESH %g ;\n", pin->lefiPin::fallThresh());
  if(pin->lefiPin::hasRiseSatcur())
    CIRCUIT_FPRINTF(fout, "    RISESATCUR %g ;\n", pin->lefiPin::riseSatcur());
  if(pin->lefiPin::hasFallSatcur())
    CIRCUIT_FPRINTF(fout, "    FALLSATCUR %g ;\n", pin->lefiPin::fallSatcur());
  if(pin->lefiPin::hasRiseSlewLimit())
    CIRCUIT_FPRINTF(fout, "    RISESLEWLIMIT %g ;\n",
                    pin->lefiPin::riseSlewLimit());
  if(pin->lefiPin::hasFallSlewLimit())
    CIRCUIT_FPRINTF(fout, "    FALLSLEWLIMIT %g ;\n",
                    pin->lefiPin::fallSlewLimit());
  if(pin->lefiPin::hasCurrentSource())
    CIRCUIT_FPRINTF(fout, "    CURRENTSOURCE %s ;\n",
                    pin->lefiPin::currentSource());
  if(pin->lefiPin::hasTables())
    CIRCUIT_FPRINTF(fout, "    IV_TABLES %s %s ;\n",
                    pin->lefiPin::tableHighName(),
                    pin->lefiPin::tableLowName());
  if(pin->lefiPin::hasTaperRule())
    CIRCUIT_FPRINTF(fout, "    TAPERRULE %s ;\n", pin->lefiPin::taperRule());
  if(pin->lefiPin::hasNetExpr())
    CIRCUIT_FPRINTF(fout, "    NETEXPR \"%s\" ;\n", pin->lefiPin::netExpr());
  if(pin->lefiPin::hasSupplySensitivity())
    CIRCUIT_FPRINTF(fout, "    SUPPLYSENSITIVITY %s ;\n",
                    pin->lefiPin::supplySensitivity());
  if(pin->lefiPin::hasGroundSensitivity())
    CIRCUIT_FPRINTF(fout, "    GROUNDSENSITIVITY %s ;\n",
                    pin->lefiPin::groundSensitivity());
  if(pin->lefiPin::hasAntennaSize()) {
    for(i = 0; i < pin->lefiPin::numAntennaSize(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNASIZE %g ",
                      pin->lefiPin::antennaSize(i));
      if(pin->lefiPin::antennaSizeLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ", pin->lefiPin::antennaSizeLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALAREA %g ",
                      pin->lefiPin::antennaMetalArea(i));
      if(pin->lefiPin::antennaMetalAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalLength()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalLength(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALLENGTH %g ",
                      pin->lefiPin::antennaMetalLength(i));
      if(pin->lefiPin::antennaMetalLengthLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaMetalLengthLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALAREA %g ",
                      pin->lefiPin::antennaPartialMetalArea(i));
      if(pin->lefiPin::antennaPartialMetalAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaPartialMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalSideArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalSideArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALSIDEAREA %g ",
                      pin->lefiPin::antennaPartialMetalSideArea(i));
      if(pin->lefiPin::antennaPartialMetalSideAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaPartialMetalSideAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialCutArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialCutArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALCUTAREA %g ",
                      pin->lefiPin::antennaPartialCutArea(i));
      if(pin->lefiPin::antennaPartialCutAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaPartialCutAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaDiffArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaDiffArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNADIFFAREA %g ",
                      pin->lefiPin::antennaDiffArea(i));
      if(pin->lefiPin::antennaDiffAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        pin->lefiPin::antennaDiffAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  for(j = 0; j < pin->lefiPin::numAntennaModel(); j++) {
    aModel = pin->lefiPin::antennaModel(j);

    CIRCUIT_FPRINTF(fout, "    ANTENNAMODEL %s ;\n",
                    aModel->lefiPinAntennaModel::antennaOxide());

    if(aModel->lefiPinAntennaModel::hasAntennaGateArea()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaGateArea(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAGATEAREA %g ",
                        aModel->lefiPinAntennaModel::antennaGateArea(i));
        if(aModel->lefiPinAntennaModel::antennaGateAreaLayer(i))
          CIRCUIT_FPRINTF(fout, "LAYER %s ",
                          aModel->lefiPinAntennaModel::antennaGateAreaLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxAreaCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i))
          CIRCUIT_FPRINTF(
              fout, "LAYER %s ",
              aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxSideAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxSideAreaCar();
          i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXSIDEAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxSideAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i))
          CIRCUIT_FPRINTF(
              fout, "LAYER %s ",
              aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxCutCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxCutCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXCUTCAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxCutCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i))
          CIRCUIT_FPRINTF(
              fout, "LAYER %s ",
              aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }

  if(pin->lefiPin::numProperties() > 0) {
    CIRCUIT_FPRINTF(fout, "    PROPERTY ");
    for(i = 0; i < pin->lefiPin::numProperties(); i++) {
      // value can either be a string or number
      if(pin->lefiPin::propValue(i)) {
        CIRCUIT_FPRINTF(fout, "%s %s ", pin->lefiPin::propName(i),
                        pin->lefiPin::propValue(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "%s %g ", pin->lefiPin::propName(i),
                        pin->lefiPin::propNum(i));
      }
      switch(pin->lefiPin::propType(i)) {
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

  numPorts = pin->lefiPin::numPorts();
  for(i = 0; i < numPorts; i++) {
    CIRCUIT_FPRINTF(fout, "    PORT\n");
    fflush(stdout);
    geometry = pin->lefiPin::port(i);
    prtGeometry(geometry);
    CIRCUIT_FPRINTF(fout, "    END\n");
  }
  CIRCUIT_FPRINTF(fout, "  END %s\n", pin->lefiPin::name());
  fflush(stdout);
}

int Replace::Circuit::ParseLef(vector< string >& lefStor,
                                bool isVerbose)  {
  //    char* outFile;

  FILE* f;
  int res;
  //    int noCalls = 0;

  //  long start_mem;
  //    int num;
  int status;
  int retStr = 0;
  //    int numInFile = 0;
  //    int fileCt = 0;
  int relax = 0;
  //    const char* version = "N/A";
  //    int setVer = 0;
  //    int msgCb = 0;

  // start_mem = (long)sbrk(0);

  char* userData = strdup("(lefrw-5100)");

  fout = stdout;

  lefVersionPtr = &(this->lefVersion);
  lefDividerPtr = &(this->lefDivider);
  lefBusBitCharPtr = &(this->lefBusBitChar);

  lefUnitPtr = &(this->lefUnit);
  lefManufacturingGridPtr = &(this->lefManufacturingGrid);

  lefLayerStorPtr = &(this->lefLayerStor);
  lefSiteStorPtr = &(this->lefSiteStor);
  lefMacroStorPtr = &(this->lefMacroStor);
  lefViaStorPtr = &(this->lefViaStor);

  // vector of vector
  lefPinStorPtr = &(this->lefPinStor);
  lefObsStorPtr = &(this->lefObsStor);

  lefPinMapStorPtr = &(this->lefPinMapStor);

  //    lefMacroToPinPtr = &(this->lefMacroToPin);

  lefMacroMapPtr = &(this->lefMacroMap);
  lefViaMapPtr = &(this->lefViaMap);
  lefLayerMapPtr = &(this->lefLayerMap);
  lefSiteMapPtr = &(this->lefSiteMap);

  // sets the parser to be case sensitive...
  // default was supposed to be the case but false...
  // lefrSetCaseSensitivity(true);

  lefrInitSession(1);

  lefrSetWarningLogFunction(printWarning);
  lefrSetAntennaInputCbk(antennaCB);
  lefrSetAntennaInoutCbk(antennaCB);
  lefrSetAntennaOutputCbk(antennaCB);
  lefrSetArrayBeginCbk(arrayBeginCB);
  lefrSetArrayCbk(arrayCB);
  lefrSetArrayEndCbk(arrayEndCB);
  lefrSetBusBitCharsCbk(busBitCharsCB);
  lefrSetCaseSensitiveCbk(caseSensCB);
  lefrSetFixedMaskCbk(fixedMaskCB);
  lefrSetClearanceMeasureCbk(clearanceCB);
  lefrSetDensityCbk(densityCB);
  lefrSetDividerCharCbk(dividerCB);
  lefrSetNoWireExtensionCbk(noWireExtCB);
  lefrSetNoiseMarginCbk(noiseMarCB);
  lefrSetEdgeRateThreshold1Cbk(edge1CB);
  lefrSetEdgeRateThreshold2Cbk(edge2CB);
  lefrSetEdgeRateScaleFactorCbk(edgeScaleCB);
  lefrSetExtensionCbk(extensionCB);
  lefrSetNoiseTableCbk(noiseTableCB);
  lefrSetCorrectionTableCbk(correctionCB);
  lefrSetDielectricCbk(dielectricCB);
  lefrSetIRDropBeginCbk(irdropBeginCB);
  lefrSetIRDropCbk(irdropCB);
  lefrSetIRDropEndCbk(irdropEndCB);
  lefrSetLayerCbk(layerCB);
  lefrSetLibraryEndCbk(doneCB);
  lefrSetMacroBeginCbk(macroBeginCB);
  lefrSetMacroCbk(macroCB);
  lefrSetMacroClassTypeCbk(macroClassTypeCB);
  lefrSetMacroOriginCbk(macroOriginCB);
  lefrSetMacroSizeCbk(macroSizeCB);
  lefrSetMacroFixedMaskCbk(macroFixedMaskCB);
  lefrSetMacroEndCbk(macroEndCB);
  lefrSetManufacturingCbk(manufacturingCB);
  lefrSetMaxStackViaCbk(maxStackViaCB);
  lefrSetMinFeatureCbk(minFeatureCB);
  lefrSetNonDefaultCbk(nonDefaultCB);
  lefrSetObstructionCbk(obstructionCB);
  lefrSetPinCbk(pinCB);
  lefrSetPropBeginCbk(propDefBeginCB);
  lefrSetPropCbk(propDefCB);
  lefrSetPropEndCbk(propDefEndCB);
  lefrSetSiteCbk(siteCB);
  lefrSetSpacingBeginCbk(spacingBeginCB);
  lefrSetSpacingCbk(spacingCB);
  lefrSetSpacingEndCbk(spacingEndCB);
  lefrSetTimingCbk(timingCB);
  lefrSetUnitsCbk(unitsCB);
  lefrSetUseMinSpacingCbk(useMinSpacingCB);
  lefrSetUserData((void*)3);
  if(!retStr)
    lefrSetVersionCbk(versionCB);
  else
    lefrSetVersionStrCbk(versionStrCB);
  lefrSetViaCbk(viaCB);
  lefrSetViaRuleCbk(viaRuleCB);
  lefrSetInputAntennaCbk(antennaCB);
  lefrSetOutputAntennaCbk(antennaCB);
  lefrSetInoutAntennaCbk(antennaCB);

  //    if (msgCb) {
  //        lefrSetLogFunction(errorCB);
  //        lefrSetWarningLogFunction(warningCB);
  //    }

  lefrSetMallocFunction(mallocCB);
  lefrSetReallocFunction(reallocCB);
  lefrSetFreeFunction(freeCB);

  if(isVerbose) {
    lefrSetLineNumberFunction(lineNumberCB);
    lefrSetDeltaNumberLines(500);
  }

  lefrSetRegisterUnusedCallbacks();

  if(relax)
    lefrSetRelaxMode();

  //    if (setVer)
  //        (void)lefrSetVersionValue(version);

  lefrSetAntennaInoutWarnings(30);
  lefrSetAntennaInputWarnings(30);
  lefrSetAntennaOutputWarnings(30);
  lefrSetArrayWarnings(30);
  lefrSetCaseSensitiveWarnings(30);
  lefrSetCorrectionTableWarnings(30);
  lefrSetDielectricWarnings(30);
  lefrSetEdgeRateThreshold1Warnings(30);
  lefrSetEdgeRateThreshold2Warnings(30);
  lefrSetEdgeRateScaleFactorWarnings(30);
  lefrSetInoutAntennaWarnings(30);
  lefrSetInputAntennaWarnings(30);
  lefrSetIRDropWarnings(30);
  lefrSetLayerWarnings(30);
  lefrSetMacroWarnings(30);
  lefrSetMaxStackViaWarnings(30);
  lefrSetMinFeatureWarnings(30);
  lefrSetNoiseMarginWarnings(30);
  lefrSetNoiseTableWarnings(30);
  lefrSetNonDefaultWarnings(30);
  lefrSetNoWireExtensionWarnings(30);
  lefrSetOutputAntennaWarnings(30);
  lefrSetPinWarnings(30);
  lefrSetSiteWarnings(30);
  lefrSetSpacingWarnings(30);
  lefrSetTimingWarnings(30);
  lefrSetUnitsWarnings(30);
  lefrSetUseMinSpacingWarnings(30);
  lefrSetViaRuleWarnings(30);
  lefrSetViaWarnings(30);

  (void)lefrSetShiftCase();  // will shift name to uppercase if caseinsensitive

  // is set to off or not set
  lefrSetOpenLogFileAppend();

  for(auto& lefName : lefStor) {
    lefrReset();

    if((f = fopen(lefName.c_str(), "r")) == 0) {
      fprintf(stderr, "\n**ERROR: Couldn't open input file '%s'\n",
              lefName.c_str());
      exit(1);
    }

    (void)lefrEnableReadEncrypted();

    status = lefwInit(fout);  // initialize the lef writer,
    // need to be called 1st
    if(status != LEFW_OK)
      return 1;

    fout = NULL;
    res = lefrRead(f, lefName.c_str(), (void*)userData);

    if(res) {
      CIRCUIT_FPRINTF(stderr, "Reader returns bad status.\n", lefName.c_str());
      return res;
    }

    (void)lefrPrintUnusedCallbacks(fout);
    (void)lefrReleaseNResetMemory();
  }

  (void)lefrUnsetCallbacks();

  // Unset all the callbacks
  void lefrUnsetAntennaInputCbk();
  void lefrUnsetAntennaInoutCbk();
  void lefrUnsetAntennaOutputCbk();
  void lefrUnsetArrayBeginCbk();
  void lefrUnsetArrayCbk();
  void lefrUnsetArrayEndCbk();
  void lefrUnsetBusBitCharsCbk();
  void lefrUnsetCaseSensitiveCbk();
  void lefrUnsetFixedMaskCbk();
  void lefrUnsetClearanceMeasureCbk();
  void lefrUnsetCorrectionTableCbk();
  void lefrUnsetDensityCbk();
  void lefrUnsetDielectricCbk();
  void lefrUnsetDividerCharCbk();
  void lefrUnsetEdgeRateScaleFactorCbk();
  void lefrUnsetEdgeRateThreshold1Cbk();
  void lefrUnsetEdgeRateThreshold2Cbk();
  void lefrUnsetExtensionCbk();
  void lefrUnsetInoutAntennaCbk();
  void lefrUnsetInputAntennaCbk();
  void lefrUnsetIRDropBeginCbk();
  void lefrUnsetIRDropCbk();
  void lefrUnsetIRDropEndCbk();
  void lefrUnsetLayerCbk();
  void lefrUnsetLibraryEndCbk();
  void lefrUnsetMacroBeginCbk();
  void lefrUnsetMacroCbk();
  void lefrUnsetMacroClassTypeCbk();
  void lefrUnsetMacroEndCbk();
  void lefrUnsetMacroOriginCbk();
  void lefrUnsetMacroSizeCbk();
  void lefrUnsetManufacturingCbk();
  void lefrUnsetMaxStackViaCbk();
  void lefrUnsetMinFeatureCbk();
  void lefrUnsetNoiseMarginCbk();
  void lefrUnsetNoiseTableCbk();
  void lefrUnsetNonDefaultCbk();
  void lefrUnsetNoWireExtensionCbk();
  void lefrUnsetObstructionCbk();
  void lefrUnsetOutputAntennaCbk();
  void lefrUnsetPinCbk();
  void lefrUnsetPropBeginCbk();
  void lefrUnsetPropCbk();
  void lefrUnsetPropEndCbk();
  void lefrUnsetSiteCbk();
  void lefrUnsetSpacingBeginCbk();
  void lefrUnsetSpacingCbk();
  void lefrUnsetSpacingEndCbk();
  void lefrUnsetTimingCbk();
  void lefrUnsetUseMinSpacingCbk();
  void lefrUnsetUnitsCbk();
  void lefrUnsetVersionCbk();
  void lefrUnsetVersionStrCbk();
  void lefrUnsetViaCbk();
  void lefrUnsetViaRuleCbk();

  //    fclose(fout);

  // Release allocated singleton data.
  //    lefrClear();

  return res;
}
void Replace::Circuit::WriteLef(FILE* _fout) {
  fout = _fout;
  //    fout = stdout;

  DumpLefVersion();
  DumpLefBusBitChar();
  DumpLefDivider();

  DumpLefUnit();
  DumpLefManufacturingGrid();
  DumpLefLayer();
  DumpLefSite();
  DumpLefVia();

  //    DumpLefSite();

  DumpLefMacro();
  DumpLefDone();
  //    DumpLefPin();

  //    fflush(stdout);
}

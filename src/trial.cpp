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
#include <climits>
#include <cfloat>
#include <iomanip>

#include "replace_private.h"
#include "opt.h"
#include "bookShelfIO.h"
#include "bin.h"

using std::min;
using std::max;


void trial_main() {
  trial_HPWLs.clear();
  trial_POTNs.clear();
  if(placementMacroCNT > 0 && numLayer >= 2)
    tmGP3DglobalPlacement_main();
  else if(placementMacroCNT > 0 && numLayer == 1)
    tmGP2DglobalPlacement_main();
  else if(placementMacroCNT == 0 && numLayer >= 2)
    tcGP3DglobalPlacement_main();
  else if(placementMacroCNT == 0 && numLayer == 1)
    tcGP2DglobalPlacement_main();

  findMinHPWLduringTrial();
  findMaxHPWLduringTrial();

  get2ndOrderDiff_HPWL_LinearHPWLtrend();
  get2ndOrderEP1();
  // printTrend ();
  get2ndOrderEP2();
  // printTrend ();
  sort2ndOrderEPs();
  store2ndOrder_ExtremePoints();

  // if (orderHPWL == thirdOrder) {
  get1stOrderDiff_HPWL_LinearHPWLtrendforThird();
  get1stOrder_ExtremePointsforThird();
  store1stOrder_ExtremePointsforThird();

  store_POTNs();

  // printTrend ();
  printEPs();
  //} else if (orderHPWL == secondOrder) {
  //    get1stOrderDiff_HPWL_LinearHPWLtrendforSecond ();
  //    get1stOrder_ExtremePointsforSecond ();
  //    store1stOrder_ExtremePointsforSecond ();
  //} else {}

  trial_HPWLs.clear();
  trial_HPWLs.shrink_to_fit();
  trial_POTNs.clear();
  trial_POTNs.shrink_to_fit();
  // exit (0);
}

void printEPs() {
  cout << "INFO:  2nd-Order EP0:  " << std::right << std::setw(4) << "0";
  printf("  \tHPWL:  %.6e", trial_HPWLs[0].first);
  printf("  \tPOTN:  %.6e\n", potnEPs[0]);
  cout << "INFO:  2nd-Order EP1:  " << std::right << std::setw(4)
       << extPt1_2ndOrder;
  printf("  \tHPWL:  %.6e", trial_HPWLs[extPt1_2ndOrder].first);
  printf("  \tPOTN:  %.6e\n", potnEPs[2]);
  cout << "INFO:  2nd-Order EP2:  " << std::right << std::setw(4)
       << extPt2_2ndOrder;
  printf("  \tHPWL:  %.6e", trial_HPWLs[extPt2_2ndOrder].first);
  printf("  \tPOTN:  %.6e\n", potnEPs[4]);
  cout << "INFO:  2nd-Order EP3:  " << std::right << std::setw(4)
       << trial_HPWLs.size() - 1;
  printf("  \tHPWL:  %.6e", trial_HPWLs[trial_HPWLs.size() - 1].first);
  printf("  \tPOTN:  %.6e\n\n", potnEPs[6]);

  cout << "INFO:  1st-Order EP1:  " << std::right << std::setw(4)
       << extPt1_1stOrder;
  printf("  \tHPWL:  %.6e", trial_HPWLs[extPt1_1stOrder].first);
  printf("  \tPOTN:  %.6e\n", potnEPs[1]);
  cout << "INFO:  1st-Order EP2:  " << std::right << std::setw(4)
       << extPt2_1stOrder;
  printf("  \tHPWL:  %.6e", trial_HPWLs[extPt2_1stOrder].first);
  printf("  \tPOTN:  %.6e\n", potnEPs[3]);
  cout << "INFO:  1st-Order EP3:  " << std::right << std::setw(4)
       << extPt3_1stOrder;
  printf("  \tHPWL:  %.6e", trial_HPWLs[extPt3_1stOrder].first);
  printf("  \tPOTN:  %.6e\n\n", potnEPs[5]);
}

void get2ndOrderEP1() {
  prec tempMaxGap = PREC_MIN;
  unsigned tempMaxIndex = UINT_MAX;

  for(unsigned i = 0; i < trial_HPWLs.size(); i++) {
    if(get_abs(trial_HPWLs[i].second) > tempMaxGap) {
      tempMaxGap = get_abs(trial_HPWLs[i].second);
      tempMaxIndex = i;
    }
  }
  extPt1_2ndOrder = tempMaxIndex;
}

void printTrend() {
  for(unsigned i = 0; i < trial_HPWLs.size(); i++) {
    cout << i << "-th iter HPWL == " << trial_HPWLs[i].first << "    "
         << trial_HPWLs[i].second << endl;
  }
  cout << endl;
  for(unsigned i = 0; i < trial_POTNs.size(); i++) {
    cout << i << "-th iter POTN == " << trial_POTNs[i] << endl;
  }
  cout << endl;
}

void sort2ndOrderEPs() {
  unsigned tempIdx = 0;

  if(extPt1_2ndOrder > extPt2_2ndOrder) {
    tempIdx = extPt1_2ndOrder;
    extPt1_2ndOrder = extPt2_2ndOrder;
    extPt2_2ndOrder = tempIdx;
  }
  // if (trial_HPWLs[extPt1_2ndOrder].first >
  // trial_HPWLs[extPt2_2ndOrder].first) {
  //    tempIdx         = extPt1_2ndOrder;
  //    extPt1_2ndOrder = extPt2_2ndOrder;
  //    extPt2_2ndOrder = tempIdx;
  //}
}

void store_POTNs() {
  potnEPs.clear();
  potnEPs.push_back(trial_POTNs[0]);
  potnEPs.push_back(trial_POTNs[extPt1_1stOrder]);
  potnEPs.push_back(trial_POTNs[extPt1_2ndOrder]);
  potnEPs.push_back(trial_POTNs[extPt2_1stOrder]);
  potnEPs.push_back(trial_POTNs[extPt2_2ndOrder]);
  potnEPs.push_back(trial_POTNs[extPt3_1stOrder]);
  potnEPs.push_back(trial_POTNs[trial_HPWLs.size() - 1]);
}

void store2ndOrder_ExtremePoints() {
  prec ratio1 =
      (trial_HPWLs[extPt1_2ndOrder].first - trial_HPWLs[0].first) /
      (trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first);
  prec ratio2 =
      (trial_HPWLs[extPt2_2ndOrder].first - trial_HPWLs[0].first) /
      (trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first);

  hpwlEPs_2ndOrder.clear();
  hpwlEPs_2ndOrder.push_back(
      std::pair< prec, prec >(trial_HPWLs[0].first, 0.0));
  hpwlEPs_2ndOrder.push_back(std::pair< prec, prec >(
      trial_HPWLs[extPt1_2ndOrder].first, ratio1 - 0.0));
  hpwlEPs_2ndOrder.push_back(std::pair< prec, prec >(
      trial_HPWLs[extPt2_2ndOrder].first, ratio2 - ratio1));
  hpwlEPs_2ndOrder.push_back(std::pair< prec, prec >(
      trial_HPWLs[trial_HPWLs.size() - 1].first, 1.0 - ratio2));
}

void store1stOrder_ExtremePointsforSecond() {
  prec ratio1 =
      (trial_HPWLs[extPt1_1stOrder].first - trial_HPWLs[0].first) /
      (trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first);
  prec ratio2 =
      (trial_HPWLs[extPt2_1stOrder].first - trial_HPWLs[0].first) /
      (trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first);

  hpwlEPs_1stOrder.clear();
  hpwlEPs_1stOrder.push_back(
      std::pair< prec, prec >(trial_HPWLs[extPt1_1stOrder].first, ratio1));
  hpwlEPs_1stOrder.push_back(
      std::pair< prec, prec >(trial_HPWLs[extPt2_1stOrder].first, ratio2));
  cout << "INFO:  1st-Order 1st Ext.Point:  " << extPt1_1stOrder
       << "  \tHPWL:  " << trial_HPWLs[extPt1_1stOrder].first << " (" << ratio1
       << ")" << endl;
  cout << "INFO:  1st-Order 2nd Ext.Point:  " << extPt2_1stOrder
       << "  \tHPWL:  " << trial_HPWLs[extPt2_1stOrder].first << " (" << ratio2
       << ")" << endl
       << endl;
}

void store1stOrder_ExtremePointsforThird() {
  prec ratio1 =
      (trial_HPWLs[extPt1_1stOrder].first - trial_HPWLs[0].first) /
      (trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first);
  prec ratio2 =
      (trial_HPWLs[extPt2_1stOrder].first - trial_HPWLs[0].first) /
      (trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first);
  prec ratio3 =
      (trial_HPWLs[extPt3_1stOrder].first - trial_HPWLs[0].first) /
      (trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first);

  hpwlEPs_1stOrder.clear();
  hpwlEPs_1stOrder.push_back(
      std::pair< prec, prec >(trial_HPWLs[extPt1_1stOrder].first, ratio1));
  hpwlEPs_1stOrder.push_back(
      std::pair< prec, prec >(trial_HPWLs[extPt2_1stOrder].first, ratio2));
  hpwlEPs_1stOrder.push_back(
      std::pair< prec, prec >(trial_HPWLs[extPt3_1stOrder].first, ratio3));
}

void calcRef_dWL() {
  prec HPWLrange;
  int tGPiterCNT;

  HPWLrange = trial_HPWLs[trial_HPWLs.size() - 1].first - trial_HPWLs[0].first;
  tGPiterCNT = trial_HPWLs.size();

  refDeltaWL = HPWLrange / (prec)tGPiterCNT / 2.0;
  cout << "INFO:  Reference DELTA Wirelength = " << refDeltaWL << endl;
}

void findMinHPWLduringTrial() {
  prec tempMinHPWL = PREC_MAX;
  unsigned tempMinIndex = UINT_MAX;

  for(unsigned i = 0; i < trial_HPWLs.size(); i++) {
    if(trial_HPWLs[i].first < tempMinHPWL) {
      tempMinHPWL = trial_HPWLs[i].first;
      tempMinIndex = i;
    }
  }

  if(0 != tempMinIndex) {
    trial_HPWLs.erase(trial_HPWLs.begin(),
                      trial_HPWLs.begin() + tempMinIndex - 1);
  }
}

void findMaxHPWLduringTrial() {
  unsigned idxThreshold = UINT_MAX;
  unsigned orgVecSize = trial_HPWLs.size();

  while(1) {
    for(unsigned i = 0; i < trial_HPWLs.size(); i++) {
      if(idxThreshold != UINT_MAX)
        continue;
      if(trial_HPWLs[i].first >= trial_HPWLs[trial_HPWLs.size() - 1].first) {
        idxThreshold = i;
      }
    }
    if(idxThreshold != UINT_MAX) {
      break;
    }
  }

  // To prevent the case that the max. HPWL is too close to the early
  // iterations...
  if(idxThreshold < (int)(trial_HPWLs.size() * 0.1)) {
    idxThreshold = trial_HPWLs.size() - 1;
  }

  cout << "INFO:  INDEX of FinalHPWLpoint: [" << idxThreshold << "]" << endl;

  if(trial_HPWLs.size() - 1 != idxThreshold) {
    for(unsigned i = idxThreshold; i < orgVecSize; i++) {
      trial_HPWLs.pop_back();
    }
  }
}

void get2ndOrderDiff_HPWL_LinearHPWLtrend() {
  prec initialHPWL = trial_HPWLs[0].first;
  prec finalHPWL = trial_HPWLs[trial_HPWLs.size() - 1].first;
  prec intervalHPWL =
      (finalHPWL - initialHPWL) / ((prec)(trial_HPWLs.size() - 1.0));

  for(unsigned i = 0; i < trial_HPWLs.size(); i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first - (initialHPWL + (prec)i * intervalHPWL);
  }
}

void get2ndOrderEP2() {
  prec initialHPWL = trial_HPWLs[0].first;
  prec ep1_HPWL = trial_HPWLs[extPt1_2ndOrder].first;
  prec finalHPWL = trial_HPWLs[trial_HPWLs.size() - 1].first;
  prec interval1HPWL = (ep1_HPWL - initialHPWL) / ((prec)(extPt1_2ndOrder));
  prec interval2HPWL = (finalHPWL - ep1_HPWL) /
                       ((prec)(trial_HPWLs.size() - 1 - extPt1_2ndOrder));
  prec tempMaxHPWLgap = PREC_MIN;
  prec tempMinHPWLgap = PREC_MAX;
  unsigned tempIndex = UINT_MAX;
  bool is_ep1_negative = false;

  if(trial_HPWLs[extPt1_2ndOrder].second < PREC_MIN)
    is_ep1_negative = true;

  for(unsigned i = 0; i < extPt1_2ndOrder + 1; i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first - (initialHPWL + (prec)i * interval1HPWL);
  }
  for(unsigned i = extPt1_2ndOrder; i < trial_HPWLs.size(); i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first -
        (ep1_HPWL + (prec)(i - extPt1_2ndOrder) * interval2HPWL);
  }

  if(is_ep1_negative) {
    // for (unsigned i=0; i<trial_HPWLs.size(); i++) {
    for(unsigned i = 0; i < extPt1_2ndOrder; i++) {
      if(trial_HPWLs[i].second > tempMaxHPWLgap) {
        tempMaxHPWLgap = trial_HPWLs[i].second;
        tempIndex = i;
      }
    }
  }
  else {
    // When the final HPWL is larger than EP1's HPWL (normal cases)
    if(finalHPWL > ep1_HPWL) {
      for(unsigned i = 0; i < trial_HPWLs.size(); i++) {
        if(trial_HPWLs[i].second < tempMinHPWLgap) {
          tempMinHPWLgap = trial_HPWLs[i].second;
          tempIndex = i;
        }
      }
    }
    // When the final HPWL is smaller than EP1's HPWL (abnormal cases)
    else {
      for(unsigned i = extPt1_2ndOrder; i < trial_HPWLs.size(); i++) {
        if(trial_HPWLs[i].second > tempMaxHPWLgap) {
          tempMaxHPWLgap = trial_HPWLs[i].second;
          tempIndex = i;
        }
      }
    }
  }
  extPt2_2ndOrder = tempIndex;
}

void reassign_trial_2ndOrder_lastEP(prec newHPWL) {
  if(orderHPWL == thirdOrder) {
    hpwlEPs_2ndOrder[3].first = newHPWL;
    hpwlEPs_1stOrder[2].first =
        min(hpwlEPs_2ndOrder[2].first,
            (prec)0.5 * (newHPWL + hpwlEPs_2ndOrder[2].first));
  }
  else if(orderHPWL == secondOrder) {
    hpwlEPs_2ndOrder[2].first = newHPWL;
    hpwlEPs_1stOrder[1].first =
        min(hpwlEPs_2ndOrder[1].first,
            (prec)0.5 * (newHPWL + hpwlEPs_2ndOrder[1].first));
  }
}

void get1stOrderDiff_HPWL_LinearHPWLtrendforSecond() {
  prec initialHPWL;
  prec finalHPWL;
  prec intervalHPWL;

  // 1st Point
  initialHPWL = trial_HPWLs[0].first;
  finalHPWL = trial_HPWLs[extPt1_2ndOrder].first;
  intervalHPWL = (finalHPWL - initialHPWL) / ((prec)extPt1_2ndOrder);
  for(unsigned i = 0; i < extPt1_2ndOrder + 1; i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first - (initialHPWL + (prec)i * intervalHPWL);
  }
  // 2nd Point
  initialHPWL = trial_HPWLs[extPt1_2ndOrder].first;
  finalHPWL = trial_HPWLs[trial_HPWLs.size() - 1].first;
  intervalHPWL = (finalHPWL - initialHPWL) /
                 ((prec)(trial_HPWLs.size() - 1.0 - extPt1_2ndOrder));
  for(unsigned i = extPt1_2ndOrder + 1; i < trial_HPWLs.size(); i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first -
        (initialHPWL + (prec)(i - extPt1_2ndOrder) * intervalHPWL);
  }
}

void get1stOrderDiff_HPWL_LinearHPWLtrendforThird() {
  prec initialHPWL;
  prec finalHPWL;
  prec intervalHPWL;

  // 1st Point
  initialHPWL = trial_HPWLs[0].first;
  finalHPWL = trial_HPWLs[extPt1_2ndOrder].first;
  intervalHPWL = (finalHPWL - initialHPWL) / ((prec)extPt1_2ndOrder);
  for(unsigned i = 0; i < extPt1_2ndOrder; i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first - (initialHPWL + (prec)i * intervalHPWL);
  }
  // 2nd Point
  initialHPWL = trial_HPWLs[extPt1_2ndOrder].first;
  finalHPWL = trial_HPWLs[extPt2_2ndOrder].first;
  intervalHPWL =
      (finalHPWL - initialHPWL) / ((prec)(extPt2_2ndOrder - extPt1_2ndOrder));
  for(unsigned i = extPt1_2ndOrder; i < extPt2_2ndOrder; i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first -
        (initialHPWL + (prec)(i - extPt1_2ndOrder) * intervalHPWL);
  }
  // 3rd Point
  initialHPWL = trial_HPWLs[extPt2_2ndOrder].first;
  finalHPWL = trial_HPWLs[trial_HPWLs.size() - 1].first;
  intervalHPWL = (finalHPWL - initialHPWL) /
                 ((prec)(trial_HPWLs.size() - 1.0 - extPt2_2ndOrder));
  for(unsigned i = extPt2_2ndOrder; i < trial_HPWLs.size(); i++) {
    trial_HPWLs[i].second =
        trial_HPWLs[i].first -
        (initialHPWL + (prec)(i - extPt2_2ndOrder) * intervalHPWL);
  }
}

void get1stOrder_ExtremePointsforSecond() {
  prec tempMinHPWLgap = PREC_MAX;
  prec tempMaxHPWLgap = PREC_MIN;
  unsigned tempMinIndex = UINT_MAX;
  unsigned tempMaxIndex = UINT_MAX;

  tempMaxHPWLgap = PREC_MIN;
  for(unsigned i = 1; i < extPt1_2ndOrder; i++) {
    if(trial_HPWLs[i].second > tempMaxHPWLgap) {
      tempMaxHPWLgap = trial_HPWLs[i].second;
      tempMaxIndex = i;
    }
  }
  extPt1_1stOrder = tempMaxIndex;

  tempMinHPWLgap = PREC_MAX;
  for(unsigned i = extPt1_2ndOrder; i < trial_HPWLs.size(); i++) {
    if(get_abs(trial_HPWLs[i].second) < tempMinHPWLgap &&
       get_abs(trial_HPWLs[i].second != 0)) {
      tempMinHPWLgap = get_abs(trial_HPWLs[i].second);
      tempMinIndex = i;
    }
  }

  tempMaxHPWLgap = PREC_MIN;
  for(unsigned i = extPt1_2ndOrder; i < trial_HPWLs.size(); i++) {
    if(trial_HPWLs[i].second > tempMaxHPWLgap) {
      tempMaxHPWLgap = trial_HPWLs[i].second;
      tempMaxIndex = i;
    }
  }

  extPt2_1stOrder =
      min(min(tempMaxIndex, tempMinIndex),
          prec2unsigned((prec)(trial_HPWLs.size() - 1 + extPt1_2ndOrder) / 2));
}

void get1stOrder_ExtremePointsforThird() {
  // prec            tempMinHPWLgap;
  prec tempMaxHPWLgap;
  // unsigned        tempMinIndex = INT_MAX;
  unsigned tempMaxIndex = UINT_MAX;
  unsigned margin;

  // 1stOrder EP1
  tempMaxHPWLgap = PREC_MIN;
  for(unsigned i = 1; i < extPt1_2ndOrder; i++) {
    if(trial_HPWLs[i].second > tempMaxHPWLgap) {
      tempMaxHPWLgap = trial_HPWLs[i].second;
      tempMaxIndex = i;
    }
  }
  extPt1_1stOrder = tempMaxIndex;

  // 1stOrder EP2
  // Search if there exist crossing points
  margin = prec2unsigned((extPt2_2ndOrder - extPt1_2ndOrder) * 0.2);
  // margin          = (unsigned)((extPt2_2ndOrder - extPt1_2ndOrder) * 0.2);
  tempMaxHPWLgap = PREC_MIN;
  for(unsigned i = extPt2_2ndOrder - margin; i > extPt1_2ndOrder + margin;
      i--) {
    // NewDS0526
    // for (unsigned i=extPt2_2ndOrder-margin; i>extPt1_2ndOrder+2*margin; i--)
    // {
    if(trial_HPWLs[i].second * trial_HPWLs[i - 1].second < PREC_MAX) {
      tempMaxIndex = i;
      break;
    }
    if(get_abs(trial_HPWLs[i].second) > tempMaxHPWLgap) {
      tempMaxHPWLgap = get_abs(trial_HPWLs[i].second);
      tempMaxIndex = i;
    }
  }
  extPt2_1stOrder = tempMaxIndex;

  // 1stOrder EP3
  // Search if there exist crossing points
  margin = prec2unsigned((trial_HPWLs.size() - extPt2_2ndOrder) * 0.2);
  // margin          = (unsigned)((trial_HPWLs.size()-extPt2_2ndOrder) * 0.2);
  tempMaxHPWLgap = PREC_MIN;
  for(unsigned i = trial_HPWLs.size() - margin; i > extPt2_2ndOrder + margin;
      i--) {
    // New2DS0526
    // for (unsigned i=trial_HPWLs.size()-margin; i>extPt2_2ndOrder+2*margin;
    // i--) {
    if(trial_HPWLs[i].second * trial_HPWLs[i - 1].second < PREC_MAX) {
      tempMaxIndex = i;
      break;
    }
    if(get_abs(trial_HPWLs[i].second) > tempMaxHPWLgap) {
      tempMaxHPWLgap = get_abs(trial_HPWLs[i].second);
      tempMaxIndex = i;
    }
  }
  extPt3_1stOrder = tempMaxIndex;
  // extPt3_1stOrder = min(tempMaxIndex,
  //            prec2unsigned((prec)(trial_HPWLs.size()-1 +extPt2_2ndOrder)/2));
}

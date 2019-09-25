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

#ifndef __GCELL__
#define __GCELL__

#include "replace_private.h"

class mypos {
 public:
  float x;
  float y;
};

class TILE {
  public:
  struct FPOS pmin;
  struct FPOS pmax;
  struct FPOS center;
  struct POS p;
  prec h_usage;
  prec v_usage;
  prec tmp_h_usage;
  prec tmp_v_usage;
  int pincnt;
  std::vector< int > cap;   // new
  std::vector< int > blkg;  // new
  // std::vector<std::vector<struct FPOS> > block_interval; // new
  std::vector< std::vector< mypos > > block_interval;  // new
  std::vector< int > route;                            // new
  prec infl_ratio;                                     // new
  std::vector< int > h_gr_usage_per_layer_l;
  std::vector< int > h_gr_usage_per_layer_r;
  std::vector< int > v_gr_usage_per_layer_l;
  std::vector< int > v_gr_usage_per_layer_r;
  int h_gr_usage_total;
  int v_gr_usage_total;
  // igkang
  // leftedge = 1
  // rightedge = 2
  prec h_supply2;
  prec v_supply2;
  prec h_supply;
  prec v_supply;
  prec area;
  prec h_inflation_ratio;
  prec v_inflation_ratio;
  prec inflation_area_thisTile;
  prec inflation_area_delta_thisTile;
  prec cell_area_befo_bloating_thisTile;
  prec inflatedRatio_thisTile;
  bool is_macro_included;
};

void tile_init_cGP2D();
void get_blockage();
void get_interval_length();
void adjust_edge_cap();
void tile_clear();
void tile_reset_gr_usages();
void get_gr_usages_total();

#endif

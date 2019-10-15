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
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "replace_private.h"
#include "bin.h"
#include "charge.h"
#include "plot.h"
#include "fft.h"
#include "wlen.h"
#include "opt.h"
#include "macro.h"
#include "gcell.h"


using std::cout;
using std::endl;
using std::max;
using std::min;
using std::string;


TILE *tile_mat;

void get_blockage() {
  struct TIER *tier = &tier_st[0];
  struct POS b0;
  struct POS b1;
  // struct FPOS interval = zeroFPoint;
  mypos interval;
  // struct TILE *bp = NULL;
  struct TILE *bpx = NULL;
  struct TILE *bpy = NULL;
  int idx = 0;
  int x, y = 0;
  prec max_x, min_x, max_y, min_y;

  std::unordered_map< string, int > term2idx;
  for(int i = 0; i < terminalCNT; ++i) {
    term2idx[terminalInstance[i].Name()] = i;
  }

  // cout <<"getting blockage..." <<endl;

  for(auto it = routeBlockageNodes.begin(); it != routeBlockageNodes.end();
      ++it) {
    auto &pmin = terminalInstance[term2idx[it->first]].pmin;
    auto &pmax = terminalInstance[term2idx[it->first]].pmax;

    b0.x = (int)((pmin.x - tier->tile_org.x) * tier->inv_tile_stp.x);
    b0.y = (int)((pmin.y - tier->tile_org.y) * tier->inv_tile_stp.y);
    b1.x = (int)((pmax.x - tier->tile_org.x) * tier->inv_tile_stp.x);
    b1.y = (int)((pmax.y - tier->tile_org.y) * tier->inv_tile_stp.y);

    if(fabs(b0.x * tier->tile_stp.x + tier->tile_org.x - pmin.x) < Epsilon) {
      b0.x--;
    }
    if(fabs(b0.y * tier->tile_stp.y + tier->tile_org.y - pmin.y) < Epsilon) {
      b0.y--;
    }

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

    idx = b0.x * tier->dim_tile.y + b0.y;
    for(x = b0.x, bpx = &tier->tile_mat[idx]; x <= b1.x;
        x++, bpx += tier->dim_tile.y) {
      min_x = max(bpx->pmin.x, pmin.x);
      max_x = min(bpx->pmax.x, pmax.x);
      for(y = b0.y, bpy = bpx; y <= b1.y; y++, bpy++) {
        max_y = min(bpy->pmax.y, pmax.y);
        min_y = max(bpy->pmin.y, pmin.y);

        for(auto m : it->second) {
          if(verticalCapacity[m - 1] != 0 && max_x >= min_x) {
            interval.x = min_x - bpx->pmin.x;
            interval.y = max_x - bpx->pmin.x;
            bpy->block_interval[m - 1].push_back(interval);
            // cout <<"tile=(" <<bpy->p.x <<"," <<bpy->p.y <<"), "
            //     <<"layer=" <<m <<"V, "
            //     //<<"interval=(" <<interval.x <<"," <<interval.y <<")"
            //     <<it->first <<", "
            //     <<"interval=(" <<min_x <<"," <<max_x <<")"
            //     <<endl;
          }
          if(horizontalCapacity[m - 1] != 0 && max_y >= min_y) {
            interval.x = min_y - bpy->pmin.y;
            interval.y = max_y - bpy->pmin.y;
            bpy->block_interval[m - 1].push_back(interval);
            // cout <<"tile=(" <<bpy->p.x <<"," <<bpy->p.y <<"), "
            //     <<"layer=" <<m <<"H, "
            //     //<<"interval=(" <<interval.x <<"," <<interval.y <<")"
            //     <<it->first <<", "
            //     <<"interval=(" <<min_y <<"," <<max_y <<")"
            //     <<endl;
          }
        }
      }
    }
  }
}

void get_interval_length() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = tier->tile_mat;

  // cout <<endl;
  // cout <<"interval sum ..." <<endl;

  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];
    for(int j = 0; j < nMetLayers; j++) {
      auto &interval = bp->block_interval[j];
      if(interval.size() == 0)
        continue;
      std::vector< prec > left;
      std::vector< prec > right;
      // cout <<"tile=(" <<bp->p.x <<"," <<bp->p.y <<"), "
      //     <<"layer=" <<j+1 <<", #int=" << interval.size() <<endl;
      for(auto m : interval) {
        left.push_back(m.x);
        right.push_back(m.y);
        // cout <<"  interval=(" <<m.x <<"," <<m.y <<")" <<endl;
      }
      // cout <<endl;
      std::sort(left.begin(), left.end());
      std::sort(right.begin(), right.end());
      int ptrl = 0;
      int ptrr = 0;
      int cnt = 0;
      bool started = false;
      prec interval_sum = 0.0;
      prec curr_left = 0.0;
      prec curr_right = 0.0;
      while(1) {
        if(ptrl < (int)left.size() && left[ptrl] <= right[ptrr]) {
          cnt++;
          if(started == false) {
            started = true;
            curr_left = left[ptrl];
          }
          ptrl++;
        }
        else {
          cnt--;
          if(started == false) {
            cout << "wrong" << endl;
            exit(0);
          }
          else {
            if(cnt <= 0) {
              started = false;
              curr_right = right[ptrr];
              interval_sum += curr_right - curr_left;
            }
            else {
              ;
            }
            ptrr++;
            if(ptrr >= (int)right.size()) {
              break;
            }
          }
        }
      }

      // cout <<"tile=(" <<bp->p.x <<"," <<bp->p.y <<"), "
      //     <<"layer=" <<j+1 <<", "
      // cout    <<"intervalsum=" <<interval_sum <<endl;

      prec tile_len = 0;
      if(horizontalCapacity[j] != 0) {
        tile_len = tileHeight;
      }
      else {
        tile_len = tileWidth;
      }
      // prec free_cap_perc = (tile_len - interval_sum * (1.0 -
      // blockagePorosity)) / tile_len;
      prec blkg_cap_perc = (interval_sum * (1.0 - blockagePorosity)) / tile_len;
      // prec free_cap = 0;
      prec blkg_cap = 0;
      // if (horizontalCapacity[j] != 0) {
      //  free_cap = free_cap_perc * horizontalCapacity[j];
      //  bp->cap[j] = (int)(free_cap);
      //} else {
      //  free_cap = free_cap_perc * verticalCapacity[j];
      //  bp->cap[j] = (int)(free_cap);
      //}
      if(horizontalCapacity[j] != 0) {
        blkg_cap = blkg_cap_perc * horizontalCapacity[j];
        bp->blkg[j] = (int)(blkg_cap);
      }
      else {
        blkg_cap = blkg_cap_perc * verticalCapacity[j];
        bp->blkg[j] = (int)(blkg_cap);
      }
    }
    bp->block_interval.clear();
    bp->block_interval.shrink_to_fit();
  }
}

void adjust_edge_cap() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = NULL;

  for(auto &m : edgeCapAdj) {
    auto col1 = std::get< 0 >(m);
    auto row1 = std::get< 1 >(m);
    auto lay1 = std::get< 2 >(m);
    auto col2 = std::get< 3 >(m);
    auto row2 = std::get< 4 >(m);
    // auto lay2 = std::get<5>(m);
    auto cap = std::get< 6 >(m);

    auto col = min(col1, col2);
    auto row = min(row1, row2);

    auto idx = col * tier->dim_tile.y + row;
    bp = &tier->tile_mat[idx];
    // bp->cap[lay1-1] = cap;
    bp->blkg[lay1 - 1] += bp->cap[lay1 - 1] - cap;
  }
}

void tile_init_temp() {
  struct TIER *tier = &tier_st[0];
  struct TILE *bp = tier->tile_mat;

  // cout <<endl;
  cout <<"resizing block_interval..." << endl;

  std::vector< int > cap(nMetLayers, 0);
  std::vector< int > blkg(nMetLayers, 0);
  std::vector< int > route(nMetLayers, 0);
  for(int i = 0; i < nMetLayers; i++) {
    if(horizontalCapacity[i] != 0) {
      cap[i] = horizontalCapacity[i];
    }
    else {
      cap[i] = verticalCapacity[i];
    }
  }

  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp = &tier->tile_mat[i];

    bp->block_interval.resize(nMetLayers);
    bp->cap = cap;
    bp->route = route;
    bp->blkg = blkg;
  }
  // cout <<"tot_tile_cnt=" <<tier->tot_tile_cnt <<", #layer="
  // <<verticalCapacity.size() <<endl;
  get_blockage();
  get_interval_length();
  adjust_edge_cap();
  // exit(0);
}

void tile_init_cGP2D() {
  struct TIER *tier = NULL;
  struct POS p;
  struct TILE *bp = NULL;
  struct TILE *bpx = NULL;
  struct TILE *bpy = NULL;
  struct POS b0;
  struct POS b1;
  prec max_x, min_x, max_y, min_y, area_share;

  int x, y;
  int idx;

  tier = &tier_st[0];
  int totNumHTracks = 0;
  int totNumVTracks = 0;
  for(int i = 0; i < nMetLayers; ++i) {
    totNumVTracks +=
        verticalCapacity[i] / (minWireWidth[i] + minWireSpacing[i]);
    totNumHTracks +=
        horizontalCapacity[i] / (minWireWidth[i] + minWireSpacing[i]);
  }
  int h_pitch = tileHeight * 1.0 / totNumHTracks * gRoute_pitch_scal;
  int v_pitch = tileWidth * 1.0 / totNumVTracks * gRoute_pitch_scal;
  tier->tile_stp.x = tileWidth;
  tier->tile_stp.y = tileHeight;
  tier->tile_area = tier->tile_stp.x * tier->tile_stp.y;
  tier->inv_tile_stp.x = 1.0 / tier->tile_stp.x;
  tier->inv_tile_stp.y = 1.0 / tier->tile_stp.y;
  tier->dim_tile.x = nXgrids;
  tier->dim_tile.y = nYgrids;
  tier->tot_tile_cnt = tier->dim_tile.x * tier->dim_tile.y;
  printf("INFO:  dim_tile_cGP2D.(x,y) = (%d, %d)\n", tier->dim_tile.x,
         tier->dim_tile.y);
  cout << "INFO:  Tile Area (Width, Height) = " << tier->tile_area << " ("
       << tier->tile_stp.x << ", " << tier->tile_stp.y << ")" << endl;
  cout << "INFO:  H_pitch = " << h_pitch << ", V_pitch = " << v_pitch << endl;
  tier->half_tile_stp.x = 0.5 * tier->tile_stp.x;
  tier->half_tile_stp.y = 0.5 * tier->tile_stp.y;
  tier->tile_org.x = gridLLx;
  tier->tile_org.y = gridLLy;
  tile_mat =
      (struct TILE *)malloc(sizeof(struct TILE) * tier->tot_tile_cnt);
  tier->tile_mat = tile_mat;
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    new(&tier->tile_mat[i]) TILE();

    p.x = i / tier->dim_tile.y;
    p.y = i % tier->dim_tile.y;
    bp = &tier->tile_mat[i];
    bp->pmin.x = tier->tile_org.x + (prec)p.x * tier->tile_stp.x;
    bp->pmin.y = tier->tile_org.y + (prec)p.y * tier->tile_stp.y;
    bp->pmax.x = bp->pmin.x + tier->tile_stp.x;
    bp->pmax.y = bp->pmin.y + tier->tile_stp.y;
    bp->center.x = bp->pmin.x + 0.5 * tier->tile_stp.x;
    bp->center.y = bp->pmin.y + 0.5 * tier->tile_stp.y;
    bp->area = (bp->pmax.x - bp->pmin.x) * (bp->pmax.y - bp->pmin.y);
    bp->p = p;
    bp->h_supply = bp->area / h_pitch;
    bp->v_supply = bp->area / v_pitch;
    bp->h_supply2 = bp->h_supply;
    bp->v_supply2 = bp->v_supply;
    bp->is_macro_included = false;
  }

  bool is_h = false;
  for(auto &m : edgeCapAdj) {
    auto col1 = std::get< 0 >(m);
    auto row1 = std::get< 1 >(m);
    auto lay1 = std::get< 2 >(m);
    auto col2 = std::get< 3 >(m);
    auto row2 = std::get< 4 >(m);
    // auto lay2 = std::get<5>(m);
    auto cap = std::get< 6 >(m);
    if(col1 > col2) {
      auto temp = col1;
      col1 = col2;
      col2 = temp;
    }
    if(row1 > row2) {
      auto temp = row1;
      row1 = row2;
      row2 = temp;
    }
    if(col1 == col2) {
      is_h = false;
    }
    else {
      is_h = true;
    }
    auto idx1 = col1 * tier->dim_tile.y + row1;
    auto idx2 = col2 * tier->dim_tile.y + row2;
    bp = &tier->tile_mat[idx1];
    if(is_h) {
      bp->h_supply2 -= (horizontalCapacity[lay1 - 1] - cap) /
                       (minWireWidth[lay1 - 1] + minWireSpacing[lay1 - 1]) /
                       edgeadj_coef * tier->tile_stp.x;
      if(lay1 <= 5 && horizontalCapacity[lay1 - 1] > 0 && cap < 0.01) {
        bp->is_macro_included = true;
      }
    }
    else {
      bp->v_supply2 -= (verticalCapacity[lay1 - 1] - cap) /
                       (minWireWidth[lay1 - 1] + minWireSpacing[lay1 - 1]) /
                       edgeadj_coef * tier->tile_stp.y;
      if(lay1 <= 5 && verticalCapacity[lay1 - 1] > 0 && cap < 0.01) {
        bp->is_macro_included = true;
      }
    }
    bp = &tier->tile_mat[idx2];
    if(is_h) {
      bp->h_supply -= (horizontalCapacity[lay1 - 1] - cap) /
                      (minWireWidth[lay1 - 1] + minWireSpacing[lay1 - 1]) /
                      edgeadj_coef * tier->tile_stp.x;
    }
    else {
      bp->v_supply -= (verticalCapacity[lay1 - 1] - cap) /
                      (minWireWidth[lay1 - 1] + minWireSpacing[lay1 - 1]) /
                      edgeadj_coef * tier->tile_stp.y;
    }
  }
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    bp->h_supply = min(bp->h_supply, bp->h_supply2);
    bp->v_supply = min(bp->v_supply, bp->v_supply2);
  }

  std::unordered_map< string, int > tempMap;
  for(int i = 0; i < terminalCNT; ++i) {
    string terminalName = terminalInstance[i].Name();
    tempMap[terminalName] = i;
  }
  for(auto it = routeBlockageNodes.begin(); it != routeBlockageNodes.end();
      ++it) {
    prec x_min = terminalInstance[tempMap[it->first]].pmin.x;
    prec x_max = terminalInstance[tempMap[it->first]].pmax.x;
    prec y_min = terminalInstance[tempMap[it->first]].pmin.y;
    prec y_max = terminalInstance[tempMap[it->first]].pmax.y;
    int blockedHTracks = 0;
    int blockedVTracks = 0;
    for(auto m : it->second) {
      blockedVTracks += verticalCapacity[m - 1] /
                        (minWireWidth[m - 1] + minWireSpacing[m - 1]);
      blockedHTracks += horizontalCapacity[m - 1] /
                        (minWireWidth[m - 1] + minWireSpacing[m - 1]);
    }
    prec porosityH = blockedHTracks * 1.0 / totNumHTracks;
    prec porosityV = blockedVTracks * 1.0 / totNumVTracks;

    b0.x = (int)((x_min - tier->tile_org.x) * tier->inv_tile_stp.x);
    b0.y = (int)((y_min - tier->tile_org.y) * tier->inv_tile_stp.y);
    b1.x = (int)((x_max - tier->tile_org.x) * tier->inv_tile_stp.x);
    b1.y = (int)((y_max - tier->tile_org.y) * tier->inv_tile_stp.y);
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

    idx = b0.x * tier->dim_tile.y + b0.y;
    for(x = b0.x, bpx = &tier->tile_mat[idx]; x <= b1.x;
        x++, bpx += tier->dim_tile.y) {
      max_x = min(bpx->pmax.x, x_max);
      min_x = max(bpx->pmin.x, x_min);
      for(y = b0.y, bpy = bpx; y <= b1.y; y++, bpy++) {
        max_y = min(bpy->pmax.y, y_max);
        min_y = max(bpy->pmin.y, y_min);
        area_share = (max_x - min_x) * (max_y - min_y);
        bpy->h_supply -= area_share / h_pitch * porosityH;
        bpy->v_supply -= area_share / v_pitch * porosityV;
        if(bpy->h_supply < 0.01) {
          bpy->h_supply = 0.01;
        }
        if(bpy->v_supply < 0.01) {
          bpy->v_supply = 0.01;
        }
        bpy->is_macro_included = true;
      }
    }
  }

  tile_init_temp();
}

void tile_clear() {
  struct TIER *tier = &tier_st[0];
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    tier->tile_mat[i].h_inflation_ratio = 0;
    tier->tile_mat[i].v_inflation_ratio = 0;
    tier->tile_mat[i].h_usage = 0;
    tier->tile_mat[i].v_usage = 0;
    tier->tile_mat[i].pincnt = 0;
    tier->tile_mat[i].tmp_h_usage = 0;
    tier->tile_mat[i].tmp_v_usage = 0;
    tier->tile_mat[i].h_gr_usage_total = 0;
    tier->tile_mat[i].v_gr_usage_total = 0;
    tier->tile_mat[i].h_gr_usage_per_layer_l.clear();
    tier->tile_mat[i].v_gr_usage_per_layer_l.clear();
    tier->tile_mat[i].h_gr_usage_per_layer_r.clear();
    tier->tile_mat[i].v_gr_usage_per_layer_r.clear();
    tier->tile_mat[i].h_gr_usage_per_layer_l.shrink_to_fit();
    tier->tile_mat[i].v_gr_usage_per_layer_l.shrink_to_fit();
    tier->tile_mat[i].h_gr_usage_per_layer_r.shrink_to_fit();
    tier->tile_mat[i].v_gr_usage_per_layer_r.shrink_to_fit();
    tier->tile_mat[i].infl_ratio = 1.0;
  }
}

void tile_reset_gr_usages() {
  struct TIER *tier = &tier_st[0];
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    for(int j = 0; j < nMetLayers; j++) {
      tier->tile_mat[i].h_gr_usage_per_layer_l.push_back(0);
      tier->tile_mat[i].v_gr_usage_per_layer_l.push_back(0);
      tier->tile_mat[i].h_gr_usage_per_layer_r.push_back(0);
      tier->tile_mat[i].v_gr_usage_per_layer_r.push_back(0);
      tier->tile_mat[i].route[j] = 0;
    }
  }
}

void get_gr_usages_total() {
  struct TIER *tier = &tier_st[0];
  double temp_h_gr_usage_total;
  double temp_v_gr_usage_total;
  for(int i = 0; i < tier->tot_tile_cnt; i++) {
    temp_h_gr_usage_total = 0;
    temp_v_gr_usage_total = 0;
    for(int j = 0; j < nMetLayers; j++) {
      temp_h_gr_usage_total +=
          max((double)tier->tile_mat[i].h_gr_usage_per_layer_l[j],
              (double)tier->tile_mat[i].h_gr_usage_per_layer_r[j]);
      temp_v_gr_usage_total +=
          max((double)tier->tile_mat[i].v_gr_usage_per_layer_l[j],
              (double)tier->tile_mat[i].v_gr_usage_per_layer_r[j]);
    }
    tier->tile_mat[i].h_gr_usage_total = (prec)temp_h_gr_usage_total;
    tier->tile_mat[i].v_gr_usage_total = (prec)temp_v_gr_usage_total;

    tier->tile_mat[i].h_usage =
        (tier->tile_mat[i].h_gr_usage_total * tileWidth);  // * 0.5;
    tier->tile_mat[i].v_usage =
        (tier->tile_mat[i].v_gr_usage_total * tileHeight);  // * 0.5;
  }
}

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

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "bookShelfIO.h"
#include "lefdefIO.h"
#include "bin.h"
#include "replace_private.h"
#include "initPlacement.h"
#include "macro.h"
#include "opt.h"


void CallDetailPlace() {
  for(int i = 0; i < numLayer; i++) {
    char tier_dir[BUF_SZ] =
        {
            0,
        },
         tier_aux[BUF_SZ] =
             {
                 0,
             },
         tier_pl[BUF_SZ] =
             {
                 0,
             },
         tier_dp[BUF_SZ] = {
             0,
         };

    sprintf(tier_dir, "%s/tiers/%d", dir_bnd, i);
    sprintf(tier_aux, "%s.aux", gbch);
    sprintf(tier_pl, "%s.pl", gbch);

    TIER *tier = &tier_st[i];
    if(tier->modu_cnt <= 0) {
      continue;
    }

    switch(detailPlacer) {
      case NTUplace3:  // Default of ePlace except SB benchmarks.
        CallNtuPlacer3(tier_dir, tier_aux, tier_pl);
        if(onlyLG_CMD) {
          sprintf(tier_dp, "%s/%s.lg.pl", tier_dir, gbch);
        }
        else {
          sprintf(tier_dp, "%s/%s.ntup.pl", tier_dir, gbch);
        }
        break;

      case NTUplace4h:  // ePlace will always use NTUplace4h for SB
        // benchmarks.
        CallNtuPlacer4h(tier_dir, tier_aux, tier_pl);

        if(onlyLG_CMD) {
          sprintf(tier_dp, "%s/%s.lg.pl", tier_dir, gbch);
        }
        else {
          sprintf(tier_dp, "%s/%s.ntup.pl", tier_dir, gbch);
        }
        break;
    }

    // update module's data
    cout << "INFO:  READ AGAIN: " << tier_dp << endl;

    // execute ReadPl in lefdefIO.cpp
    ReadPl(tier_dp);
  }
}

void preprocess_SB_inputs(char *tier_dir) {
  char cmd[BUF_SZ];

  sprintf(cmd, "%s/convert1.tcl %s %s", currentDir, tier_dir, gbch);
  system(cmd);
}

void postprocess_SB_inputs(char *tier_dir) {
  char cmd[BUF_SZ];

  sprintf(cmd, "%s/convert2.tcl %s %s", currentDir, tier_dir, gbch);
  system(cmd);
}

void CallNtuPlacer3(const char *tier_dir, const char *tier_aux, const char *tier_pl) {
  char cmd[BUF_SZ] = {
      0,
  };

  sprintf(cmd, "ln -s %s %s/", detailPlacerLocation.c_str(), tier_dir);
  cout << cmd << endl;
  system(cmd);

  //    if(INPUT_FLG == ISPD || INPUT_FLG == MMS || INPUT_FLG == ETC) {
  sprintf(cmd,
          "cd %s && ./ntuplace3 -aux %s -loadpl %s -util %.2lf -noglobal \n",
          tier_dir, tier_aux, tier_pl,
          (hasDensityDP) ? densityDP : target_cell_den);

  // call
  cout << "[INFO] Call NtuPlacer3" << endl;
  cout << cmd << endl;
  system(cmd);
 /* 

  // Below is for checking additional RC
  sprintf( cmd, "ln -s /home/mgwoo/01_placement/RePlAce/router/convert.py  %s/", tier_dir);
  cout << cmd << endl;
  system(cmd);

  char realDirBnd[PATH_MAX] = {0, };
  char* ptr = realpath(dir_bnd, realDirBnd);
  
  char realCurDir[PATH_MAX] = {0, };
  ptr = realpath(currentDir, realCurDir);

  sprintf( cmd, "ln -s %s/router_base/%s.route %s/", realDirBnd, gbch, tier_dir);
  cout << cmd << endl;
  system(cmd);
  
  sprintf( cmd, "ln -s %s/../router/*.dat %s/", realCurDir, tier_dir);
  cout << cmd << endl;
  system(cmd);

  sprintf( cmd, "cd %s && python ./convert.py %s.aux", tier_dir, gbch); 
  cout << cmd << endl;
  system(cmd);
  
  sprintf( cmd, "cd %s && %s/../router/%s ICCAD %s_mapped.aux %s_mapped.pl %s/../router/ICCAD12.NCTUgr.set %s_gp.est", tier_dir, currentDir, global_router, gbch, gbch, 
      currentDir, gbch);
  cout << cmd << endl;
  system(cmd);

  sprintf( cmd, "cd %s && %s/../router/%s ICCAD %s_mapped.aux %s_mapped.ntup.pl %s/../router/ICCAD12.NCTUgr.set %s_dp.est", tier_dir, currentDir, global_router, gbch, gbch, 
      currentDir, gbch);
  cout << cmd << endl;
  system(cmd);
  
  sprintf( cmd, "ln -s %s/../router/*.pl %s/", realCurDir, tier_dir); 
  cout << cmd << endl;
  system(cmd);
  
  sprintf( cmd, "cd %s && perl iccad2012_evaluate_solution.pl -p %s_mapped.aux %s_mapped.pl %s_gp.est", tier_dir, gbch, gbch, gbch); 
  cout << cmd << endl;
  system(cmd);

  sprintf( cmd, "cd %s && perl iccad2012_evaluate_solution.pl -p %s_mapped.aux %s_mapped.ntup.pl %s_dp.est", tier_dir, gbch, gbch, gbch); 
  cout << cmd << endl;
  system(cmd);
  */
}

void CallNtuPlacer4h(const char *tier_dir, const char *tier_aux, const char *tier_pl) {
  char cmd[BUF_SZ] = {
      0,
  };

  sprintf(cmd, "ln -s %s %s/", detailPlacerLocation.c_str(), tier_dir);
  cout << cmd << endl;
  system(cmd);

  sprintf(cmd,
          "cd %s && ./ntuplace4h -aux %s -loadpl %s -noglobal -nodetail \n",
          tier_dir, tier_aux, tier_pl);

  // call
  cout << "[INFO] Call NtuPlacer4h" << endl;
  cout << cmd << endl;
  system(cmd);

  

  // Below is for checking additional RC
  char realDirBnd[PATH_MAX] = {0, };
  char* ptr = realpath(dir_bnd, realDirBnd);
  
  char realCurDir[PATH_MAX] = {0, };
  ptr = realpath(currentDir, realCurDir);

//  sprintf( cmd, "ln -s %s/router_base/%s.route %s/", realDirBnd, gbch, tier_dir);
//  cout << cmd << endl;
//  system(cmd);

/*  
  sprintf( cmd, "ln -s %s/../router/convert.py  %s/", realCurDir, tier_dir);
  cout << cmd << endl;
  system(cmd);
  
  sprintf( cmd, "ln -s %s/../router/*.dat %s/", realCurDir, tier_dir);
  cout << cmd << endl;
  system(cmd);

  sprintf( cmd, "cd %s && python ./convert.py %s.aux", tier_dir, gbch); 
  cout << cmd << endl;
  system(cmd);
  
  sprintf( cmd, "cd %s && %s ICCAD %s_mapped.aux %s_mapped.pl %s %s_gp.est", tier_dir, globalRouterPosition.c_str(), gbch, gbch, 
      globalRouterSetPosition.c_str(), gbch);
  cout << cmd << endl;
  system(cmd);

  sprintf( cmd, "cd %s && %s ICCAD %s_mapped.aux %s_mapped.ntup.pl %s %s_dp.est", tier_dir, globalRouterPosition.c_str(), gbch, gbch, 
      globalRouterSetPosition.c_str(), gbch);
  cout << cmd << endl;
  system(cmd);
  
  sprintf( cmd, "ln -s %s/../router/*.pl %s/", realCurDir, tier_dir); 
  cout << cmd << endl;
  system(cmd);
  
//  sprintf( cmd, "cd %s && perl iccad2012_evaluate_solution.pl -p %s_mapped.aux %s_mapped.pl %s_gp.est", tier_dir, gbch, gbch, gbch); 
  sprintf( cmd, "cd %s && perl iccad2012_evaluate_solution.pl %s_mapped.aux %s_mapped.pl %s_gp.est", tier_dir, gbch, gbch, gbch); 
  cout << cmd << endl;
  system(cmd);

//  sprintf( cmd, "cd %s && perl iccad2012_evaluate_solution.pl -p %s_mapped.aux %s_mapped.ntup.pl %s_dp.est", tier_dir, gbch, gbch, gbch); 
  sprintf( cmd, "cd %s && perl iccad2012_evaluate_solution.pl %s_mapped.aux %s_mapped.ntup.pl %s_dp.est", tier_dir, gbch, gbch, gbch); 
  cout << cmd << endl;
  system(cmd);
  */
}


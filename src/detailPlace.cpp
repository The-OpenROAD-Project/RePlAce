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

#include <error.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "bookShelfIO.h"
#include "lefdefIO.h"
#include "bin.h"
#include "global.h"
#include "initPlacement.h"
#include "macro.h"
#include "opt.h"

void CallNtuPlacer3(char *tier_dir, char *tier_aux, char *tier_pl);
void CallNtuPlacer4h(char *tier_dir, char *tier_aux, char *tier_pl);

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

    //        if(dpMode == NTUplace4h && !strcmp(bmFlagCMD.c_str(), "sb")) {
    //            link_original_SB_files_to_Dir(tier_dir);
    //        }

    TIER *tier = &tier_st[i];
    if(tier->modu_cnt <= 0) {
      continue;
    }

    switch(detailPlacer) {
      case FastPlace:
        call_FastDP_tier(tier_dir, tier_aux, tier_pl);
        sprintf(tier_dp, "%s/%s_FP_dp.pl", tier_dir, gbch);
        break;

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

void call_FastDP_tier(char *tier_dir, char *tier_aux, char *tier_pl) {
  char cmd[BUF_SZ];

#ifdef NO_DP_3DIC

  sprintf(cmd,
          "%s -legalize -noFlipping -noDp -target_density %.2lf -window 10 "
          "%s %s %s %s\n",
          detailPlacerLocationCMD.c_str(), target_cell_den, tier_dir, tier_aux,
          tier_dir, tier_pl);

#else

  sprintf(cmd,
          "%s -legalize -noFlipping -target_density %.2lf -window 10 %s %s "
          "%s %s\n",
          detailPlacerLocationCMD.c_str(), target_cell_den, tier_dir, tier_aux,
          tier_dir, tier_pl);

#endif

  system(cmd);
}

void CallNtuPlacer3(char *tier_dir, char *tier_aux, char *tier_pl) {
  char cmd[BUF_SZ] = {
      0,
  };

  sprintf(cmd, "ln -s %s %s/", detailPlacerLocationCMD.c_str(), tier_dir);
  cout << cmd << endl;
  system(cmd);

  //    if(INPUT_FLG == ISPD || INPUT_FLG == MMS || INPUT_FLG == ETC) {
  sprintf(cmd,
          "cd %s && ./ntuplace3 -aux %s -loadpl %s -util %.2lf -noglobal \n",
          tier_dir, tier_aux, tier_pl,
          (hasDensityDP) ? densityDP : target_cell_den);

  //    }
  //    else {
  //        sprintf(cmd, "cd %s && ./ntuplace3 -aux %s -loadpl %s -noglobal \n",
  //                tier_dir, tier_aux, tier_pl);
  //    }

  // call
  cout << "Call NtuPlacer3" << endl;
  cout << cmd << endl;
  system(cmd);

  /*
  // mv All generated pl files
  cout << "Move to " << tier_dir << "..." << endl;
  sprintf(cmd,"mv ./%s.ntup.pl %s", gbch, tier_dir);
  cout << cmd << endl;
  system(cmd);

  sprintf(cmd,"mv ./%s.ntup.plt %s", gbch, tier_dir);
  cout << cmd << endl;
  system(cmd);

  sprintf(cmd,"mv ./%s.lg.pl %s", gbch, tier_dir);
  cout << cmd << endl;
  system(cmd);

  sprintf(cmd,"mv ./%s.lg.plt %s", gbch, tier_dir);
  cout << cmd << endl;
  system(cmd);
  */
}

void CallNtuPlacer4h(char *tier_dir, char *tier_aux, char *tier_pl) {
  char cmd[BUF_SZ] = {
      0,
  };
  sprintf(cmd, "%s -aux %s/%s -loadpl %s/%s -noglobal \n",
          detailPlacerLocationCMD.c_str(), tier_dir, tier_aux, tier_dir,
          tier_pl);

  cout << "INFO:  Execute NTUPlacer4h" << endl;
  cout << cmd << endl;

  system(cmd);
  //    chdir(currentDir);
  // sprintf (cmd,"mv ./%s.ntup.pl %s", gbch, tier_dir);
  // system (cmd);
  // sprintf (cmd,"mv ./%s.ntup.plt %s", gbch, tier_dir);
  // system (cmd);

  // sprintf (cmd,"mv ./%s.lg.pl %s", gbch, tier_dir);
  // system (cmd);
  // sprintf (cmd,"mv ./%s.lg.plt %s", gbch, tier_dir);
  // system (cmd);

  // sprintf (cmd,"mv ./%s.init.plt %s", gbch, tier_dir);
  // system (cmd);
  // sprintf (cmd,"mv ./%s.dp.plt %s", gbch, tier_dir);
  // system (cmd);
}

/*
 * unused code

void call_DP(void) {
#ifdef NO_WLEN
    return;
#endif

#ifdef NO_DEN
    return;
#endif

    switch(dpMode) {
        case FastPlace:
            call_FastDP();
            break;
        case NTUplace3:
            call_NTUpl3();
            break;
    }
}
void call_FastDP(void) {
    char cmd[BUF_SZ];

    switch(INPUT_FLG) {
        case IBM:
            sprintf(cmd, "%s -legalize -noFlipping %s %s %s %s\n",
dpLocation.c_str(),
                    gbch_dir, gbch_aux, dir_bnd, gGP_pl_file);
            break;
        case ISPD05:
            sprintf(cmd, "%s -legalize -noFlipping %s %s %s %s\n",
dpLocation.c_str(),
                    gbch_dir, gbch_aux, dir_bnd, gGP_pl_file);
            break;
        case ISPD06:
            sprintf(cmd,
                    "%s -legalize -noFlipping -target_density %.2lf -window "
                    "10 %s %s %s %s\n",
                    dpLocation.c_str(), target_cell_den, gbch_dir, gbch_aux,
dir_bnd,
                    gGP_pl_file);
            break;
        case ISPD06m:
            sprintf(cmd,
                    "%s -legalize -noFlipping -target_density %.2lf -window "
                    "10 %s %s %s %s\n",
                    dpLocation.c_str(), target_cell_den, gbch_dir, gbch_aux,
dir_bnd,
                    gGP_pl_file);
            break;
        case MMS:
            sprintf(cmd,
                    "%s -legalize -noFlipping -target_density %.2lf -window "
                    "10 %s %s %s %s\n",
                    dpLocation.c_str(), target_cell_den, gbch_dir, gbch_aux,
dir_bnd,
                    gGP_pl_file);

            break;
        case SB:
            sprintf(cmd,
                    "%s -legalize -noFlipping -target_density %.2lf -window "
                    "10 %s %s %s %s\n",
                    dpLocation.c_str(), target_cell_den, gbch_dir, gbch_aux,
dir_bnd,
                    gGP_pl_file);

            break;
        case ETC:
            sprintf(cmd,
                    "%s -legalize -noFlipping -target_density %.2lf -window "
                    "10 %s %s %s %s\n",
                    dpLocation.c_str(), target_cell_den, gbch_dir, gbch_aux,
dir_bnd,
                    gGP_pl_file);

            break;
    }

    system(cmd);
}

void call_NTUpl3(void) {
    char cmd[BUF_SZ];

    switch(INPUT_FLG) {
        case IBM:
            sprintf(cmd, "%s -aux %s/%s -loadpl %s/%s -noglobal \n",
                    dpLocation.c_str(), gbch_dir, gbch_aux,
                    // target_cell_den ,
                    dir_bnd, gGP_pl_file);
            break;
        case ISPD05:
            sprintf(cmd, "%s -aux %s/%s -loadpl %s/%s -noglobal \n",
                    dpLocation.c_str(), gbch_dir, gbch_aux,
                    // target_cell_den ,
                    dir_bnd, gGP_pl_file);
            break;
        case ISPD06:
            sprintf(cmd,
                    "%s -aux %s/%s -util %.2lf -loadpl %s/%s -noglobal \n",
                    dpLocation.c_str(), gbch_dir, gbch_aux, target_cell_den,
dir_bnd,
                    gGP_pl_file);
            break;
        case ISPD06m:
            sprintf(cmd,
                    "%s -aux %s/%s -util %.2lf -loadpl %s/%s -noglobal \n",
                    dpLocation.c_str(), gbch_dir, gbch_aux, target_cell_den,
dir_bnd,
                    gGP_pl_file);
            break;
        case MMS:
            sprintf(cmd,
                    "%s -aux %s/%s -util %.2lf -loadpl %s/%s -noglobal \n",
                    dpLocation.c_str(), gbch_dir, gbch_aux, target_cell_den,
dir_bnd,
                    gGP_pl_file);
            break;
        case SB:
            sprintf(cmd,
                    "%s -aux %s/%s -util %.2lf -loadpl %s/%s -noglobal \n",
                    detailPlacerLocationCMD.c_str(), gbch_dir, gbch_aux,
target_cell_den, dir_bnd,
                    gGP_pl_file);
            break;
        case ETC:
            sprintf(cmd,
                    "%s -aux %s/%s -util %.2lf -loadpl %s/%s -noglobal \n",
                    detailPlacerLocationCMD.c_str(), gbch_dir, gbch_aux,
target_cell_den, dir_bnd,
                    gGP_pl_file);
            break;
    }

    system(cmd);
}
*/

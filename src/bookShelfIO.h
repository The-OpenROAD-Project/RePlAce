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

#ifndef __PL_IO__
#define __PL_IO__

#include <sparsehash/dense_hash_map>
#include "global.h"
#include "lefdefIO.h"
#include "routeOpt.h"

#define BUFFERSIZE 1000
#define LINESIZE 2000

using google::dense_hash_map;
using std::vector;
using std::tuple;

// for *.nodes files
struct NODES {
  long index;
  bool isTerminal;
  bool isTerminalNI;
  NODES(long _index, bool _isTerminal, bool _isTerminalNI)
      : index(_index), isTerminal(_isTerminal), isTerminalNI(_isTerminalNI){};
  NODES() : index(0), isTerminal(false), isTerminalNI(false){};
};

// for *.scl files
struct ROW {
  prec site_wid;  // SiteWidth
  prec site_spa;  // SiteSpacing

  string ori;
  //    string sym;
  bool isXSymmetry;
  bool isYSymmetry;
  bool isR90Symmetry;

  int x_cnt;  // NumSites
  FPOS pmin;
  FPOS pmax;
  FPOS size;

  ROW()
      : site_wid(0),
        site_spa(0),
        ori(""),
        isXSymmetry(false),
        isYSymmetry(false),
        isR90Symmetry(false),
        x_cnt(0) {
    pmin.SetZero();
    pmax.SetZero();
    size.SetZero();
  }

  void Dump(string a) {
    cout << a << endl;
    cout << "site_wid: " << site_wid << endl;
    cout << "site_spa: " << site_spa << endl;
    cout << "ori: " << ori << endl;
    cout << "x_cnt: " << x_cnt << endl;
    pmin.Dump("pmin");
    pmax.Dump("pmax");
    size.Dump("size");
    cout << endl;
  }
};

class BookshelfNameMap {
  public:
    // bs means Bookshelf
    dense_hash_map<string, string> moduleToBsMap, bsToModuleMap;
    dense_hash_map<string, string> terminalToBsMap, bsToTerminalMap;
    dense_hash_map<string, string> netToBsMap, bsToNetMap;
    int bsModuleCnt, bsTerminalCnt, bsNetCnt;
    void Init();

    const char* GetOrigModuleName( const char* name );
    const char* GetOrigTerminalName( const char* name );
    const char* GetBsModuleName( const char* name );
    const char* GetBsTerminalName( const char* name );
    const char* GetBsNetName( const char* name );
};

extern BookshelfNameMap _bsMap;

// Due to sorting in *.nets!!!
struct BsNetInfo {
  string name, io;
  prec x, y;
  BsNetInfo(string _name, string _io, prec _x, prec _y):
    name(_name), io(_io), x(_x), y(_y) {};

  void Print( FILE* file );
};

void runtimeError(string error_text);
void ParseBookShelf();
int read_nodes_3D(char *input);
int read_shapes_3D(char *input);
int read_routes_3D(char *input);
int read_nets_3D(char *input);
int read_pl2(char *input);
int read_scl(char *input);

// void post_read_2d(void);
void post_read_3d(void);
void transform_3d(FPOS *tier_min, FPOS *tier_max, int tier_row_cnt);

void extract_dir(char *f, char *d);
void extract_sfx(char *f, char *s);

void output_pl(char *output);
void output_cGP3D_pl(char *output);
void output_mGP2D_pl(char *output);
void output_cGP2D_pl(char *output);

void AddPinInfoForModuleAndTerminal(PIN ***pin, FPOS **pof, int currentPinCount,
                                    FPOS offset, int curModuleIdx,
                                    int curNetIdx, int curPinIdxInNet,
                                    int curPinIdx, int curPinDirection,
                                    int isTerminal);

void get_mms_3d_dim(FPOS *tier_min, FPOS *tier_max, int *tier_row_cnt);

void write_new_bench(void);
void write_3d_bench(void);  // useless

void WriteBookshelfWithTier(char* targetDir, int z, int lab, bool isShapeDrawing = true, 
    bool isNameConvert = false, bool isMetal1Removed = false);
void ReadPlBookshelf(const char *fileName);

void output_tier_pl_global_router(string plName, int z, int lab, bool isNameConvert = false);
void link_original_SB_files_to_Dir(char *dir);
void LinkConvertedBookshelf(char* newDir);
void delete_input_files_in(char *dir);
void read_routing_file(char *dir, string routeName);

// void gp_sol_trans_to_dim_one(void);
// void gp_sol_trans_from_dim_one(void);
void output_final_pl(char *output);
// void output_net_hpwl(char *fn);
// void output_hgraph_nofix(char *fn);


#endif

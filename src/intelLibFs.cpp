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

#include <iostream>
#include <cstdio>
#include <cstring>

#include "global.h"
#include "intelLibFs.h"
#include "mkl.h"
#include "ipp.h"

using std::string;
using std::cout;
using std::endl;

extern prec *wx_2d_iL;
extern prec *wy_2d_iL;
extern prec *wx2_2d_iL;
extern prec *wy2_2d_iL;
extern prec *wx_2d_st;
extern prec *wx2_2d_st;
extern prec *wy_2d_st;
extern prec *wy2_2d_st;

/// Intel MKL and Intel IPP DCT access data array in different way.
/// One interleaves and the other in-order.
/// Please implement your code carefully.
int intelLIB::IPP::fwdDCT2D(const Ipp32f *src, Ipp32f *dst,
                            const MKL_INT numGrids) {
  Ipp8u *pMemInit = 0;
  Ipp8u *pMemBuffer = 0;
  IppiDCTFwdSpec_32f *pMemSpec;
  IppiSize roiSize = {(int)numGrids, (int)numGrids};
  int srcStep;
  int dstStep;
  int sizeSpec;
  int sizeInit;
  int sizeBuffer;
  /// Get srcStep and dstStep
  srcStep = dstStep = numGrids * sizeof(Ipp32f);
  /// Get sizes for required buffers
  ippiDCTFwdGetSize_32f(roiSize, &sizeSpec, &sizeInit, &sizeBuffer);
  /// Allocate memory for required buffers
  pMemSpec = (IppiDCTFwdSpec_32f *)ippMalloc(sizeSpec);
  if(sizeInit > 0)
    pMemInit = (Ipp8u *)ippMalloc(sizeInit);
  if(sizeBuffer > 0)
    pMemBuffer = (Ipp8u *)ippMalloc(sizeBuffer);
  /// Initialize DCT specification structure
  ippiDCTFwdInit_32f(pMemSpec, roiSize, pMemInit);
  /// Free initialization buffer
  if(sizeInit > 0)
    ippFree(pMemInit);
  /// Perform Forward 2D DCT
  ippiDCTFwd_32f_C1R(src, srcStep, dst, dstStep, pMemSpec, pMemBuffer);
  /// Free buffers
  if(sizeBuffer > 0)
    ippFree(pMemBuffer);
  ippFree(pMemSpec);

  return 0;
}

int intelLIB::IPP::invDCT2D(const Ipp32f *src, Ipp32f *dst,
                            const MKL_INT numGrids) {
  Ipp8u *pMemInit = 0;
  Ipp8u *pMemBuffer = 0;
  IppiDCTInvSpec_32f *pMemSpec;
  IppiSize roiSize = {(int)numGrids, (int)numGrids};
  int srcStep;
  int dstStep;
  int sizeSpec;
  int sizeInit;
  int sizeBuffer;
  /// Get srcStep and dstStep
  srcStep = dstStep = numGrids * sizeof(Ipp32f);
  /// Get sizes for required buffers
  ippiDCTInvGetSize_32f(roiSize, &sizeSpec, &sizeInit, &sizeBuffer);
  /// Allocate memory for required buffers
  pMemSpec = (IppiDCTInvSpec_32f *)ippMalloc(sizeSpec);
  if(sizeInit > 0)
    pMemInit = (Ipp8u *)ippMalloc(sizeInit);
  if(sizeBuffer > 0)
    pMemBuffer = (Ipp8u *)ippMalloc(sizeBuffer);
  /// Initialize DCT specification structure
  ippiDCTInvInit_32f(pMemSpec, roiSize, pMemInit);
  /// Free initialization buffer
  if(sizeInit > 0)
    ippFree(pMemInit);
  /// Perform Inverse 2D DCT
  ippiDCTInv_32f_C1R(src, srcStep, dst, dstStep, pMemSpec, pMemBuffer);
  /// Free buffers
  if(sizeBuffer > 0)
    ippFree(pMemBuffer);
  ippFree(pMemSpec);

  return 0;
}

int intelLIB::MKL::DCT1D(float *inout, const int direction, MKL_INT numGrids) {
  /// Trigonometric Transform Type
  MKL_INT ttt = MKL_STAGGERED_COSINE_TRANSFORM;
  /// MKL routine status
  MKL_INT stat;
  /// MKL options with INT array type
  MKL_INT *ipar;
  /// MKL computation space
  float *spar;
  float *tmp;
  float **aRow;

  DFTI_DESCRIPTOR_HANDLE handle = 0;
  aRow = (float **)mkl_malloc(sizeof(float *) * numGrids, 64);
  for(int x = 0; x < numGrids; x++) {
    aRow[x] = inout + x * numGrids;
  }

  tmp = (float *)mkl_malloc(numGrids * sizeof(float), 64);
  spar = (float *)mkl_malloc((5 * numGrids / 2 + 2) * sizeof(float), 64);
  ipar = (MKL_INT *)malloc((128) * sizeof(MKL_INT));
  s_init_trig_transform(&numGrids, &ttt, ipar, spar, &stat);
  s_commit_trig_transform(aRow[numGrids - 1], &handle, ipar, spar, &stat);
  if(direction == 1) {
    for(int x = 0; x < numGrids; x++) {
      memcpy(tmp, aRow[x], numGrids * sizeof(float));
      s_forward_trig_transform(tmp, &handle, ipar, spar, &stat);
      memcpy(aRow[x], tmp, numGrids * sizeof(float));
    }
  }
  else if(direction == -1) {
    for(int x = 0; x < numGrids; x++) {
      memcpy(tmp, aRow[x], numGrids * sizeof(float));
      s_backward_trig_transform(tmp, &handle, ipar, spar, &stat);
      memcpy(aRow[x], tmp, numGrids * sizeof(float));
    }
  }
  else {
  }

  free_trig_transform(&handle, ipar, &stat);
  mkl_free(tmp);
  mkl_free(aRow);
  mkl_free(spar);
  free(ipar);

  return 0;
}

int intelLIB::MKL::DST1D(float *inout, const int direction, MKL_INT numGrids) {
  /// Trigonometric Transform Type
  MKL_INT ttt = MKL_STAGGERED_SINE_TRANSFORM;
  /// MKL routine status
  MKL_INT stat;
  /// MKL options with INT array type
  MKL_INT *ipar;
  /// MKL computation space
  float *spar;
  float *tmp;
  float **aRow;

  DFTI_DESCRIPTOR_HANDLE handle = 0;
  aRow = (float **)mkl_malloc(sizeof(float *) * numGrids, 64);
  for(int x = 0; x < numGrids; x++) {
    aRow[x] = inout + x * numGrids;
  }

  tmp = (float *)mkl_malloc(numGrids * sizeof(float), 64);
  spar = (float *)mkl_malloc((5 * numGrids / 2 + 2) * sizeof(float), 64);
  ipar = (MKL_INT *)malloc((128) * sizeof(MKL_INT));
  s_init_trig_transform(&numGrids, &ttt, ipar, spar, &stat);
  s_commit_trig_transform(aRow[numGrids - 1], &handle, ipar, spar, &stat);
  if(direction == 1) {
    for(int x = 0; x < numGrids; x++) {
      memcpy(tmp, aRow[x], numGrids * sizeof(float));
      tmp[numGrids] = 0.0000;
      s_forward_trig_transform(tmp, &handle, ipar, spar, &stat);
      memcpy(aRow[x], tmp + 1, numGrids * sizeof(float));
    }
  }
  else if(direction == -1) {
    for(int x = 0; x < numGrids; x++) {
      memcpy(tmp, aRow[x], numGrids * sizeof(float));
      tmp[numGrids] = 0.0000;
      s_backward_trig_transform(tmp, &handle, ipar, spar, &stat);
      memcpy(aRow[x], tmp + 1, numGrids * sizeof(float));
    }
  }
  else {
  }

  free_trig_transform(&handle, ipar, &stat);
  mkl_free(tmp);
  mkl_free(aRow);
  mkl_free(spar);
  free(ipar);

  return 0;
}

int intelLIB::MKL::transpose(const float *src, float *dst, MKL_INT numGrids) {
  ///// For efficient cache access
  // MKL_INT         default_stride  = 64;
  // MKL_INT         stride          = default_stride;
  //
  // if (numGrids < default_stride) stride = numGrids;

  /// Transpose
  mkl_somatcopy('R' /* row-major ordering */,
                'T' /* matrix will be transposed */, numGrids /* rows */,
                numGrids /* cols */, 1 /* scales the input matrix */,
                src /* source matrix */,
                // stride    /* src_stride */,
                numGrids /* src_stride */, dst /* destination matrix */,
                // stride    /* dst_stride */);
                numGrids /* dst_stride */);

  return 0;
}

void intelLIB::precompute_coefficients(float *coeffi1Dds, float *eField_x,
                                       float *eField_y, MKL_INT numGrids) {
  float wx = 0.0;
  float wx2 = 0.0;
  float wy = 0.0;
  float wy2 = 0.0;
  /// Preparation to compute POTENTIAL, E-FIELDs
  for(int x = 0; x < numGrids; x++) {
    wx = (float)wx_2d_iL[x];
    wx2 = (float)wx2_2d_iL[x];
    for(int y = 0; y < numGrids; y++) {
      wy = (float)wy_2d_iL[y];
      wy2 = (float)wy2_2d_iL[y];
      if(x == 0 && y == 0) {
        coeffi1Dds[0] = eField_x[0] = eField_y[0] = 0.0;
      }
      else {
        coeffi1Dds[x * numGrids + y] /= (float)(wx2 + wy2);
        eField_x[x * numGrids + y] = coeffi1Dds[x * numGrids + y] * wx;
        eField_y[x * numGrids + y] = coeffi1Dds[x * numGrids + y] * wy;
      }
    }
  }
}

void intelLIB::print(const float *result, const string step, MKL_INT numGrids) {
  cout << "\nINFO:  Intel Library Package, " << step << endl;
  printf(
      "------------------------------------------------------------------------"
      "--------\n");
  for(int x = 0; x < numGrids; x++) {
    for(int y = 0; y < numGrids; y++) {
      printf("%.4f  ", result[x * numGrids + y]);
    }
    cout << endl;
  }
  printf(
      "------------------------------------------------------------------------"
      "--------\n");
}

void intelLIB::scaling(float *target, MKL_INT numGrids) {
  float scalingFactor = 2.0 / (float)numGrids;
  for(int i = 0; i < numGrids; i++) {
    target[i] *= INV_SQRT2;
  }
  for(int j = 0; j < numGrids * numGrids; j += numGrids) {
    target[j] *= INV_SQRT2;
  }
  for(int y = 0; y < numGrids; y++) {
    for(int x = 0; x < numGrids; x++) {
      target[y * numGrids + x] *= scalingFactor;
    }
  }
}

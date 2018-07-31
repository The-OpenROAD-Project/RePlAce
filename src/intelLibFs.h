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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include        <cstdio>
#include        <cstdlib>
#include        <cstring>
#include        <cmath>
#include        <ctime>
#include        <sys/time.h>
#include        <error.h>
#include        <unistd.h>
#include        <sys/stat.h>
#include        <sys/types.h>
#include        <vector>

#include        "mkl.h"
#include        "ipp.h"

#ifndef         __INTEL_LIBS__
#define         __INTEL_LIBS__

class myBin {
    public:
        prec        density;
        prec        potential;
        myBin () {
            density = (prec  )rand() / RAND_MAX * 100.000;
        };
        // For the test purpose
        myBin (prec   den) {
            density = den;
        };
        ~myBin ();
};


namespace ooura {
    int main (std::vector < std::vector < myBin* > > &);
}


namespace intelLIB {
    int main (std::vector < std::vector < myBin* > > &);
    void precompute_coefficients (float*, float*, float*, MKL_INT);
    void scaling (float*, MKL_INT);
    void print (const float*, const string, MKL_INT);

    /// Intel IPP may malfunction if we use non-IPP data type.
    /// Please be careful if we want to change data types for IPP functions.
    namespace IPP {
        int fwdDCT2D (const Ipp32f*, Ipp32f*, const MKL_INT);
        int invDCT2D (const Ipp32f*, Ipp32f*, const MKL_INT);
    }

    /// Intel MKL can digest general C data types as well as IPP data types.
    /// E.g., we can use Ipp32f instead of float.
    namespace MKL {
        int DCT1D (float*, const int, MKL_INT);
        int DST1D (float*, const int, MKL_INT);
        int transpose (const float*, float*, MKL_INT);
    }
}

#endif

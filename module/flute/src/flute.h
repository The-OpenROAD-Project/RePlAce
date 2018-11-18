#ifndef _FLUTE_
#define _FLUTE_

// [NOTE] Modified by Guilherme Flach - 19/Jun/2014
// Added FLUTE_ prefix to FLUTE macros to avoid name conflicts.

// Guilherme Flach - 2016/11/06
#include "dbu.h"

namespace Flute {
	
using namespace std;

#define FLUTE_POWVFILE "POWV9.dat"    // LUT for POWV (Wirelength Vector)
#define FLUTE_PORTFILE "PORT9.dat"    // LUT for PORT (Routing Tree)
#define FLUTE_D 9        // LUT is used for d <= D, D <= 9
#define FLUTE_FLUTEROUTING 1   // 1 to construct routing, 0 to estimate WL only
#define FLUTE_REMOVE_DUPLICATE_PIN 0  // Remove dup. pin for flute_wl() & flute()
#define FLUTE_ACCURACY 8  // Default accuracy is 3
//#define FLUTE_MAXD 2008840  // max. degree of a net that can be handled
#define FLUTE_MAXD 1000  // max. degree of a net that can be handled


#ifndef FLUTE_DTYPE   // Data type for distance
#define FLUTE_DTYPE DBU
#endif

typedef struct
{
    FLUTE_DTYPE x, y;   // starting point of the branch
    int n;   // index of neighbor
} Branch;

typedef struct
{
    int deg;   // degree
    FLUTE_DTYPE length;   // total wirelength
    Branch *branch;   // array of tree branches
} Tree;


// Major functions
extern void readLUT(std::string powvName, std::string portName);
extern FLUTE_DTYPE flute_wl(int d, FLUTE_DTYPE x[], FLUTE_DTYPE y[], int acc);
//Macro: DTYPE flutes_wl(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);
extern Tree flute(int d, FLUTE_DTYPE x[], FLUTE_DTYPE y[], int acc, int mapping[]);
//Macro: Tree flutes(int d, DTYPE xs[], DTYPE ys[], int s[], int acc);
extern FLUTE_DTYPE wirelength(Tree t);
extern void printtree(Tree t);

//extern Tree flautist(int d, DTYPE x[], DTYPE y[], int acc, const uofm::vector<BBox> &obs, const uofm::vector<unsigned> &relevantObs, unsigned &legal);


// Other useful functions
extern FLUTE_DTYPE flutes_wl_LD(int d, FLUTE_DTYPE xs[], FLUTE_DTYPE ys[], int s[]);
extern FLUTE_DTYPE flutes_wl_MD(int d, FLUTE_DTYPE xs[], FLUTE_DTYPE ys[], int s[], int acc);
extern FLUTE_DTYPE flutes_wl_RDP(int d, FLUTE_DTYPE xs[], FLUTE_DTYPE ys[], int s[], int acc);
extern Tree flutes_LD(int d, FLUTE_DTYPE xs[], FLUTE_DTYPE ys[], int s[]);
extern Tree flutes_MD(int d, FLUTE_DTYPE xs[], FLUTE_DTYPE ys[], int s[], int acc);
extern Tree flutes_RDP(int d, FLUTE_DTYPE xs[], FLUTE_DTYPE ys[], int s[], int acc);

#if FLUTE_REMOVE_DUPLICATE_PIN==1
  #define flutes_wl(d, xs, ys, s, acc) flutes_wl_RDP(d, xs, ys, s, acc) 
  #define flutes(d, xs, ys, s, acc) flutes_RDP(d, xs, ys, s, acc) 
#else
  #define flutes_wl(d, xs, ys, s, acc) flutes_wl_ALLD(d, xs, ys, s, acc) 
  #define flutes(id, d, xs, ys, s, acc) flutes_ALLD(id, d, xs, ys, s, acc) 
#endif

#define flutes_wl_ALLD(d, xs, ys, s, acc) flutes_wl_LMD(d, xs, ys, s, acc)
#define flutes_ALLD(id, d, xs, ys, s, acc) flutes_LMD(id, d, xs, ys, s, acc)

#define flutes_wl_LMD(d, xs, ys, s, acc) \
    (d<=FLUTE_D ? flutes_wl_LD(d, xs, ys, s) : flutes_wl_MD(d, xs, ys, s, acc))
#define flutes_LMD(id, d, xs, ys, s, acc) \
    (d<=FLUTE_D ? flutes_LD(d, xs, ys, s) : flutes_MD(id, d, xs, ys, s, acc))

#define ADIFF(x,y) ((x)>(y)?(x-y):(y-x))  // Absolute difference

////////////////////////////////////////////////////////////////////////////////
// Added by Guilherme Flach - 2016-02-22
// Returns the relative cost in terms of runtime to build a tree of degree n.
//
// Note that this is not an absolute measure of the runtime, but instead a 
// rough measure of how longer it will take to generate a tree with degree 
// n1 compared to a tree with degree n2. That is the n1-tree takes n1/n2 longer
// to be generated than the n2-tree.
//
// The cost is intended to be used for load-balancing in multi-threading.
//
// The cost was computed using flute() function, which was a little but tweaked
// in order to best suite our requirements.
//
// Configuration used in the measurement:
// #define FLUTE_SORTING_THRESHOLD 200
// #define FLUTE_FAST_SORT_Y 1
// #define FLUTE_FAST_SORT_Y 0
////////////////////////////////////////////////////////////////////////////////

inline double generation_cost(const int n) {
	static const double COST_UP_TO_20[] = {
		 0.0000000000, 0.0000000000,  0.0465398638,  0.0641802706,  0.0809460239, 
		 0.0997421556, 0.1197798820,  0.1448438153,  0.1763411899,  0.2461886417, 
		 1.0000000000, 1.2675247404,  2.3991262851,  3.4054468756,  4.9176023987,
		 6.3498472641, 8.1995467193, 10.2411162153, 12.5221830672, 14.9206758856,
		17.5287992755};

	if (n <= 20) {
		return COST_UP_TO_20[n];
	} else if (n <= 220) {
		// Use quartic regression.
		static const double a = -24.8887424569;
		static const double b = 1.2597076234;
		static const double c = 0.0422073141;
		static const double d = -0.0001848177;
		static const double e = 3.11854362710948e-7;
		
		const double x1 = n; // to avoid precision issues
		const double x2 = x1*x1;
		const double x3 = x2*x1;
		const double x4 = x3*x1;
		
		return e*x4 + d*x3 + c*x2 + b*x1 + a;
	} else {
		// Use linear regression.
		static const double a = 6.3561;
		static const double b = -357.47;
		
		return a*n + b;
	} // end else
} // end method

} // end namespace

#endif



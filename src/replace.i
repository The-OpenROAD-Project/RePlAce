%module replace

%{
#include "openroad/OpenRoad.hh"
#include "replace/Replace.h"

namespace ord {
replace::Replace*
getReplace();
}

using ord::getReplace;
using replace::Replace;
%}

%inline %{

void 
replace_init_place_cmd()
{
  Replace* replace = getReplace();
  replace->doInitialPlace();
}

void 
replace_nesterov_place_cmd()
{
  Replace* replace = getReplace();
  replace->doNesterovPlace();
}

void
set_replace_density_cmd(float density)
{
  Replace* replace = getReplace();
  replace->setTargetDensity(density);
}

void
set_replace_init_place_iter_cmd(int iter)
{
  Replace* replace = getReplace();
  replace->setInitialPlaceMaxIter(iter); 
}

void
set_replace_nesv_place_iter_cmd(int iter)
{
  Replace* replace = getReplace();
  replace->setNesterovPlaceMaxIter(iter);
}

void
set_replace_bin_grid_cnt_x_cmd(int cnt_x)
{
  Replace* replace = getReplace();
  replace->setBinGridCntX(cnt_x); 
}

void
set_replace_bin_grid_cnt_y_cmd(int cnt_y)
{
  Replace* replace = getReplace();
  replace->setBinGridCntY(cnt_y);
}

void
set_replace_overflow_cmd(float overflow)
{
  Replace* replace = getReplace();
  replace->setTargetOverflow(overflow);
}

void
set_replace_min_pcoef_cmd(float min_pcoef)
{
  Replace* replace = getReplace();
  replace->setMinPCoef(min_pcoef);
}

void
set_replace_max_pcoef_cmd(float max_pcoef) 
{
  Replace* replace = getReplace();
  replace->setMaxPCoef(max_pcoef);
}

void
set_replace_init_penalty_factor_cmd(float penaltyFactor)
{
  Replace* replace = getReplace();
  replace->setInitPenalityFactor(penaltyFactor);
}

void
set_replace_verbose_level_cmd(int verbose)
{
  Replace* replace = getReplace();
  replace->setVerboseLevel(verbose);
}



%} // inline

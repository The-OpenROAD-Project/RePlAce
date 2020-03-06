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
replace_initial_place_cmd()
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
set_replace_initial_place_max_iter_cmd(int iter)
{
  Replace* replace = getReplace();
  replace->setInitialPlaceMaxIter(iter); 
}

void
set_replace_initial_place_max_fanout_cmd(int fanout)
{
  Replace* replace = getReplace();
  replace->setInitialPlaceMaxFanout(fanout);
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
set_replace_min_phi_coef_cmd(float min_phi_coef)
{
  Replace* replace = getReplace();
  replace->setMinPhiCoef(min_phi_coef);
}

void
set_replace_max_phi_coef_cmd(float max_phi_coef) 
{
  Replace* replace = getReplace();
  replace->setMaxPhiCoef(max_phi_coef);
}

void
set_replace_init_density_penalty_factor_cmd(float penaltyFactor)
{
  Replace* replace = getReplace();
  replace->setInitDensityPenalityFactor(penaltyFactor);
}

void
set_replace_init_wirelength_coef_cmd(float coef)
{
  Replace* replace = getReplace();
  replace->setInitWireLengthCoef(coef);
}

void
set_replace_incremental_place_mode_cmd()
{
  Replace* replace = getReplace();
  replace->setIncrementalPlaceMode(true);
}

void
set_replace_verbose_level_cmd(int verbose)
{
  Replace* replace = getReplace();
  replace->setVerboseLevel(verbose);
}



%} // inline

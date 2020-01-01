
#include "db.h"
#include "lefin.h"
#include "defin.h"

#include "replace_external.h"
#include "wlen.h"
#include "initPlacement.h"
#include "dbLefDefIO.h"
#include "plot.h"
#include "routeOpt.h"
#include "timing.h"
#include "fft.h"
#include "openroad/OpenRoad.hh"

using namespace std;

replace_external::
replace_external() : 
  timing_driven_mode(false), 
  write_bookshelf_mode(false),
  unit_r(0.0f), unit_c(0.0f) {
  initGlobalVars();
};

replace_external::
~replace_external() {};

void
replace_external::help() {
  cout << endl;
  cout << "==== File I/O Commands ====" << endl;
  cout << "set_output [directory_location]" << endl;
  cout << "    Specify the location of output results. " << endl;
  cout << "    Default: ./output " << endl;
     
  cout << endl;
  cout << "==== Flow Control ==== " << endl;
  cout << "init_replace" << endl;
  cout << "    Initialize RePlAce." << endl;
  cout << endl; 
  cout << "place_cell_init_place" << endl;
  cout << "    Execute BiCGSTAB engine for initial placement." << endl;
  cout << endl; 
  cout << "place_cell_nesterov_place" << endl;
  cout << "    Execute Nesterov engine for global placement. " << endl;
  cout << endl; 
  cout << "==== Timing-driven Mode ====" << endl;
  cout << "set_timing_driven [true/false]" << endl;
  cout << "  Enable timing-driven placement" << endl;
  cout << endl; 
  cout << "set_unit_res [resistor]" << endl;
  cout << "    Resisance per micron. Unit: Ohm. " << endl;
  cout << "    (Used for RC estimation)" << endl;
  cout << endl; 
  
  cout << "set_unit_cap [capacitance]" << endl;
  cout << "    Capacitance per micron. Unit: Farad. " << endl;
  cout << "    (Used for RC estimation)" << endl;
  cout << endl; 
  
  cout << "==== RePlAce tunning parameters ====" << endl;
  cout << "** Note that the following tunning parameters " << endl;
  cout << "   must be defined before executing init_replace command" << endl;
  cout << endl; 
  cout << "set_density [density]" << endl;
  cout << "    Set target density. [0-1, float]. Default: 1.00" << endl;
  cout << endl; 
  cout << "set_bin_grid_count [num]" << endl;
  cout << "    Set bin_grid_count. [64,128,256,512,..., int]. " << endl;
  cout << "    Default: Defined by internal algorithm." << endl;
  cout << endl; 
  cout << "set_lambda [lambda]" << endl;
  cout << "    Set lambda for RePlAce tunning. [float]." << endl;
  cout << "    Default : 8e-5~10e5" << endl;
  cout << endl; 
  cout << "set_pcof_min [pcof_min]" << endl;
  cout << "    Set pcof_min(µ_k Lower Bound). [0.95-1.05, float]." << endl;
  cout << "    Default: 0.95" << endl;
  cout << endl; 
  cout << "set_pcof_max [pcof_max]" << endl;
  cout << "    Set pcof_max(µ_k Upper Bound). [1.00-1.20, float]." << endl;
  cout << "    Default: 1.05" << endl;
  cout << endl; 
  cout << "set_step_scale [step_scale]" << endl;
  cout << "    Set step_scale(∆HPWL_REF). Default: 346000" << endl;
  cout << endl; 
  cout << "set_target_overflow [overflow]" << endl;
  cout << "    Set target overflow termination condition." << endl;
  cout << "    [0.01-1.00, float]. Default: 0.1" << endl;
  cout << endl; 
  
  cout << "==== Timing-driven related tuning parameters ==== " << endl;
  cout << "set_net_weight_min [weight_min]" << endl;
  cout << "    Set net_weight_min. [1.0-1.8, float]" << endl;
  cout << endl; 
  cout << "set_net_weight_max [weight_max]" << endl;
  cout << "    Set net_weight_max. [weight_min-1.8, float]" << endl;
  cout << endl; 
  cout << "set_net_weight_scale [weight_scale]" << endl;
  cout << "    Set net_weight_scale. [200-, float]" << endl;
  cout << endl; 
  
  cout << "==== Other options ==== " << endl;
  cout << "set_plot_enable [mode]" << endl;
  cout << "    Set plot modes; " << endl;
  cout << "    This mode will plot layout every 10 iterations" << endl;
  cout << "    (Cell, bin, and arrow plots) [true/false]." << endl;
  cout << "    Default: False" << endl;
  cout << endl; 
  cout << "set_seed_init_enable [true/false]" << endl;
  cout << "    Start global place with the given placed locations." << endl;
  cout << "    Default: False" << endl;
  cout << endl; 
  cout << "set_fast_mode_enable [true/false]" << endl;
  cout << "    Fast and quick placement for IO-pin placement." << endl;
  cout << "    Please do NOT use this command in general purposes" << endl;
  cout << "    (It'll not spread all cells enough). Default: False" << endl;
  cout << endl; 
  cout << "set_verbose_level [level]" << endl;
  cout << "    Specify the verbose level. [1-3, int]. Default: 1" << endl;
  cout << endl; 
  
  cout << "==== Query results ==== " << endl;
  cout << "** Note that the following commands will work " << endl;
  cout << "   after init_replace command" << endl;
  cout << endl; 
  cout << "get_hpwl" << endl;
  cout << "    Returns HPWL results on Micron. [float]" << endl;
  cout << endl; 
  cout << "get_wns" << endl;
  cout << "    Returns WNS from OpenSTA." << endl;
  cout << "    (Only available when timing-driven mode is enabled) [float] " << endl;
  cout << endl; 
  cout << "get_tns" << endl;
  cout << "    Returns TNS from OpenSTA." << endl;
  cout << "    (Only available when timing-driven mode is enabled) [float]" << endl;
  cout << endl; 
  
  cout << "print_instances" << endl;
  cout << "    Print out all of instances' information. " << endl;
  cout << "    (Not recommended for huge design)" << endl;
  cout << endl; 
  cout << "get_instance_list_size" << endl;
  cout << "    Returns total number of instances in RePlAce. [size_t]" << endl;
  cout << endl; 
  cout << "get_x [index]" << endl;
  cout << "    Returns x coordinates of specified instances' index. [float]" << endl;
  cout << endl; 
  cout << "get_y [index]" << endl;
  cout << "    Returns y coordinates of specified instances' index. [float]" << endl;
  cout << endl; 
  cout << "get_master_name [index]" << endl;
  cout << "    Returns master name of specified instances' index. [string]" << endl;
  cout << endl; 
  cout << "get_instance_name [index]" << endl;
  cout << "    Returns instance name of specified instances' index. [string]" << endl;
  cout << endl; 

}

void
replace_external::set_output(const char* output) {
  output_loc = output;
}

void
replace_external::set_output_experiment_name(const char* output) {
  experimentCMD = output;
}


void
replace_external::set_timing_driven(bool is_true) {
  timing_driven_mode = is_true;
}

void
replace_external::set_unit_res(double r) {
  unit_r = r;
}

void 
replace_external::set_unit_cap(double c) {
  unit_c = c;
}

void
replace_external::set_plot_enable(bool plot_mode) {
  isPlot = plot_mode;
}

void
replace_external::set_verbose_level(int verbose) {
  gVerbose = verbose;
}

void
replace_external::set_fast_mode_enable(bool fast_mode) {
  isFastMode = fast_mode;
}

void
replace_external::set_seed_init_enable(bool seed_init) {
  isInitSeed = seed_init;
}


void
replace_external::set_plot_color_file(std::string color_file) {
  plotColorFile = color_file;
}

void
replace_external::set_write_bookshelf_enable(bool write_mode) {
  write_bookshelf_mode = true;
}

void
replace_external::set_density(double density) {
  target_cell_den = target_cell_den_orig = density;
}

void
replace_external::set_bin_grid_count(size_t grid_count) {
  dim_bin.x = dim_bin.y = grid_count;
  isBinSet = true;
}

void
replace_external::set_lambda(double lambda) {
  INIT_LAMBDA_COF_GP = lambda; 
}

void
replace_external::set_pcof_min(double pcof_min) {
  LOWER_PCOF = pcof_min;
}

void
replace_external::set_pcof_max(double pcof_max) {
  UPPER_PCOF = pcof_max;
}

void
replace_external::set_step_scale(double step_scale) {
  refDeltaWL = step_scale;
}

void
replace_external::set_target_overflow(double overflow) {
  overflowMin = overflow;
}

void
replace_external::set_net_weight_apply(bool mode){
  netWeightApply = mode;
}

void
replace_external::set_net_weight_min(double net_weight_min) {
  netWeightBase = net_weight_min;
}

void
replace_external::set_net_weight_max(double net_weight_max) {
  netWeightBound = net_weight_max;
}

void
replace_external::set_net_weight_scale(double net_weight_scale) {
  netWeightScale = net_weight_scale;
}

bool 
replace_external::init_replace() {
  ord::OpenRoad *openroad = ord::OpenRoad::openRoad();
  Timing::_sta = openroad->getSta();
  _db = openroad->getDb();

  outputCMD = (output_loc == "")? "./output" : output_loc;

  if( timing_driven_mode == true ) {
    capPerMicron = unit_c;
    resPerMicron = unit_r;
    isTiming = true; 
  }

  initGlobalVarsAfterParse();
  init();

  FillReplaceStructures(_db);
  
  net_update_init();
  init_tier();
  build_data_struct();
  update_instance_list();
  return true; 
}

bool 
replace_external::free_replace() {
  ord::OpenRoad *openroad = nullptr;
  Timing::_sta = nullptr;
  _db = nullptr;
  timing_driven_mode = false;

  // Clear the moduleInstance
  for(int i=0; i<moduleCNT; i++) {
    if( moduleInstance[i].pin ) {
      free( moduleInstance[i].pin );
    }
    moduleInstance[i].pin = nullptr;
    if( moduleInstance[i].pof) {
      free( moduleInstance[i].pof );
    }
    moduleInstance[i].pof = nullptr;
  }
  if( moduleInstance ) {
    free(moduleInstance);
  }
  moduleInstance = nullptr;
  moduleCNT = 0;
  vector<string> ().swap(moduleNameStor);
  vector<vector<string>> ().swap(mPinName);

  // Clear the terminalInstance
  for(int i=0; i<terminalCNT; i++) {
    if( terminalInstance[i].pin ) {
      free( terminalInstance[i].pin );
    }
    terminalInstance[i].pin = nullptr;
    if( terminalInstance[i].pof) {
      free( terminalInstance[i].pof );
    }
    terminalInstance[i].pof = nullptr;
  }
  if( terminalInstance ) {
    free(terminalInstance);
  }
  terminalInstance = nullptr;
  terminalCNT = 0;
  vector<string> ().swap(terminalNameStor);
  vector<vector<string>> ().swap(tPinName);

  // clear the netInstance
  if( netInstance ) {
    for(int i=0; i<netCNT; i++) {
      if( netInstance[i].pin ) {
        free(netInstance[i].pin);
      }
      netInstance[i].pin = nullptr;
    }
    free(netInstance);
  }
  netInstance = nullptr;
  netCNT = 0;
  vector<string> ().swap(netNameStor);

  // clear the pinInstance
  if( pinInstance ) {
    free(pinInstance);
  }
  pinInstance = nullptr;
  pinCNT = 0;

  // clear the gcell_st
  if( gcell_st ) { 
    free(gcell_st);
  }
  gcell_st = nullptr;
  gcell_cnt = 0;
  vector<string> ().swap(cellNameStor);

  // clear the row_st
  if( row_st ) {
    free( row_st );
  }
  row_st = nullptr;
  row_cnt = 0;

  // clear the place_st
  if( place_st ) {
    free( place_st );
  }
  place_st = nullptr;
  place_st_cnt = 0;

  // clear the tier_st
  if( tier_st ) {
    for(int z=0; z<numLayer; z++) {
      TIER* tier = &tier_st[z];
      if( tier->bin_mat ) {
        free(tier->bin_mat);
      }
      tier->bin_mat = nullptr;

      // tier->row_st and row_st share the same pointer
      tier->row_st = nullptr;

      if( tier->modu_st ) {
        free(tier->modu_st);
      }
      tier->modu_st = nullptr;

      // terminalInstance and tier->term_st share the same pointer
      tier->term_st = nullptr;

      if( tier->mac_st) {
        free(tier->mac_st);
      }
      tier->mac_st = nullptr;

      // tier->cell_st, tier->cell_st_tmp, and gcell_st share the same pointer
      tier->cell_st = nullptr;
      tier->cell_st_tmp = nullptr;

      
      // tier->tile_mat ....
      tier->tile_mat = nullptr;
    }
    free(tier_st);
  }
  tier_st = nullptr;
  charge_fft_delete_2d();
  gsum_phi = gsum_ovfl = 0;
  initGlobalVars();
   
  return true; 
}

bool 
replace_external::place_cell_init_place() {
  initialPlacement_main();
  update_instance_list();
  return true;
}

bool
replace_external::place_cell_nesterov_place() {
  setup_before_opt();
  if( placementMacroCNT > 0 ) {
    mGP2DglobalPlacement_main();
  }
  else {
    cGP2DglobalPlacement_main();
  }
  update_instance_list();
  update_dbinst_locations();
  if( isPlot ) {
    SaveCellPlotAsJPEG("Global Placement Result", false,
        string(dir_bnd) + string("/globalPlace"));
  }
  return true;
}




size_t
replace_external::get_instance_list_size() {
  return instance_list.size();
}

size_t
replace_external::get_module_size() {
  return moduleCNT;
}

size_t
replace_external::get_terminal_size() {
  return terminalCNT;
}

size_t
replace_external::get_net_size() {
  return netCNT;
}

size_t
replace_external::get_pin_size() {
  return pinCNT;
}

size_t
replace_external::get_row_size() {
  return row_cnt;
}

// examples for checking component names
std::string
replace_external::get_master_name(size_t idx) {
  return instance_list[idx].master;
}

// examples for checking component names
std::string
replace_external::get_instance_name(size_t idx) {
  return instance_list[idx].name;
}

float
replace_external::get_x(size_t idx) {
  return instance_list[idx].x;
}

float
replace_external::get_y(size_t idx) {
  return instance_list[idx].y;
}

void
replace_external::print_instances() {
  std::cout << "Total Instance: " << instance_list.size() << endl; 
  for(auto& cur_inst : instance_list ) {
    std::cout << cur_inst.name << " (" << cur_inst.master << ") x:";
    std::cout << cur_inst.x << " y:" << cur_inst.y << std::endl;
  }
}


float
replace_external::get_hpwl() {
  auto res = GetUnscaledHpwl();
  return res.first + res.second;
}

float
replace_external::get_wns() {
  return globalWns;
}

float
replace_external::get_tns() {
  return globalTns;
}


void 
replace_external::update_instance_list() {
  if( instance_list.size() == 0 ) {

    odb::dbChip* chip = _db->getChip();
    odb::dbBlock* block = chip->getBlock();

    for(int i=0; i<moduleCNT; i++) {
      MODULE* module = &moduleInstance[i];
      instance_info tmp;
      tmp.name = module->Name();

      // This should NOT be using names to find the dbInst.
      // There should be a POINTER to it. -cherry
      odb::dbInst* curInst = block->findInst( module->Name() );
      odb::dbMaster* curMaster = curInst->getMaster();
      tmp.master = curMaster->getConstName();

      tmp.x = module->pmin.x;
      tmp.y = module->pmin.y;
      instance_list.push_back(tmp);
    }
  }
  else {
    for(int i=0; i<moduleCNT; i++) {
      MODULE* module = &moduleInstance[i];
      instance_list[i].x = module->pmin.x;
      instance_list[i].y = module->pmin.y;
    }
  }
}

// Update the instance locations from the internal replace structs.
void replace_external::update_dbinst_locations() {
  odb::dbBlock* block = _db->getChip()->getBlock();

  for(int i=0; i<moduleCNT; i++)  {
    MODULE* curModule = &moduleInstance[i];
    
    // This should NOT be using names to find the dbInst.
    // There should be a POINTER to it. -cherry
    odb::dbInst* curInst = block->findInst( curModule->Name() );  
    curInst->setLocation(GetScaleUpPointX(curModule->pmin.x), 
			 GetScaleUpPointY(curModule->pmin.y));
    curInst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
}

// TODO
bool
replace_external::save_jpeg(const char* jpeg) {
  return true;
}

void
replace_external::import_custom_net_weight(const char* input_file) {
  hasCustomNetWeight = true;
  net_weight_file = input_file;
}

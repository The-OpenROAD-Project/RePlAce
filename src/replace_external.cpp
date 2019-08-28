#include "replace_external.h"
#include "wlen.h"
#include "initPlacement.h"


replace_external::
replace_external() : 
  timing_driven_mode(false), ckt(&Replace::__ckt), unit_r(0.0f), unit_c(0.0f) {};

replace_external::
~replace_external() {};

void 
replace_external::import_lef(const char* lef){ 
  lef_stor.push_back(lef);
}

void 
replace_external::import_def(const char* def){
  def_stor.push_back(def);
}

void 
replace_external::export_def(const char* def){
  WriteDef(def);
}

void
replace_external::set_output(const char* output) {
  output_loc = output;
}

bool 
replace_external::init_replace() {
  if( lef_stor.size() == 0 ) {
    std::cout << "ERROR: Specify at least one LEF file!" << std::endl;
    exit(1);
  }
  else if( def_stor.size() == 0 ) {
    std::cout << "ERROR: Specify at least one DEF file!" << std::endl;
    exit(1);
  }

  initGlobalVars();

  lefStor = lef_stor;
  defName = def_stor[0];
  outputCMD = output_loc;

  initGlobalVarsAfterParse();

  init();

  inputMode = InputMode::lefdef;
  ParseLefDef();

  net_update_init();
  init_tier();
  build_data_struct(false);
  return true;
}

void
replace_external::set_timing_driven() {
  timing_driven_mode = true;
}

void 
replace_external::import_sdc(const char* sdc) {
  sdc_file = sdc;
}

void
replace_external::import_verilog(const char* verilog) {
  verilog_stor.push_back(verilog);
}

size_t
replace_external::get_instance_list_size() {
  return instance_list.size();
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
  return true;
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
replace_external::update_instance_list() {
  if( instance_list.size() == 0 ) {
    for(int i=0; i<moduleCNT; i++) {
      MODULE* module = &moduleInstance[i];
      instance_info tmp;
      tmp.name = module->Name();
      auto cmPtr = ckt->defComponentMap.find(tmp.name);
      tmp.master = ckt->defComponentStor[cmPtr->second].name();
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

bool
replace_external::save_jpeg(const char* jpeg) {
  return true;
}

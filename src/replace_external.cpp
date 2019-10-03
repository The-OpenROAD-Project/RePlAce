
#include "db.h"
#include "lefin.h"
#include "defin.h"

#include "replace_external.h"
#include "wlen.h"
#include "initPlacement.h"
#include "dbLefDefIO.h"


replace_external::
replace_external() : 
  timing_driven_mode(false), ckt(&Replace::__ckt), 
  unit_r(0.0f), unit_c(0.0f), use_db(true), db_id(INT_MAX) {
  initGlobalVars();
};

replace_external::
~replace_external() {};

void 
replace_external::import_lef(const char* lef){ 
  ads::dbDatabase * db = NULL;
  if( db_id == INT_MAX ) {
    db = ads::dbDatabase::create();
    db_id = db->getId();
  }
  else {
    db = ads::dbDatabase::getDatabase(db_id);
  }
  ads::lefin lefReader(db, false);
  lefReader.createTechAndLib("testlib", lef);
}

void 
replace_external::import_def(const char* def){
  ads::dbDatabase * db = NULL;
  if( db_id == INT_MAX ) {
    db = ads::dbDatabase::create();
    db_id = db->getId();
  }
  else {
    db = ads::dbDatabase::getDatabase(db_id);
  }
  ads::defin defReader(db);

  std::vector<ads::dbLib *> search_libs;
  ads::dbSet<ads::dbLib> libs = db->getLibs();
  ads::dbSet<ads::dbLib>::iterator itr;
  for( itr = libs.begin(); itr != libs.end(); ++itr ) {
    search_libs.push_back(*itr);
  }
  ads::dbChip* chip = defReader.createChip( search_libs,  def );
}

void 
replace_external::import_lib(const char* lib){
  lib_stor.push_back(lib);
}

void
replace_external::import_db(const char* dbLoc) {
//  ads::dbDatabase* db = NULL;
  ads::dbDatabase* db = ads::dbDatabase::create();

  FILE* fp = fopen(dbLoc, "rb");
  if( fp == NULL ) {
    cout << "ERROR: Can't open " <<  dbLoc << endl;
    exit(1);
  }
  db->ads::dbDatabase::read(fp);
  fclose(fp);
  db_id = db->getId(); 
}

void
replace_external::export_db(const char* dbLoc) {
  ads::dbDatabase* db = ads::dbDatabase::getDatabase( db_id );  
  
  FILE* fp = fopen(dbLoc, "wb");
  if( fp == NULL ) {
    cout << "ERROR: Can't open " <<  dbLoc << endl;
    exit(1);
  }

  db->ads::dbDatabase::write(fp);
  fclose(fp);
}

void 
replace_external::export_def(const char* def){
  if( use_db ) {
    ads::dbDatabase* db = ads::dbDatabase::getDatabase( db_id ); 
    WriteDefDb(db, def);
  }
  else {
    WriteDef(def);
  }
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

  lefStor = lef_stor;
  defName = def_stor[0];
  outputCMD = output_loc;

  if( timing_driven_mode == true ) {
    capPerMicron = unit_c;
    resPerMicron = unit_r;
    libStor = lib_stor;
    sdcName = sdc_file;
    verilogName = verilog_stor[0];
    isTiming = true; 
  }

  initGlobalVarsAfterParse();

  init();
  ParseInput();

  net_update_init();
  init_tier();
  build_data_struct();
  update_instance_list();
  return true;
}

bool 
replace_external::init_replace_db() {
  using namespace ads;
//  dbDatabase * db = dbDatabase::open( "chip.db", dbCreate );

//  Logger::initLogger(_interp);
  ads::dbDatabase * db = NULL;
  if( db_id == INT_MAX ) {
    db = ads::dbDatabase::create();
    db_id = db->getId();
  }
  else {
    db = dbDatabase::getDatabase(db_id);
  }

  /*
  dbDatabase * db = dbDatabase::create();
  db_id = db->getId();
  lefin lefReader(db, false);
  
  lefReader.createTechAndLib("testlib", lefList);

  defin defReader(db);

  std::vector<dbLib *> search_libs;
  dbSet<dbLib> libs = db->getLibs();
  dbSet<dbLib>::iterator itr;
  for( itr = libs.begin(); itr != libs.end(); ++itr ) {
    search_libs.push_back(*itr);
  }

  dbChip* chip = defReader.createChip( search_libs,  def_stor[0].c_str() );
  if( chip == NULL ) { 
    cout << "Failed to read def file: " 
      << def_stor[0] << endl;
    exit(1);
  }*/

  
//  lefStor = lef_stor;
//  defName = def_stor[0];
  outputCMD = output_loc;

  initGlobalVarsAfterParse();
  init();

  FillReplaceStructures(db);
  
  net_update_init();
  init_tier();
  build_data_struct();
  update_instance_list();
  return true; 
}

void
replace_external::set_timing_driven(bool is_true) {
  timing_driven_mode = is_true;
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

void
replace_external::print_instances() {
  std::cout << "Total Instance: " << instance_list.size() << endl; 
  for(auto& cur_inst : instance_list ) {
    std::cout << cur_inst.name << " (" << cur_inst.master << ") x:";
    std::cout << cur_inst.x << " y:" << cur_inst.y << std::endl;
  }
}


float
replace_external::get_x(size_t idx) {
  return instance_list[idx].x;
}

float
replace_external::get_y(size_t idx) {
  return instance_list[idx].y;
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
replace_external::set_verbose_level(int verbose) {
  gVerbose = verbose;
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
replace_external::set_bin_grid_count(size_t grid_count) {
  dim_bin.x = dim_bin.y = grid_count;
  isBinSet = true;
}

void
replace_external::set_density(double density) {
  target_cell_den = target_cell_den_orig = density;
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

void
replace_external::set_plot_enable() {
  isPlot = true;
}


void 
replace_external::update_instance_list() {
  if( instance_list.size() == 0 ) {
    if( use_db ) {
      using namespace ads;
      dbDatabase* db = dbDatabase::getDatabase( db_id ); 

      dbChip* chip = db->getChip();
      dbBlock* block = chip->getBlock();

      for(int i=0; i<moduleCNT; i++) {
        MODULE* module = &moduleInstance[i];
        instance_info tmp;
        tmp.name = module->Name();
        
        dbInst* curInst = block->findInst( module->Name() );
        dbMaster* curMaster = curInst->getMaster();
        tmp.master = curMaster->getConstName();

        tmp.x = module->pmin.x;
        tmp.y = module->pmin.y;
        instance_list.push_back(tmp);
      }
    }
    else {
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

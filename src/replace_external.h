#ifndef __REPLACE_EXTERNAL__
#define __REPLACE_EXTERNAL__ 0

// No hope to isolate right now...
#include "replace_private.h"
#include "lefdefIO.h"


// SWIG refuse to be inside replace_external...
struct instance_info {
  std::string name;
  std::string master;
  float x;
  float y;
};


class replace_external {
public:

  replace_external();
  ~replace_external();
  void import_lef(const char* lef);
  void import_def(const char* def);
  void set_output(const char* output);

  void export_def(const char* def);

  void set_timing_driven(bool is_true);
  void import_sdc(const char* sdc);
  void import_verilog(const char* verilog);
  void import_lib(const char* lib);
  void set_unit_res(double unit_r);
  void set_unit_cap(double unit_c);

  void set_verbose_level(int verbose);
  void set_density(double density);
  void set_bin_grid_count(size_t grid_count);
  
  void set_lambda(double lambda);
  void set_pcof_min(double pcof_min);
  void set_pcof_max(double pcof_max);
  
  bool init_replace();

  size_t get_instance_list_size();
  std::string get_master_name(size_t idx);
  std::string get_instance_name(size_t idx);
  float get_x(size_t idx); 
  float get_y(size_t idx);
  void print_instances();

  void set_net_weight_min(double net_weight_min);
  void set_net_weight_max(double net_weight_max);
  void set_net_weight_scale(double net_weight_scale);

  float get_hpwl();
  float get_wns();
  float get_tns(); 

  bool save_jpeg(const char* jpeg);
  bool place_cell_init_place();
  bool place_cell_nesterov_place();

private:
  std::vector<instance_info> instance_list;
  void update_instance_list();

  Replace::Circuit* ckt;

  std::vector<std::string> lef_stor;
  std::vector<std::string> def_stor;
  std::vector<std::string> verilog_stor;
  std::vector<std::string> lib_stor;
  std::string sdc_file;
  std::string output_loc;
  bool timing_driven_mode;
  double unit_r;
  double unit_c;
};

#endif

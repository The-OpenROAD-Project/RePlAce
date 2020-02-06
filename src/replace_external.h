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
  
  void help();

  void import_lef(const char* lef);
  void import_def(const char* def);
  void export_def(const char* def);
  void set_output(const char* output);
  void set_output_experiment_name(const char* output);

  void set_timing_driven(bool is_true);
  void import_sdc(const char* sdc);
  void import_verilog(const char* verilog);
  void import_lib(const char* lib);
  void set_unit_res(double unit_r);
  void set_unit_cap(double unit_c);


  void set_plot_enable(bool plot_mode);
  void set_verbose_level(int verbose);
  void set_fast_mode_enable(bool fast_mode);
  void set_seed_init_enable(bool seed_init);
  void set_plot_color_file(std::string color_file);
  void set_write_bookshelf_enable(bool write_mode);
  
  void set_density(double density);
  void set_number_of_bin_grids(size_t grid_count);
  void set_lambda(double lambda);
  void set_min_pcof(double pcof_min);
  void set_max_pcof(double pcof_max);
  void set_step_scale(double step_scale);
  void set_target_overflow(double overflow);
  
  void set_net_weight_apply(bool mode);
  void set_min_net_weight(double net_weight_min);
  void set_max_net_weight(double net_weight_max);
  void set_net_weight_scale(double net_weight_scale);

  void set_routability_driven(bool mode);
  
  bool init_replace();
  bool place_cell_init_place();
  bool place_cell_nesterov_place();

  size_t get_instance_list_size();
  std::string get_master_name(size_t idx);
  std::string get_instance_name(size_t idx);
  float get_x(size_t idx); 
  float get_y(size_t idx);
  void print_instances();

  float get_hpwl();
  float get_wns();
  float get_tns(); 

  size_t get_module_size();
  size_t get_terminal_size();
  size_t get_net_size();
  size_t get_pin_size();
  size_t get_row_size();

  void import_custom_net_weights(const char* net_weight);

  bool save_jpeg(const char* jpeg);

private:
  std::vector<instance_info> instance_list;
  void update_instance_list();

  Replace::Circuit* ckt;

  std::vector<std::string> lef_stor;
  std::vector<std::string> def_stor;
  std::vector<std::string> verilog_stor;
  std::vector<std::string> lib_stor;
  std::string net_weight_file;
  std::string sdc_file;
  std::string output_loc;
  bool timing_driven_mode;
  bool routability_driven_mode;
  bool write_bookshelf_mode;
  double unit_r;
  double unit_c;
};

#endif

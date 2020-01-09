# Usage with TCL Interpreter

RePlAce has internal TCL Interpreter. The following line will create replace_external objects.

    replace_external rep
    
After having a replace_external object, a user can type any TCL commands after one spacing from the object name(e.g. rep).

    rep [tcl_command]


## File I/O Commands
* __import_lef__ [file_name] : \*.lef location (Multiple lef files supported. __Technology LEF must be ahead of other LEFs.__)
* __import_def__ [file_name] : \*.def location (Required due to FloorPlan information)
* __export_def__ [file_name] : Output DEF location
* __set_output__ [directory_location] : Specify the location of output results. Default: ./output
   
## Flow Control
* __init_replace__ : Initialize RePlAce's structure based on LEF and DEF.
* __place_cell_init_place__ : Execute BiCGSTAB engine for initial place.
* __place_cell_nesterov_place__ : Execute Nesterov engine for global placement.


## Timing-driven Mode
* __set_timing_driven__ [true/false] : Enable timing-driven modes
* __import_lib__ [file_name] : \*.lib location (Multiple lib files supported. Required for OpenSTA)
* __import_sdc__ [file_name] : \*.sdc location (Required for OpenSTA). SDC: Synopsys Design Constraint (SDC)
* __import_verilog__ [file_name] : \*.v location (Required for OpenSTA)
* __set_unit_res__ [resistor] : Resisance per micron. Unit: Ohm. (Used for Internal RC Extraction)
* __set_unit_cap__ [capacitance] : Capacitance per micron. Unit: Farad. (Used for Internal RC Extraction)

## RePlAce tunning parameters
__Note that the following tunning parameters must be defined before executing init_replace command__
* __set_density__ [density] : Set target density. [0-1, float]. Default: 1.00
* __set_bin_grid_count__ [num] : Set bin_grid_count. [64,128,256,512,..., int]. Default: Defined by internal algorithm.
* __set_lambda__ [lambda] : Set lambda for RePlAce tunning. [float]. Default : 8e-5~10e5
* __set_min_pcof__ [pcof_min] : Set pcof_min(µ_k Lower Bound). [0.95-1.05, float]. Default: 0.95
* __set_max_pcof__ [pcof_max] : Set pcof_max(µ_k Upper Bound). [1.00-1.20, float]. Default: 1.05
* __set_step_scale__ [step_scale] : Set step_scale(∆HPWL_REF). Default: 346000
* __set_target_overflow__ [overflow] : Set target overflow termination condition. [0.01-1.00, float]. Default: 0.1

__Timing-driven related tuning parameters__
* __set_min_net_weight__ [weight_min] : Set net_weight_min. [1.0-1.8, float]
* __set_max_net_weight__ [weight_max] : Set net_weight_max. [weight_min-1.8, float]
* __set_net_weight_scale__ [weight_scale] : Set net_weight_scale. [200-, float]

## Other options
* __set_plot_enable__ [mode] : Set plot modes; This mode will plot layout every 10 iterations (Cell, bin, and arrow plots) [true/false]. Default: False
* __set_seed_init_enable__ [true/false] : Start global place with the given placed locations. Default: False
* __set_fast_mode_enable__ [true/false] : Fast and quick placement for IO-pin placement. Please do __NOT__ use this command in general purposes (It'll not spread all cells enough). Default: False
* __set_verbose_level__ [level] : Specify the verbose level. [1-3, int]. Default: 1

## Query results
__Note that the following commands will work after init_replace command__
* __get_hpwl__ : Returns HPWL results on Micron. [float]
* __get_wns__ : Returns WNS from OpenSTA. (Only available when timing-driven mode is enabled) [float]
* __get_tns__ : Returns TNS from OpenSTA. (Only available when timing-driven mode is enabled) [float]
* __print_instances__ : Print out all of instances' information. (Not recommended for huge design)
* __get_instance_list_size__ : Returns total number of instances in RePlAce. [size_t]
* __get_x__ [index] : Returns x coordinates of specified instances' index. [float]
* __get_y__ [index] : Returns y coordinates of specified instances' index. [float]
* __get_master_name__ [index] : Returns master name of specified instances' index. [string]
* __get_instance_name__ [index] : Returns instance name of specified instances' index. [string]

## Example TCL scripts
* Non Timing-Driven RePlAce: [gcd_nontd_test.tcl](../test/gcd_nontd_test.tcl), [wb_nontd_test.tcl](../test/wb_nontd_test.tcl)
* Timing-Driven RePlAce: [gcd_td_test.tcl](../test/gcd_td_test.tcl), [wb_td_test.tcl](../test/wb_td_test.tcl)

FYI, All of the TCL commands are defined in the [replace_external.h](../src/replace_external.h) header files.

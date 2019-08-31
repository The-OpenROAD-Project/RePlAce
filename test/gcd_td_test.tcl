# 
# Examples for Timing-driven RePlAce with TCL usage
#

set design gcd
set lib_dir ./library/nangate45/
set design_dir ./design/nangate45/${design}

replace_external rep

# Import LEF/DEF files
rep import_lef ${lib_dir}/NangateOpenCellLibrary.lef
rep import_def ${design_dir}/${design}.def

# timing-driven parameters
rep import_verilog ${design_dir}/${design}.v
rep import_sdc ${design_dir}/${design}.sdc
rep import_lib ${lib_dir}/NangateOpenCellLibrary_typical.lib

rep set_unit_res 16
rep set_unit_cap 0.23e-15
rep set_timing_driven 1 

# set output folder location
rep set_output ./output/

# Initialize RePlAce
rep init_replace

# place_cell with initial place (BiCGSTAB)
rep place_cell_init_place

puts "initPlace HPWL: [rep get_hpwl]"

# print out instances' x/y coordinates
rep print_instances 

# place_cell with Nesterov (RePlAce)
rep place_cell_nesterov_place 

# print out instances' x/y coordinates
rep print_instances 

# Export DEF file
rep export_def ./gcd_TD.def
puts "nesterovPlace HPWL: [rep get_hpwl]"

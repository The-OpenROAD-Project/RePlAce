# 
# Examples for Non Timing-driven RePlAce with TCL usage
#

set design gcd
set lib_dir ./library/nangate45/
set design_dir ./design/nangate45/${design}


replace_external rep

# Import LEF/DEF files
rep import_lef ${lib_dir}/NangateOpenCellLibrary.lef
rep import_def ${design_dir}/${design}.def
rep set_output ./output/

rep set_verbose_level 0

# Initialize RePlAce
rep init_replace

# place_cell with BiCGSTAB 
rep place_cell_init_place


# print out instances' x/y coordinates
#rep print_instances

# place_cell with Nesterov method
rep place_cell_nesterov_place

# print out instances' x/y coordinates
#rep print_instances

# Export DEF file
rep export_def ./${design}_nan45_nontd.def
puts "Final HPWL: [rep get_hpwl]"


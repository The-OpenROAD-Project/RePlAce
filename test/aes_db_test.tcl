# 
# Examples for Non Timing-driven RePlAce with TCL usage
#

set design aes_cipher_top 
set lib_dir ./library/nangate45/
set design_dir ./design/nangate45/${design}


replace_external rep

# Import LEF/DEF files
rep import_lef ${lib_dir}/NangateOpenCellLibrary.lef
rep import_def ${design_dir}/${design}_60.def
rep set_output ./output/

rep set_verbose_level 0

# rep set_plot_enable
rep set_density 0.7

# Initialize RePlAce
rep init_replace_db

# place_cell with BiCGSTAB 
rep place_cell_init_place


# print out instances' x/y coordinates
#rep print_instances

# place_cell with Nesterov method
rep place_cell_nesterov_place

# print out instances' x/y coordinates
#rep print_instances

# Export DEF file
rep export_def ./aes_cipher_top_placed_60.def
puts "Final HPWL: [rep get_hpwl]"


# 
# Timing-driven RePlAce with TCL usage
#

set design wb_dma_top 
set lib_dir ./library/a2a/
set design_dir ./design/a2a/${design}

# Import LEF/DEF files
read_lef ${lib_dir}/contest.lef
read_def ${design_dir}/${design}.def

read_liberty ${lib_dir}/contest.lib
read_sdc ${design_dir}/${design}.sdc

set rep [replace_external]
$rep set_output ./output/

$rep set_unit_res 16
$rep set_unit_cap 0.23e-15
$rep set_timing_driven 1

$rep set_bin_grid_count 64

# Initialize RePlAce
$rep init_replace

# place_cell with Nesterov method 
$rep place_cell_nesterov_place

puts "nesterovPlace HPWL: [$rep get_hpwl]"
puts "final WNS: [$rep get_wns]"
puts "final TNS: [$rep get_tns]"

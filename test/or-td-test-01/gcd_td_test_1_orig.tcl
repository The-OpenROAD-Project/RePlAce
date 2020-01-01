# 
# Examples for Timing-driven RePlAce with TCL usage
#

set design gcd
set lib_dir library/nangate45/
set design_dir design/nangate45/${design}
set exp_folder or-td-test-01 


# Import LEF/DEF files
read_lef ${lib_dir}/NangateOpenCellLibrary.lef
read_def ${design_dir}/${design}.def

read_liberty ${lib_dir}/NangateOpenCellLibrary_typical.lib
read_sdc ${design_dir}/${design}.sdc

set rep [replace_external]
$rep set_verbose_level 0
$rep set_unit_res 16
$rep set_unit_cap 0.23e-15
$rep set_net_weight_apply false
$rep set_timing_driven 1 
$rep init_replace
$rep place_cell_nesterov_place

global_placement -timing_driven -wire_res 16 -wire_cap 0.23e-15

set fp [open ${exp_folder}/${design}_1_orig.rpt w]
puts $fp "HPWL: [$rep get_hpwl]"
puts $fp "WNS: [$rep get_wns]"
puts $fp "TNS: [$rep get_tns]"
close $fp

exit

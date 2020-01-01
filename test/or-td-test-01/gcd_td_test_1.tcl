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

# timing-driven parameters
read_liberty ${lib_dir}/NangateOpenCellLibrary_typical.lib
read_sdc ${design_dir}/${design}.sdc

set_wire_rc -resistance 16 -capacitance 0.23e-15

set rep [replace_external]
$rep set_verbose_level 3 

global_placement -skip_initial_place -timing_driven
global_placement -skip_initial_place -timing_driven
global_placement -skip_initial_place -timing_driven
global_placement -skip_initial_place -timing_driven
global_placement -skip_initial_place -timing_driven

set fp [open ${exp_folder}/${design}_1_td.rpt w]

puts $fp "HPWL: [$rep get_hpwl]"
puts $fp "WNS: [$rep get_wns]"
puts $fp "TNS: [$rep get_tns]"
close $fp

exit

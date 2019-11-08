# 
# Examples for Timing-driven RePlAce with TCL usage
#

set design gcd
set lib_dir ../library/nangate45/
set design_dir ../design/nangate45/${design}
set exp_folder exp

replace_external rep

# Import LEF/DEF files
rep import_lef ${lib_dir}/NangateOpenCellLibrary.lef
rep import_def ${design_dir}/${design}.def

# timing-driven parameters
rep import_verilog ${design_dir}/${design}.v
rep import_sdc ${design_dir}/${design}.sdc
rep import_lib ${lib_dir}/NangateOpenCellLibrary_typical.lib

rep set_unit_res 1.6
rep set_unit_cap 0.23e-14
rep set_timing_driven 1
rep set_net_weight_apply false

rep set_verbose_level 0

# Initialize RePlAce
rep init_replace

# place_cell with initial place (BiCGSTAB)
rep place_cell_init_place

# print out instances' x/y coordinates
# rep print_instances 

# place_cell with Nesterov (RePlAce)
rep place_cell_nesterov_place 

# print out instances' x/y coordinates
# rep print_instances 

# Export DEF file
# rep export_def ./${design}_nan45_td.def

if {![file exists ${exp_folder}/]} {
  exec mkdir ${exp_folder}
}

set fp [open ${exp_folder}/${design}_2_orig.rpt w]
puts $fp "HPWL: [rep get_hpwl]"
puts $fp "WNS: [rep get_wns]"
puts $fp "TNS: [rep get_tns]"
close $fp

exit

# 
# Examples for Timing-driven RePlAce with TCL usage
#

set design wb_dma_top 
set lib_dir ../library/a2a/
set design_dir ../design/a2a/${design}
set exp_folder exp

replace_external rep

# Import LEF/DEF files
rep import_lef ${lib_dir}/contest.lef
rep import_def ${design_dir}/${design}.def

# timing-driven parameters
rep import_verilog ${design_dir}/${design}.v
rep import_sdc ${design_dir}/${design}.sdc
rep import_lib ${lib_dir}/contest.lib

rep set_unit_res 16
rep set_unit_cap 0.23e-15
rep set_timing_driven 1

rep set_verbose_level 0

rep init_replace

# place_cell with Nesterov (RePlAce)
rep place_cell_nesterov_place 

# print out instances' x/y coordinates
# rep print_instances 

# Export DEF file
# rep export_def ./${design}_nan45_td.def

if {![file exists ${exp_folder}/]} {
  exec mkdir ${exp_folder}
}

set fp [open ${exp_folder}/${design}_1_td.rpt w]
puts $fp "HPWL: [rep get_hpwl]"
puts $fp "WNS: [rep get_wns]"
puts $fp "TNS: [rep get_tns]"
close $fp

exit

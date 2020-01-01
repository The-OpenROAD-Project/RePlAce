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

global_placement -timing_driven -wire_res 16 -wire_cap 0.23e-15 -bin_grid_count 64

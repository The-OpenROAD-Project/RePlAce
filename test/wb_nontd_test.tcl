# 
# Non-Timing-driven RePlAce with TCL usage
#

set design wb_dma_top 
set lib_dir ./library/a2a/
set design_dir ./design/a2a/${design}

# Import LEF/DEF files
read_lef ${lib_dir}/contest.lef
read_def ${design_dir}/${design}.def

global_placement -bin_grid_count 64

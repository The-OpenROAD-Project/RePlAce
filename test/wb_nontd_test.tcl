replace_external rep

# Import LEF/DEF files
rep import_lef ./library/a2a/contest.lef
rep import_def ./design/a2a/wb_dma_top/wb_dma_top.def
rep set_output ./output/

rep set_bin_grid_count 64

# Initialize RePlAce
rep init_replace

# place_cell with Nesterov method 
rep place_cell_nesterov_place

# print out instances' x/y coordinates
set insts_cnt [rep get_instance_list_size]
puts "Total # Instance: $insts_cnt"
for {set i 0} {$i <$insts_cnt} {incr i} {
  puts [format "%10s (%10s) x: %0.4f y: %0.4f" [rep get_instance_name $i] [rep get_master_name $i] [rep get_x $i] [rep get_y $i]]
}

# Export DEF file
rep export_def ./output_wb.def

exit

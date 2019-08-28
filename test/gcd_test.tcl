replace_external rep

# Import LEF/DEF files
rep import_lef ./library/nangate45/NangateOpenCellLibrary.lef
rep import_def ./design/nangate45/gcd/gcd.def
rep set_output ./output/

# Initialize RePlAce
rep init_replace

# place_cell with BiCGSTAB 
rep place_cell_init_place

# print out instances' x/y coordinates
set insts_cnt [rep get_instance_list_size]

# print out instances' x/y coordinates
puts "Total # Instance: $insts_cnt"
for {set i 0} {$i <$insts_cnt} {incr i} {
  puts [format "%10s (%10s) x: %0.4f y: %0.4f" [rep get_instance_name $i] [rep get_master_name $i] [rep get_x $i] [rep get_y $i]]
}

# place_cell with Nesterov method 
rep place_cell_nesterov_place

# print out instances' x/y coordinates
puts "Total # Instance: $insts_cnt"
for {set i 0} {$i <$insts_cnt} {incr i} {
  puts [format "%10s (%10s) x: %0.4f y: %0.4f" [rep get_instance_name $i] [rep get_master_name $i] [rep get_x $i] [rep get_y $i]]
}

# Export DEF file
rep export_def ./output.def

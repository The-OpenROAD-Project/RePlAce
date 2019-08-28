# 
# Examples for Timing-driven RePlAce with TCL usage
#
proc print_instances {rep} {
  set insts_cnt [rep get_instance_list_size]
  puts "Total # Instance: $insts_cnt"
  for {set i 0} {$i <$insts_cnt} {incr i} {
    puts [format "%10s (%10s) x: %0.4f y: %0.4f" [rep get_instance_name $i] [rep get_master_name $i] [rep get_x $i] [rep get_y $i]]
  }
}

set design gcd
set lib_dir ./library/nangate45/
set design_dir ./design/nangate45/${design}


replace_external rep

# Import LEF/DEF files
rep import_lef ${lib_dir}/NangateOpenCellLibrary.lef
rep import_def ${design_dir}/${design}.def
rep set_output ./output/

# Initialize RePlAce
rep init_replace

# place_cell with BiCGSTAB 
rep place_cell_init_place


# print out instances' x/y coordinates
print_instances rep

# place_cell with Nesterov method 
rep place_cell_nesterov_place

# print out instances' x/y coordinates
print_instances rep

# Export DEF file
rep export_def ./gcd_nonTD.def
puts "Final HPWL: [rep get_hpwl]"


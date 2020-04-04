source helpers.tcl
set test_name medium01 
read_lef ./nangate45.lef
read_def ./$test_name.def

# fastroute -write_route -write_est -max_routing_layer 3
global_placement -verbose 1  -skip_initial_place 

exit

set def_file [make_result_file route01.def]
write_def $def_file
diff_file $def_file route01.defok
exit

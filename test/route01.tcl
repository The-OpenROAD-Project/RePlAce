source helpers.tcl
set test_name medium01 
read_lef ./nangate45.lef
read_def ./$test_name.def

#global_placement -disable_routability_driven
#fastroute -write_route -write_est
global_placement -verbose 7 


set def_file [make_result_file route01.def]
write_def $def_file
diff_file $def_file route01.defok
exit

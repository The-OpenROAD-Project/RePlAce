replace_external rep

rep read_lef ./library/a2a/contest.lef
rep read_def ./design/a2a/wb_dma_top/wb_dma_top.def
rep set_output ./output/

rep init_design

set inst_names [rep get_instance_names]
set sorted_inst_names [lsort $inst_names]

foreach inst $sorted_inst_names {
  # puts "hello"
  # puts "$inst"
}


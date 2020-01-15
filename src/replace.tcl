sta::define_cmd_args "global_placement" {
  [-skip_initial_place]\
  [-density target_density]\
  [-timing_driven]\
    [-bin_grid_count grid_count]}

proc global_placement { args } {
  sta::parse_key_args "global_placement" args \
    keys {-bin_grid_count -wire_res -wire_cap -density \
      -lambda -min_pcoef -max_pcoef -overflow -verbose_level} \
      flags {-skip_initial_place -timing_driven}
    
  set target_density 0.7
  if { [info exists keys(-density)] } {
    set target_density $keys(-density) 
    sta::check_positive_float "-density" $target_density
  }
  set_replace_density_cmd $target_density

  # Support -wire_res/-wire_cap as overrides but do not make
  # them user visible. -cherry
  #
  #set wire_res 0.0
  #if { [info exists keys(-wire_res)] } {
  #  set wire_res $keys(-wire_res)
  #  sta::check_positive_float "-wire_res" $wire_res
  #} else {
  #  set wire_res [expr [sta::wire_resistance] * 1e-6]
  #}
  #$rep set_unit_res $wire_res

  #set wire_cap 0.0
  #if { [info exists keys(-wire_cap)] } {
  #  set wire_cap $keys(-wire_cap)
  #  sta::check_positive_float "-wire_cap" $wire_cap
  #} else {
  #  set wire_cap [expr [sta::wire_capacitance] * 1e-6]
  #}
  #$rep set_unit_cap $wire_cap  
  
  # hidden parameter to control the RePlAce divergence
  if { [info exists keys(-min_pcoef)] } { 
    set min_pcoef $keys(-min_pcoef)
    sta::check_positive_float "-min_pcoef" $min_pcoef
    set_replace_min_pcoef_cmd $min_pcoef
  } 

  if { [info exists keys(-max_pcoef)] } { 
    set max_pcoef $keys(-max_pcoef)
    sta::check_positive_float "-max_pcoef" $max_pcoef
    set_replace_max_pcoef_cmd $max_pcoef  
  }

  if { [info exists keys(-lambda)] } {
    set lambda $keys(-lambda)
    sta::check_positive_float "-lambda" $lambda
    set_replace_lambda_cmd $lambda
  }
  
  if { [info exists keys(-overflow)] } {
    set overflow $keys(-overflow)
    sta::check_positive_float "-overflow" $overflow
    set_replace_overflow_cmd $overflow
  }

  if { [info exists keys(-verbose_level)] } {
    set verbose_level $keys(-verbose_level)
    sta::check_positive_integer "-verbose_level" $verbose_level
    set_replace_verbose_level_cmd $verbose_level
  } 

  if { [info exists keys(-bin_grid_count)] } {
    set bin_grid_count  $keys(-bin_grid_count)
    sta::check_positive_integer "-bin_grid_count" $bin_grid_count
    set_replace_bin_grid_cnt_x_cmd $bin_grid_count
    set_replace_bin_grid_cnt_y_cmd $bin_grid_count    
  }

  if { ![info exists flags(-skip_initial_place)] } {
    set_replace_init_place_iter_cmd 0
  }

  if { [ord::db_has_rows] } {
    sta::check_argc_eq0 "global_placement" $args
  
    replace_init_place_cmd
    replace_nesterov_place_cmd
    
    puts "Worst slack: [format %.2e [sta::worst_slack]]"
    puts "Total negative slack: [format %.2e [sta::total_negative_slack]]"
  } else {
    puts "Error: no rows defined in design. Use initialize_floorplan to add rows."
  }
}

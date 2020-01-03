sta::define_cmd_args "global_placement" {
  [-skip_initial_place]\
  [-density target_density]\
  [-timing_driven]\
    [-bin_grid_count grid_count]}

proc global_placement { args } {
  sta::parse_key_args "global_placement" args \
    keys {-bin_grid_count -wire_res -wire_cap -density \
      -lambda -min_pcof -max_pcof -overflow -verbose_level} \
      flags {-skip_initial_place -timing_driven}
    
  set rep [replace_external]

  set target_density 0.7
  if { [info exists keys(-density)] } {
    set target_density $keys(-density) 
    sta::check_positive_float "-density" $target_density
  }
  $rep set_density $target_density

  # Support -wire_res/-wire_cap as overrides but do not make
  # them user visible. -cherry
  set wire_res 0.0
  if { [info exists keys(-wire_res)] } {
    set wire_res $keys(-wire_res)
    sta::check_positive_float "-wire_res" $wire_res
  } else {
    set wire_res [expr [sta::wire_resistance] * 1e-6]
  }
  $rep set_unit_res $wire_res

  set wire_cap 0.0
  if { [info exists keys(-wire_cap)] } {
    set wire_cap $keys(-wire_cap)
    sta::check_positive_float "-wire_cap" $wire_cap
  } else {
    set wire_cap [expr [sta::wire_capacitance] * 1e-6]
  }
  $rep set_unit_cap $wire_cap  
  
  # hidden parameter to control the RePlAce divergence
  if { [info exists keys(-min_pcof)] } { 
    set min_pcof $keys(-min_pcof)
    sta::check_positive_float "-min_pcof" $min_pcof
    $rep set_pcof_min $min_pcof
  } 

  if { [info exists keys(-max_pcof)] } { 
    set max_pcof $keys(-max_pcof)
    sta::check_positive_float "-max_pcof" $max_pcof
    $rep set_pcof_max $max_pcof
  }

  if { [info exists keys(-lambda)] } {
    set lambda $keys(-lambda)
    sta::check_positive_float "-lambda" $lambda
    $rep set_lambda $lambda
  }
  
  if { [info exists keys(-overflow)] } {
    set overflow $keys(-overflow)
    sta::check_positive_float "-overflow" $overflow
    $rep set_target_overflow $overflow
  }

  if { [info exists keys(-verbose_level)] } {
    set verbose_level $keys(-verbose_level)
    sta::check_positive_integer "-verbose_level" $verbose_level
    $rep set_verbose_level $verbose_level
  } else {
    $rep set_verbose_level 0
  }

  $rep set_timing_driven [info exists flags(-timing_driven)]

  if { [info exists keys(-bin_grid_count)] } {
    set bin_grid_count  $keys(-bin_grid_count)
    sta::check_positive_integer "-bin_grid_count" $bin_grid_count
    $rep set_bin_grid_count $bin_grid_count
  }
  sta::check_argc_eq0 "global_placement" $args

  if { [ord::db_has_rows] } {
    # Don't shit all over the file system
    $rep set_output /dev/null

    # Initialize RePlAce
    $rep init_replace

    if { ![info exists flags(-skip_initial_place)] } {
      # initial placement with BiCGSTAB
      $rep place_cell_init_place
    }
    # place_cell with Nesterov method 
    $rep place_cell_nesterov_place
    
    puts "HP wire length: [format %.0f [$rep get_hpwl]]"
    puts "Worst slack: [format %.2e [sta::worst_slack]]"
    puts "Total negative slack: [format %.2e [sta::total_negative_slack]]"
    $rep free_replace
  } else {
    puts "Error: no rows defined in design. Use initialize_floorplan to add rows."
  }
}

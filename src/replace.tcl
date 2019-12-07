sta::define_cmd_args "global_placement" {
  [-timing_driven]\
    [-bin_grid_count grid_count]}

proc global_placement { args } {
  sta::parse_key_args "global_placement" args \
    keys {-bin_grid_count -wire_res -wire_cap -skip_initial_place} flags {-timing_driven}

  set rep [replace_external]

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

  $rep set_timing_driven [info exists flags(-timing_driven)]

  if { [info exists keys(-bin_grid_count)] } {
    set bin_grid_count  $keys(-bin_grid_count)
    sta::check_positive_integer "-bin_grid_count" $bin_grid_count
    $rep set_bin_grid_count $bin_grid_count
  }

  if { [ord::db_has_rows] } {
    # Unfortunately this does not really turn off the noise. -cherry
    $rep set_verbose_level 0
    # Don't shit all over the file system
    $rep set_output /dev/null

    # Initialize RePlAce
    $rep init_replace

    if { [info exists keys(-skip_initial_place)] == false } {
      # initial placement with BiCGSTAB
      $rep place_cell_init_place
    }
    # place_cell with Nesterov method 
    $rep place_cell_nesterov_place
    
    puts "HP wire length: [format %.0f [$rep get_hpwl]]"
    puts "Worst slack: [format %.2e [sta::worst_slack]]"
    puts "Total negative slack: [format %.2e [sta::total_negative_slack]]"
  } else {
    puts "Error: no rows defined in design. Use initialize_floorplan to add rows."
  }
}

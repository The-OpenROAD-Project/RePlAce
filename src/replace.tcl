sta::define_cmd_args "global_placement" {
  [-timing_driven]\
    [-wire_res res]\
    [-wire_cap cap]\
    [-bin_grid_count grid_count]}

proc global_placement { args } {
  sta::parse_key_args "global_placement" args \
    keys {-bin_grid_count -wire_res -wire_cap} flags {-timing_driven}

  set rep [replace_external]

  set wire_res 0.0
  if { [info exists keys(-wire_res)] } {
    set wire_res $keys(-wire_res)
    sta::check_positive_float "-wire_res" $wire_res
  }
  $rep set_unit_res $wire_res

  set wire_cap 0.0
  if { [info exists keys(-wire_cap)] } {
    set wire_cap $keys(-wire_cap)
    sta::check_positive_float "-wire_cap" $wire_cap
  }
  $rep set_unit_cap $wire_cap

  $rep set_timing_driven [info exists flags(-timing_driven)]

  if { [info exists keys(-bin_grid_count)] } {
    set bin_grid_count  $keys(-bin_grid_count)
    sta::check_positive_integer "-bin_grid_count" $bin_grid_count
    $rep set_bin_grid_count $bin_grid_count
  }

  # Initialize RePlAce
  $rep init_replace
  
  # place_cell with Nesterov method 
  $rep place_cell_nesterov_place

  puts "HP wire length: [format %.0f [$rep get_hpwl]]"
  puts "Worst negative slack: [format %.2e [$rep get_wns]]"
  puts "Total negative slack: [format %.2e [$rep get_tns]]"
}

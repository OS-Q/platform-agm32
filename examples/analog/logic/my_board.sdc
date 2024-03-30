# pio_begin
if { ! [info exists ::HSI_PERIOD] } {
    set ::HSI_PERIOD 100.0
}
create_clock -name PIN_HSI -period $::HSI_PERIOD [get_ports PIN_HSI]
set_clock_groups -asynchronous -group PIN_HSI
if { ! [info exists ::HSE_PERIOD] } {
    set ::HSE_PERIOD 125.0
}
create_clock -name PIN_HSE -period $::HSE_PERIOD [get_ports PIN_HSE]
set_clock_groups -asynchronous -group PIN_HSE
derive_pll_clocks -create_base_clocks
set_false_path -from rv32|resetn_out
# pio_end

set SYS_CLK [get_clocks pll_inst|*clk*0*]
if { [get_clocks -nowarn pll_inst|*clk*3*] != {} } {
  set BUS_CLK [get_clocks pll_inst|*clk*3*]

  # Always make sure there is an extra cycle of margin for inter domain transfers between SYS_CLK
  # and BUS_CLK. The extra cycle is always in terms of the to (latching) clock.
  set_multicycle_path -from $SYS_CLK -to $BUS_CLK -setup 2
  set_multicycle_path -from $SYS_CLK -to $BUS_CLK -hold  1
  set_multicycle_path -from $BUS_CLK -to $SYS_CLK -setup 2
  set_multicycle_path -from $BUS_CLK -to $SYS_CLK -hold  1

  # These are for mem_ahb_hreadyout going into rv32, since it's ok for rv32 to receive
  # mem_ahb_hreadyout 1 cycle late. They theoretically should help useful skew.
  set_multicycle_path -from $SYS_CLK -to rv32 -setup 2
  set_multicycle_path -from $SYS_CLK -to rv32 -hold  1
}

set_false_path -from [get_pins rv32|sys_ctrl_stop]

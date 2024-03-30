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
# pio_end

set SYS_CLK {pll_inst|*clk*0*}
set BUS_CLK {pll_inst|*clk*3*}

set spi_pins {PIN_2 PIN_3 PIN_4 PIN_97}
create_generated_clock -name sck_out -source $BUS_CLK [get_ports PIN_98]

set_input_delay -clock sck_out -clock_fall -max 6 $spi_pins
set_input_delay -clock sck_out -clock_fall -min 1 $spi_pins

set_output_delay -clock sck_out -max  2 $spi_pins
set_output_delay -clock sck_out -min -3 $spi_pins

set_multicycle_path -from *|hwrite_reg* -to [get_clocks $BUS_CLK] -setup 2
set_multicycle_path -from *|hwrite_reg* -to [get_clocks $BUS_CLK] -hold  1
set_multicycle_path -from *|hsize_reg* -to *|ahb_hrdata* -setup 2
set_multicycle_path -from *|hsize_reg* -to *|ahb_hrdata* -hold  1
set_multicycle_path -from [get_clocks $SYS_CLK] -to *|spi_tx_data* -setup 2
set_multicycle_path -from [get_clocks $SYS_CLK] -to *|spi_tx_data* -hold  1

set_false_path -to *|sck_resetn*
set_false_path -to *|clrn
set_false_path -from *|spi_cmd_quad*


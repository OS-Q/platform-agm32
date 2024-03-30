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
set sck_pin   {rv32|gpio4_io_out_data[5]}
set data_pins {PIN_2 PIN_3 PIN_4 PIN_97}
create_generated_clock -name sck -divide_by 2 -source rv32|sys_clk -master pll_inst|*clk*0* $sck_pin
create_generated_clock -name sck_out -divide_by 1 -source $sck_pin -master sck [get_ports PIN_98]
set_input_delay -clock sck_out -clock_fall -max 6 $data_pins
set_input_delay -clock sck_out -clock_fall -min 1 $data_pins
set_multicycle_path -from *|spi_data_reg* -setup 2
set_multicycle_path -from *|spi_data_reg* -hold  1

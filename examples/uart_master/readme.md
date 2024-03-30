# Programming AGRV2K through a UART master
 
* 5 pins are needed:

| Pin     | Connection
| ---     | ---
| UART_RX | UART0_TX (PIN_68 for L100, PIN_42 for L64, PIN_30 for L48, PIN_20 for Q32)
| UART_TX | UART0_RX (PIN_69 for L100, PIN_43 for L64, PIN_31 for L48, PIN_21 for Q32)
| NRST    | NRST     (PIN_14 for L100, PIN_7  for L64, PIN_7  for L48, PIN_4  for Q32)
| BOOT0   | BOOT0    (PIN_94 for L100, PIN_60 for L64, PIN_44 for L48, PIN_30 for Q32)
| BOOT1   | Recommended to be weakly pulled down. Do not have to be controlled by the uart master

* Source files:
  * port.c/.h: Functions that must be provided when porting the UART master from AGRV2K to another 
    platform or CPU
  * uart_master.c/.h: Main functions. The following API's are provided:
    * uart_init: Must be called at the start of programming.
    * uart_init_flash: Call to program for the first time or any time to erase the entire flash.
    * uart_update_logic: Call to update the programmable logic in flash. Both compressed and 
      non-compressed bit streams are supported.
    * uart_update_program: Call to update the MCU program in flash. Should be called after 
      uart_update_logic.
    * uart_sram: Call to update programmable logic in SRAM, volatile. Only compressed bit stream is 
      supported. Do not reset the target in uart_final when using this.
    * uart_final: Must be called at the end of programming. Can optionally reset target.
  * main.c: An example of how to use the provided API's.

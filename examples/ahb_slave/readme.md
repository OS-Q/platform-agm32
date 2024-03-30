# AHB slave example

This example shows how to use the AHB slave port. The AHB bus data (clocked by the fast sys_clock) is 
converted to a slower APB bus (clocked by bus_clock) in module ahb2apb. Then data is saved to RAM (also 
clocked by the slower bus_clock) in module apb2ram. When trigger signal is sent from MCU core, the data 
saved in RAM is sent to an APB bus in module ram2apb, and is in turn sent to the faster AHB slave port.

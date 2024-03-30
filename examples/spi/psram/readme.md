### PSRAM example design
* This is an example for MCU to access a QSPI PSRAM via AHB.
* PSRAM is accessed at MMIO_BASE (0x60000000)
* AHB clock is driven by SYSCLK, which can be at most 200MHz
* SCK clock is driven by BUSCLK, which must be divided by 2, 3, or 4 from SYSCLK.
* QPI mode is used by default, with write command 'h38 and read command 'hEB. These can be changed in 
  file ahb2qspi.v.
* Tested with Lyonteck LY68L6400 under 200MHz SYSCLK and 100MHz BUSCLK.

### Fast SPI example design for SPI0/SPI1
* Internal SPI can run at about 40MHz when directly connected to top level pins.
* Read operation will fail for higher frequency due to timing.
* To achieve 100MHz SCK, do the following:
  * Insert 2 levels of pipeline for read path in logic. An example can be found in logic/fast_spi.v
  * Insert 2 cycles of dummy clock cycles in software by using an extra dummy TX phase. This can be done 
    in 2 ways:
    1. Define SPI_RX_PIPELINE=1 in platformio.ini via build_flags.
    2. When defining user functions, add an extra phase before RX phase like this:
    ```
    SPI_SetPhaseCtrl(SPIx, SPI_PHASE_y, SPI_PHASE_ACTION_DUMMY_TX, SPI_PHASE_MODE_QUAD, 1);
    ```

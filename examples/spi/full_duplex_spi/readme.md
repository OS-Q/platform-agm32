### Full duplex SPI example design
* Internal SPI only support either RX or TX action in any phase.
* To achieve full duplex SPI transfer, use TX action in software, and use a shift register in logic to 
  catch the input data from SO pin. After SPI transfer is done, read the shift register from AHB to get 
  the RX data.
* With this IP:
  1. SI_IO0 becomes the dedicated output MOSI.
  2. SO_IO1 becomes the dedicated input MISO.
  3. Only SPI_PHASE_MODE_SINGLE and SPI_PHASE_ACTION_TX can be used.
  4. RX data is read through logic.
* Supported features:
  1. CPOL and CPHA.
  2. DMA.
  3. Big and little endian.

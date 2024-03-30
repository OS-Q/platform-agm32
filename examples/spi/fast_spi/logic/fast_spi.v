module fast_spi (
  output        csn,
  inout         holdn_io3,
  inout         sck,
  inout         si_io0,
  inout         so_io1,
  inout         wpn_io2,
  input         csn_out_data,
  input         csn_out_en,
  output        holdn_io3_in,
  input         holdn_io3_out_data,
  input         holdn_io3_out_en,
  input         sck_out_data,
  input         sck_out_en,
  output        si_io0_in,
  input         si_io0_out_data,
  input         si_io0_out_en,
  output        so_io1_in,
  input         so_io1_out_data,
  input         so_io1_out_en,
  output        wpn_io2_in,
  input         wpn_io2_out_data,
  input         wpn_io2_out_en,
  input         sys_clock,
  input         bus_clock,
  input         resetn,
  input         stop,
  input  [1:0]  mem_ahb_htrans,
  input         mem_ahb_hready,
  input         mem_ahb_hwrite,
  input  [31:0] mem_ahb_haddr,
  input  [2:0]  mem_ahb_hsize,
  input  [2:0]  mem_ahb_hburst,
  input  [31:0] mem_ahb_hwdata,
  output tri1   mem_ahb_hreadyout,
  output        mem_ahb_hresp,
  output [31:0] mem_ahb_hrdata,
  output        slave_ahb_hsel,
  output tri1   slave_ahb_hready,
  input         slave_ahb_hreadyout,
  output [1:0]  slave_ahb_htrans,
  output [2:0]  slave_ahb_hsize,
  output [2:0]  slave_ahb_hburst,
  output        slave_ahb_hwrite,
  output [31:0] slave_ahb_haddr,
  output [31:0] slave_ahb_hwdata,
  input         slave_ahb_hresp,
  input  [31:0] slave_ahb_hrdata,
  output [3:0]  ext_dma_DMACBREQ,
  output [3:0]  ext_dma_DMACLBREQ,
  output [3:0]  ext_dma_DMACSREQ,
  output [3:0]  ext_dma_DMACLSREQ,
  input  [3:0]  ext_dma_DMACCLR,
  input  [3:0]  ext_dma_DMACTC,
  output [3:0]  local_int
);
assign mem_ahb_hreadyout = 1'b1;
assign slave_ahb_hready  = 1'b1;

reg [3:0] spi_read_reg;
reg [3:0] spi_data_reg;

// Add 2 levels of pipeline for read data. The clocks used here are deliberately designed. Do NOT change them.
always @ (negedge sck) begin
  spi_read_reg <= { holdn_io3, wpn_io2, so_io1, si_io0 };
end

always @ (negedge sck_out_data) begin
  spi_data_reg <= spi_read_reg;
end

// Connection to top level
assign csn       = csn_out_en       ? csn_out_data       : 1'bz;
assign sck       = sck_out_en       ? sck_out_data       : 1'bz;
assign si_io0    = si_io0_out_en    ? si_io0_out_data    : 1'bz;
assign so_io1    = so_io1_out_en    ? so_io1_out_data    : 1'bz;
assign wpn_io2   = wpn_io2_out_en   ? wpn_io2_out_data   : 1'bz;
assign holdn_io3 = holdn_io3_out_en ? holdn_io3_out_data : 1'bz;

// Connection to MCU core
assign si_io0_in    = spi_data_reg[0];
assign so_io1_in    = spi_data_reg[1];
assign wpn_io2_in   = spi_data_reg[2];
assign holdn_io3_in = spi_data_reg[3];

endmodule

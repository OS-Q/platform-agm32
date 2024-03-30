module psram (
  output        csn,
  inout         holdn_io3,
  inout         sck,
  inout         si_io0,
  inout         so_io1,
  inout         wpn_io2,
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
assign slave_ahb_hready = 1'b1;

wire [3:0] data_o;
wire [3:0] data_oe;
wire [3:0] data_i = {holdn_io3, wpn_io2, so_io1, si_io0};
assign si_io0    = data_oe[0] ? data_o[0] : 1'bz;
assign so_io1    = data_oe[1] ? data_o[1] : 1'bz;
assign wpn_io2   = data_oe[2] ? data_o[2] : 1'bz;
assign holdn_io3 = data_oe[3] ? data_o[3] : 1'bz;

(* keep = 1 *) wire sck_i = sck;
ahb2qspi u_ahb2qspi(
  .reset        (!resetn          ),
  .ahb_clock    (sys_clock        ),
  .ahb_htrans   (mem_ahb_htrans   ),
  .ahb_hready   (mem_ahb_hready   ),
  .ahb_hwrite   (mem_ahb_hwrite   ),
  .ahb_haddr    (mem_ahb_haddr    ),
  .ahb_hsize    (mem_ahb_hsize    ),
  .ahb_hburst   (mem_ahb_hburst   ),
  .ahb_hwdata   (mem_ahb_hwdata   ),
  .ahb_hrdata   (mem_ahb_hrdata   ),
  .ahb_hreadyout(mem_ahb_hreadyout),
  .ahb_hresp    (mem_ahb_hresp    ),

  .csn    (csn      ),
  .sck    (bus_clock),
  .sck_o  (sck_o    ),
  .sck_i  (sck_i    ),
  .data_i (data_i   ),
  .data_o (data_o   ),
  .data_oe(data_oe  )
);
assign sck = resetn ? sck_o : 1'bz;

endmodule

`timescale 1ns/1ps

module custom_ip (
  input         top_in,
  inout         top_inout,
  output        top_out,
  output        ip_inout_in,
  input         ip_inout_out_data,
  input         ip_inout_out_en,
  output        ip_pin_in,
  input         ip_pin_out_data,
  input         ip_pin_out_en,
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
  output tri0   mem_ahb_hresp,
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
parameter RAM_SIZE  = 4096; // In bytes
parameter ADDR_BITS = $clog2(RAM_SIZE);

wire [ADDR_BITS-3:0] ram_addr_a;
wire [3:0]           ram_byteena_a;
wire [31:0]          ram_data_a;
wire                 ram_wren_a;
wire                 ram_rden_a;
wire [31:0]          ram_q_a;

wire [ADDR_BITS-3:0] ram_addr_b;
wire [3:0]           ram_byteena_b;
wire [31:0]          ram_data_b;
wire                 ram_wren_b;
wire                 ram_rden_b;
wire [31:0]          ram_q_b;

ahb2ram #(ADDR_BITS) u_ahb2ram(
  .resetn       (resetn           ),
  .ahb_clock    (sys_clock        ),
  .ahb_htrans   (mem_ahb_htrans   ),
  .ahb_hsize    (mem_ahb_hsize    ),
  .ahb_hwdata   (mem_ahb_hwdata   ),
  .ahb_haddr    (mem_ahb_haddr    ),
  .ahb_hwrite   (mem_ahb_hwrite   ),
  .ahb_hrdata   (mem_ahb_hrdata   ),
  .ahb_hreadyout(mem_ahb_hreadyout),
  .ram_addr     (ram_addr_a       ),
  .ram_byteena  (ram_byteena_a    ),
  .ram_data     (ram_data_a       ),
  .ram_wren     (ram_wren_a       ),
  .ram_rden     (ram_rden_a       ),
  .ram_q        (ram_q_a          )
);

wire ram_trigger = ip_pin_out_en && ip_pin_out_data;

ram2ahb #(ADDR_BITS) u_ram2ahb2(
  .trigger      (ram_trigger        ),
  .finished     (ip_pin_in          ),
  .resetn       (resetn             ),
  .ahb_clock    (sys_clock          ),
  .ahb_hsel     (slave_ahb_hsel     ),
  .ahb_hready   (slave_ahb_hready   ),
  .ahb_hreadyout(slave_ahb_hreadyout),
  .ahb_htrans   (slave_ahb_htrans   ),
  .ahb_hsize    (slave_ahb_hsize    ),
  .ahb_hburst   (slave_ahb_hburst   ),
  .ahb_hwrite   (slave_ahb_hwrite   ),
  .ahb_haddr    (slave_ahb_haddr    ),
  .ahb_hwdata   (slave_ahb_hwdata   ),
  .ahb_hresp    (slave_ahb_hresp    ),
  .ahb_hrdata   (slave_ahb_hrdata   ),
  .ram_addr     (ram_addr_b         ),
  .ram_byteena  (ram_byteena_b      ),
  .ram_data     (ram_data_b         ),
  .ram_wren     (ram_wren_b         ),
  .ram_rden     (ram_rden_b         ),
  .ram_q        (ram_q_b            )
);

`ifdef ALTA_SYN
dpram u_ram(
  .Clk0       (bus_clock    ),
  .Clk1       (bus_clock    ),
  .ClkEn0     (1'b1         ),
  .ClkEn1     (1'b1         ),
  .AsyncReset0(1'b0         ),
  .AsyncReset1(1'b0         ),
  .WeA        (ram_wren_a   ),
  .ReA        (ram_rden_a   ),
  .WeB        (ram_wren_b   ),
  .ReB        (ram_rden_b   ),
  .DataInA    (ram_data_a   ),
  .AddressA   (ram_addr_a   ),
  .ByteEnA    (ram_byteena_a),
  .DataInB    (ram_data_b   ),
  .AddressB   (ram_addr_b   ),
  .ByteEnB    (ram_byteena_b),
  .DataOutA   (ram_q_a      ),
  .DataOutB   (ram_q_b      )
);
`else
altsyncram u_ram(
  .wren_a        (ram_wren_a   ),
  .wren_b        (ram_wren_b   ),
  .rden_a        (ram_rden_a   ),
  .rden_b        (ram_rden_b   ),
  .data_a        (ram_data_a   ),
  .data_b        (ram_data_b   ),
  .address_a     (ram_addr_a   ),
  .address_b     (ram_addr_b   ),
  .clock0        (sys_clock    ),
  .clock1        (sys_clock    ),
  .clocken0      (1'b1         ),
  .clocken1      (1'b1         ),
  .clocken2      (1'b1         ),
  .clocken3      (1'b1         ),
  .aclr0         (1'b0         ),
  .aclr1         (1'b0         ),
  .byteena_a     (ram_byteena_a),
  .byteena_b     (ram_byteena_b),
  .addressstall_a(1'b0         ),
  .addressstall_b(1'b0         ),
  .q_a           (ram_q_a      ),
  .q_b           (ram_q_b      ),
  .eccstatus     (             )
);
defparam
  u_ram.byte_size                     = 8,
  u_ram.clock_enable_input_a          = "BYPASS",
  u_ram.clock_enable_output_a         = "BYPASS",
  u_ram.intended_device_family        = "Cyclone IV E",
  u_ram.lpm_hint                      = "ENABLE_RUNTIME_MOD=NO",
  u_ram.operation_mode                = "BIDIR_DUAL_PORT",
  u_ram.numwords_a                    = RAM_SIZE/4,
  u_ram.widthad_a                     = ADDR_BITS - 2,
  u_ram.width_a                       = 32,
  u_ram.width_byteena_a               = 4,
  u_ram.outdata_aclr_a                = "NONE",
  u_ram.outdata_reg_a                 = "UNREGISTERED",
  u_ram.numwords_b                    = RAM_SIZE/4,
  u_ram.widthad_b                     = ADDR_BITS - 2,
  u_ram.width_b                       = 32,
  u_ram.width_byteena_b               = 4,
  u_ram.outdata_aclr_b                = "NONE",
  u_ram.outdata_reg_b                 = "UNREGISTERED",
  u_ram.ram_block_type                = "M9K",
  u_ram.read_during_write_mode_port_a = "NEW_DATA_NO_NBE_READ";
`endif


assign top_out = !top_in;
assign top_inout = ip_inout_out_en ? ip_inout_out_data : 1'bz;
assign ip_inout_in = top_inout;

endmodule

module apb2ram #(parameter ADDR_BITS = 32) (
  input                  resetn,
  input                  apb_clock,
  input                  apb_psel,
  input                  apb_penable,
  input                  apb_pwrite,
  input  [ADDR_BITS-1:0] apb_paddr,
  input  [31:0]          apb_pwdata,
  input  [3:0]           apb_pstrb,
  input  [2:0]           apb_pprot,
  output                 apb_pready,
  output                 apb_pslverr,
  output [31:0]          apb_prdata,
  output [ADDR_BITS-3:0] ram_addr,
  output [3:0]           ram_byteena,
  output [31:0]          ram_data,
  output                 ram_wren,
  output                 ram_rden,
  input  [31:0]          ram_q
);

assign apb_pready = 1'b1;
assign apb_pslverr = 1'b0;
assign apb_prdata = ram_q;

assign ram_addr = apb_paddr[ADDR_BITS-1:2];
assign ram_byteena = apb_pstrb;
assign ram_data = apb_pwdata;
assign ram_wren = apb_psel && !apb_penable && apb_pwrite;
assign ram_rden = apb_psel && !apb_penable && !apb_pwrite;

endmodule

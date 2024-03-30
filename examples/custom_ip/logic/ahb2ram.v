`timescale 1ns/1ps

module ahb2ram #(parameter ADDR_BITS = 32) (
  input                      resetn,
  input                      ahb_clock,
  input      [1:0]           ahb_htrans,
  input      [2:0]           ahb_hsize,
  input      [31:0]          ahb_hwdata,
  input      [31:0]          ahb_haddr,
  input                      ahb_hwrite,
  output reg [31:0]          ahb_hrdata,
  output reg                 ahb_hreadyout,
  output     [ADDR_BITS-3:0] ram_addr,
  output reg [3:0]           ram_byteena,
  output     [31:0]          ram_data,
  output reg                 ram_wren,
  output                     ram_rden,
  input      [31:0]          ram_q
);
// All AHB signals assume 1 cycle latency from RAM (no output register)

reg [ADDR_BITS-1:0] haddr_reg;
reg [2:0]           hsize_reg;
reg [31:0]          ram_qd;
reg                 rd_en_d1;
reg                 rd_en_d2;
reg                 rd_en_d3;
wire rd_en = !ahb_hwrite && (ahb_htrans[1] || !ahb_hreadyout);

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    ram_byteena <= 4'b1111;
  end else if (ahb_htrans[1] && ahb_hwrite) begin
    if (ahb_hsize == 3'b000) begin // byte write
      case(ahb_haddr[1:0])
        2'b00:	 ram_byteena <= 4'b0001;
        2'b01:	 ram_byteena <= 4'b0010;
        2'b10:	 ram_byteena <= 4'b0100;
        2'b11:	 ram_byteena <= 4'b1000;
        default: ram_byteena <= 4'hX;
      endcase
    end else if (ahb_hsize == 3'b001) begin // halfword write
      case (ahb_haddr[1])
        1'b0:	ram_byteena <= 4'b0011;
        1'b1:	ram_byteena <= 4'b1100;
        default:ram_byteena <= 4'hX;
      endcase
    end else begin // word (including above) write
      ram_byteena <= 4'b1111;
    end
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    hsize_reg <= 3'b0;
  end else if (ahb_htrans[1]) begin
    hsize_reg <= ahb_hsize;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn)
    ram_wren <= 0;
  else if (ahb_htrans[1] && ahb_hwrite)
    ram_wren <= 1;
  else
    ram_wren <= 0;
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn)
    haddr_reg <= 0;
  else if (ahb_htrans[1] & ahb_hwrite && ahb_hreadyout)
    haddr_reg <= ahb_haddr[ADDR_BITS-1:0];
  else if (ahb_htrans == 2'b10 && !ahb_hwrite && ahb_hreadyout)
    haddr_reg <= ahb_haddr[ADDR_BITS-1:0];
  else if (ahb_htrans == 2'b11 && !ahb_hwrite && rd_en_d1)
    haddr_reg <= haddr_reg + (1 << hsize_reg);
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    rd_en_d1 <= 0;
    rd_en_d2 <= 0;
    rd_en_d3 <= 0;
  end else begin
    rd_en_d1 <= rd_en;
    rd_en_d2 <= rd_en_d1;
    rd_en_d3 <= rd_en_d2;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn)
    ahb_hreadyout <= 1;
  else if (rd_en && !rd_en_d1)
    ahb_hreadyout <= 0;
  else if (rd_en_d3 && rd_en_d2)
    ahb_hreadyout <= 1;
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn)
    ahb_hrdata <= 0;
  else if (rd_en_d3)
    ahb_hrdata <= ram_qd;
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn)
    ram_qd <= 0;
  else if (rd_en_d2)
    ram_qd <= ram_q;
end

assign ram_addr = haddr_reg[ADDR_BITS-1:2];
assign ram_data = ahb_hwdata;
assign ram_rden = rd_en_d1;

endmodule

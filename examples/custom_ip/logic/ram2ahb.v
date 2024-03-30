`timescale 1ns/1ps

module ram2ahb #(parameter ADDR_BITS = 32) (
  input                      trigger,
  output reg                 finished,
  input                      resetn,
  input                      ahb_clock,
  output                     ahb_hsel,
  output                     ahb_hready,
  input                      ahb_hreadyout,
  output     [1:0]           ahb_htrans,
  output     [2:0]           ahb_hsize,
  output     [2:0]           ahb_hburst,
  output                     ahb_hwrite,
  output     [31:0]          ahb_haddr,
  output     [31:0]          ahb_hwdata,
  input                      ahb_hresp,
  input      [31:0]          ahb_hrdata,
  output reg [ADDR_BITS-3:0] ram_addr,
  output     [3:0]           ram_byteena,
  output     [31:0]          ram_data,
  output reg                 ram_wren,
  output                     ram_rden,
  input      [31:0]          ram_q
);
// All AHB signals assume 1 cycle latency from RAM (no output register)

parameter AHB_INCR   = 3'b001; // Burst with no length defined. Much slower.
parameter AHB_INCR4  = 3'b011;
parameter AHB_INCR8  = 3'b101;
parameter AHB_INCR16 = 3'b111;
parameter AHB_HBURST = AHB_INCR16;

reg        hsel;
reg [1:0]  htrans;
reg        hwrite;
reg [31:0] haddr;
reg [31:0] hwdata;
reg [31:0] hrdata;

reg ahb_data_phase;
reg ahb_write;
reg ahb_run;
reg ahb_addr_valid;
reg [31:0] ram_qd;
reg [ADDR_BITS-3:0] hcount; // Only used in AHB_INCR to determine the end of transfer

wire burst_end =
  AHB_HBURST == AHB_INCR4  ? (haddr[3:2] == 0) :
  AHB_HBURST == AHB_INCR8  ? (haddr[4:2] == 0) :
  AHB_HBURST == AHB_INCR16 ? (haddr[5:2] == 0) :
  ahb_hreadyout && (hcount == (1 << (ADDR_BITS-2)) - 1);
wire ram_end = ram_addr == (1 << (ADDR_BITS-2)) - 1;
wire ahb_stop = ahb_hsel && ahb_data_phase && burst_end && ram_end;

reg trigger_d;
wire trigger_rise = !trigger_d && trigger;

// When triggered first time, ahb_write is 0. Data is read from AHB and saved to RAM. When triggered for the second time, 
// ahb_write is 1. Data is read from RAM and written to AHB.
always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    ahb_write <= 1'b0;
  end else if (finished && trigger_rise) begin
    ahb_write <= !ahb_write;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    trigger_d <= 1'b0;
  end else begin
    trigger_d <= trigger;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    ahb_run <= 1'b0;
  end else if (trigger_rise) begin
    ahb_run <= 1'b1;
  end else if (ahb_stop) begin
    ahb_run <= 1'b0;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    finished <= 1'b0;
  end else if (trigger_rise) begin
    finished <= 1'b0;
  end else if (ahb_hsel && !ahb_run && ahb_hreadyout) begin
    finished <= 1'b1;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    ahb_addr_valid <= 1'b0;
  end else begin
    ahb_addr_valid <= ahb_run;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    ahb_data_phase <= 1'b0;
  end else if (htrans[1]) begin
    ahb_data_phase <= 1'b1;
  end else if (ahb_hreadyout) begin
    ahb_data_phase <= 1'b0;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    ram_wren <= 1'b0;
  end else begin
    ram_wren <= !ahb_write && ahb_data_phase && ahb_hreadyout;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    ram_addr <= 'h0;
  end else if (!ahb_run && !ahb_data_phase) begin
    ram_addr <= 'h0;
  end else if (!ram_end && (ahb_write && ram_rden || !ahb_write && ram_wren)) begin
    ram_addr <= ram_addr + 1;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    hsel   <= 1'b0;
    htrans <= 2'b00;
    hwrite <= 1'b0;
  end else if (ahb_run && ahb_addr_valid && !ahb_stop) begin
    hsel   <= 1'b1;
    hwrite <= ahb_write;
    if (ahb_hreadyout) begin
      if (htrans == 2'b00) begin
        htrans <= 2'b10;
      end else begin
        case (AHB_HBURST)
          AHB_INCR4:  htrans <= haddr[3:2] == 2'h3 ? 2'b00 : 2'b11;
          AHB_INCR8:  htrans <= haddr[4:2] == 3'h7 ? 2'b00 : 2'b11;
          AHB_INCR16: htrans <= haddr[5:2] == 4'hf ? 2'b00 : 2'b11;
          default:    htrans <= burst_end           ? 2'b00 : 2'b11; // No burst
        endcase
      end
    end
  end else begin
    if (finished) begin
      hsel <= 1'b0;
    end
    htrans <= 2'b00;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    haddr <= 32'h0;
    hcount <= 0;
  end else if (ahb_run && ahb_hreadyout && ahb_addr_valid) begin
    if (!hsel) begin
      haddr <= ram_q;
      hcount <= 0;
    end else if (htrans[1]) begin
      haddr <= haddr + 4;
      if (!burst_end) begin
        hcount <= hcount + 1;
      end
    end
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    hrdata <= 32'h0;
  end else if (!ahb_write && ahb_data_phase && ahb_hreadyout) begin
    hrdata <= ahb_hrdata;
  end
end

always @ (posedge ahb_clock or negedge resetn) begin
  if (!resetn) begin
    hwdata <= 32'h0;
    ram_qd <= 32'h0;
  end else if (ram_rden) begin
    hwdata <= ram_qd;
    ram_qd <= ram_q;
  end
end

assign ahb_hsel   = hsel;
assign ahb_hready = ahb_hreadyout;
assign ahb_htrans = htrans;
assign ahb_hsize  = 3'b010; // Word access
assign ahb_hburst = AHB_HBURST;
assign ahb_hwrite = hwrite;
assign ahb_haddr  = haddr;
assign ahb_hwdata = hwdata;

assign ram_byteena = 4'hf;
assign ram_data    = hrdata;
assign ram_rden    = ahb_run && (ahb_htrans[1] || !hsel) && ahb_hreadyout && (ahb_write || !ahb_addr_valid);

endmodule

module ram2apb #(parameter ADDR_BITS = 32, DATA_BITS = 32) (
  input                      trigger,
  output reg                 finished,
  input                      resetn,
  input                      apb_clock,
  output reg                 apb_psel,
  output reg                 apb_penable,
  output reg                 apb_pwrite,
  output reg [31:0]          apb_paddr,
  output     [DATA_BITS-1:0] apb_pwdata,
  output     [3:0]           apb_pstrb,
  output     [2:0]           apb_pprot,
  input                      apb_pready,
  input                      apb_pslverr,
  input      [DATA_BITS-1:0] apb_prdata,
  output reg [ADDR_BITS-3:0] ram_addr,
  output     [3:0]           ram_byteena,
  output     [31:0]          ram_data,
  output reg                 ram_wren,
  output                     ram_rden,
  input      [31:0]          ram_q
);

reg apb_run;
reg apb_addr_valid;
reg apb_write;
reg apb_data_phase;
reg [31:0] prdata;

wire ram_end = apb_addr_valid && (!apb_write && ram_addr == (1 << (ADDR_BITS-2)) - 1 || apb_write && ram_addr == 0);
wire apb_stop = ram_end && apb_psel && apb_penable && apb_pready;

reg trigger_d, trigger_dd;
wire trigger_rise = !trigger_dd && trigger_d;

// When triggered first time, apb_write is 0. Data is read from APB and saved to RAM. When triggered for the second time, 
// apb_write is 1. Data is read from RAM and written to APB.
always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    apb_write <= 1'b0;
  end else if (finished && trigger_rise) begin
    apb_write <= !apb_write;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    trigger_d  <= 1'b0;
    trigger_dd <= 1'b0;
  end else begin
    trigger_d  <= trigger;
    trigger_dd <= trigger_d;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    apb_run  <= 1'b0;
  end else if (trigger_rise) begin
    apb_run  <= 1'b1;
  end else if (apb_stop) begin
    apb_run  <= 1'b0;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    finished <= 1'b0;
  end else if (trigger_rise) begin
    finished <= 1'b0;
  end else if (!apb_run && apb_addr_valid && apb_pready) begin
    finished <= 1'b1;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    apb_addr_valid <= 1'b0;
  end else begin
    apb_addr_valid <= apb_run;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    apb_data_phase <= 1'b0;
  end else if (apb_psel && !apb_penable) begin
    apb_data_phase <= 1'b1;
  end else if (apb_pready) begin
    apb_data_phase <= 1'b0;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    ram_wren <= 1'b0;
  end else begin
    ram_wren <= !apb_write && apb_psel && apb_penable && apb_pready;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    ram_addr <= 'h0;
  end else if (!apb_run && !apb_data_phase) begin
    ram_addr <= 'h0;
  end else if (!ram_end && (apb_write && ram_rden || !apb_write && ram_wren)) begin
    ram_addr <= ram_addr + 1;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    apb_psel    <= 1'b0;
    apb_penable <= 1'b0;
    apb_pwrite  <= 1'b0;
  end else if (apb_run && apb_addr_valid && !apb_stop) begin
    apb_psel    <= 1'b1;
    if (apb_psel && !apb_penable) begin
      apb_penable <= 1'b1;
    end else if (apb_pready) begin
      apb_penable <= 1'b0;
    end
    apb_pwrite  <= apb_write;
  end else begin
    apb_psel    <= 1'b0;
    apb_penable <= 1'b0;
    apb_pwrite  <= 1'b0;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    apb_paddr <= 32'h0;
  end else if (apb_run && apb_pready && apb_addr_valid && apb_psel == apb_penable) begin
    apb_paddr <= !apb_psel ? ram_q : apb_paddr + 4;
  end else if (!apb_run && !apb_addr_valid) begin
    apb_paddr <= 32'h0;
  end
end

always @ (posedge apb_clock or negedge resetn) begin
  if (!resetn) begin
    prdata <= 32'h0;
  end else if (!apb_write && apb_data_phase && apb_pready) begin
    prdata <= apb_prdata;
  end
end

assign apb_pwdata = ram_q;
assign apb_pstrb  = 4'hf;
assign apb_pprot  = 3'b001;

assign ram_byteena = 4'hf;
assign ram_data    = prdata;
assign ram_rden    = apb_run && (apb_penable || !apb_addr_valid) && apb_pready && (apb_write || !apb_addr_valid);

endmodule

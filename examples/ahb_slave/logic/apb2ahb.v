module apb2ahb #(parameter integer
  ADDR_BITS = 32,
  DATA_BITS = 32
) (
  input                      reset,
  input                      apb_clock,
  input                      apb_psel,
  input                      apb_penable,
  input                      apb_pwrite,
  input      [ADDR_BITS-1:0] apb_paddr,
  input      [DATA_BITS-1:0] apb_pwdata,
  input      [3:0]           apb_pstrb,
  input      [2:0]           apb_pprot,
  output reg                 apb_pready,
  output reg                 apb_pslverr,
  output reg [DATA_BITS-1:0] apb_prdata,
  input                      ahb_clock,
  output                     ahb_hmastlock,
  output     [1:0]           ahb_htrans,
  output                     ahb_hsel,
  output                     ahb_hready,
  output                     ahb_hwrite,
  output     [ADDR_BITS-1:0] ahb_haddr,
  output     [2:0]           ahb_hsize,
  output     [2:0]           ahb_hburst,
  output     [3:0]           ahb_hprot,
  output     [DATA_BITS-1:0] ahb_hwdata,
  input      [DATA_BITS-1:0] ahb_hrdata,
  input                      ahb_hreadyout,
  input                      ahb_hresp
);

reg       hsel;
reg       hready;
reg       hwrite;
reg [1:0] haddr;
reg [2:0] hsize;
reg [31:0] hrdata;

reg pvalid;
reg hvalid;
reg herror;
reg ahb_pending, ahb_pending_d;
reg data_pending;

always @ (posedge apb_clock or posedge reset) begin
  if (reset) begin
    apb_pready <= 1'b1;
    apb_prdata <= 32'h0;
  end else if (apb_psel && !apb_penable) begin
    apb_pready <= 1'b0;
  end else if (apb_psel && apb_penable && !ahb_pending && !ahb_pending_d && data_pending) begin
    apb_pready <= 1'b1;
    apb_prdata <= hrdata;
  end
end

always @ (posedge apb_clock or posedge reset) begin
  if (reset) begin
    apb_pslverr <= 1'b0;
  end else if (apb_psel && apb_penable && !ahb_pending && !ahb_pending_d && data_pending && herror) begin
    apb_pslverr <= 1'b1;
  end else begin
    apb_pslverr <= 1'b0;
  end
end

always @ (posedge apb_clock or posedge reset) begin
  if (reset) begin
    pvalid <= 1'b0;
  end else if (apb_psel && apb_penable && apb_pready) begin
    pvalid <= 1'b0;
  end else if (apb_psel) begin
    pvalid <= 1'b1;
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    hvalid <= 1'b0;
  end else if (hvalid) begin
    hvalid <= 1'b0;
  end else if (apb_psel && !apb_penable && !data_pending) begin
    hvalid <= 1'b1;
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    herror <= 1'b0;
  end else if (apb_pslverr) begin
    herror <= 1'b0;
  end else if (ahb_hresp) begin
    herror <= 1'b1;
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    hsel <= 1'b0;
  end else if (hsel) begin
    hsel <= 1'b0;
  end else if (hvalid && !data_pending) begin
    hsel <= 1'b1;
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    hwrite <= 1'b0;
  end else if (hvalid) begin
    hwrite <= apb_pwrite;
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    hrdata <= 32'h0;
  end else if (ahb_pending) begin
    hrdata <= ahb_hrdata;
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    hsize <= 3'b000;
    haddr <= 2'b00;
  end else if (hvalid) begin
    if (apb_pwrite) begin
      case (apb_pstrb)
        4'b0001: begin
          hsize <= 3'b000;
          haddr <= 2'b00;
        end
        4'b0010: begin
          hsize <= 3'b000;
          haddr <= 2'b01;
        end
        4'b0100: begin
          hsize <= 3'b000;
          haddr <= 2'b10;
        end
        4'b1000: begin
          hsize <= 3'b000;
          haddr <= 2'b11;
        end
        4'b0011: begin
          hsize <= 3'b001;
          haddr <= 2'b00;
        end
        4'b1100: begin
          hsize <= 3'b001;
          haddr <= 2'b10;
        end
        default: begin
          hsize <= 3'b010;
          haddr <= 2'b00;
      end
      endcase
    end else begin
      hsize <= 3'b010;
      haddr <= 2'b00;
    end
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    ahb_pending <= 1'b0;
  end else if (ahb_hsel && ahb_htrans[1] && ahb_hreadyout) begin
    ahb_pending <= 1'b1;
  end else if (ahb_hreadyout) begin
    ahb_pending <= 1'b0;
  end
end

// Provide an extra cycle for data transfers from ahb to apb domain.
always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    ahb_pending_d <= 1'b0;
  end else begin
    ahb_pending_d <= ahb_pending;
  end
end

always @ (posedge ahb_clock or posedge reset) begin
  if (reset) begin
    data_pending <= 1'b0;
  end else if (ahb_hsel && ahb_htrans[1] && ahb_hreadyout) begin
    data_pending <= 1'b1;
  end else if (ahb_hreadyout && apb_pready) begin
    data_pending <= 1'b0;
  end
end

assign ahb_htrans = {hsel, 1'b0};
assign ahb_hsel   = hsel;
assign ahb_hready = 1'b1; // Can use 1'b1 instead of ahb_hreadyout only if non-sequential xfer is used (htrans = 2'b10)
assign ahb_hwrite = apb_pwrite;
assign ahb_haddr  = {apb_paddr[ADDR_BITS-1:2], haddr};
assign ahb_hsize  = hsize;

assign ahb_hburst = 3'b0; // Single transfer
assign ahb_hprot  = 4'b0011;
assign ahb_hwdata = apb_pwdata;

endmodule

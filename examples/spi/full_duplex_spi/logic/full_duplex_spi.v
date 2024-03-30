module full_duplex_spi (
  output             csn,
  output             sck,
  output             si_io0,
  input              so_io1,
  input              csn_out_data,
  input              csn_out_en,
  input              sck_out_data,
  input              sck_out_en,
  input              si_io0_out_data,
  input              si_io0_out_en,
  input              sys_clock,
  input              bus_clock,
  input              resetn,
  input              stop,
  input       [1:0]  mem_ahb_htrans,
  input              mem_ahb_hready,
  input              mem_ahb_hwrite,
  input       [31:0] mem_ahb_haddr,
  input       [2:0]  mem_ahb_hsize,
  input       [2:0]  mem_ahb_hburst,
  input       [31:0] mem_ahb_hwdata,
  output tri1        mem_ahb_hreadyout,
  output tri0        mem_ahb_hresp,
  output tri0 [31:0] mem_ahb_hrdata,
  output tri0        slave_ahb_hsel,
  output tri1        slave_ahb_hready,
  input              slave_ahb_hreadyout,
  output tri0 [1:0]  slave_ahb_htrans,
  output tri0 [2:0]  slave_ahb_hsize,
  output tri0 [2:0]  slave_ahb_hburst,
  output tri0        slave_ahb_hwrite,
  output tri0 [31:0] slave_ahb_haddr,
  output tri0 [31:0] slave_ahb_hwdata,
  input              slave_ahb_hresp,
  input       [31:0] slave_ahb_hrdata,
  output tri0 [3:0]  ext_dma_DMACBREQ,
  output tri0 [3:0]  ext_dma_DMACLBREQ,
  output tri0 [3:0]  ext_dma_DMACSREQ,
  output tri0 [3:0]  ext_dma_DMACLSREQ,
  input       [3:0]  ext_dma_DMACCLR,
  input       [3:0]  ext_dma_DMACTC,
  output tri0 [3:0]  local_int
);
assign slave_ahb_hready = 1'b1;
// Use a fifo in SPI rx to avoid data loss when DMA is very busy and cannot respond timely.
`define SPI_USE_FIFO

reg hreadyout_reg;
reg hwrite_reg;
reg [11:0] haddr_reg;
reg [31:0] hrdata_reg;
reg [31:0] hrdata_ctrl;
reg [31:0] hrdata_data;

reg csn_reg;
reg sck_reg;
reg si_io0_out;
reg [5:0] sck_cnt;
reg [7:0] sck_psc;
reg [7:0] spi_rx_byte;
reg [31:0] spi_rx_reg;
reg spi_ctrl_cpol;
reg spi_ctrl_cpha;

wire sck_pos = !csn_out_data & !sck_reg & sck_out_data;
wire sck_neg = (!sck_out_data | csn_out_data) & sck_reg;
wire sck_edge = spi_ctrl_cpha ? sck_neg : sck_pos;
reg sck_edge_del0, sck_edge_del1;

reg spi_rx_endian;
reg spi_rx_dma_en;
reg spi_rx_dma_req;
wire spi_rx_dma_clr = ext_dma_DMACCLR[0];
assign ext_dma_DMACBREQ[0] = spi_rx_dma_req;

assign si_io0 = si_io0_out_en ? si_io0_out : 1'bz;
assign sck    = sck_out_en    ? sck_reg ^ spi_ctrl_cpol : 1'bz;
assign csn    = csn_reg;

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    csn_reg <= 1'b1;
  end else if (!hreadyout_reg && hwrite_reg) begin
    csn_reg <= 1'b1;
  end else if (csn_reg || !csn_out_data || !sck_reg && sck_psc == 1) begin
    csn_reg <= csn_out_data;
  end
end

always @(posedge sys_clock) begin
  sck_edge_del0 <= sck_edge;
  sck_edge_del1 <= sck_edge_del0;
end

always @(posedge sys_clock) begin
  sck_reg <= !csn_out_data & sck_out_data;
end

always @(posedge sys_clock) begin
  if (!spi_ctrl_cpha || sck_pos) begin
    si_io0_out <= si_io0_out_data;
  end
end

always @(posedge sys_clock) begin
  if (csn_reg) begin
    sck_psc <= 0;
  end else if (!csn_reg && sck_reg) begin
    sck_psc <= sck_psc + 1;
  end else if (!csn_reg && !sck_reg && sck_psc > 0) begin
    sck_psc <= sck_psc - 1;
  end
end

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    sck_cnt <= 32;
  end else if (csn_reg) begin
    sck_cnt <= 32;
  end else if (sck_edge_del0) begin
    sck_cnt <= sck_cnt == 0 ? 31 : sck_cnt - 1;
  end
end

reg rx_fifo_wrreq;
reg rx_fifo_rdreq;
reg rx_fifo_rddel;
wire [31:0] rx_fifo_q;
`ifdef SPI_USE_FIFO
  wire rx_fifo_empty;
  scfifo rx_fifo(
    .data        (spi_rx_reg    ),
    .clock       (sys_clock     ),
    .wrreq       (rx_fifo_wrreq ),
    .rdreq       (rx_fifo_rdreq ),
    .aclr        (!resetn       ),
    .sclr        (!spi_rx_dma_en),
    .q           (rx_fifo_q     ),
    .usedw       (              ),
    .full        (              ),
    .empty       (rx_fifo_empty ),
  `ifdef SCFIFO_HAS_ECC
    .eccstatus   (              ),
  `endif
    .almost_full (              ),
    .almost_empty(              )
  );
  defparam
    rx_fifo.intended_device_family  = "Cyclone IV E",
    rx_fifo.lpm_width               = 32,
    rx_fifo.lpm_widthu              = 8,
    rx_fifo.lpm_numwords            = 256,
    rx_fifo.underflow_checking      = "OFF",
    rx_fifo.overflow_checking       = "OFF",
    rx_fifo.allow_rwcycle_when_full = "OFF",
    rx_fifo.use_eab                 = "ON",
    rx_fifo.add_ram_output_register = "ON";
`else
  reg rx_fifo_empty;
  assign rx_fifo_q = spi_rx_reg;
  always @(posedge sys_clock or negedge resetn) begin
    if (!resetn) begin
      rx_fifo_empty <= 1;
    end else if (rx_fifo_wrreq) begin
      rx_fifo_empty <= 0;
    end else if (rx_fifo_rdreq) begin
      rx_fifo_empty <= 1;
    end
  end
`endif

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    rx_fifo_wrreq <= 0;
  end else begin
    rx_fifo_wrreq <= csn_reg && sck_cnt < 32 && sck_cnt > 0 || sck_cnt == 0 && sck_edge_del1;
  end
end

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    rx_fifo_rdreq <= 1'b0;
    rx_fifo_rddel <= 1'b0;
  end else begin
    rx_fifo_rdreq <= !rx_fifo_empty && mem_ahb_htrans[1] && mem_ahb_hready && !mem_ahb_hwrite;
    rx_fifo_rddel <= rx_fifo_rdreq;
  end
end

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    spi_rx_dma_en <= 1'b0;
    spi_rx_endian <= 1'b0;
    spi_ctrl_cpol <= 1'b0;
    spi_ctrl_cpha <= 1'b0;
  end else if (!hreadyout_reg && hwrite_reg) begin
    case (haddr_reg >> 2)
      0: begin
        spi_rx_dma_en <= mem_ahb_hwdata[8];  // SPI_CTRL_DMA_EN
        spi_rx_endian <= mem_ahb_hwdata[10]; // SPI_CTRL_ENDIAN
        spi_ctrl_cpol <= mem_ahb_hwdata[24];
        spi_ctrl_cpha <= mem_ahb_hwdata[25];
      end
    endcase
  end
end

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    spi_rx_dma_req <= 1'b0;
  end else if (!spi_rx_dma_en || spi_rx_dma_clr) begin
    spi_rx_dma_req <= 1'b0;
  end else if (spi_rx_dma_en && !rx_fifo_empty) begin
    spi_rx_dma_req <= 1'b1;
  end
end

// Catch so input with a shift register. Data is in big endian.
always @(posedge sys_clock) begin
  if (!csn_reg & sck_edge_del0) begin
    spi_rx_byte <= (spi_rx_byte << 1) | so_io1;
  end
end

always @(posedge sys_clock) begin
  if (sck_edge_del1 && sck_cnt[2:0] == 0) begin
    if (spi_rx_endian) begin
      // Litten endian
      spi_rx_reg[(3-sck_cnt[4:3])*8+:8] <= spi_rx_byte;
    end else begin
      // Big endian
      spi_rx_reg[sck_cnt[4:3]*8+:8] <= spi_rx_byte;
    end
  end
end

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    hreadyout_reg <= 1'b1;
  end else if (mem_ahb_htrans[1] && mem_ahb_hready) begin
    hreadyout_reg <= 1'b0;
  end else if (!rx_fifo_rdreq && !rx_fifo_rddel) begin
    hreadyout_reg <= 1'b1;
  end
end

always @(posedge sys_clock or negedge resetn) begin
  if (!resetn) begin
    hwrite_reg <= 1'b0;
    haddr_reg  <= 0;
  end else if (mem_ahb_htrans[1] && mem_ahb_hready) begin
    hwrite_reg <= mem_ahb_hwrite;
    haddr_reg  <= mem_ahb_haddr;
  end
end

always @(posedge sys_clock) begin
  hrdata_ctrl <= (spi_rx_dma_en << 8) | (spi_rx_endian << 10) | (spi_ctrl_cpol << 24) | (spi_ctrl_cpha << 25);
  hrdata_data <= spi_rx_dma_en ? rx_fifo_q : spi_rx_reg;
end

always @(posedge sys_clock) begin
  if (!hreadyout_reg && !hwrite_reg) begin
    case (haddr_reg >> 2)
      0: hrdata_reg <= hrdata_ctrl;
      1: hrdata_reg <= hrdata_data;
    endcase
  end
end
assign mem_ahb_hrdata = hrdata_reg;
assign mem_ahb_hreadyout = hreadyout_reg;

endmodule

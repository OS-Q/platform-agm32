module ahb2qspi #(parameter ADDR_BITS = 24, DATA_BITS = 32) (
  input                      reset,
  input                      ahb_clock,
  input      [1:0]           ahb_htrans,
  input                      ahb_hready,
  input                      ahb_hwrite,
  input      [ADDR_BITS-1:0] ahb_haddr,
  input      [2:0]           ahb_hsize,
  input      [2:0]           ahb_hburst,
  input      [DATA_BITS-1:0] ahb_hwdata,
  output reg [DATA_BITS-1:0] ahb_hrdata,
  output reg                 ahb_hreadyout,
  output reg                 ahb_hresp,

  // sck must be synchronous to ahb_clock and must be divided by 2, 3, or 4 from ahb_clock
  input         sck,
  output        csn,
  output        sck_o,
  input         sck_i,
  input  [3:0]  data_i,
  output [3:0]  data_o,
  output [3:0]  data_oe
);

`ifndef SPI_QUAD
`define SPI_QUAD 1
`endif
`ifndef SPI_CMD_INIT
`define SPI_CMD_INIT (`SPI_QUAD ? 8'h35 :8'hf5)
`endif
`ifndef SPI_CMD_RD
`define SPI_CMD_RD (`SPI_QUAD ? 8'hEB : 8'h0B)
`endif
`ifndef SPI_CMD_WR
`define SPI_CMD_WR (`SPI_QUAD ? 8'h38 : 8'h02)
`endif

parameter AHB_INIT  = 3'h0;
parameter AHB_IDLE  = 3'h1;
parameter AHB_READ  = 3'h2;
parameter AHB_WRITE = 3'h3;

reg [1:0] curstate, nxtstate;
wire ahb_init  = curstate == AHB_INIT;
wire ahb_idle  = curstate == AHB_IDLE;
wire ahb_read  = curstate == AHB_READ;
wire ahb_write = curstate == AHB_WRITE;

wire ahb_resetn = !reset;
wire ahb_start = ahb_htrans == 2'b10 && ahb_hreadyout && !ahb_hresp;
wire ahb_seq   = ahb_htrans[0];
wire ahb_byte_valid;
reg ahb_running;
reg ahb_stop;
reg hwrite_reg;
reg [2:0] hsize_reg;
reg [ADDR_BITS-1:0] haddr_reg;
reg ahb_data_valid;
reg ahb_word_start;
reg ahb_word_done;

reg        sck_resetn;
reg        spi_start;
wire       spi_data_valid;
wire       spi_done_phase;
wire       spi_idle_phase;
wire [7:0] spi_rx_data;
wire [1:0] spi_data_idx;

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_hreadyout <= 1'b1;
  end else if (!hwrite_reg && ahb_word_done && ahb_byte_valid) begin
    ahb_hreadyout <= 1'b1;
  end else if (hwrite_reg && ahb_word_start) begin
    ahb_hreadyout <= 1'b1;
  end else if (ahb_htrans[1]) begin
    ahb_hreadyout <= 1'b0;
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_hresp <= 1'b0;
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_hrdata <= 0;
  end else if (ahb_read && ahb_byte_valid) begin
    case (hsize_reg)
      3'b000: ahb_hrdata <= {4{spi_rx_data}};
      3'b001: ahb_hrdata <= {2{spi_rx_data, ahb_hrdata[15:8]}};
      3'b010: ahb_hrdata <= {  spi_rx_data, ahb_hrdata[31:8] };
    endcase
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    hwrite_reg <= 1'b0;
    hsize_reg  <= 3'b0;
    haddr_reg  <= 0;
  end else if (ahb_start) begin
    hwrite_reg <= ahb_hwrite;
    hsize_reg  <= ahb_hsize;
    haddr_reg  <= ahb_haddr;
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_running <= 0;
  end else if (ahb_start) begin
    ahb_running <= 1;
  end else if ((ahb_read || ahb_write) && spi_start) begin
    ahb_running <= 0;
  end else if (ahb_hresp) begin
    ahb_running <= 0;
  end else if (ahb_hreadyout) begin
    ahb_running <= 0;
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_stop <= 0;
  end else if (spi_start) begin
    ahb_stop <= 0;
  end else if (ahb_hreadyout && !ahb_seq) begin
    ahb_stop <= 1;
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_data_valid <= 1'b0;
  end else if (ahb_read || ahb_write) begin
    ahb_data_valid <= spi_data_valid;
  end
end
assign ahb_byte_valid = !ahb_stop && spi_data_valid && !ahb_data_valid;

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_word_start <= 1'b0;
  end else if (ahb_byte_valid && (hsize_reg == 2 && spi_data_idx == 2 || hsize_reg == 1 && spi_data_idx[0] == 0 || hsize_reg == 0)) begin
    ahb_word_start <= 1'b1;;
  end else begin
    ahb_word_start <= 1'b0;
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    ahb_word_done <= 1'b0;
  end else if (spi_start) begin
    ahb_word_done <= 1'b0;
  end else if ((ahb_read || ahb_write) && (hsize_reg == 0 || hsize_reg == 1 && spi_data_idx[0] == 1 || hsize_reg == 2 && spi_data_idx == 3)) begin
    ahb_word_done <= 1'b1;
  end else begin
    ahb_word_done <= 1'b0;
  end
end

always @ (posedge ahb_clock or negedge ahb_resetn) begin
  if (!ahb_resetn) begin
    curstate <= AHB_INIT;
  end else begin
    curstate <= nxtstate;
  end
end

always @ (*) begin
  nxtstate = curstate;
  case (curstate)
      AHB_INIT:
        if (spi_done_phase)
          nxtstate = AHB_IDLE;
      AHB_IDLE:
        if (ahb_running)
          nxtstate = hwrite_reg ? AHB_WRITE : AHB_READ;
      AHB_READ:
        if (ahb_hreadyout && !ahb_seq)
          nxtstate = AHB_IDLE;
      AHB_WRITE:
        if (ahb_hreadyout && !ahb_seq)
          nxtstate = AHB_IDLE;
  endcase
end

QspiCtrl u_ctrl(
  .sck           (sck           ),
  .resetn        (sck_resetn    ),
  .haddr_reg     (haddr_reg     ),
  .hsize_reg     (hsize_reg     ),
  .hwrite_reg    (hwrite_reg    ),
  .ahb_hwdata    (ahb_hwdata    ),
  .ahb_seq       (ahb_seq       ),
  .ahb_running   (ahb_running   ),
  .ahb_read      (ahb_read      ),
  .ahb_write     (ahb_write     ),
  .spi_start     (spi_start     ),
  .spi_data_valid(spi_data_valid),
  .spi_done_phase(spi_done_phase),
  .spi_idle_phase(spi_idle_phase),
  .spi_rx_data   (spi_rx_data   ),
  .spi_data_idx  (spi_data_idx  ),
  .csn           (csn           ),
  .sck_o         (sck_o         ),
  .sck_i         (sck_i         ),
  .data_i        (data_i        ),
  .data_o        (data_o        ),
  .data_oe       (data_oe       )
);

always @ (posedge sck or posedge reset) begin
  if (reset) begin
    sck_resetn <= 1'b0;
  end else begin
    sck_resetn <= !reset;
  end
end

always @ (posedge sck or negedge sck_resetn) begin
  if (!sck_resetn) begin
    spi_start <= 0;
  end else if (spi_start || ahb_hresp) begin
    spi_start <= 0;
  end else if (spi_idle_phase && csn && ahb_running) begin
    spi_start <= 1;
  end
end

endmodule

module QspiCtrl(
  input         sck,
  input         resetn,
  input         spi_start,
  input  [23:0] haddr_reg,
  input  [2:0]  hsize_reg,
  input         hwrite_reg,
  input  [31:0] ahb_hwdata,
  input         ahb_seq,
  input         ahb_running,
  input         ahb_read,
  input         ahb_write,
  output        spi_data_valid, // spi_rx_data or spi_tx_data is valid when this is high
  output        spi_done_phase,
  output        spi_idle_phase,
  output [7:0]  spi_rx_data,
  output reg [1:0]  spi_data_idx,
  output        csn,
  output        sck_o,
  input         sck_i,
  input  [3:0]  data_i,
  output [3:0]  data_o,
  output [3:0]  data_oe
);
parameter QSPI_IDLE  = 3'h0;
parameter QSPI_CMD   = 3'h1;
parameter QSPI_ADDR  = 3'h2;
parameter QSPI_DUMMY = 3'h3;
parameter QSPI_DATA  = 3'h4;
parameter QSPI_DONE  = 3'h5;
reg [3:0] curstate, nxtstate;

reg [7:0]  spi_cmd;
reg        spi_cmd_quad;
reg        spi_init_done;
reg [23:0] spi_addr;
reg [2:0]  spi_dummy_bytes;
reg [4:0]  spi_data_bytes;
reg        spi_data_input;
reg        spi_data_next;
reg [7:0]  spi_tx_data;

wire spi_data_phase;
wire spi_byte_valid;

wire spi_fire = curstate == QSPI_IDLE && spi_start;
wire quad_cmd  = spi_cmd_quad;
wire quad_addr = `SPI_QUAD;
wire quad_data = `SPI_QUAD;

wire has_dummy_phase = spi_dummy_bytes != 0;
wire has_data_phase  = spi_data_bytes  != 0;
wire has_addr_phase  = spi_init_done;

wire [4:0] spi_done_count = 3;
wire [4:0] spi_dummy_count = (spi_dummy_bytes << (`SPI_QUAD ? 1 : 3)) - 1;
wire [4:0] spi_data_count = (spi_data_bytes << (`SPI_QUAD ? 1 : 3)) - 1;

reg [4:0] phase_counter;

always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    spi_cmd         <= 8'h0;
    spi_addr        <= 0;
    spi_data_bytes  <= 0;
    spi_dummy_bytes <= 0;
  end else if (spi_start) begin
    spi_cmd         <= !spi_init_done ? `SPI_CMD_INIT : hwrite_reg ? `SPI_CMD_WR : `SPI_CMD_RD;
    spi_addr        <= haddr_reg[23:0];
    spi_data_bytes  <= !spi_init_done ? 0 : (1 << hsize_reg);
    spi_dummy_bytes <= !spi_init_done ? 0 : hwrite_reg ? 0 : (`SPI_QUAD ? 3 : 1);
  end else if (curstate == QSPI_CMD) begin
    spi_cmd <= spi_cmd << (quad_cmd ? 4 : 1);
  end else if (curstate == QSPI_ADDR) begin
    spi_addr <= spi_addr << (quad_addr ? 4: 1);
  end
end

always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    spi_cmd_quad <= !`SPI_QUAD;
  end else begin
    spi_cmd_quad <= !spi_init_done ? !`SPI_QUAD : `SPI_QUAD;
  end
end

wire [1:0] spi_data_idx_nxt = spi_data_idx + 1;
always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    spi_tx_data <= 0;
  end else if (spi_start) begin
    spi_tx_data <= ahb_hwdata[haddr_reg[1:0]*8+:8];
  end else if (spi_data_phase && spi_byte_valid) begin
    spi_tx_data <= ahb_hwdata[spi_data_idx_nxt*8+:8];
  end else if (spi_data_phase && !spi_data_input) begin
    spi_tx_data <= spi_tx_data << (quad_data ? 4 : 1);
  end
end

always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    spi_init_done <= 1'b0;
  end else if (spi_done_phase) begin
    spi_init_done <= 1'b1;
  end
end

always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    spi_data_next <= 1'b0;
  end else if (spi_start) begin
    spi_data_next <= 1'b1;
  end else if (ahb_running || !ahb_seq &&
               (`SPI_QUAD ? (spi_data_phase && phase_counter == hsize_reg * 2 || hsize_reg == 0 && curstate == QSPI_ADDR) : 
                            (spi_data_phase && phase_counter == 0))) begin
    spi_data_next <= 1'b0;
  end
end

always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    spi_data_input <= 1'b1;
  end else if (spi_start) begin
    spi_data_input <= !hwrite_reg;
  end
end

always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    spi_data_idx <= 0;
  end else if (spi_start) begin
    if (ahb_read || ahb_write) begin
      spi_data_idx <= haddr_reg[1:0];
    end else begin
      spi_data_idx <= 0;
    end
  end else if (curstate == QSPI_DATA && spi_data_valid) begin
    spi_data_idx <= spi_data_idx + 1;
  end
end

always @ (posedge sck) begin
  if (spi_fire) begin
    phase_counter <= quad_cmd ? 1 : 7;
  end else if (phase_counter == 0) begin
    case (curstate)
      QSPI_CMD:   phase_counter <= has_addr_phase  ? (quad_addr ? 5 : 23) : has_dummy_phase ? spi_dummy_count : has_data_phase ? spi_data_count : spi_done_count;
      QSPI_ADDR:  phase_counter <= has_dummy_phase ? spi_dummy_count : has_data_phase ? spi_data_count : spi_done_count;
      QSPI_DUMMY: phase_counter <= has_data_phase  ? spi_data_count  : spi_done_count;
      QSPI_DATA:  phase_counter <= spi_data_next   ? spi_data_count  : spi_done_count;
    endcase
  end else if (curstate == QSPI_DATA && !spi_data_next && phase_counter == 0) begin
    phase_counter <= spi_done_count;
  end else begin
    phase_counter <= phase_counter - 1;
  end
end

assign spi_byte_valid = curstate == QSPI_DATA && (quad_data ? phase_counter[0] == 0 : phase_counter[2:0] == 0) ||
                        quad_data && curstate == QSPI_DONE ||
                        curstate == QSPI_ADDR && phase_counter == 0 && !spi_data_input && hsize_reg == 0;
reg [2:0] data_valid; // Tick 0: data launched from flash, Tick 1: data latched by read_reg, Tick 2: data latched by rx_data_reg
always @ (negedge sck or negedge resetn) begin
  if (!resetn) begin
    data_valid <= 3'b0;
  end else if (curstate == QSPI_CMD) begin
    data_valid <= 3'b0;
  end else if (has_data_phase && spi_data_input) begin
    data_valid <= { data_valid[1:0], spi_byte_valid };
  end else if (has_data_phase) begin
    data_valid <= {3{spi_byte_valid}};
  end
end
assign spi_data_valid = data_valid[2];

reg [3:0] read_reg; // Provide an extra cycle of latency to achieve higher fmax
always @ (negedge sck_i) begin
  read_reg <= data_i; // Do not put any condition here to avoid any clock xfer from sck to sck_i
end

reg [7:0] rx_data_reg;
always @ (negedge sck or negedge resetn) begin
  if (!resetn) begin
    rx_data_reg <= 0;
  end else if (curstate == QSPI_DATA || data_valid[0] || data_valid[1]) begin
    if (quad_data) begin
      rx_data_reg <= { rx_data_reg[3:0], read_reg };
    end else begin
      rx_data_reg <= { rx_data_reg[6:0], read_reg[1] };
    end
  end
end
assign spi_rx_data = rx_data_reg;

reg csn_reg;
always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    csn_reg <= 1;
  end else if (spi_fire) begin
    csn_reg <= 0;
  end else if (curstate == QSPI_DONE) begin
    csn_reg <= 1;
  end
end
assign csn = csn_reg;

reg sck_en;
always @ (negedge sck or negedge resetn) begin
  if (!resetn) begin
    sck_en <= 0;
  end else if (!csn_reg) begin
    sck_en <= curstate != QSPI_DONE;
  end
end

reg [3:0] data_o_reg;
reg [3:0] data_oe_reg;
always @ (negedge sck or negedge resetn) begin
  if (!resetn) begin
    data_o_reg  <= 0;
    data_oe_reg <= 0;
  end else if (curstate == QSPI_CMD) begin
    data_o_reg  <= quad_cmd ? spi_cmd[7:4] : {3'b1, spi_cmd[7]};
    data_oe_reg <= quad_cmd ? 4'b1111 : 4'b0001;
  end else if (curstate == QSPI_ADDR) begin
    data_o_reg  <= quad_addr ? spi_addr[23:20] : spi_addr[23];
    data_oe_reg <= quad_addr ? 4'b1111 : 4'b0001;
  end else if (curstate == QSPI_DATA && !spi_data_input) begin
    data_o_reg  <= quad_data ? spi_tx_data[7:4] : {3'b1, spi_tx_data[7]};
    data_oe_reg <= quad_data ? 4'b1111 : 4'b0001;
  end else begin
    data_oe_reg <= 0;
  end
end
assign data_o  = data_o_reg;
assign data_oe = data_oe_reg;
assign sck_o   = sck & sck_en;

assign spi_data_phase = curstate == QSPI_DATA;
assign spi_done_phase = curstate == QSPI_DONE && phase_counter == 0;
assign spi_idle_phase = curstate == QSPI_IDLE;

always @ (posedge sck or negedge resetn) begin
  if (!resetn) begin
    curstate <= QSPI_IDLE;
  end else begin
    curstate <= nxtstate;
  end
end

always @ (*) begin
  nxtstate = curstate;
  case (curstate)
    QSPI_IDLE: begin
      if (spi_start) begin
        nxtstate = QSPI_CMD;
      end
    end
    QSPI_CMD: begin
      if (phase_counter == 0) begin
        nxtstate = has_addr_phase ? QSPI_ADDR : has_dummy_phase ? QSPI_DUMMY : has_data_phase ? QSPI_DATA : QSPI_DONE;
      end
    end
    QSPI_ADDR: begin
      if (phase_counter == 0) begin
        nxtstate = has_dummy_phase ? QSPI_DUMMY : has_data_phase ? QSPI_DATA : QSPI_DONE;
      end
    end
    QSPI_DUMMY: begin
      if (phase_counter == 0) begin
        nxtstate = has_data_phase ? QSPI_DATA : QSPI_DONE;
      end
    end
    QSPI_DATA: begin
      if (!spi_data_next && phase_counter == 0) begin
        nxtstate = QSPI_DONE;
      end
    end
    QSPI_DONE: begin
      if (phase_counter == 0) begin
        nxtstate = QSPI_IDLE;
      end
    end
  endcase
end

endmodule

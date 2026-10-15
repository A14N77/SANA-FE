// riscv_soma_top.sv
//
// Top level of the RISC-V soma core. The host (SANA-FE via Verilator) only
// talks to this port list; everything inside is yours.
//
// Protocol:
//  1. Firmware load: host pulses rst_n low, then writes one word per cycle via
//     imem_we / imem_waddr (word address) / imem_wdata. imem survives reset.
//  2. Update: host drives io_* inputs, pulses rst_n low, pulses start for one
//     cycle. Core runs from pc=0 until firmware stores to MMIO_DONE, which
//     raises `done`. Host then samples io_potential_out / io_spike.
//
// Memory map (bit 31 set = MMIO, low byte selects register):
//  0x8000_0000 R current_in   0x8000_0004 R potential_in  0x8000_0008 R threshold
//  0x8000_000C R reset_value  0x8000_0010 R leak_shift    0x8000_0014 R bias
//  0x8000_0018 W potential_out 0x8000_001C W spike (bit0) 0x8000_0020 W done
// Everything below 0x8000_0000 is plain data RAM (dmem).
`timescale 1ns/1ps

module riscv_soma_top #(
    parameter int IMEM_WORDS = 256,
    parameter int DMEM_WORDS = 256
) (
    input  logic        clk,
    input  logic        rst_n,

    input  logic        imem_we,
    input  logic [31:0] imem_waddr,
    input  logic [31:0] imem_wdata,

    input  logic        start,
    output logic        done,

    input  logic signed [31:0] io_current_in,
    input  logic signed [31:0] io_potential_in,
    input  logic signed [31:0] io_threshold,
    input  logic signed [31:0] io_reset_value,
    input  logic        [31:0] io_leak_shift,
    input  logic signed [31:0] io_bias,
    output logic signed [31:0] io_potential_out,
    output logic               io_spike
);

    localparam int IMEM_AW = $clog2(IMEM_WORDS);

    logic [31:0] imem [0:IMEM_WORDS-1];
    logic [31:0] dmem [0:DMEM_WORDS-1];

    always_ff @(posedge clk) begin
        if (imem_we) imem[imem_waddr[IMEM_AW-1:0]] <= imem_wdata;
    end

    // MMIO: the core drives these for stores with addr[31] set
    logic        mmio_we;
    logic [31:0] mmio_addr;
    logic [31:0] mmio_wdata;

    function automatic logic [31:0] mmio_read(input logic [31:0] addr);
        case (addr[7:0])
            8'h00:   mmio_read = io_current_in;
            8'h04:   mmio_read = io_potential_in;
            8'h08:   mmio_read = io_threshold;
            8'h0C:   mmio_read = io_reset_value;
            8'h10:   mmio_read = io_leak_shift;
            8'h14:   mmio_read = io_bias;
            default: mmio_read = 32'h0;
        endcase
    endfunction

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            done             <= 1'b0;
            io_potential_out <= 32'sd0;
            io_spike         <= 1'b0;
        end else if (start) begin
            done     <= 1'b0;
            io_spike <= 1'b0;
        end else if (mmio_we) begin
            case (mmio_addr[7:0])
                8'h18:   io_potential_out <= mmio_wdata;
                8'h1C:   io_spike         <= mmio_wdata[0];
                8'h20:   done             <= 1'b1;
                default: ;
            endcase
        end
    end

    // ------------------------------------------------------------------
    // TODO(Alan): pipeline goes here. Drive mmio_we/mmio_addr/mmio_wdata,
    //  read mmio_read(addr), fetch from imem, use dmem for other loads/stores.
    // ------------------------------------------------------------------
    assign mmio_we    = 1'b0;
    assign mmio_addr  = 32'h0;
    assign mmio_wdata = 32'h0;

endmodule

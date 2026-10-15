// riscv_soma_top.sv
//
// Implement: a RISC-V core that, after `start`, executes imem from pc=0 and
// services loads/stores to 0x8000_00xx as the MMIO registers below (bit 31 set
// = MMIO), raising `done` when the firmware stores to 0x8000_0020.
//
// MMIO: 0x00 R current_in  0x04 R potential_in  0x08 R threshold  0x0C R reset
//       0x10 R leak_shift  0x14 R bias  0x18 W potential_out  0x1C W spike  0x20 W done
// Host loads firmware one word per cycle via imem_we/imem_waddr/imem_wdata.
`timescale 1ns/1ps

module riscv_soma_top #(
    parameter int IMEM_WORDS = 256
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

    // TODO(Alan)

endmodule

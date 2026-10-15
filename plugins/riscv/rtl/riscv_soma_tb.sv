// riscv_soma_tb.sv: drives riscv_soma_top the way sv_core.cpp does, no Verilator needed.
//   cd plugins/riscv
//   iverilog -g2012 -o /tmp/tb rtl/riscv_soma_top.sv rtl/riscv_soma_tb.sv && vvp /tmp/tb
// One update: v=1.0, i=1.0, threshold=2.0 -> expect spike=1, potential_out=0.
`timescale 1ns/1ps
module riscv_soma_tb;
    logic clk = 0; always #5 clk = ~clk;
    logic rst_n = 0, imem_we = 0, start = 0, done, io_spike;
    logic [31:0] imem_waddr = 0, imem_wdata = 0, io_leak_shift = 0;
    logic signed [31:0] io_current_in = 0, io_potential_in = 0, io_threshold = 0,
                        io_reset_value = 0, io_bias = 0, io_potential_out;
    riscv_soma_top dut (.*);

    logic [31:0] fw [0:255];
    int i, cycles;

    initial begin
        for (i = 0; i < 256; i++) fw[i] = 0;
        $readmemh("firmware/lif.hex", fw);
        repeat (2) @(negedge clk); rst_n = 1;
        for (i = 0; i < 256; i++) begin
            @(negedge clk); imem_we = 1; imem_waddr = i; imem_wdata = fw[i];
        end
        @(negedge clk); imem_we = 0;

        io_potential_in = 32'sd65536;   // 1.0 in Q16.16
        io_current_in   = 32'sd65536;   // 1.0
        io_threshold    = 32'sd131072;  // 2.0
        @(negedge clk); rst_n = 0; repeat (2) @(negedge clk); rst_n = 1;
        @(negedge clk); start = 1; @(negedge clk); start = 0;
        cycles = 1;
        while (!done && cycles < 1000) begin @(negedge clk); cycles++; end

        if (!done) $display("FAIL: no DONE after %0d cycles", cycles);
        else $display("%s: potential_out=%0d spike=%0d cycles=%0d (expect 0, 1)",
                      (io_potential_out == 0 && io_spike) ? "PASS" : "FAIL",
                      io_potential_out, io_spike, cycles);
        $finish;
    end
endmodule

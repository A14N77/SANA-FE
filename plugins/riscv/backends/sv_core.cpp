// Copyright (c) 2026 - The University of Texas at Austin
//  This work was produced under contract #2317831 to National Technology and
//  Engineering Solutions of Sandia, LLC which is under contract
//  No. DE-NA0003525 with the U.S. Department of Energy.
// sv_core.cpp
//
// Verilator backend: wraps the Verilated rtl/riscv_soma_top.sv. Only compiled
//  when CMake finds Verilator (RISCV_SOMA_HAVE_SV is then defined).
//
// Protocol with the RTL (see the port list in riscv_soma_top.sv):
//   load_firmware: pulse rst_n low, then one imem_we write per word.
//   run:           drive io_* inputs, pulse rst_n low (imem survives reset),
//                  pulse start for one cycle, clock until done, read io_*
//                  outputs. cycles = clocks from start until done is seen.
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Vriscv_soma_top.h"
#include "verilated.h"

#include "riscv_core.hpp"

namespace
{
class SvCore final : public riscv::Core
{
public:
    SvCore()
            : context(std::make_unique<VerilatedContext>())
            , top(std::make_unique<Vriscv_soma_top>(context.get()))
    {
        top->clk = 0;
        top->rst_n = 0;
        top->start = 0;
        top->imem_we = 0;
        top->imem_waddr = 0;
        top->imem_wdata = 0;
        top->eval();
    }

    ~SvCore() override
    {
        top->final();
    }

    std::string name() const override
    {
        return "sv";
    }

    void load_firmware(const std::vector<uint32_t> &words) override
    {
        reset_core();
        for (size_t i = 0; i < words.size(); ++i)
        {
            top->imem_we = 1;
            top->imem_waddr = static_cast<uint32_t>(i);
            top->imem_wdata = words[i];
            tick();
        }
        top->imem_we = 0;
        tick();
    }

    riscv::NeuronOutputs run(const riscv::NeuronInputs &in,
            const uint32_t max_cycles) override
    {
        top->io_current_in = static_cast<uint32_t>(in.current_in);
        top->io_potential_in = static_cast<uint32_t>(in.potential_in);
        top->io_threshold = static_cast<uint32_t>(in.threshold);
        top->io_reset_value = static_cast<uint32_t>(in.reset_value);
        top->io_leak_shift = static_cast<uint32_t>(in.leak_shift);
        top->io_bias = static_cast<uint32_t>(in.bias);

        reset_core();

        riscv::NeuronOutputs out;
        top->start = 1;
        tick();
        top->start = 0;
        out.cycles = 1;
        while ((top->done == 0) && (out.cycles < max_cycles))
        {
            tick();
            ++out.cycles;
        }
        out.completed = (top->done != 0);
        out.potential_out = static_cast<int32_t>(top->io_potential_out);
        out.spike = (top->io_spike != 0);
        return out;
    }

private:
    std::unique_ptr<VerilatedContext> context;
    std::unique_ptr<Vriscv_soma_top> top;

    void tick()
    {
        top->clk = 0;
        top->eval();
        context->timeInc(1);
        top->clk = 1;
        top->eval();
        context->timeInc(1);
    }

    void reset_core()
    {
        top->start = 0;
        top->imem_we = 0;
        top->rst_n = 0;
        tick();
        tick();
        top->rst_n = 1;
        tick();
    }
};
} // namespace

std::unique_ptr<riscv::Core> riscv::make_sv_core()
{
    return std::make_unique<SvCore>();
}

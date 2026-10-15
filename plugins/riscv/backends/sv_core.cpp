// sv_core.cpp: Verilator wrapper for rtl/riscv_soma_top.sv
#include <cstdint>
#include <memory>
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
        reset();
        for (size_t i = 0; i < words.size(); ++i)
        {
            top->imem_we = 1;
            top->imem_waddr = static_cast<uint32_t>(i);
            top->imem_wdata = words[i];
            tick();
        }
        top->imem_we = 0;
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
        reset();

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
        top->clk = 1;
        top->eval();
        context->timeInc(1);
    }

    void reset()
    {
        top->start = 0;
        top->imem_we = 0;
        top->rst_n = 0;
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

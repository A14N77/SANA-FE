// reference_core.cpp: golden model, no instruction execution
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "riscv_core.hpp"

namespace
{
class ReferenceCore final : public riscv::Core
{
public:
    std::string name() const override
    {
        return "reference";
    }

    void load_firmware(const std::vector<uint32_t> & /*words*/) override {}

    riscv::NeuronOutputs run(const riscv::NeuronInputs &in,
            const uint32_t max_cycles) override
    {
        riscv::NeuronOutputs out = riscv::lif_reference(in);
        out.cycles = 18U;
        out.completed = (out.cycles <= max_cycles);
        return out;
    }
};
} // namespace

std::unique_ptr<riscv::Core> riscv::make_reference_core()
{
    return std::make_unique<ReferenceCore>();
}

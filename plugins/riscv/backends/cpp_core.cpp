// cpp_core.cpp
//
// Implement run(): reset the core, expose `in` through the MMIO map in
// riscv_core.hpp, execute `imem` from pc=0 until the firmware stores to
// mmio_done, then return potential_out, spike and the cycle count.
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "riscv_core.hpp"

namespace
{
class CppCore final : public riscv::Core
{
public:
    std::string name() const override
    {
        return "cpp";
    }

    void load_firmware(const std::vector<uint32_t> &words) override
    {
        imem = words;
    }

    riscv::NeuronOutputs run(const riscv::NeuronInputs & /*in*/,
            const uint32_t /*max_cycles*/) override
    {
        // TODO(Alan)
        throw std::logic_error("CppCore::run() not implemented");
    }

private:
    std::vector<uint32_t> imem;
};
} // namespace

std::unique_ptr<riscv::Core> riscv::make_cpp_core()
{
    return std::make_unique<CppCore>();
}

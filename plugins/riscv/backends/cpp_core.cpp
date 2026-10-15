// Copyright (c) 2026 - The University of Texas at Austin
//  This work was produced under contract #2317831 to National Technology and
//  Engineering Solutions of Sandia, LLC which is under contract
//  No. DE-NA0003525 with the U.S. Department of Energy.
// cpp_core.cpp
//
// C++ RISC-V backend. Implement run(): reset, present the inputs (memory map
//  in riscv_core.hpp), execute `imem` from pc=0 until the firmware writes
//  mmio_done, and report cycles.
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
        throw std::logic_error("CppCore::run() not implemented "
                               "(plugins/riscv/backends/cpp_core.cpp)");
    }

private:
    std::vector<uint32_t> imem;
};
} // namespace

std::unique_ptr<riscv::Core> riscv::make_cpp_core()
{
    return std::make_unique<CppCore>();
}

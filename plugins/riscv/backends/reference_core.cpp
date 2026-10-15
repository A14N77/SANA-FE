// Copyright (c) 2026 - The University of Texas at Austin
//  This work was produced under contract #2317831 to National Technology and
//  Engineering Solutions of Sandia, LLC which is under contract
//  No. DE-NA0003525 with the U.S. Department of Energy.
// reference_core.cpp
//
// Golden-model backend. It does not execute instructions at all: it computes
//  the LIF update directly with the integer semantics the firmware is meant to
//  implement, and charges a fixed nominal cycle count. Use it to:
//   - run the plugin end-to-end before any real core exists, and
//   - bit-match the cpp and sv backends (same inputs must give the same
//     potential_out and spike).
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "riscv_core.hpp"

namespace
{
// Roughly the dynamic instruction count of firmware/lif.hex on a core with no
//  stalls. Purely nominal; a real backend reports what it measured.
constexpr uint32_t reference_cycles_per_update = 18U;

class ReferenceCore final : public riscv::Core
{
public:
    std::string name() const override
    {
        return "reference";
    }

    void load_firmware(const std::vector<uint32_t> & /*words*/) override
    {
        // Nothing to load; behaviour is hard-coded
    }

    riscv::NeuronOutputs run(const riscv::NeuronInputs &in,
            const uint32_t max_cycles) override
    {
        riscv::NeuronOutputs out = riscv::lif_reference(in);
        out.cycles = reference_cycles_per_update;
        out.completed = (out.cycles <= max_cycles);
        return out;
    }
};
} // namespace

std::unique_ptr<riscv::Core> riscv::make_reference_core()
{
    return std::make_unique<ReferenceCore>();
}

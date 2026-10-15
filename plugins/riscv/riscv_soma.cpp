// Copyright (c) 2026 - The University of Texas at Austin
//  This work was produced under contract #2317831 to National Technology and
//  Engineering Solutions of Sandia, LLC which is under contract
//  No. DE-NA0003525 with the U.S. Department of Energy.
// riscv_soma.cpp
//
// SANA-FE soma plugin that models the soma pipeline unit as a small RISC-V
//  core running firmware. Each neuron update becomes one firmware run:
//  synaptic current and neuron state go in through memory-mapped registers,
//  the core runs until it signals DONE, and the new potential plus spike flag
//  come back out. Cycle count becomes latency and energy.
//
// Architecture attributes (soma section of the arch YAML):
//   model: riscv_soma
//   plugin: <path to libriscv_soma.so>
//   backend: reference | cpp | sv        (default: reference)
//   firmware: lif.hex                     (resolved against cwd, the plugin
//                                          directory, then plugins/riscv/firmware)
//   clock_period: 1.0e-9                  (seconds per core cycle)
//   energy_per_cycle: 0.0                 (joules per core cycle)
//   max_cycles: 10000                     (give up if DONE never arrives)
// Do NOT also set energy_*_neuron / latency_*_neuron on this unit: the plugin
//  simulates energy and latency itself and SANA-FE rejects both being set.
//
// Neuron attributes (SNN description):
//   threshold, reset, bias   (floats, converted to Q16.16)
//   leak_shift               (int, potential -= potential >> leak_shift)
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "attribute.hpp"
#include "mapped.hpp"
#include "pipeline.hpp"
#include "print.hpp"

#include "riscv_core.hpp"

class RiscvSoma : public sanafe::SomaUnit
{
public:
    static inline const std::unordered_map<std::string, std::string>
            riscv_soma_attributes{
                    {"backend",
                            "(str) Core backend: reference, cpp or sv."},
                    {"firmware", "(str) RV32I hex image run on every update."},
                    {"clock_period", "(float) Seconds per core clock cycle."},
                    {"energy_per_cycle", "(float) Joules per core clock cycle."},
                    {"max_cycles",
                            "(int) Abort if the core does not finish in time."},
                    {"threshold", "(float) Spike when potential >= threshold."},
                    {"reset", "(float) Potential after a spike."},
                    {"bias", "(float) Added to the potential every update."},
                    {"leak_shift",
                            "(int) potential -= potential >> leak_shift (0 = "
                            "no leak)."},
            };

    RiscvSoma()
    {
        register_attributes(riscv_soma_attributes);
    }
    RiscvSoma(const RiscvSoma &) = delete;
    RiscvSoma(RiscvSoma &&) = delete;
    RiscvSoma &operator=(const RiscvSoma &) = delete;
    RiscvSoma &operator=(RiscvSoma &&) = delete;
    ~RiscvSoma() override = default;

    void set_attribute_hw(const std::string &attribute_name,
            const sanafe::ModelAttribute &param) override
    {
        if (attribute_name == "backend")
        {
            backend = static_cast<std::string>(param);
        }
        else if (attribute_name == "firmware")
        {
            firmware_name = static_cast<std::string>(param);
        }
        else if (attribute_name == "clock_period")
        {
            clock_period = static_cast<double>(param);
        }
        else if (attribute_name == "energy_per_cycle")
        {
            energy_per_cycle = static_cast<double>(param);
        }
        else if (attribute_name == "max_cycles")
        {
            max_cycles = static_cast<uint32_t>(static_cast<int>(param));
        }
    }

    void set_attribute_neuron(const size_t neuron_address,
            const std::string &attribute_name,
            const sanafe::ModelAttribute &param) override
    {
        NeuronState &n = neuron(neuron_address);
        if (attribute_name == "threshold")
        {
            n.threshold = riscv::to_fixed(static_cast<double>(param));
        }
        else if (attribute_name == "reset")
        {
            n.reset_value = riscv::to_fixed(static_cast<double>(param));
        }
        else if (attribute_name == "bias")
        {
            n.bias = riscv::to_fixed(static_cast<double>(param));
        }
        else if (attribute_name == "leak_shift")
        {
            n.leak_shift = static_cast<int>(param);
            if ((n.leak_shift < 0) || (n.leak_shift > 31))
            {
                throw std::invalid_argument(
                        "riscv_soma: leak_shift must be in 0..31");
            }
        }
    }

    void reset() override
    {
        for (NeuronState &n : neurons)
        {
            n.potential = 0;
        }
    }

    double get_potential(const size_t neuron_address) override
    {
        return riscv::from_fixed(neuron(neuron_address).potential);
    }

    sanafe::PipelineResult update(const size_t neuron_address,
            const std::optional<double> current_in,
            const long int /*timestep*/) override
    {
        ensure_core();
        NeuronState &n = neuron(neuron_address);

        riscv::NeuronInputs in;
        in.current_in = riscv::to_fixed(current_in.value_or(0.0));
        in.potential_in = n.potential;
        in.threshold = n.threshold;
        in.reset_value = n.reset_value;
        in.leak_shift = n.leak_shift;
        in.bias = n.bias;

        const riscv::NeuronOutputs out = core->run(in, max_cycles);
        if (!out.completed)
        {
            INFO("Error: riscv_soma (%s backend) did not signal DONE within "
                 "%u cycles for neuron %zu\n",
                    core->name().c_str(), max_cycles, neuron_address);
            throw std::runtime_error(
                    "riscv_soma: core did not finish within max_cycles");
        }
        n.potential = out.potential_out;
        TRACE1(PLUGINS, "riscv_soma nid:%zu i:%d v:%d spike:%d cycles:%u\n",
                neuron_address, in.current_in, out.potential_out,
                static_cast<int>(out.spike), out.cycles);

        sanafe::PipelineResult result;
        result.status = out.spike ? sanafe::fired : sanafe::updated;
        result.latency = static_cast<double>(out.cycles) * clock_period;
        result.energy = static_cast<double>(out.cycles) * energy_per_cycle;
        return result;
    }

private:
    struct NeuronState
    {
        int32_t potential{0};
        int32_t threshold{riscv::to_fixed(1.0)};
        int32_t reset_value{0};
        int32_t leak_shift{0};
        int32_t bias{0};
    };

    std::vector<NeuronState> neurons;
    std::unique_ptr<riscv::Core> core;
    std::string backend{"reference"};
    std::string firmware_name{"lif.hex"};
    double clock_period{1.0e-9};
    double energy_per_cycle{0.0};
    uint32_t max_cycles{10000U};

    NeuronState &neuron(const size_t neuron_address)
    {
        if (neuron_address >= neurons.size())
        {
            neurons.resize(neuron_address + 1);
        }
        return neurons[neuron_address];
    }

    // The core is created on first use so that all h/w attributes have been
    //  applied, regardless of the order SANA-FE sets them in.
    void ensure_core()
    {
        if (core)
        {
            return;
        }
        core = riscv::make_core(backend);
        const std::filesystem::path firmware_path = resolve_firmware();
        INFO("riscv_soma: backend=%s firmware=%s\n", core->name().c_str(),
                firmware_path.string().c_str());
        core->load_firmware(riscv::read_hex_file(firmware_path));
    }

    std::filesystem::path resolve_firmware() const
    {
        std::vector<std::filesystem::path> candidates;
        candidates.emplace_back(firmware_name);
        if (plugin_lib.has_value())
        {
            candidates.push_back(plugin_lib->parent_path() / firmware_name);
        }
#ifdef RISCV_PLUGIN_SOURCE_DIR
        candidates.push_back(std::filesystem::path(RISCV_PLUGIN_SOURCE_DIR) /
                "firmware" / firmware_name);
#endif
        for (const auto &candidate : candidates)
        {
            if (std::filesystem::is_regular_file(candidate))
            {
                return candidate;
            }
        }
        std::string error = "riscv_soma: firmware '" + firmware_name +
                "' not found. Tried:";
        for (const auto &candidate : candidates)
        {
            error += "\n  " + candidate.string();
        }
        INFO("Error: %s\n", error.c_str());
        throw std::runtime_error(error);
    }
};

// Class factory: SANA-FE looks up create_<model name>
extern "C" sanafe::PipelineUnit *create_riscv_soma()
{
    TRACE1(PLUGINS, "Creating riscv_soma instance\n");
    return static_cast<sanafe::PipelineUnit *>(new RiscvSoma());
}

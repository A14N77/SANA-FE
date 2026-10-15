// riscv_soma.cpp: SANA-FE soma unit backed by a RISC-V core
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "attribute.hpp"
#include "mapped.hpp"
#include "pipeline.hpp"
#include "print.hpp"

#include "riscv_core.hpp"

class RiscvSoma : public sanafe::SomaUnit
{
public:
    RiscvSoma()
    {
        register_attributes(std::set<std::string>{"backend", "firmware",
                "clock_period", "energy_per_cycle", "max_cycles", "threshold",
                "reset", "bias", "leak_shift"});
    }

    void set_attribute_hw(const std::string &name,
            const sanafe::ModelAttribute &param) override
    {
        if (name == "backend")
        {
            backend = static_cast<std::string>(param);
        }
        else if (name == "firmware")
        {
            firmware = static_cast<std::string>(param);
        }
        else if (name == "clock_period")
        {
            clock_period = static_cast<double>(param);
        }
        else if (name == "energy_per_cycle")
        {
            energy_per_cycle = static_cast<double>(param);
        }
        else if (name == "max_cycles")
        {
            max_cycles = static_cast<uint32_t>(static_cast<int>(param));
        }
    }

    void set_attribute_neuron(const size_t address, const std::string &name,
            const sanafe::ModelAttribute &param) override
    {
        Neuron &n = neuron(address);
        if (name == "threshold")
        {
            n.threshold = riscv::to_fixed(static_cast<double>(param));
        }
        else if (name == "reset")
        {
            n.reset_value = riscv::to_fixed(static_cast<double>(param));
        }
        else if (name == "bias")
        {
            n.bias = riscv::to_fixed(static_cast<double>(param));
        }
        else if (name == "leak_shift")
        {
            n.leak_shift = static_cast<int>(param);
        }
    }

    void reset() override
    {
        for (Neuron &n : neurons)
        {
            n.potential = 0;
        }
    }

    double get_potential(const size_t address) override
    {
        return riscv::from_fixed(neuron(address).potential);
    }

    sanafe::PipelineResult update(const size_t address,
            const std::optional<double> current_in,
            const long int /*timestep*/) override
    {
        if (!core)
        {
            core = riscv::make_core(backend);
            core->load_firmware(riscv::read_hex_file(firmware_path()));
        }
        Neuron &n = neuron(address);

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
            throw std::runtime_error("riscv_soma: no DONE within max_cycles");
        }
        n.potential = out.potential_out;

        sanafe::PipelineResult result;
        result.status = out.spike ? sanafe::fired : sanafe::updated;
        result.latency = out.cycles * clock_period;
        result.energy = out.cycles * energy_per_cycle;
        return result;
    }

private:
    struct Neuron
    {
        int32_t potential{0};
        int32_t threshold{riscv::to_fixed(1.0)};
        int32_t reset_value{0};
        int32_t leak_shift{0};
        int32_t bias{0};
    };

    std::vector<Neuron> neurons;
    std::unique_ptr<riscv::Core> core;
    std::string backend{"reference"};
    std::string firmware{"lif.hex"};
    double clock_period{1.0e-9};
    double energy_per_cycle{0.0};
    uint32_t max_cycles{10000U};

    Neuron &neuron(const size_t address)
    {
        if (address >= neurons.size())
        {
            neurons.resize(address + 1);
        }
        return neurons[address];
    }

    // As given, else next to the plugin library
    std::filesystem::path firmware_path() const
    {
        if (std::filesystem::exists(firmware) || !plugin_lib.has_value())
        {
            return firmware;
        }
        return plugin_lib->parent_path() / firmware;
    }
};

extern "C" sanafe::PipelineUnit *create_riscv_soma()
{
    return new RiscvSoma();
}

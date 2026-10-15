// riscv_core.hpp
#ifndef RISCV_CORE_HPP
#define RISCV_CORE_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace riscv
{
// All neuron values are signed Q16.16; leak_shift is a plain int
int32_t to_fixed(double value);
double from_fixed(int32_t value);

constexpr uint32_t mmio_base = 0x80000000U;
enum MmioOffset : uint32_t
{
    mmio_current_in = 0x00,
    mmio_potential_in = 0x04,
    mmio_threshold = 0x08,
    mmio_reset_value = 0x0C,
    mmio_leak_shift = 0x10,
    mmio_bias = 0x14,
    mmio_potential_out = 0x18,
    mmio_spike = 0x1C,
    mmio_done = 0x20,
};

struct NeuronInputs
{
    int32_t current_in{0};
    int32_t potential_in{0};
    int32_t threshold{0};
    int32_t reset_value{0};
    int32_t leak_shift{0};
    int32_t bias{0};
};

struct NeuronOutputs
{
    int32_t potential_out{0};
    bool spike{false};
    uint32_t cycles{0};
    bool completed{false};
};

class Core
{
public:
    virtual ~Core() = default;
    virtual std::string name() const = 0;
    virtual void load_firmware(const std::vector<uint32_t> &words) = 0;
    virtual NeuronOutputs run(const NeuronInputs &in, uint32_t max_cycles) = 0;
};

NeuronOutputs lif_reference(const NeuronInputs &in);
std::vector<uint32_t> read_hex_file(const std::filesystem::path &path);

std::unique_ptr<Core> make_core(const std::string &backend);
std::unique_ptr<Core> make_reference_core();
std::unique_ptr<Core> make_cpp_core();
std::unique_ptr<Core> make_sv_core();
} // namespace riscv

#endif

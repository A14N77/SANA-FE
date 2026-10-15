// Copyright (c) 2026 - The University of Texas at Austin
//  This work was produced under contract #2317831 to National Technology and
//  Engineering Solutions of Sandia, LLC which is under contract
//  No. DE-NA0003525 with the U.S. Department of Energy.
// riscv_core.hpp
//
// Backend-agnostic contract between the SANA-FE soma plugin (riscv_soma.cpp)
//  and a RISC-V core that executes one neuron update per call.
//
// Every backend (reference / cpp / sv) sees the same thing:
//   1. A firmware image (RV32I words) loaded once at start-up.
//   2. Per update: the neuron's inputs appear in memory-mapped I/O registers,
//      the core is reset and released, it runs until it writes the DONE
//      register (or hits max_cycles), and the outputs are read back.
//   3. The number of clock cycles consumed is reported back so the plugin can
//      turn it into latency and energy.
//
// Fixed-point format: every neuron quantity (potential, current, threshold,
//  reset value, bias) is a signed 32-bit Q16.16 number. leak_shift is a plain
//  integer: the core computes potential -= potential >> leak_shift when
//  leak_shift != 0 (valid range 1..31; 0 disables the leak).
#ifndef RISCV_CORE_HPP
#define RISCV_CORE_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace riscv
{
// ---------------------------------------------------------------------------
// Fixed point helpers
// ---------------------------------------------------------------------------
constexpr int fixed_frac_bits = 16;
constexpr double fixed_one = 65536.0; // 2^16
int32_t to_fixed(double value); // Saturating conversion
double from_fixed(int32_t value);

// ---------------------------------------------------------------------------
// Memory map, shared by the firmware, the SV top-level and every backend
// ---------------------------------------------------------------------------
// Addresses with bit 31 set are MMIO. Word offsets from mmio_base:
constexpr uint32_t mmio_base = 0x80000000U;
enum MmioOffset : uint32_t
{
    mmio_current_in = 0x00, // R  Q16.16 synaptic current this timestep
    mmio_potential_in = 0x04, // R  Q16.16 membrane potential from last timestep
    mmio_threshold = 0x08, // R  Q16.16
    mmio_reset_value = 0x0C, // R  Q16.16 potential after a spike
    mmio_leak_shift = 0x10, // R  int    potential -= potential >> leak_shift
    mmio_bias = 0x14, // R  Q16.16 added every update
    mmio_potential_out = 0x18, // W  Q16.16 new membrane potential
    mmio_spike = 0x1C, // W  bit 0 = neuron fired
    mmio_done = 0x20, // W  any value: update complete, core halts
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
    uint32_t cycles{0}; // Clock cycles consumed by this update
    bool completed{false}; // False if the core never wrote DONE
};

// ---------------------------------------------------------------------------
// Core interface
// ---------------------------------------------------------------------------
class Core
{
public:
    Core() = default;
    Core(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(const Core &) = delete;
    Core &operator=(Core &&) = delete;
    virtual ~Core() = default;

    virtual std::string name() const = 0;
    // Load an RV32I program; word i goes to byte address 4*i. Called once.
    virtual void load_firmware(const std::vector<uint32_t> &words) = 0;
    // Reset the core, present the inputs, run until DONE or max_cycles.
    virtual NeuronOutputs run(const NeuronInputs &in, uint32_t max_cycles) = 0;
};

// Golden integer semantics of one LIF update, identical to what firmware/lif.hex
//  computes. Use this to bit-match any backend.
NeuronOutputs lif_reference(const NeuronInputs &in);

// Parse a hex file: one 32-bit word per line, 8 hex digits, '#' or '//'
//  comments allowed.
std::vector<uint32_t> read_hex_file(const std::filesystem::path &path);

// Backend factories. make_core() dispatches on "reference", "cpp" or "sv".
std::unique_ptr<Core> make_core(const std::string &backend);
std::unique_ptr<Core> make_reference_core();
std::unique_ptr<Core> make_cpp_core();
std::unique_ptr<Core> make_sv_core(); // Throws unless built with Verilator
} // namespace riscv

#endif

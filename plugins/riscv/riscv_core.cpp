// riscv_core.cpp
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "riscv_core.hpp"

int32_t riscv::to_fixed(const double value)
{
    const double scaled = std::round(value * 65536.0);
    if (scaled <= std::numeric_limits<int32_t>::min())
    {
        return std::numeric_limits<int32_t>::min();
    }
    if (scaled >= std::numeric_limits<int32_t>::max())
    {
        return std::numeric_limits<int32_t>::max();
    }
    return static_cast<int32_t>(scaled);
}

double riscv::from_fixed(const int32_t value)
{
    return static_cast<double>(value) / 65536.0;
}

riscv::NeuronOutputs riscv::lif_reference(const NeuronInputs &in)
{
    uint32_t v = static_cast<uint32_t>(in.potential_in);
    if (in.leak_shift != 0)
    {
        const auto shift = static_cast<uint32_t>(in.leak_shift) & 31U;
        v -= static_cast<uint32_t>(static_cast<int32_t>(v) >> shift);
    }
    v += static_cast<uint32_t>(in.current_in);
    v += static_cast<uint32_t>(in.bias);

    NeuronOutputs out;
    out.spike = (static_cast<int32_t>(v) >= in.threshold);
    out.potential_out = out.spike ? in.reset_value : static_cast<int32_t>(v);
    out.completed = true;
    return out;
}

std::vector<uint32_t> riscv::read_hex_file(const std::filesystem::path &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("riscv: cannot open " + path.string());
    }
    std::vector<uint32_t> words;
    std::string line;
    while (std::getline(file, line))
    {
        line = line.substr(0, line.find('#'));
        const size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos)
        {
            continue;
        }
        words.push_back(static_cast<uint32_t>(
                std::stoul(line.substr(first), nullptr, 16)));
    }
    if (words.empty())
    {
        throw std::runtime_error("riscv: empty firmware " + path.string());
    }
    return words;
}

std::unique_ptr<riscv::Core> riscv::make_core(const std::string &backend)
{
    if (backend == "reference")
    {
        return make_reference_core();
    }
    if (backend == "cpp")
    {
        return make_cpp_core();
    }
    if (backend == "sv")
    {
        return make_sv_core();
    }
    throw std::runtime_error("riscv: unknown backend " + backend);
}

#ifndef RISCV_SOMA_HAVE_SV
std::unique_ptr<riscv::Core> riscv::make_sv_core()
{
    throw std::runtime_error("riscv: built without Verilator");
}
#endif

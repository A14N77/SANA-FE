// Copyright (c) 2026 - The University of Texas at Austin
//  This work was produced under contract #2317831 to National Technology and
//  Engineering Solutions of Sandia, LLC which is under contract
//  No. DE-NA0003525 with the U.S. Department of Energy.
// riscv_core.cpp
//
// Shared helpers: fixed-point conversion, hex loader, golden LIF semantics and
//  the backend factory.
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
    const double scaled = std::round(value * fixed_one);
    constexpr double lo = std::numeric_limits<int32_t>::min();
    constexpr double hi = std::numeric_limits<int32_t>::max();
    if (scaled <= lo)
    {
        return std::numeric_limits<int32_t>::min();
    }
    if (scaled >= hi)
    {
        return std::numeric_limits<int32_t>::max();
    }
    return static_cast<int32_t>(scaled);
}

double riscv::from_fixed(const int32_t value)
{
    return static_cast<double>(value) / fixed_one;
}

riscv::NeuronOutputs riscv::lif_reference(const NeuronInputs &in)
{
    // All arithmetic is done in uint32_t so that overflow wraps exactly like a
    //  32-bit datapath, then reinterpreted as signed for the comparisons.
    uint32_t v = static_cast<uint32_t>(in.potential_in);
    if (in.leak_shift != 0)
    {
        // RISC-V SRA only looks at the low 5 bits of the shift amount
        const auto shift = static_cast<uint32_t>(in.leak_shift) & 31U;
        const int32_t leak = static_cast<int32_t>(v) >> shift;
        v -= static_cast<uint32_t>(leak);
    }
    v += static_cast<uint32_t>(in.current_in);
    v += static_cast<uint32_t>(in.bias);

    NeuronOutputs out;
    out.spike = (static_cast<int32_t>(v) >= in.threshold);
    if (out.spike)
    {
        v = static_cast<uint32_t>(in.reset_value);
    }
    out.potential_out = static_cast<int32_t>(v);
    out.completed = true;
    out.cycles = 0; // The caller decides what a "cycle" costs
    return out;
}

std::vector<uint32_t> riscv::read_hex_file(const std::filesystem::path &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error(
                "riscv: could not open firmware file: " + path.string());
    }

    std::vector<uint32_t> words;
    std::string line;
    size_t line_number = 0;
    while (std::getline(file, line))
    {
        ++line_number;
        // Strip comments
        for (const char *marker : {"#", "//"})
        {
            const size_t pos = line.find(marker);
            if (pos != std::string::npos)
            {
                line.erase(pos);
            }
        }
        // Trim whitespace
        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            continue;
        }
        const size_t last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, last - first + 1);
        if (line.rfind("@", 0) == 0)
        {
            // $readmemh-style address directive is not supported; keep images
            //  contiguous from address 0
            throw std::runtime_error("riscv: '@' address directives are not "
                                     "supported in firmware hex (line " +
                    std::to_string(line_number) + ")");
        }
        try
        {
            words.push_back(static_cast<uint32_t>(std::stoul(line, nullptr, 16)));
        }
        catch (const std::exception &)
        {
            throw std::runtime_error("riscv: bad hex word '" + line +
                    "' in " + path.string() + " line " +
                    std::to_string(line_number));
        }
    }
    if (words.empty())
    {
        throw std::runtime_error(
                "riscv: firmware file is empty: " + path.string());
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
    throw std::runtime_error("riscv: unknown backend '" + backend +
            "' (expected reference, cpp or sv)");
}

#ifndef RISCV_SOMA_HAVE_SV
std::unique_ptr<riscv::Core> riscv::make_sv_core()
{
    throw std::runtime_error(
            "riscv: this plugin was built without Verilator, so the 'sv' "
            "backend is unavailable. Install Verilator, re-run cmake, and "
            "check for 'riscv_soma: Verilator ... found' in the output.");
}
#endif

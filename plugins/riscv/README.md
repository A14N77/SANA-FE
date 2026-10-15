# riscv_soma

SANA-FE soma plugin that models the soma stage as a RISC-V core running
firmware. Per neuron update: inputs go in through MMIO, the core runs until it
writes DONE, potential/spike come out, cycle count becomes latency and energy.

- `riscv_soma.cpp` plugin glue (`create_riscv_soma`)
- `riscv_core.hpp` contract: Q16.16 fixed point, memory map, `Core` interface
- `backends/reference_core.cpp` golden model, no instruction execution, works now
- `backends/cpp_core.cpp` **TODO**: C++ core
- `backends/sv_core.cpp` Verilator wrapper, built only if Verilator is found
- `rtl/riscv_soma_top.sv` **TODO**: ports and MMIO are fixed, pipeline is yours
- `firmware/lif.hex` LIF update in RV32I (instructions in the comments)
- `arch/riscv_chip.yaml`, `snn/riscv_test.yaml` example

Build with the normal CMake build; the library lands at
`<build>/plugins/riscv/libriscv_soma.so` with `lif.hex` beside it. Run from
the build directory:

```bash
./sim -s -v ../plugins/riscv/arch/riscv_chip.yaml ../plugins/riscv/snn/riscv_test.yaml 10
```

Soma attributes: `backend` (reference|cpp|sv), `firmware`, `clock_period`,
`energy_per_cycle`, `max_cycles`. Do not also set `energy_*_neuron` /
`latency_*_neuron` on this unit. Neuron attributes: `threshold`, `reset`,
`bias`, `leak_shift`.

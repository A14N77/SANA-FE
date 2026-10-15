cmake_minimum_required(VERSION 3.16)
project(RiscvSomaPlugin CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(RISCV_SOMA_ENABLE_SV "Build the Verilated SystemVerilog backend if Verilator is found" ON)

set(RISCV_SOMA_SOURCES
    riscv_soma.cpp
    riscv_core.cpp
    backends/reference_core.cpp
    backends/cpp_core.cpp
)

add_library(riscv_soma SHARED ${RISCV_SOMA_SOURCES})
target_include_directories(riscv_soma PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
)
target_link_libraries(riscv_soma PRIVATE ${CMAKE_DL_LIBS})
# Fixed name on every platform so arch files can say plugins/riscv/libriscv_soma.so
set_target_properties(riscv_soma PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
# Lets the plugin find firmware/*.hex in the source tree as a last resort
target_compile_definitions(riscv_soma PRIVATE
    RISCV_PLUGIN_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Strict warnings on hand-written sources only; Verilator-generated code is
#  left alone.
if(MSVC)
    set_source_files_properties(${RISCV_SOMA_SOURCES} backends/sv_core.cpp
        PROPERTIES COMPILE_OPTIONS "/W4;/WX")
else()
    set_source_files_properties(${RISCV_SOMA_SOURCES} backends/sv_core.cpp
        PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-pedantic;-Werror")
endif()

# ---------------------------------------------------------------------------
# SystemVerilog backend (optional, needs Verilator >= 4.2)
# ---------------------------------------------------------------------------
if(RISCV_SOMA_ENABLE_SV)
    find_package(verilator QUIET HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})
endif()

if(verilator_FOUND)
    message(STATUS "riscv_soma: Verilator ${verilator_VERSION} found, SV backend enabled")
    target_sources(riscv_soma PRIVATE backends/sv_core.cpp)
    target_compile_definitions(riscv_soma PRIVATE RISCV_SOMA_HAVE_SV=1)
    verilate(riscv_soma
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rtl/riscv_soma_top.sv"
        TOP_MODULE riscv_soma_top
        PREFIX Vriscv_soma_top
        VERILATOR_ARGS -Wall -Wno-fatal --x-assign fast --x-initial fast
    )
else()
    message(STATUS "riscv_soma: Verilator not found, SV backend disabled "
                   "(backend: sv will error at runtime; reference and cpp still work)")
endif()

# ---------------------------------------------------------------------------
# Copy firmware images next to the library so `firmware: lif.hex` resolves
#  relative to the plugin path given in the arch file.
# ---------------------------------------------------------------------------
file(GLOB RISCV_SOMA_FIRMWARE "${CMAKE_CURRENT_SOURCE_DIR}/firmware/*.hex")
foreach(hex ${RISCV_SOMA_FIRMWARE})
    add_custom_command(TARGET riscv_soma POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${hex}" "$<TARGET_FILE_DIR:riscv_soma>"
        COMMENT "Copying $<PATH:GET_FILENAME,${hex}> next to libriscv_soma.so"
    )
endforeach()

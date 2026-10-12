#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "alife/core/rng.hpp"

namespace alife::genetics {

inline constexpr std::size_t kVmRegisters = 32;
inline constexpr std::size_t kVmMaxInstructions = 1024;
inline constexpr std::size_t kVmInputs = 20;
inline constexpr std::size_t kVmOutputs = 8;

enum class VmOp : std::uint8_t {
    Nop,
    Sense,
    Load,
    Move,
    Add,
    Subtract,
    Multiply,
    Minimum,
    Maximum,
    Absolute,
    Tanh,
    Memory,
    Learn,
    GreaterThan,
    LessThan,
    JumpIfPositive,
    JumpIfNegative,
    Output,
    Count
};

inline constexpr std::size_t kVmOpCount = static_cast<std::size_t>(VmOp::Count);

struct VmInstruction {
    VmOp op = VmOp::Nop;
    std::uint8_t dst = 0;
    std::uint8_t srcA = 0;
    std::uint8_t srcB = 0;
    std::int16_t immediate = 0;
};

using Program = std::vector<VmInstruction>;
using VmInputs = std::array<float, kVmInputs>;
using VmOutputs = std::array<float, kVmOutputs>;
using VmState = std::array<float, kVmRegisters>;

struct VmEvaluation {
    VmOutputs outputs{};
    std::uint16_t executed = 0;
};

Program makeSeedProgram();
void repairProgram(Program& program) noexcept;
VmEvaluation executeProgram(std::span<const VmInstruction> program, const VmInputs& inputs,
                            VmState& state) noexcept;
std::size_t countOutputConnectedInstructions(std::span<const VmInstruction> program) noexcept;
void mutateProgram(Program& program, float rate, CounterRng& rng);
Program crossoverProgram(std::span<const VmInstruction> a, std::span<const VmInstruction> b,
                         CounterRng& rng);

}  // namespace alife::genetics

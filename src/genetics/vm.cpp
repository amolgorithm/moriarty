#include "alife/genetics/vm.hpp"

#include <algorithm>
#include <cmath>

namespace alife::genetics {
namespace {

std::uint8_t randomRegister(CounterRng& rng) {
    return static_cast<std::uint8_t>(rng.next() % kVmRegisters);
}

VmInstruction randomInstruction(CounterRng& rng) {
    VmInstruction instruction;
    instruction.op = static_cast<VmOp>(rng.next() % kVmOpCount);
    instruction.dst = randomRegister(rng);
    instruction.srcA = randomRegister(rng);
    instruction.srcB = randomRegister(rng);
    instruction.immediate = static_cast<std::int16_t>(
        static_cast<std::int32_t>(rng.next() % 2049) - 1024);
    if (instruction.op == VmOp::Sense)
        instruction.srcA = static_cast<std::uint8_t>(rng.next() % kVmInputs);
    if (instruction.op == VmOp::Output)
        instruction.dst = static_cast<std::uint8_t>(rng.next() % kVmOutputs);
    return instruction;
}

void mutateInstruction(VmInstruction& instruction, float rate, CounterRng& rng) {
    switch (rng.next() % 5) {
        case 0:
            instruction.op = static_cast<VmOp>(rng.next() % kVmOpCount);
            break;
        case 1:
            instruction.dst = randomRegister(rng);
            if (instruction.op == VmOp::Output)
                instruction.dst = static_cast<std::uint8_t>(rng.next() % kVmOutputs);
            break;
        case 2:
            instruction.srcA = instruction.op == VmOp::Sense
                                   ? static_cast<std::uint8_t>(rng.next() % kVmInputs)
                                   : randomRegister(rng);
            break;
        case 3:
            instruction.srcB = randomRegister(rng);
            break;
        default: {
            const float sigma = std::max(1.0f, rate * 4096.0f);
            const auto step = static_cast<std::int32_t>(std::lround(sigma * rng.normal()));
            instruction.immediate = static_cast<std::int16_t>(std::clamp(
                static_cast<std::int32_t>(instruction.immediate) + step, -4096, 4096));
            break;
        }
    }
}

std::size_t randomBlockLength(std::size_t available, CounterRng& rng) {
    return std::min<std::size_t>(available, 1 + static_cast<std::size_t>(rng.next() % 8));
}

}  // namespace

Program makeSeedProgram() {
    Program program;
    program.reserve(8);
    static constexpr std::array<std::uint8_t, 4> sensors{0, 1, 8, 9};
    for (std::uint8_t output = 0; output < sensors.size(); ++output) {
        const std::uint8_t reg = output;
        program.push_back({VmOp::Sense, reg, sensors[output], 0, 0});
        program.push_back({VmOp::Output, output, reg, 0, 0});
    }
    return program;
}

void repairProgram(Program& program) noexcept {
    if (program.size() > kVmMaxInstructions) program.resize(kVmMaxInstructions);
    for (auto& instruction : program) {
        if (static_cast<std::size_t>(instruction.op) >= kVmOpCount)
            instruction.op = VmOp::Nop;
        instruction.dst = static_cast<std::uint8_t>(
            instruction.dst % (instruction.op == VmOp::Output ? kVmOutputs : kVmRegisters));
        instruction.srcA = instruction.op == VmOp::Sense
                              ? static_cast<std::uint8_t>(instruction.srcA % kVmInputs)
                              : static_cast<std::uint8_t>(instruction.srcA % kVmRegisters);
        instruction.srcB = static_cast<std::uint8_t>(instruction.srcB % kVmRegisters);
        instruction.immediate = static_cast<std::int16_t>(
            std::clamp(static_cast<int>(instruction.immediate), -4096, 4096));
    }
}

VmEvaluation executeProgram(std::span<const VmInstruction> program, const VmInputs& inputs,
                            VmState& state) noexcept {
    VmEvaluation evaluation;
    const std::size_t gas = std::min(program.size(), kVmMaxInstructions);
    std::size_t pc = 0;
    while (pc < gas && evaluation.executed < gas) {
        const VmInstruction& instruction = program[pc];
        const float a = state[instruction.srcA % kVmRegisters];
        const float b = state[instruction.srcB % kVmRegisters];
        bool jumped = false;
        switch (instruction.op) {
            case VmOp::Nop:
                break;
            case VmOp::Sense:
                state[instruction.dst % kVmRegisters] = inputs[instruction.srcA % kVmInputs];
                break;
            case VmOp::Load:
                state[instruction.dst % kVmRegisters] =
                    static_cast<float>(instruction.immediate) * (1.0f / 1024.0f);
                break;
            case VmOp::Move:
                state[instruction.dst % kVmRegisters] = a;
                break;
            case VmOp::Add:
                state[instruction.dst % kVmRegisters] = std::clamp(a + b, -1.0f, 1.0f);
                break;
            case VmOp::Subtract:
                state[instruction.dst % kVmRegisters] = std::clamp(a - b, -1.0f, 1.0f);
                break;
            case VmOp::Multiply:
                state[instruction.dst % kVmRegisters] = std::clamp(2.0f * a * b, -1.0f, 1.0f);
                break;
            case VmOp::Minimum:
                state[instruction.dst % kVmRegisters] = std::min(a, b);
                break;
            case VmOp::Maximum:
                state[instruction.dst % kVmRegisters] = std::max(a, b);
                break;
            case VmOp::Absolute:
                state[instruction.dst % kVmRegisters] = std::fabs(a);
                break;
            case VmOp::Tanh:
                state[instruction.dst % kVmRegisters] = std::tanh(a);
                break;
            case VmOp::Memory:
                state[instruction.dst % kVmRegisters] =
                    0.8f * state[instruction.dst % kVmRegisters] + 0.2f * std::tanh(a + b);
                break;
            case VmOp::Learn: {
                const float learningRate =
                    static_cast<float>(instruction.immediate) * (1.0f / 4096.0f);
                const auto destination = instruction.dst % kVmRegisters;
                state[destination] = std::clamp(state[destination] + learningRate * a * b,
                                                -1.0f, 1.0f);
                break;
            }
            case VmOp::GreaterThan:
                state[instruction.dst % kVmRegisters] = a > b ? 1.0f : -1.0f;
                break;
            case VmOp::LessThan:
                state[instruction.dst % kVmRegisters] = a < b ? 1.0f : -1.0f;
                break;
            case VmOp::JumpIfPositive:
            case VmOp::JumpIfNegative: {
                const bool condition = instruction.op == VmOp::JumpIfPositive ? a > 0.0f : a < 0.0f;
                if (condition && instruction.immediate != 0) {
                    const auto target = static_cast<std::int64_t>(pc) + instruction.immediate;
                    if (target >= 0 && target < static_cast<std::int64_t>(gas)) {
                        pc = static_cast<std::size_t>(target);
                        jumped = true;
                    }
                }
                break;
            }
            case VmOp::Output:
                evaluation.outputs[instruction.dst % kVmOutputs] =
                    std::clamp(a, -1.0f, 1.0f);
                break;
            case VmOp::Count:
                break;
        }
        ++evaluation.executed;
        if (!jumped) ++pc;
    }
    return evaluation;
}

std::size_t countOutputConnectedInstructions(std::span<const VmInstruction> program) noexcept {
    std::array<bool, kVmRegisters> needed{};
    bool affectsOutput = false;
    std::size_t connected = 0;
    for (std::size_t i = std::min(program.size(), kVmMaxInstructions); i-- > 0;) {
        const auto& instruction = program[i];
        if (instruction.op == VmOp::Output) {
            needed[instruction.srcA % kVmRegisters] = true;
            ++connected;
            affectsOutput = true;
            continue;
        }
        if (instruction.op == VmOp::JumpIfPositive || instruction.op == VmOp::JumpIfNegative) {
            if (affectsOutput) {
                ++connected;
                needed[instruction.srcA % kVmRegisters] = true;
            }
            continue;
        }
        const bool writesRegister =
            instruction.op != VmOp::Nop && instruction.op != VmOp::Output;
        const std::size_t destination = instruction.dst % kVmRegisters;
        if (!writesRegister || !needed[destination]) continue;
        ++connected;
        needed[destination] = false;
        switch (instruction.op) {
            case VmOp::Move:
            case VmOp::Absolute:
            case VmOp::Tanh:
                needed[instruction.srcA % kVmRegisters] = true;
                break;
            case VmOp::Add:
            case VmOp::Subtract:
            case VmOp::Multiply:
            case VmOp::Minimum:
            case VmOp::Maximum:
            case VmOp::Memory:
            case VmOp::Learn:
            case VmOp::GreaterThan:
            case VmOp::LessThan:
                needed[instruction.srcA % kVmRegisters] = true;
                needed[instruction.srcB % kVmRegisters] = true;
                break;
            default:
                break;
        }
    }
    return connected;
}

void mutateProgram(Program& program, float rate, CounterRng& rng) {
    if (program.empty()) program = makeSeedProgram();
    const std::size_t editCount = std::clamp<std::size_t>(
        1 + static_cast<std::size_t>(std::max(0.0f, rate) * 6.0f), 1, 12);
    for (std::size_t i = 0; i < editCount; ++i) {
        auto& instruction = program[static_cast<std::size_t>(rng.next() % program.size())];
        mutateInstruction(instruction, rate, rng);
    }

    const bool canInsert = program.size() < kVmMaxInstructions;
    const bool canDelete = program.size() > kVmOutputs;
    if (rng.uniform() < std::clamp(0.30f + rate, 0.30f, 0.50f) && (canInsert || canDelete)) {
        const bool insert = canInsert && (!canDelete || rng.uniform() < 0.5f);
        if (insert) {
            const std::size_t length = randomBlockLength(kVmMaxInstructions - program.size(), rng);
            const std::size_t source = static_cast<std::size_t>(rng.next() % program.size());
            const std::size_t position = static_cast<std::size_t>(rng.next() % (program.size() + 1));
            std::vector<VmInstruction> block;
            block.reserve(length);
            for (std::size_t i = 0; i < length; ++i)
                block.push_back(rng.uniform() < 0.75f ? program[(source + i) % program.size()]
                                                       : randomInstruction(rng));
            program.insert(program.begin() + static_cast<std::ptrdiff_t>(position),
                           block.begin(), block.end());
        } else {
            const std::size_t length = randomBlockLength(program.size() - kVmOutputs, rng);
            const std::size_t position =
                static_cast<std::size_t>(rng.next() % (program.size() - length + 1));
            program.erase(program.begin() + static_cast<std::ptrdiff_t>(position),
                          program.begin() + static_cast<std::ptrdiff_t>(position + length));
        }
    }
    repairProgram(program);
}

Program crossoverProgram(std::span<const VmInstruction> a, std::span<const VmInstruction> b,
                         CounterRng& rng) {
    a = a.first(std::min(a.size(), kVmMaxInstructions));
    b = b.first(std::min(b.size(), kVmMaxInstructions));
    if (a.empty() || b.empty()) {
        const auto source = a.empty() ? b : a;
        Program child(source.begin(), source.end());
        if (child.size() < kVmOutputs) child = makeSeedProgram();
        repairProgram(child);
        return child;
    }
    const std::size_t cutA = static_cast<std::size_t>(rng.next() % (a.size() + 1));
    const std::size_t cutB = static_cast<std::size_t>(rng.next() % (b.size() + 1));
    Program child;
    child.reserve(std::min(cutA + b.size() - cutB, kVmMaxInstructions));
    child.insert(child.end(), a.begin(), a.begin() + static_cast<std::ptrdiff_t>(cutA));
    child.insert(child.end(), b.begin() + static_cast<std::ptrdiff_t>(cutB),
                 b.begin() + static_cast<std::ptrdiff_t>(cutB + std::min(
                     b.size() - cutB, kVmMaxInstructions - child.size())));
    if (child.size() < kVmOutputs) {
        child.clear();
        const std::size_t fromA = std::min(kVmOutputs, a.size());
        child.insert(child.end(), a.begin(), a.begin() + static_cast<std::ptrdiff_t>(fromA));
        const std::size_t fromB = std::min(kVmOutputs - child.size(), b.size());
        child.insert(child.end(), b.begin(), b.begin() + static_cast<std::ptrdiff_t>(fromB));
    }
    repairProgram(child);
    return child;
}

}  // namespace alife::genetics

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>

#include "alife/genetics/vm.hpp"

using namespace alife::genetics;

TEST_CASE("seed virtual genome routes environmental sensors to each action port") {
    const Program program = makeSeedProgram();
    REQUIRE(program.size() == 8);
    VmInputs inputs{};
    VmState state{};
    inputs[0] = 0.6f;
    inputs[1] = -0.4f;
    inputs[8] = -0.8f;
    inputs[9] = 0.2f;
    const auto result = executeProgram(program, inputs, state);
    CHECK(result.outputs[0] == doctest::Approx(0.6f));
    CHECK(result.outputs[1] == doctest::Approx(-0.4f));
    CHECK(result.outputs[2] == doctest::Approx(-0.8f));
    CHECK(result.outputs[3] == doctest::Approx(0.2f));
    CHECK(result.outputs[4] == 0.0f);
    CHECK(result.outputs[5] == 0.0f);
    CHECK(result.outputs[6] == 0.0f);
    CHECK(result.outputs[7] == 0.0f);
    CHECK(result.executed == program.size());
}

TEST_CASE("virtual genome branches are gas bounded and memory persists") {
    Program loop{{VmOp::Load, 0, 0, 0, 1024},
                 {VmOp::JumpIfPositive, 0, 0, 0, -1}};
    VmInputs inputs{};
    VmState state{};
    const auto bounded = executeProgram(loop, inputs, state);
    CHECK(bounded.executed == loop.size());

    Program memoryProgram{{VmOp::Load, 0, 0, 0, 1024},
                          {VmOp::Memory, 1, 0, 0, 0},
                          {VmOp::Output, 0, 1, 0, 0}};
    const auto first = executeProgram(memoryProgram, inputs, state);
    REQUIRE(first.executed == memoryProgram.size());
    CHECK(first.outputs[0] > 0.0f);
    const float memoryBefore = state[1];
    const auto second = executeProgram(memoryProgram, inputs, state);
    CHECK(state[1] > memoryBefore);
    CHECK(second.executed == memoryProgram.size());
}

TEST_CASE("genetically encoded learning uses the previous energy balance") {
    Program learning{{VmOp::Sense, 1, 19, 0, 0},
                     {VmOp::Load, 2, 0, 0, 1024},
                     {VmOp::Learn, 0, 1, 2, 2048}};
    VmInputs inputs{};
    inputs[19] = 0.5f;
    VmState state{};
    executeProgram(learning, inputs, state);
    CHECK(state[0] == doctest::Approx(0.25f));
    executeProgram(learning, inputs, state);
    CHECK(state[0] == doctest::Approx(0.5f));
}

TEST_CASE("virtual genome mutation and crossover stay inside the instruction budget") {
    Program program = makeSeedProgram();
    alife::CounterRng rng(27, 13, 99);
    for (int i = 0; i < 2000; ++i) mutateProgram(program, 0.12f, rng);
    CHECK(program.size() >= kVmOutputs);
    CHECK(program.size() <= kVmMaxInstructions);
    for (const auto& instruction : program) {
        CHECK(static_cast<std::size_t>(instruction.op) < kVmOpCount);
        CHECK((instruction.dst < kVmRegisters || instruction.op == VmOp::Output));
        CHECK((instruction.srcA < kVmInputs || instruction.op != VmOp::Sense));
    }
    const Program parentA = makeSeedProgram();
    Program parentB = parentA;
    parentB.push_back({VmOp::JumpIfNegative, 0, 0, 0, -3});
    const Program child = crossoverProgram(parentA, parentB, rng);
    CHECK(child.size() <= kVmMaxInstructions);

    const Program oversized(kVmMaxInstructions + 16, VmInstruction{VmOp::Nop});
    const Program boundedChild = crossoverProgram(oversized, oversized, rng);
    CHECK(boundedChild.size() >= kVmOutputs);
    CHECK(boundedChild.size() <= kVmMaxInstructions);
    CHECK(crossoverProgram({}, parentA, rng).size() >= kVmOutputs);
}

TEST_CASE("virtual machine can execute thousand-instruction genomes with fixed-size state") {
    Program program(kVmMaxInstructions, VmInstruction{VmOp::Nop});
    program.back() = {VmOp::Load, 0, 0, 0, 1024};
    VmState state{};
    const auto result = executeProgram(program, VmInputs{}, state);
    CHECK(result.executed == kVmMaxInstructions);
    CHECK(state[0] == doctest::Approx(1.0f));
}

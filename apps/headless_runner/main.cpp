#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"
#include "alife/sim/scheduler.hpp"
#include "alife/telemetry/collect.hpp"

// Usage: moriarty_headless [seconds_of_sim_time] [report_every_s] [seed] [resource_regrowth]
int main(int argc, char** argv) {
    const double total = argc > 1 ? std::atof(argv[1]) : 600.0;
    const double every = argc > 2 ? std::atof(argv[2]) : 30.0;
    alife::SimConfig config;
    if (argc > 3) config.seed = std::strtoull(argv[3], nullptr, 0);
    if (argc > 4) config.eco.regrow = static_cast<float>(std::atof(argv[4]));

    alife::World world(config);
    alife::ThreadPool pool;
    alife::sim::Scheduler sched(world, pool);
    const float dt = world.cfg.dtPhys;
    const auto ticks = static_cast<std::uint64_t>(total / static_cast<double>(dt));
    const auto step = static_cast<std::uint64_t>(every / static_cast<double>(dt));

    std::printf("seed: %llu, resource regrowth: %.3f/s\n",
                static_cast<unsigned long long>(world.cfg.seed),
                static_cast<double>(world.cfg.eco.regrow));
    std::printf("%7s %6s %5s %5s %6s %6s %6s %7s %7s %7s %6s %6s %6s %8s %8s %6s %6s %6s %10s\n",
                "t[s]", "pop", "taxa", "spp8+", "graze", "omni", "pred", "births", "kills", "starve", "H",
                "instr", "gen", "food E", "prey E", "forage", "hunt", "evade", "balance");
    for (std::uint64_t t = 1; t <= ticks; ++t) {
        sched.stepOnce();
        if (t % step == 0) {
            using alife::genetics::Role;
            int role[3] = {0, 0, 0};
            std::uint32_t action[5] = {};
            std::uint64_t generationSum = 0;
            std::uint32_t maxGeneration = 0;
            std::vector<std::uint32_t> liveCounts(world.species.all().size(), 0);
            for (std::uint32_t i = 0; i < world.pool.count(); ++i) ++liveCounts[world.pool.species[i]];
            for (std::uint32_t i = 0; i < world.pool.count(); ++i) {
                generationSum += world.pool.generation[i];
                maxGeneration = std::max(maxGeneration, world.pool.generation[i]);
            }
            std::uint32_t extantTaxa = 0;
            for (std::size_t i = 0; i < liveCounts.size(); ++i) {
                const auto count = liveCounts[i];
                if (count == 0) continue;
                ++extantTaxa;
                role[static_cast<int>(world.species.all()[i].role)] += static_cast<int>(count);
            }
            for (std::uint32_t i = 0; i < world.pool.count(); ++i)
                ++action[std::min<std::size_t>(world.pool.action[i], std::size(action) - 1)];
            const auto tel = alife::telemetry::collect(world);
            const double balance = world.energyInventory() + world.stats.dissipatedEnergy -
                                   world.stats.initialEnergy - world.stats.externalEnergy;
            std::printf("%7.0f %6u %5u %5u %6d %6d %6d %7llu %7llu %7llu %6.2f %6.1f %6.1f %8.0f %8.0f %6u %6u %6u %10.3g\n",
                        static_cast<double>(t) * static_cast<double>(dt),
                        tel.population, extantTaxa, tel.speciesCount, role[0], role[1], role[2],
                        static_cast<unsigned long long>(world.stats.births), static_cast<unsigned long long>(world.stats.kills),
                        static_cast<unsigned long long>(world.stats.starved), static_cast<double>(tel.shannonBits),
                        static_cast<double>(tel.meanProgramInstructions),
                        world.pool.count() ? static_cast<double>(generationSum) / world.pool.count() : 0.0,
                        world.stats.plantEnergy,
                        world.stats.animalEnergy, action[static_cast<std::size_t>(alife::AgentAction::Forage)],
                        action[static_cast<std::size_t>(alife::AgentAction::Hunt)],
                        action[static_cast<std::size_t>(alife::AgentAction::Evade)], balance);
        }
    }
    std::vector<std::uint32_t> liveCounts(world.species.all().size(), 0);
    for (std::uint32_t i = 0; i < world.pool.count(); ++i) ++liveCounts[world.pool.species[i]];
    std::vector<std::uint32_t> extant;
    for (std::size_t i = 0; i < liveCounts.size(); ++i)
        if (liveCounts[i] > 0) extant.push_back(static_cast<std::uint32_t>(i));
    std::vector<std::array<double, 3>> taxonTraits(liveCounts.size());
    for (std::uint32_t i = 0; i < world.pool.count(); ++i) {
        const auto species = world.pool.species[i];
        taxonTraits[species][0] += world.pool.immunity[i];
        taxonTraits[species][1] += world.pool.repair[i];
        taxonTraits[species][2] += world.pool.digestion[i];
    }
    std::sort(extant.begin(), extant.end(),
              [&liveCounts](std::uint32_t a, std::uint32_t b) { return liveCounts[a] > liveCounts[b]; });
    std::uint64_t generationSum = 0;
    std::uint32_t maxGeneration = 0;
    double meanImmunity = 0.0, meanRepair = 0.0, meanDigestion = 0.0, meanMutationSigma = 0.0;
    for (std::uint32_t i = 0; i < world.pool.count(); ++i) {
        generationSum += world.pool.generation[i];
        maxGeneration = std::max(maxGeneration, world.pool.generation[i]);
        meanImmunity += world.pool.immunity[i];
        meanRepair += world.pool.repair[i];
        meanDigestion += world.pool.digestion[i];
        const float* genome = &world.pool.genome[static_cast<std::size_t>(i) * alife::kGenomeLen];
        meanMutationSigma += 0.012f + 0.168f * genome[alife::genetics::kMutRate];
    }
    std::printf("generation depth: mean %.2f, max %u\n",
                world.pool.count() ? static_cast<double>(generationSum) / world.pool.count() : 0.0,
                maxGeneration);
    if (world.pool.count() > 0) {
        const double n = static_cast<double>(world.pool.count());
        std::printf("mean heritable traits: immunity %.3f, repair %.3f, digestion %.3f, mutation sigma %.3f/gene\n",
                    meanImmunity / n, meanRepair / n, meanDigestion / n, meanMutationSigma / n);
    }
    const std::size_t shown = std::min<std::size_t>(25, extant.size());
    std::printf("\nextant taxa at end: %zu total; showing up to 25\n", extant.size());
    for (std::size_t i = 0; i < shown; ++i) {
        const auto& sp = world.species.all()[extant[i]];
        std::printf("  %-22s n=%-5u instructions=%.1f active=%.1f aggr=%.2f size=%.1f speed=%.0f def=%.2f diet=%.2f heat=%.2f immune=%.2f repair=%.2f digest=%.2f %s\n",
                    sp.fullName().c_str(), liveCounts[extant[i]],
                    static_cast<double>(sp.meanProgramInstructions),
                    static_cast<double>(sp.meanOutputConnectedInstructions),
                    static_cast<double>(sp.meanAggr), static_cast<double>(sp.meanSize), static_cast<double>(sp.meanSpeed),
                    static_cast<double>(sp.meanDef), static_cast<double>(sp.meanDiet),
                    static_cast<double>(sp.meanHeatTolerance),
                    taxonTraits[extant[i]][0] / liveCounts[extant[i]],
                    taxonTraits[extant[i]][1] / liveCounts[extant[i]],
                    taxonTraits[extant[i]][2] / liveCounts[extant[i]],
                    sp.parent == alife::genetics::SpeciesTable::kNone ? "founder" : "derived");
    }
    double connectedInstructions = 0.0, executedInstructions = 0.0;
    std::size_t totalInstructions = 0, memoryInstructions = 0, learningInstructions = 0;
    std::size_t comparisonInstructions = 0;
    std::size_t branchInstructions = 0;
    std::array<std::uint32_t, alife::genetics::kVmOutputs> outputCarriers{};
    for (std::uint32_t i = 0; i < world.pool.count(); ++i) {
        const auto& program = world.pool.program[i];
        std::array<bool, alife::genetics::kVmOutputs> hasOutput{};
        connectedInstructions += static_cast<double>(
            alife::genetics::countOutputConnectedInstructions(program));
        executedInstructions += world.pool.vmExecuted[i];
        for (const auto& instruction : program) {
            ++totalInstructions;
            memoryInstructions += instruction.op == alife::genetics::VmOp::Memory;
            learningInstructions += instruction.op == alife::genetics::VmOp::Learn;
            comparisonInstructions += instruction.op == alife::genetics::VmOp::GreaterThan ||
                                     instruction.op == alife::genetics::VmOp::LessThan;
            branchInstructions += instruction.op == alife::genetics::VmOp::JumpIfPositive ||
                                 instruction.op == alife::genetics::VmOp::JumpIfNegative;
            if (instruction.op == alife::genetics::VmOp::Output)
                hasOutput[instruction.dst % hasOutput.size()] = true;
        }
        for (std::size_t output = 0; output < hasOutput.size(); ++output)
            outputCarriers[output] += hasOutput[output] ? 1u : 0u;
    }
    std::printf("virtual genome: output-connected instructions %.2f / total %.2f; executed %.2f/tick; branches %zu, comparisons %zu, memory %zu, learning %zu\n",
                world.pool.count() ? connectedInstructions / world.pool.count() : 0.0,
                world.pool.count() ? static_cast<double>(totalInstructions) / world.pool.count() : 0.0,
                world.pool.count() ? executedInstructions / world.pool.count() : 0.0,
                branchInstructions, comparisonInstructions, memoryInstructions,
                learningInstructions);
    std::printf("routed-output carriers: x %u/%u y %u/%u uptake %u/%u resource %u/%u\n",
                outputCarriers[0], world.pool.count(), outputCarriers[1], world.pool.count(),
                outputCarriers[2], world.pool.count(), outputCarriers[3], world.pool.count());
    std::printf("routed-output carriers: defense %u/%u cohesion %u/%u hunt %u/%u reproduction %u/%u\n",
                outputCarriers[4], world.pool.count(), outputCarriers[5], world.pool.count(),
                outputCarriers[6], world.pool.count(), outputCarriers[7], world.pool.count());
    const double balance = world.energyInventory() + world.stats.dissipatedEnergy -
                           world.stats.initialEnergy - world.stats.externalEnergy;
    std::printf("energy ledger: start %.3f + external %.3f = inventory %.3f + dissipated %.3f (residual %.3g)\n",
                world.stats.initialEnergy, world.stats.externalEnergy, world.energyInventory(),
                world.stats.dissipatedEnergy, balance);
    return 0;
}

#pragma once
#include "alife/genetics/genome.hpp"

namespace alife::genetics {

// Decoded phenotype (genotype-phenotype map).
struct Phenotype {
    float eta = 1.0f;
    float senseR = 20.0f;
    float vMax = 30.0f;
    float aggression = 0.0f;
    float repThresh = 0.7f;  // fraction of eMax
    float mutRate = 0.02f;
    float radius = 5.0f;
    float mass = 1.5f;
    float eMax = 60.0f;
    float defense = 0.0f;
    float explore = 0.5f;
};

Phenotype decode(const float* genomeRow, float vTop);

}  // namespace alife::genetics

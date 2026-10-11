#pragma once

namespace alife::genetics {

// Decoded phenotype. Genes live in [0,1] and get scaled to physical ranges here.
struct Phenotype {
    float eta = 1.0f;       // metabolic efficiency
    float senseR = 8.0f;
    float vMax = 1.0f;
    float aggression = 0.0f;
    float repThresh = 1.0f;
    float mutRate = 0.02f;
};

Phenotype decode(const float* genomeRow);  // genotype-phenotype map

}  // namespace alife::genetics

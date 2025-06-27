#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>

struct SystemConfig {
    int numCPU = 1;
    std::string scheduler = "fcfs";
    uint32_t quantumCycles = 0;
    uint32_t batchProcessFreq = 1;
    uint32_t minInstructions = 1;
    uint32_t maxInstructions = 1;
    uint32_t delayPerExec = 0;
    bool initialized = false;
};

extern SystemConfig systemConfig;

extern SystemConfig systemConfig;

#endif
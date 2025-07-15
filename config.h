#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>

struct SystemConfig {
    int numCPU = 2;
    std::string scheduler = "rr";
    int quantumCycles = 4;
    int batchProcessFreq = 1;
    int minInstructions = 100;
    int maxInstructions = 100;
    int delayPerExec = 0;

    size_t maxOverallMemory = 16384;
    size_t memoryPerFrame = 16;
    size_t memoryPerProcess = 4096;
};

extern SystemConfig systemConfig;

#endif
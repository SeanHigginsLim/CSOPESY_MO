#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#include <vector>
#include <string>
#include "process.h"

class MemoryManager {
public:
    MemoryManager();
    bool allocate(Process& p);
    void release(int pid);
    int getExternalFragmentationKB();
    void printMemoryLayout(int quantumCycle);

private:
    std::vector<Process> memory;
    int totalMemory;
};

#endif
#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <queue>
#include "process.h"
#include "memory_manager.h"

class Scheduler {
public:
    Scheduler();
    void addProcess(const Process& p);
    void run();
    void stop();
    void printActiveProcesses();

private:
    std::queue<Process> readyQueue;
    MemoryManager memory;
    int cycleCount;
    bool running;
};

#endif

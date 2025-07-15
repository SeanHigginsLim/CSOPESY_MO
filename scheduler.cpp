#include "scheduler.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "config.h"

Scheduler::Scheduler() : cycleCount(0), running(false) {}

void Scheduler::addProcess(const Process& p) {
    readyQueue.push(p);
}

void Scheduler::run() {
    running = true;
    while (running && !readyQueue.empty()) {
        ++cycleCount;

        int cpuRun = std::min(systemConfig.numCPU, (int)readyQueue.size());
        for (int i = 0; i < cpuRun; ++i) {
            Process p = readyQueue.front();
            readyQueue.pop();

            if (!p.inMemory) {
                if (!memory.allocate(p)) {
                    readyQueue.push(p);
                    continue;
                }
            }

            p.remainingInstructions -= systemConfig.quantumCycles;
            if (p.remainingInstructions <= 0) {
                memory.release(p.pid);
            } else {
                readyQueue.push(p);
            }
        }

        memory.printMemoryLayout(cycleCount);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void Scheduler::stop() {
    running = false;
}

void Scheduler::printActiveProcesses() {
    std::cout << "Ready queue size: " << readyQueue.size() << "\n";
}
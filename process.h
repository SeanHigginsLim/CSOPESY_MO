#ifndef PROCESS_H
#define PROCESS_H
#include <string>
#include <vector>

struct Process {
    int pid;
    int remainingInstructions;
    int baseAddr;
    int limitAddr;
    bool inMemory;

    Process(int id, int ins = 100)
        : pid(id), remainingInstructions(ins), baseAddr(-1), limitAddr(-1), inMemory(false) {}
};

#endif
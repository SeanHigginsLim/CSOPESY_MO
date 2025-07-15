#include "memory_manager.h"
#include "config.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <algorithm>

MemoryManager::MemoryManager() {
    totalMemory = systemConfig.maxOverallMemory;
}

bool MemoryManager::allocate(Process& p) {
    int memSize = systemConfig.memoryPerProcess;
    int address = 0;

    std::sort(memory.begin(), memory.end(), [](const Process& a, const Process& b) {
        return a.baseAddr < b.baseAddr;
    });

    for (auto& proc : memory) {
        if (proc.baseAddr - address >= memSize) {
            p.baseAddr = address;
            p.limitAddr = address + memSize;
            p.inMemory = true;
            memory.push_back(p);
            return true;
        }
        address = proc.limitAddr;
    }

    if (totalMemory - address >= memSize) {
        p.baseAddr = address;
        p.limitAddr = address + memSize;
        p.inMemory = true;
        memory.push_back(p);
        return true;
    }

    return false;
}

void MemoryManager::release(int pid) {
    memory.erase(std::remove_if(memory.begin(), memory.end(), [pid](const Process& p) {
        return p.pid == pid;
    }), memory.end());
}

int MemoryManager::getExternalFragmentationKB() {
    std::sort(memory.begin(), memory.end(), [](const Process& a, const Process& b) {
        return a.baseAddr < b.baseAddr;
    });

    int frag = 0, prevEnd = 0;
    for (const auto& p : memory) {
        if (p.baseAddr > prevEnd)
            frag += p.baseAddr - prevEnd;
        prevEnd = p.limitAddr;
    }

    if (prevEnd < totalMemory)
        frag += totalMemory - prevEnd;

    return frag / 1024;
}

void MemoryManager::printMemoryLayout(int quantumCycle) {
    std::ofstream out("memory_stamp_" + std::to_string(quantumCycle) + ".txt");

    std::time_t now = std::time(nullptr);
    char buf[80];
    std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", std::localtime(&now));

    out << "Timestamp: " << buf << "\n";
    out << "Number of processes in memory: " << memory.size() << "\n";
    out << "Total external fragmentation in KB: " << getExternalFragmentationKB() << "\n\n";

    out << "----end---- = " << totalMemory << "\n";
    int current = totalMemory;

    std::sort(memory.begin(), memory.end(), [](const Process& a, const Process& b) {
        return a.baseAddr > b.baseAddr;
    });

    for (const auto& p : memory) {
        out << current << "\n";
        out << "P" << p.pid << "\n";
        current = p.baseAddr;
        out << current << "\n";
    }

    out << "----start---- = 0\n";
    out.close();
}
#include "memory_manager.h"
#include "process.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>

MemoryManager memManager;

MemoryManager::MemoryManager(int totalMemoryBytes, int pageSize)
    : totalMemory(totalMemoryBytes), pageSize(pageSize) {
    frameCount = totalMemory / pageSize;
    frames.resize(frameCount, "EMPTY");

    backingStore.open("csopesy-backing-store.txt", std::ios::out);
    if (!backingStore.is_open()) {
        std::cerr << "Failed to open backing store.\n";
    }
}

int MemoryManager::allocateProcess(const std::string& processName, int memoryBytes) {
    if (memoryBytes < 64 || memoryBytes < pageSize || memoryBytes > 65536 || (memoryBytes & (memoryBytes - 1)) != 0) {
        std::cerr << "[ERROR] Invalid memory allocation for process " << processName << ": " << memoryBytes << " bytes\n";
        return -1;
    }

    int pageCount = (memoryBytes + pageSize - 1) / pageSize;

    ProcessMemory proc;
    proc.pid = processes.size() + 1;
    proc.processName = processName;
    proc.allocatedBytes = memoryBytes;
    proc.pageCount = pageCount;
    proc.pageTable.resize(pageCount);

    int nextBaseAddr = 0;
    for (const auto& [_, existingProc] : processes) {
        int endAddr = existingProc.baseAddr + existingProc.allocatedBytes;
        if (endAddr > nextBaseAddr) nextBaseAddr = endAddr;
    }
    proc.baseAddr = nextBaseAddr;

    processes[processName] = proc;
    std::cout << "[MEM] Allocated " << memoryBytes << " bytes (" << pageCount << " page(s)) to process " << processName << "\n";
    return proc.pid;
}

void MemoryManager::accessPage(const std::string& processName, int pageNumber) {
    if (!processes.count(processName)) return;

    auto& proc = processes[processName];
    if (pageNumber >= proc.pageCount) return;

    auto& page = proc.pageTable[pageNumber];

    if (!page.inMemory) {
        std::cout << "[PAGE FAULT] Loading page " << pageNumber << " of " << processName << " into memory...\n";
        pageIn(processName, pageNumber);
    }
}

void MemoryManager::pageIn(const std::string& processName, int pageNumber) {
    int frame = findFreeFrame();
    if (frame == -1) {
        frame = replacePage();
    }

    frames[frame] = processName + "@" + std::to_string(pageNumber);
    pageHistory.push_back(frames[frame]);

    auto& proc = processes[processName];
    auto& page = proc.pageTable[pageNumber];
    page.inMemory = true;
    page.frameIndex = frame;

    backingStore << "[LOAD] " << processName << " page " << pageNumber << " -> frame " << frame << "\n";
}

void MemoryManager::pageOut(int frameIndex) {
    std::string tag = frames[frameIndex];
    if (tag == "EMPTY") return;

    auto pos = tag.find('@');
    std::string procName = tag.substr(0, pos);
    int pageIdx = std::stoi(tag.substr(pos + 1));

    auto& page = processes[procName].pageTable[pageIdx];
    page.inMemory = false;
    page.frameIndex = -1;

    backingStore << "[EVICT] " << procName << " page " << pageIdx << " from frame " << frameIndex << "\n";
    frames[frameIndex] = "EMPTY";
}

int MemoryManager::findFreeFrame() {
    for (int i = 0; i < frameCount; ++i) {
        if (frames[i] == "EMPTY") return i;
    }
    return -1;
}

int MemoryManager::replacePage() {
    std::string oldest = pageHistory.front();
    pageHistory.erase(pageHistory.begin());

    auto pos = oldest.find('@');
    std::string procName = oldest.substr(0, pos);
    int pageIdx = std::stoi(oldest.substr(pos + 1));

    int frameIndex = processes[procName].pageTable[pageIdx].frameIndex;
    pageOut(frameIndex);
    return frameIndex;
}

void MemoryManager::deallocateProcess(const std::string& processName) {
    if (!processes.count(processName)) return;

    auto& proc = processes[processName];
    for (int i = 0; i < proc.pageCount; ++i) {
        if (proc.pageTable[i].inMemory) {
            pageOut(proc.pageTable[i].frameIndex);
        }
    }

    processes.erase(processName);
    std::cout << "[MEM] Deallocated memory of " << processName << "\n";
}

void MemoryManager::printProcessSMI() {
    std::cout << "\n========== process-smi ==========\n";
    std::cout << "Total Memory: " << totalMemory << " bytes (" << frameCount << " frames)\n";
    int used = std::count_if(frames.begin(), frames.end(), [](const std::string& s) { return s != "EMPTY"; });
    std::cout << "Used Frames : " << used << "\n";
    std::cout << "Free Frames : " << (frameCount - used) << "\n";

    std::cout << "\nFrame Table:\n";
    for (int i = 0; i < frameCount; ++i) {
        std::cout << "  Frame[" << std::setw(2) << i << "]: " << frames[i] << "\n";
    }

    std::cout << "\nProcess List:\n";
    for (const auto& [name, proc] : processes) {
        std::cout << "  " << name << ": " << proc.allocatedBytes << " bytes, " << proc.pageCount << " pages\n";
    }

    std::cout << "=================================\n";
}

void MemoryManager::printVMStat() {
    std::cout << "\n========== vmstat ==========\n";
    std::cout << "Total Frames: " << frameCount << "\n";
    std::cout << "Free Frames : " << std::count(frames.begin(), frames.end(), "EMPTY") << "\n";
    std::cout << "Used Frames : " << frameCount - std::count(frames.begin(), frames.end(), "EMPTY") << "\n";

    std::cout << "\nActive Processes:\n";
    for (const auto& [name, proc] : processes) {
        std::cout << "  " << name << " (" << proc.pageCount << " pages):\n";
        for (int i = 0; i < proc.pageTable.size(); ++i) {
            std::cout << "    Page[" << i << "] -> ";
            if (proc.pageTable[i].inMemory)
                std::cout << "Frame " << proc.pageTable[i].frameIndex;
            else
                std::cout << "NOT IN MEMORY";
            std::cout << "\n";
        }
    }

    std::cout << "============================\n";
}

bool MemoryManager::isValidAccess(const std::string& processName, int pageNumber) const {
    auto it = processes.find(processName);
    if (it == processes.end()) return false;

    const auto& proc = it->second;
    return (pageNumber >= 0 && pageNumber < proc.pageCount);
}

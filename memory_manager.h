#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

class MemoryManager {
public:
    struct Page {
        int frameIndex = -1;     // Where it is in physical memory
        bool inMemory = false;   // Is it loaded?
        bool dirty = false;      // If written, mark as dirty (for backing store writes)
        std::vector<uint8_t> data; // Page data (simulate contents)
    };

    struct ProcessMemory {
        int pid;
        std::string processName;
        int allocatedBytes;
        int pageCount;
        int baseAddr;
        std::vector<Page> pageTable;
    };

    MemoryManager(int totalMemoryBytes = 4096, int pageSize = 256); // Default 4KB RAM

    int allocateProcess(const std::string& processName, int memoryBytes);
    void deallocateProcess(const std::string& processName);

    void accessPage(const std::string& processName, int pageNumber);
    void printProcessSMI();
    void printVMStat();
    bool isValidAccess(const std::string& processName, int pageNumber) const;

    void initializeBackingStore();  // Clear backing store file
    
    const std::unordered_map<std::string, ProcessMemory>& getProcesses() const {
        return processes;
    }

private:
    int totalMemory;
    int pageSize;
    int frameCount;

    std::vector<std::string> frames; // Holds processName@pageIndex in physical memory
    std::unordered_map<std::string, ProcessMemory> processes;

    std::ofstream backingStore;

    void pageIn(const std::string& processName, int pageNumber);
    void pageOut(int frameIndex);

    int findFreeFrame();
    int replacePage(); // FIFO for now

    void writePageToBackingStore(const std::string& processName, int pageNumber, const std::vector<uint8_t>& pageData);

    std::vector<std::string> pageHistory; // FIFO replacement
};

extern MemoryManager memManager;

#endif

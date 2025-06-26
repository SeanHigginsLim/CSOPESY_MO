#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class FCFSScheduler {
private:
    std::queue<Process*> readyQueue;
    std::vector<Process*> runningProcesses;
    std::vector<Process*> finishedProcesses;
    std::vector<std::thread> workerThreads;
    int coreCount;

public:
    FCFSScheduler(int cores = 4);
    void addProcess(Process* p);
    Process* findProcess(const std::string& name);
    void start();
    void stop();
    void workerThread(int coreId);
    void printStatus();
    const std::vector<Process*>& getRunningProcesses() const;
    const std::vector<Process*>& getFinishedProcesses() const;
};

extern std::mutex consoleMutex;
extern std::mutex processMutex;
extern std::atomic<bool> schedulerRunning;
extern FCFSScheduler scheduler;

#endif

#include "scheduler.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

std::mutex consoleMutex;
std::mutex processMutex;
std::atomic<bool> schedulerRunning(false);
FCFSScheduler scheduler;

// Global CPU tick counter
std::atomic<int> cpuTickCount(0);

FCFSScheduler::FCFSScheduler(int cores) : coreCount(cores) {
    runningProcesses.resize(coreCount, nullptr);
}

void FCFSScheduler::addProcess(Process* p) {
    std::lock_guard<std::mutex> lock(processMutex);
    readyQueue.push(p);
}

void FCFSScheduler::start() {
    schedulerRunning = true;
    for (int i = 0; i < coreCount; i++) {
        workerThreads.emplace_back(&FCFSScheduler::workerThread, this, i);
    }
}

void FCFSScheduler::stop() {
    schedulerRunning = false;
    for (auto& t : workerThreads) {
        if (t.joinable()) t.join();
    }

    for (auto* p : finishedProcesses) delete p;
}

// Scheduler thread
void FCFSScheduler::workerThread(int coreId) {
    while (schedulerRunning) {
        Process* p = nullptr;

        {
            std::lock_guard<std::mutex> lock(processMutex);
            if (!readyQueue.empty()) {
                p = readyQueue.front();
                readyQueue.pop();
                runningProcesses[coreId] = p;
                p->coreAssigned = coreId;
            }
        }

        if (p) {
            while (!p->isFinished && schedulerRunning) {
                // Simulate a CPU tick every 150ms
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                cpuTickCount++;

                if (p->isSleeping()) {
                    p->tickSleep();
                    continue;
                }

                p->executePrint(coreId, cpuTickCount.load());
            }

            // Mark process as finished and clean up
            {
                std::lock_guard<std::mutex> lock(processMutex);
                runningProcesses[coreId] = nullptr;
                finishedProcesses.push_back(p);
            }
        }
    }
}

void FCFSScheduler::printStatus() {
    std::lock_guard<std::mutex> lock(processMutex);
    std::lock_guard<std::mutex> consoleLock(consoleMutex);

    std::cout << "\n----------------------------------------------------\n";
    std::cout << "Running processes:\n";

    bool anyRunning = false;
    for (int i = 0; i < coreCount; i++) {
        Process* p = runningProcesses[i];
        if (p) {
            anyRunning = true;
            std::cout << std::left << std::setw(12) << p->name
                << " (" << p->timestamp << ")"
                << "   Core: " << i
                << "   " << p->currentLine << " / " << p->totalLines << "\n";
        }
    }
    if (!anyRunning) {
        std::cout << "None\n";
    }

    std::cout << "\nFinished processes:\n";
    if (finishedProcesses.empty()) {
        std::cout << "None\n";
    } else {
        for (auto* p : finishedProcesses) {
            std::cout << std::left << std::setw(12) << p->name
                << " (" << p->timestamp << ")"
                << "   Finished"
                << "   " << p->totalLines << " / " << p->totalLines << "\n";
        }
    }
    
    // std::cout << "\nTotal CPU Ticks: " << cpuTickCount.load() << "\n";
    std::cout << "----------------------------------------------------\n";
}
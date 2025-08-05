#include "scheduler.h"
#include <iostream>
#include <fstream> 
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>

std::mutex consoleMutex;
std::mutex processMutex;
std::atomic<bool> schedulerRunning(false);
FCFSScheduler scheduler;
std::atomic<int> cpuTickCount(0);

FCFSScheduler::FCFSScheduler(int cores) : coreCount(cores) {
    runningProcesses.resize(coreCount, nullptr);
}

void FCFSScheduler::addProcess(Process* p) {
    std::lock_guard<std::mutex> lock(processMutex);
    readyQueue.push(p);
}

Process* FCFSScheduler::findProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(processMutex);

    for (auto* p : runningProcesses) {
        if (p && p->name == name) return p;
    }

    std::queue<Process*> tempQueue = readyQueue;
    while (!tempQueue.empty()) {
        Process* p = tempQueue.front();
        tempQueue.pop();
        if (p && p->name == name) return p;
    }

    for (auto* p : finishedProcesses) {
        if (p && p->name == name) return p;
    }

    return nullptr;
}

const std::vector<Process*>& FCFSScheduler::getRunningProcesses() const {
    return runningProcesses;
}

const std::vector<Process*>& FCFSScheduler::getFinishedProcesses() const {
    return finishedProcesses;
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

    for (auto* p : finishedProcesses) {
        delete p;
    }
    finishedProcesses.clear();
}

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
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                cpuTickCount++;

                if (p->isSleeping()) {
                    p->tickSleep();
                    continue;
                }
                
                // Skip if no instructions or already finished
                if (p->instructions.empty() || p->isFinished) continue;
                p->executePrint(coreId, cpuTickCount.load());
            }

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

    std::cout << "----------------------------------------------------\n";
}

void FCFSScheduler::saveStatusToFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(processMutex);

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open log file: " << path << std::endl;
        return;
    }

    file << "CPU utilization: 100%\n";
    file << "Cores used: " << coreCount << "\n";
    file << "Cores available: 0\n";
    file << "========================================\n";

    file << "Running processes:\n";
    bool anyRunning = false;
    for (int i = 0; i < coreCount; i++) {
        Process* p = runningProcesses[i];
        if (p) {
            anyRunning = true;
            file << std::left << std::setw(12) << p->name
                 << " (" << p->timestamp << ")"
                 << "   Core: " << i
                 << "   " << p->currentLine << " / " << p->totalLines << "\n";
        }
    }
    if (!anyRunning) {
        file << "None\n";
    }

    file << "\nFinished processes:\n";
    if (finishedProcesses.empty()) {
        file << "None\n";
    } else {
        for (auto* p : finishedProcesses) {
            file << std::left << std::setw(12) << p->name
                 << " (" << p->timestamp << ")"
                 << "   Finished"
                 << "   " << p->totalLines << " / " << p->totalLines << "\n";
        }
    }

    file << "========================================\n";
    file.close();

    std::lock_guard<std::mutex> consoleLock(consoleMutex);
    std::cout << "Report generated at " << path << "!\n";
}

#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <fstream>
#include <vector>
#include <stack>

// Header file for process.cpp
struct Process {
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;
    int coreAssigned;
    bool isFinished;
    std::ofstream logFile;

    // NEW: Store log entries in memory for screen display
    std::deque<std::string> logs;      // Show recent logs
    static const size_t MAX_LOGS = 10; // Only keep last 10

    // Sleep state
    bool sleeping = false;
    int sleepTicksRemaining = 0;
    
    // Instructions and control flow
    std::vector<std::string> instructions;
    int instructionPointer = 0;

    // For loop pointer and counter
    std::stack<int> forStartPointers;
    std::stack<int> forLoopCounters;

    Process(std::string n, int total);  // Process constructor
    ~Process();                         // Prcoess destructor

    void executePrint(int core, int tick);  // Print into logs
    void sleepFor(int ticks);               // Set sleeping state
    void tickSleep();                       // Decrement tick
    bool isSleeping() const;                // Check if sleeping
};
#endif
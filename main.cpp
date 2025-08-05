#include "console.h"
#include "config.h"
#include "scheduler.h"
#include "variable_manager.h"
#include "memory_manager.h"
#include "process.h"
#include <regex>
#include <iostream>
#include <sstream>
#include <string>

// Helper function to enter the process screen
void enterProcessScreen(Process* process) {
    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif

    if (!process) {
        std::cout << "Error: Process pointer is null.\n";
        return;
    }

    std::string cmd;

    // Print process info upfront
    std::cout << "=== Process Information ===\n";
    std::cout << "Name          : " << process->name << "\n";
    std::cout << "Total Lines   : " << process->totalLines << "\n";
    std::cout << "Current Line  : " << process->currentLine << "\n";
    std::cout << "Core Assigned : " << (process->coreAssigned >= 0 ? std::to_string(process->coreAssigned) : "None") << "\n";
    std::cout << "Sleeping      : " << (process->isSleeping() ? "Yes" : "No") << "\n";
    std::cout << "Finished      : " << (process->isFinished ? "Yes" : "No") << "\n";
    std::cout << "============================\n";

    // Clear any leftover input before starting the loop
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    while (true) {
        std::cout << "\n(" << process->name << ") > " << std::flush;
        std::getline(std::cin, cmd);

        if (cmd.empty()) continue;

        if (cmd == "exit") break;
        else if (cmd == "process-smi") {
            std::cout << "Current instruction line: " << process->currentLine << " / " << process->totalLines << "\n";
            std::cout << "Sleeping: " << (process->isSleeping() ? "Yes" : "No") << "\n";
            std::cout << "Finished: " << (process->isFinished ? "Yes" : "No") << "\n";
        } else {
            std::cout << "Unknown command inside process screen.\n";
        }
    }
}

// Main command interpreter
void verifyCommand(const std::string& input) {
    if (!systemConfig.initialized) {
        if (input == "initialize") {
            Console::initializeFromConfig();
        } else {
            std::cout << "Please run 'initialize' first.\n";
        }
        return;
    }

    // screen -c <name> <size> "<instruction1;instruction2>"
    if (input.rfind("screen -c", 0) == 0) {
        std::istringstream iss(input);
        std::string screen, dash_c, process_name, memory_size_str, instructions_str;

        iss >> screen >> dash_c >> process_name >> memory_size_str;
        std::getline(iss, instructions_str);

        if (screen != "screen" || dash_c != "-c" || process_name.empty() || memory_size_str.empty() || instructions_str.empty()) {
            std::cout << "Invalid screen -c command format.\n";
            return;
        }

        instructions_str.erase(0, instructions_str.find_first_not_of(" \t"));
        instructions_str.erase(instructions_str.find_last_not_of(" \t") + 1);

        if (instructions_str.front() == '"' && instructions_str.back() == '"') {
            instructions_str = instructions_str.substr(1, instructions_str.length() - 2);
        } else {
            std::cout << "Instructions must be in double quotes.\n";
            return;
        }

        int mem_size;
        try {
            mem_size = std::stoi(memory_size_str);
        } catch (...) {
            std::cout << "Invalid memory size.\n";
            return;
        }

        std::vector<std::string> instructions;
        std::stringstream ss(instructions_str);
        std::string instr;
        while (std::getline(ss, instr, ';')) {
            instr.erase(0, instr.find_first_not_of(" \t"));
            instr.erase(instr.find_last_not_of(" \t") + 1);
            if (!instr.empty()) instructions.push_back(instr);
        }

        if (instructions.empty() || instructions.size() > 50) {
            std::cout << "Instruction list must be between 1 and 50 instructions.\n";
            return;
        }

        int pid = memManager.allocateProcess(process_name, mem_size);
        if (pid == -1) {
            std::cout << "[ERROR] Could not allocate memory for process.\n";
            return;
        }

        Process* p = new Process(process_name, instructions.size());
        p->instructions = instructions;
        p->pid = pid;

        // Assign baseAddr and limitAddr from MemoryManager's record for this process
        const auto& processes = memManager.getProcesses();
        auto it = processes.find(process_name);
        if (it == processes.end()) {
            std::cout << "[ERROR] Process info not found for " << process_name << "\n";
            return;
        }
        const auto& procMem = it->second;
        p->baseAddr = procMem.baseAddr;
        p->limitAddr = procMem.allocatedBytes;

        scheduler.addProcess(p);

        std::cout << "Process '" << process_name << "' created with " << instructions.size()
                  << " instruction(s) and " << mem_size << " bytes memory.\n";
        return;
    }

    // screen -s <name> <memory_size>
    else if (input.rfind("screen -s", 0) == 0) {
        std::istringstream iss(input);
        std::string cmd, dash_s, name, memStr;
        iss >> cmd >> dash_s >> name >> memStr;

        if (cmd != "screen" || dash_s != "-s" || name.empty() || memStr.empty()) {
            std::cout << "Usage: screen -s <name> <memory_size>\n";
            return;
        }

        int memSize;
        try {
            memSize = std::stoi(memStr);
        } catch (...) {
            std::cout << "Invalid memory size.\n";
            return;
        }

        int pid = memManager.allocateProcess(name, memSize);
        if (pid == -1) {
            std::cout << "[ERROR] Could not allocate memory for process.\n";
            return;
        }

        Process* process = new Process(name, 10);
        process->pid = pid;

        // Assign baseAddr and limitAddr
        const auto& processes = memManager.getProcesses();
        auto it = processes.find(name);
        if (it == processes.end()) {
            std::cout << "[ERROR] Process memory info not found for " << name << "\n";
            return;
        }
        const auto& procMem = it->second;
        process->baseAddr = procMem.baseAddr;
        process->limitAddr = procMem.allocatedBytes;


        scheduler.addProcess(process);
        std::cout << "Created new process: " << name << " with " << memSize << " bytes\n";

        enterProcessScreen(process);
        return;
    }

    else if (input.rfind("screen -r", 0) == 0) {
        if (input.size() <= 10) {
            std::cout << "Please provide a process name. Usage: screen -r <name>\n";
            return;
        }
        std::string processName = input.substr(10);
        Process* process = scheduler.findProcess(processName);

        if (!process || process->isFinished) {
            std::cout << "Process " << processName << " not found.\n";
            return;
        }

        int pageToCheck = process->currentLine / 4;
        if (!memManager.isValidAccess(process->name, pageToCheck)) {
            std::cout << "[MEMORY ERROR] Access violation: Process " << process->name << " attempted invalid memory access.\n";
            return;
        }

        enterProcessScreen(process);
    }

    else if (input == "screen -ls") {
        scheduler.printStatus();
    } else if (input == "scheduler-start") {
        std::cout << "Starting scheduler test..." << std::endl;
        Console::initializeTestProcesses();
        scheduler.printStatus();
    } else if (input == "scheduler-stop") {
        std::cout << "Stopping scheduler..." << std::endl;
        scheduler.stop();
    } else if (input == "process-smi") {
        memManager.printProcessSMI();
    } 
    else if (input == "vmstat") {
        memManager.printVMStat();
    } else if (input == "report-util") {
        scheduler.saveStatusToFile("C:/csopesy-log.txt");
    } else if (input == "clear") {
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
        Console::printHeader();
    } else if (input == "exit") {
        scheduler.stop();
        std::cout << "Exiting..." << std::endl;
        exit(0);
    } else {
        std::cout << "[ERROR] Unknown command.\n";
    }
}


int main() {
    Console::printHeader();
    std::string command;
    while (true) {
        command = Console::acceptCommand();
        verifyCommand(command);
    }
    return 0;
}

#include "console.h"
#include "scheduler.h"
#include "variable_manager.h"
#include "memory_manager.h"
#include <regex>
#include <iostream>
#include <sstream>

void verifyCommand(const std::string& input) {
    std::smatch match;

    if (input == "initialize") {
        std::cout << "Initializing..." << std::endl;
        // Console::initializeTestProcesses(); // Disabled
    } 
    else if (std::regex_match(input, match, std::regex(R"(screen\s+-s\s+(\w+)\s+(\d+))"))) {
        std::string processName = match[1];
        int memorySize = std::stoi(match[2]);

        if ((memorySize < 64 || memorySize > 65536) || (memorySize & (memorySize - 1)) != 0) {
            std::cout << "[ERROR] Invalid memory allocation size: " << memorySize << " bytes\n";
            std::cout << "Allowed: powers of 2, range [64, 65536]\n";
            return;
        }

        int pid = memManager.allocateProcess(processName, memorySize);
        if (pid != -1) {
            Process* p = new Process(processName, memorySize); // Dynamic memorySize
            p->instructions = Console::generateRandomInstructions(processName);
            scheduler.addProcess(p);
            std::cout << "[INFO] Process " << processName << " scheduled with PID " << pid << "\n";
        }

    } 
    else if (input == "screen -ls") {
        scheduler.printStatus();
    } 
    else if (input == "scheduler-start") {
        std::cout << "Starting scheduler test..." << std::endl;
        Console::initializeTestProcesses();
        scheduler.printStatus();
    } 
    else if (input == "scheduler-stop") {
        std::cout << "Stopping scheduler..." << std::endl;
        scheduler.stop();
    } 
    else if (input == "process-smi") {
        memManager.printProcessSMI();
    } 
    else if (input == "vmstat") {
        memManager.printVMStat();
    } 
    else if (input == "clear") {
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
        Console::printHeader();
    } 
    else if (input == "exit") {
        scheduler.stop();
        std::cout << "Exiting..." << std::endl;
        exit(0);
    } 
    else {
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

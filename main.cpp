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

    std::string cmd;
    while (true) {
        std::cout << "\n(" << process->name << ") > ";
        std::getline(std::cin, cmd);

        if (cmd.empty()) continue;

        if (cmd == "exit") {
            break;
        } else if (cmd == "process-smi") {
            std::cout << "\nProcess Name: " << process->name;
            std::cout << "\nTimestamp: " << process->timestamp;
            std::cout << "\nInstruction: " << process->currentLine << " / " << process->totalLines;

            if (process->isFinished) {
                std::cout << "\nStatus: Finished!";
            }

            std::cout << "\n\nRecent Logs:\n";
            for (const auto& log : process->logs) {
                std::cout << log << "\n";
            }
        } else {
            std::cout << "Unknown command inside process screen.\n";
        }
    }

    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif
    Console::printHeader();
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
   if (input.rfind("screen -s", 0) == 0) {
        if (input.size() <= 10) {
            std::cout << "Please provide a process name. Usage: screen -s <name>\n";
            return;
        }
        std::string processName = input.substr(10);
        Process* process = scheduler.findProcess(processName);

        if (!process) {
            // Create a new process with dummy totalLines (e.g., 10)
            process = new Process(processName, 10);
            scheduler.addProcess(process);
            std::cout << "Created new process: " << processName << "\n";
        }

        enterProcessScreen(process);
    } else if (input.rfind("screen -r", 0) == 0) {
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

        enterProcessScreen(process);
    } else if (input == "screen -ls") {
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

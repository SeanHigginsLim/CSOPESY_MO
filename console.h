#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>

// Header file for console.cpp
class Console {
public:
    static void printHeader();              // Prints the command line header
    static std::string acceptCommand();     // Accepts user command
    static void initializeTestProcesses();  // Runs the test processes

    static std::vector<std::string> generateRandomInstructions(const std::string& processName); // Added to console.h so it can be called in main
    static void initializeFromConfig(); 
    static void handleScreenCreateCommand(const std::string& command);
};

#endif
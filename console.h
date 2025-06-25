#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>

// Header file for console.cpp
class Console {
public:
    static void printHeader();              // Prints the command line header
    static std::string acceptCommand();     // Accepts user command
    static void initializeTestProcesses();  // Runs the test processes
};

#endif
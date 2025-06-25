#include "console.h"
#include "scheduler.h"
#include "variable_manager.h"
#include <regex>
#include <iostream>

void verifyCommand(const std::string& input) {
    if (input == "initialize") {
        std::cout << "Initializing..." << std::endl;
        // Console::initializeTestProcesses(); // Should scan from config file
    } else if(input.rfind("screen -s", 0) == 0) {
        // std::smatch match;

        // // PRINT\( Matches PRINT(
        // // \"([^\"]*)\" Matches a quoted message
        // // \s* Allows for optional white spaces
        // // \+ Matches +
        // // \s* Allows for optional white spaces
        // // ([^)]+) Matches variable name until )
        // // \) Matches )
        // std::regex process(R"(PRINT\(\"([^\"]*)\"\s*\+\s*([^)]+)\))");

        // if (std::regex_match(input, match, process)) {
        // }
    } else if (input == "screen -ls") {
        scheduler.printStatus(); // Prints status of scheduler processes
    } else if (input == "scheduler-start") {
        std::cout << "Starting scheduler test..." << std::endl;
        Console::initializeTestProcesses(); // Starts the scheduler
        scheduler.printStatus();
    } else if (input == "scheduler-stop") {
        std::cout << "Stopping scheduler..." << std::endl;
        scheduler.stop(); // Stops the scheduler
    } else if (input == "clear") {
        #ifdef _WIN32
        system("cls"); // Clears the console
        #else
        system("clear");
        #endif
        Console::printHeader();
    } else if (input == "exit") {
        scheduler.stop();
        std::cout << "Exiting..." << std::endl;
        exit(0);
    } else {
        std::cout << "Unknown command." << std::endl;
    }
}

int main() {
    Console::printHeader(); // Prints command line header
    std::string command;
    while (true) {
        command = Console::acceptCommand(); // Accepts user command
        verifyCommand(command); // Verifies user command
    }
    return 0;
}
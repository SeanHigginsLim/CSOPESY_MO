#include "console.h"
#include "scheduler.h"
#include "process.h"
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

// Global RNG
std::random_device rd;
std::mt19937 rng(rd());

// Random integer between min and max
int randomInt(int min, int max) {
    std::uniform_int_distribution<> dist(min, max);
    return dist(rng);
}

// Random variable name
std::string generateRandomVarName() {
    static const std::string charset = "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<> lenDist(1, 3);
    int len = lenDist(rng);
    std::string result;
    for (int i = 0; i < len; ++i)
        result += charset[randomInt(0, charset.size() - 1)];
    return "var_" + result;
}

void Console::printHeader() {
    std::cout << R"(
.---------------------------------------------------------------------------.
|  ______     _______.  ______   .______    _______     _______.____    ____|
| /      |   /       | /  __  \  |   _  \  |   ____|   /       |\   \  /   /|
||  ,----'  |   (----`|  |  |  | |  |_)  | |  |__     |   (----` \   \/   / |
||  |        \   \    |  |  |  | |   ___/  |   __|     \   \      \_    _/  |
||  `----.----)   |   |  `--'  | |  |      |  |____.----)   |       |  |    |
| \______|_______/     \______/  | _|      |_______|_______/        |__|    |
'---------------------------------------------------------------------------'
Hello, welcome to CSOPESY commandline!
)";
}

std::string Console::acceptCommand() {
    std::string command;
    std::cout << R"(
List of Commands:
    - initialize
    - screen
        > screen -s
        > screen -r
        > screen -ls
    - scheduler-start
    - scheduler-stop
    - process-smi
    - vmstat
    - report-util
    - clear
    - exit

Enter a command: )";
    std::getline(std::cin, command);
    return command;
}

// Recursive FOR loop generator
std::string generateNestedFor(int depth = 0) {
    int repeat = randomInt(2, 4); // Repeat count for this loop
    std::vector<std::string> body;
    int instrCount = randomInt(2, 4);

    // std::cout << "\n\nInstruction Count: " << instrCount << "\n\n";
    for (int i = 0; i < instrCount; ++i) {
        int type = randomInt(1, 5); // Avoid recursive FOR here (only upper levels)

        std::string var1 = generateRandomVarName();
        std::string var2 = generateRandomVarName();
        std::string var3 = generateRandomVarName();
        int val1 = randomInt(0, 65535);
        int val2 = randomInt(0, 65535);
        int sleepTicks = randomInt(1, 10);

        switch (type) {
            case 1:
                body.push_back("DECLARE(" + var1 + ", " + std::to_string(val1) + ")");
                break;
            case 2:
                body.push_back("ADD(" + var1 + ", " + std::to_string(val1) + ", " + std::to_string(val2) + ")");
                break;
            case 3:
                if (val2 > val1) std::swap(val1, val2);
                body.push_back("SUBTRACT(" + var1 + ", " + std::to_string(val1) + ", " + std::to_string(val2) + ")");
                break;
            case 4:
                body.push_back("SLEEP(" + std::to_string(sleepTicks) + ")");
                break;
            case 5:
                body.push_back("PRINT(\"Hello world!\")");
                break;
        }
    }

    // Optionally add a nested FOR if depth < 3
    if (depth < 3 && randomInt(0, 1)) {
        std::string nested = generateNestedFor(depth + 1);
        body.push_back(nested);
    }

    std::string result = "FOR([";
    for (size_t i = 0; i < body.size(); ++i) {
        result += body[i];
        if (i != body.size() - 1) result += " @@ ";
    }
    result += "], " + std::to_string(repeat) + ")";
    // std::cout << "\n\n'" << result << "'\n\n";
    return result;
}

// Randomized instruction list generator
std::vector<std::string> Console::generateRandomInstructions(const std::string& processName) {
    std::vector<std::string> instructions;
    std::vector<std::string> declaredVars;
    int count = randomInt(5, 10);

    for (int i = 0; i < count; ++i) {
        int type = randomInt(1,6); // 1-DECLARE, 2-ADD, 3-SUBTRACT, 4-SLEEP, 5-PRINT, 6-FOR
        std::string var1 = generateRandomVarName();
        std::string var2 = generateRandomVarName();
        std::string var3 = generateRandomVarName();
        int val1 = randomInt(0, 65535);
        int val2 = randomInt(0, 65535);
        int sleepTicks = randomInt(1, 10);

        switch (type) {
            case 1: // DECLARE
                instructions.push_back("DECLARE(" + var1 + ", " + std::to_string(val1) + ")");
                declaredVars.push_back(var1);
                break;

            case 2: // ADD
                instructions.push_back("ADD(" + var1 + ", " + std::to_string(val1) + ", " + std::to_string(val2) + ")");
                declaredVars.push_back(var1);
                break;

            case 3: // SUBTRACT
                if (val2 > val1) std::swap(val1, val2); // avoid underflow
                instructions.push_back("SUBTRACT(" + var1 + ", " + std::to_string(val1) + ", " + std::to_string(val2) + ")");
                declaredVars.push_back(var1);
                break;

            case 4: // SLEEP
                instructions.push_back("SLEEP(" + std::to_string(sleepTicks) + ")");
                break;

            case 5: // PRINT
                if (!declaredVars.empty() && randomInt(0, 1)) {
                    std::string randVar = declaredVars[randomInt(0, declaredVars.size() - 1)];
                    instructions.push_back("PRINT(\"Value is: \" + " + randVar + ")");
                } else {
                    instructions.push_back("PRINT(\"Hello world!\")");
                }
                break;

            case 6: // FOR
                std::string loop = generateNestedFor(0); // max depth 3
                instructions.push_back(loop);
                break;
        }
    }

    return instructions;
}

// Initialize test processes with random instructions
void Console::initializeTestProcesses() {
    for (int i = 1; i <= 5; ++i) {
        std::string name = "process" + std::to_string(i);
        Process* p = new Process(name, 10000); // 10000 cycles so you can see them executing one by one
        p->instructions = generateRandomInstructions(name);

        // Print instructions when created
        for (const auto& instr : p->instructions) {
            // std::cout << "[" << name << "] Instruction: " << instr << "\n";
        }

        scheduler.addProcess(p);
    }
    scheduler.start();
}
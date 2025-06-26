#include "process.h"
#include "variable_manager.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <iostream>
#include <regex>

extern std::mutex processMutex;

// FOR loop helper structure
struct Loop {
    std::string body;
    int repeats;
};

// Helper: split instructions by "@@" delimiter
std::vector<std::string> splitInstructions(const std::string& body) {
    std::vector<std::string> result;
    size_t start = 0, end;
    while ((end = body.find("@@", start)) != std::string::npos) {
        result.push_back(body.substr(start, end - start));
        start = end + 2;
    }
    result.push_back(body.substr(start));
    return result;
}

// Fixed nested FOR extractor with bracket tracking
std::vector<Loop> extractNestedLoops(const std::string& cmd, const std::string& processName) {
    std::vector<Loop> loops;
    std::string current = cmd;

    std::cout << "[DEBUG] Parsing FOR loop for process: " << processName << "\n";

    for (int depth = 0; depth < 4; ++depth) {
        std::smatch match;
        std::regex pattern(R"(FOR\(\[(.*)\],\s*(\d+)\))");
        if (!std::regex_match(current, match, pattern)) break;

        std::string body = match[1].str();
        int repeat = std::stoi(match[2].str());

        std::cout << "[DEBUG] Matched FOR at depth " << depth << " with repeat = " << repeat << "\n";

        // Bracket balancing to find deepest nested FOR
        int forStart = body.find("FOR([");
        int openBrackets = 0;
        int nestedStart = -1;
        int nestedEnd = -1;

        for (size_t i = 0; i < body.size(); ++i) {
            if (body.substr(i, 5) == "FOR([") {
                if (openBrackets == 0) nestedStart = i;
                openBrackets++;
                i += 4;
            } else if (body[i] == ']') {
                openBrackets--;
                if (openBrackets == 0 && nestedStart != -1) {
                    // look forward for ')'
                    size_t closeParen = body.find(')', i);
                    if (closeParen != std::string::npos) {
                        nestedEnd = closeParen;
                        break;
                    }
                }
            }
        }

        if (nestedStart != -1 && nestedEnd != -1) {
            std::string outer = body.substr(0, nestedStart - 4); // Remove trailing "@@"
            std::string nested = body.substr(nestedStart, nestedEnd - nestedStart + 1);

            std::cout << "[DEBUG] Level " << depth << " body (outer): " << outer << "\n";
            loops.push_back({outer, repeat});
            current = nested;
        } else {
            std::cout << "[DEBUG] Final level body: " << body << "\n";
            loops.push_back({body, repeat});
            break;
        }
    }

    std::reverse(loops.begin(), loops.end());

    std::cout << "[DEBUG] Detected " << loops.size() << " nested FOR loop(s)\n";
    for (int i = 0; i < loops.size(); ++i) {
        std::cout << "  Loop Level " << i << " - Repeats: " << loops[i].repeats << "\n";
        for (auto& ins : splitInstructions(loops[i].body)) {
            std::cout << "    [L" << i << "] " << ins << "\n";
        }
    }

    return loops;
}

Process::Process(std::string n, int total)
    : name(n), currentLine(0), totalLines(total), coreAssigned(-1), isFinished(false) {
    time_t now = time(nullptr);
    tm* localTime = localtime(&now);
    std::stringstream ss;
    ss << std::put_time(localTime, "%m/%d/%Y %I:%M:%S%p");
    timestamp = ss.str();
    logFile.open(name + "_log.txt");
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file for " << name << "\n";
    }
}

Process::~Process() {
    if (logFile.is_open()) logFile.close();
}

void Process::executePrint(int core, int tick) {
    std::lock_guard<std::mutex> lock(processMutex);

    if (instructionPointer >= instructions.size()) {
        if (!forLoopCounters.empty() && forLoopCounters.top() > 1) {
            forLoopCounters.top()--;
            instructionPointer = forStartPointers.top();
        } else if (!forLoopCounters.empty()) {
            forLoopCounters.pop();
            forStartPointers.pop();
        } else {
            isFinished = true;
            return;
        }
    }

    std::string cmd = instructions[instructionPointer++];
    // std::cout << "[" << name << "] Executing: " << cmd << "\n";

    time_t now = time(nullptr);
    tm* localTime = localtime(&now);
    std::stringstream ts;
    ts << std::put_time(localTime, "%m/%d/%Y %I:%M:%S%p");
    std::string timestamp = ts.str();

    if (cmd.rfind("SLEEP(", 0) == 0) {
        std::smatch match;
        std::regex pattern(R"(SLEEP\((\d+)\))");

        if (std::regex_match(cmd, match, pattern)) {
            int ticks = std::stoi(match[1]);
            sleepFor(std::min(255, std::max(0, ticks)));
            logFile << "(" << timestamp << ") Core:" << core
                << " \"SLEEP(" << ticks << ") from " << name << "\"\n";
            logFile.flush();
            return;
        }
    } else if (cmd.rfind("PRINT(", 0) == 0) {
        std::smatch match;
        std::regex withVar(R"(PRINT\(\"([^\"]*)\"\s*\+\s*([^)]+)\))");
        std::regex justMsg(R"(PRINT\(\"([^\"]*)\"\))");

        std::string logEntry;

        if (std::regex_match(cmd, match, withVar)) {
            std::string msg = match[1];
            std::string var = match[2];
            int val = varManager.getValue(var);
            logEntry = "(" + timestamp + ") Core:" + std::to_string(core) + " \"" + msg + std::to_string(val) + " from " + name + "\"";
        } else if (std::regex_match(cmd, match, justMsg)) {
            std::string msg = match[1];
            logEntry = "(" + timestamp + ") Core:" + std::to_string(core) + " \"" + msg + " from " + name + "\"";
        }

        if (!logEntry.empty()) {
            logFile << logEntry << "\n";
            logFile.flush();

            // NEW: Save to in-memory log list
            logs.push_back(logEntry);
            if (logs.size() > MAX_LOGS) logs.pop_front();
        }
    } else if (cmd.rfind("DECLARE(", 0) == 0) {
        std::smatch match;
        std::regex pattern(R"(DECLARE\(([^,]+),\s*([^)]+)\))");
        if (std::regex_match(cmd, match, pattern)) {
            std::string var = match[1];
            int val = varManager.getValue(match[2]);
            varManager.declare(var, VariableManager::clamp16(val));
            logFile << "(" << timestamp << ") Core:" << core
                << " \"DECLARE " << var << " = " << VariableManager::clamp16(val) << " from " << name << "\"\n";
            logFile.flush();
        }
    } else if (cmd.rfind("ADD(", 0) == 0) {
        std::smatch match;
        std::regex pattern(R"(ADD\(([^,]+),\s*([^,]+),\s*([^)]+)\))");
        if (std::regex_match(cmd, match, pattern)) {
            std::string var1 = match[1];
            int val2 = varManager.getValue(match[2]);
            int val3 = varManager.getValue(match[3]);
            int result = val2 + val3;
            varManager.declare(var1, VariableManager::clamp16(result));
            logFile << "(" << timestamp << ") Core:" << core
                << " \"ADD(" << var1 << ", " << val2 << ", " << val3 << ") from " << name << "\"\n";
            logFile.flush();
        }
    } else if (cmd.rfind("SUBTRACT(", 0) == 0) {
        std::smatch match;
        std::regex pattern(R"(SUBTRACT\(([^,]+),\s*([^,]+),\s*([^)]+)\))");
        if (std::regex_match(cmd, match, pattern)) {
            std::string var1 = match[1];
            int val2 = varManager.getValue(match[2]);
            int val3 = varManager.getValue(match[3]);
            int result = VariableManager::clamp16(std::max(0, val2 - val3));
            varManager.declare(var1, VariableManager::clamp16(result));
            logFile << "(" << timestamp << ") Core:" << core
                << " \"SUBTRACT(" << var1 << ", " << val2 << ", " << val3 << ") from " << name << "\"\n";
            logFile.flush();
        }
    } else if (cmd.rfind("FOR([", 0) == 0) {
        std::cout << "\n\nFOR loop structure: " << cmd << "\n\n";
        std::cout << "[DEBUG] Parsing FOR loop for process: " << name << "\n";
        std::vector<Loop> loops = extractNestedLoops(cmd, name);
        std::cout << "[DEBUG] Detected " << loops.size() << " nested FOR loop(s)\n";

        std::vector<std::string> result;

        for (size_t i = 0; i < loops.size(); ++i) {
            std::cout << "  Loop Level " << i << " - Repeats: " << loops[i].repeats << "\n";
            auto lines = splitInstructions(loops[i].body);
            for (const auto& line : lines) {
                std::cout << "    [L" << i << "] " << line << "\n";
            }
        }

        // Build nested loops
        switch (loops.size()) {
            case 1: {
                auto lvl0 = splitInstructions(loops[0].body);
                for (int i = 0; i < loops[0].repeats; ++i)
                    result.insert(result.end(), lvl0.begin(), lvl0.end());
                break;
            }
            case 2: {
                auto l0 = splitInstructions(loops[0].body);
                auto l1 = splitInstructions(loops[1].body);
                for (int i = 0; i < loops[1].repeats; ++i) {
                    result.insert(result.end(), l1.begin(), l1.end());
                    for (int j = 0; j < loops[0].repeats; ++j)
                        result.insert(result.end(), l0.begin(), l0.end());
                }
                break;
            }
            case 3: {
                auto l0 = splitInstructions(loops[0].body);
                auto l1 = splitInstructions(loops[1].body);
                auto l2 = splitInstructions(loops[2].body);
                for (int i = 0; i < loops[2].repeats; ++i) {
                    result.insert(result.end(), l2.begin(), l2.end());
                    for (int j = 0; j < loops[1].repeats; ++j) {
                        result.insert(result.end(), l1.begin(), l1.end());
                        for (int k = 0; k < loops[0].repeats; ++k)
                            result.insert(result.end(), l0.begin(), l0.end());
                    }
                }
                break;
            }
            case 4: {
                auto l0 = splitInstructions(loops[0].body);
                auto l1 = splitInstructions(loops[1].body);
                auto l2 = splitInstructions(loops[2].body);
                auto l3 = splitInstructions(loops[3].body);
                for (int i = 0; i < loops[3].repeats; ++i) {
                    result.insert(result.end(), l3.begin(), l3.end());
                    for (int j = 0; j < loops[2].repeats; ++j) {
                        result.insert(result.end(), l2.begin(), l2.end());
                        for (int k = 0; k < loops[1].repeats; ++k) {
                            result.insert(result.end(), l1.begin(), l1.end());
                            for (int l = 0; l < loops[0].repeats; ++l)
                                result.insert(result.end(), l0.begin(), l0.end());
                        }
                    }
                }
                break;
            }
        }

        instructions.insert(instructions.begin() + instructionPointer, result.begin(), result.end());
        return;
    }

    currentLine++;
    if (currentLine >= totalLines || (instructionPointer >= instructions.size() && forLoopCounters.empty())) {
        isFinished = true;
    }
}

// Changes process state to sleep, initializes sleepTicksRemaining value
void Process::sleepFor(int ticks) {
    sleeping = true;
    sleepTicksRemaining = ticks;
}

// Checks the sleep state of a process, and reduces the sleep timer until 0
void Process::tickSleep() {
    if (sleeping && --sleepTicksRemaining <= 0) {
        sleeping = false;
    }
}

// Checks the sleep state of a process
bool Process::isSleeping() const {
    return sleeping;
}
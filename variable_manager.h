#ifndef VARIABLE_MANAGER_H
#define VARIABLE_MANAGER_H

#include <unordered_map>
#include <string>
#include <cstdint>

class VariableManager {
private:
    std::unordered_map<std::string, int> variables;

public:
    int getValue(const std::string& token);
    void declare(const std::string& varName, uint16_t value);
    void printAll();
    bool has(const std::string& varName);
    
    static uint16_t clamp16(int value);
};

extern VariableManager varManager;

#endif
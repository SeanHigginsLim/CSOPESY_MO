#include "variable_manager.h"
#include <iostream>
#include <sstream>

VariableManager varManager;

int VariableManager::getValue(const std::string& token) {
    std::istringstream iss(token);
    int value;
    if (iss >> value) return value; // literal number
    return variables[token]; // defaults to 0 if not found
}

void VariableManager::declare(const std::string& varName, uint16_t value) {
    variables[varName] = value;
}

bool VariableManager::has(const std::string& varName) {
    return variables.find(varName) != variables.end();
}

void VariableManager::printAll() {
    std::cout << "Declared Variables:\n";
    for (const auto& [key, val] : variables) {
        std::cout << key << " = " << val << "\n";
    }
}

uint16_t VariableManager::clamp16(int value) {
    return static_cast<uint16_t>(std::max(0, std::min(65535, value)));
}
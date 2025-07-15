#ifndef CONSOLE_H
#define CONSOLE_H
#include <string>
#include "scheduler.h"

class Console {
public:
    void run();
private:
    Scheduler scheduler;
    void printHeader();
};

#endif
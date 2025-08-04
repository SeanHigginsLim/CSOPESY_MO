# CSOPESY M02 Multitasking OS Emulator

Created by: 
Instructor: Dr. Neil Patrick Del Gallego  
Course: CSOPESY – Operating Systems  
Term: Term 3, A.Y. 2024-2025

---

## Project Overview

This is a command-line OS emulator that supports **multitasking, virtual memory, and demand paging**.  
It simulates memory management through paging, process scheduling, and memory visualization, similar to real-world OS tools like `nvidia-smi`, `vmstat`, and Linux `screen`.

---

## Features
* Command-line shell with Linux-style commands
* Demand paging memory allocator
* Backing store for swapped pages (`csopesy-backing-store.txt`)  
* Symbol table for process variables
* `screen -c` with user-defined instructions
* `process-smi` and `vmstat` memory visualizers
* Memory access violation handling  

---

## Build Instructions
### Requirements
- C++11 or higher
- g++ / clang++ or Visual Studio

### Compile with g++ (Linux/macOS/WSL)
```bash
g++ -std=c++11 -o os_emulator main.cpp console.cpp [add...]

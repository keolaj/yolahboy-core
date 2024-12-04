#pragma once
#include "../mmu/mmu.h"
#include "operations.h"

void init_cpu(Cpu* cpu);
Cycles cpu_step(Emulator* emu, Operation op);
Operation get_operation(Emulator* emu);
void print_registers(Cpu* cpu);
Cycles run_halted(Emulator* emu);

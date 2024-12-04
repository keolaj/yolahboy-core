#pragma once

#include "global_definitions.h"

void update_emu_controller(Emulator* emu, Controller controller);
int init_emulator(Emulator* emu, int sample_rate, int buffer_size);
void destroy_emulator(Emulator* emu);
int step(Emulator* emu);
bool cartridge_loaded(Emulator* emu);
void skip_bootrom(Emulator* emu);
#pragma once
#include "../global_definitions.h"
#include "gpu_definitions.h"
#include "../mmu/mmu.h"

int init_gpu(Gpu* gpu);
void destroy_gpu(Gpu* gpu);
void gpu_step(Emulator* emu, u8 cycles);
u8 read_tile(Emulator* emu, int tile_index, u8 x, u8 y);
u32 pixel_from_palette(u8 palette, u8 id);

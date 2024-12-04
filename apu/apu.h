#pragma once
#include "../global_definitions.h"

void init_apu(Apu* apu, int sample_rate, int buffer_size);
void apu_step(Apu* apu, u8 cycles);
void destroy_apu(Apu* apu);
float* get_buffer(Apu* apu);
void trigger_channel(Channel* channel);

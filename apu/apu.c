#include <stdlib.h>
#include "apu.h"

static bool duty_cycles[4][16] = {
	{ 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0 },
	{ 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0 },
	{ 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0 },
	{ 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1 }
};


void init_apu(Apu* apu, int sample_rate, int buffer_size) {

	memset(apu, 0, sizeof(Apu)); // This resets all Apu registers and controls
	
	apu->buffer = (float*)malloc(sizeof(float) * buffer_size * 2); // Allocate main buffers
	if (apu->buffer == NULL) {
		return;
	}

	memset(apu->buffer, 0, buffer_size * 2 * sizeof(float));

	apu->lfsr = 0xFFFF;

	apu->buffer_size = buffer_size;
	apu->sample_rate = sample_rate;
}

void destroy_apu(Apu* apu) {
	if (apu->buffer != NULL) free(apu->buffer);
}

void div_apu_step(Apu* apu, u8 cycles) {
	apu->div_apu_internal += cycles;
	if (apu->div_apu_internal >= 8192) {
		apu->div_apu_internal -= 8192;
		++apu->div_apu_counter;
		if (apu->div_apu_counter == 8) apu->div_apu_counter = 0;

		// Timer Controls
		if ((apu->div_apu_counter + 1) % 2 == 0) { // length timer
			for (int i = 0; i < 4; ++i) {
				if (apu->channel[i].length_enabled && apu->channel[i].enabled) {
					++apu->channel[i].length_timer;
				}
			}
		}
		if (apu->div_apu_counter == 7) { // envelope timer
			for (int i = 0; i < 4; ++i) {
				if (apu->channel[i].env_sweep_pace == 0) {
					continue;
				}
				--apu->channel[i].env_timer;
				if (apu->channel[i].env_timer <= 0 && apu->channel[i].enabled) {
					if (!apu->channel[i].env_dir) { // decreasing volume envelope
						if (apu->channel[i].volume == 0) {
							apu->channel[i].enabled = false;
							continue;
						}
						--apu->channel[i].volume;
						apu->channel[i].env_timer = apu->channel[i].env_sweep_pace;
					}
					else {
						if (apu->channel[i].volume == 0xF) {
							apu->channel[i].enabled = false;
							continue;
						}
						++apu->channel[i].volume;
						apu->channel[i].env_timer = apu->channel[i].env_sweep_pace;
					}
				}
			}
		}
		if (apu->div_apu_counter == 2 || apu->div_apu_counter == 6) {
			for (int i = 0; i < 4; ++i) {
				--apu->channel[i].freq_sweep_timer;
			}
		}
	}
}

// CHANNEL FUNCTIONS
void trigger_channel(Channel* channel) {
	channel->enabled = true;
	channel->frequency_timer = channel->frequency;
	channel->volume = channel->env_initial_volume;
	channel->length_timer = channel->length;
	channel->env_timer = channel->env_sweep_pace;
	if (channel->env_dir == false && channel->volume == 0) channel->enabled = false;
	if (channel->dac_enable == false) channel->enabled = false;
}

void channel_1_step(Apu* apu, u8 cycles) {
	if (apu->channel[0].enabled) {
		apu->channel[0].frequency_timer -= cycles;
		if (apu->channel[0].length_enabled) {
			if (apu->channel[0].length_timer == 64) {
				apu->channel[0].enabled = false;
			}
		}
		if (apu->channel[0].frequency_timer < 0) {
			apu->channel[0].frequency_timer += apu->channel[0].frequency;
			++apu->channel[0].wave_index;
			if (apu->channel[0].wave_index > 15) {
				apu->channel[0].wave_index = 0;
			}
		}
	}
}

float channel_1_sample(Apu* apu) {
	if (apu->channel[0].enabled) {
		return (duty_cycles[(apu->channel[0].wave_select & 0b11000000) >> 6][apu->channel[0].wave_index] ? 1.0 : 0.0) * (apu->channel[0].volume / (float)0xf);
	}
	else return 0.0f;
}

void channel_2_step(Apu* apu, u8 cycles) {
	if (apu->channel[1].enabled) {
		apu->channel[1].frequency_timer -= cycles;
		if (apu->channel[1].length_enabled) {
			if (apu->channel[1].length_timer == 64) {
				apu->channel[1].enabled = false;
			}
		}
		if (apu->channel[1].frequency_timer < 0) {
			apu->channel[1].frequency_timer += apu->channel[1].frequency;
			++apu->channel[1].wave_index;
			if (apu->channel[1].wave_index > 15) {
				apu->channel[1].wave_index = 0;
			}
		}
	}

}
float channel_2_sample(Apu* apu) {
	if (apu->channel[1].enabled) {
		return (duty_cycles[(apu->channel[1].wave_select & 0b11000000) >> 6][apu->channel[1].wave_index] ? 1.0 : 0.0) * (apu->channel[1].volume / (float)0xf);
	}
	else return 0.0f;

}

void step_lfsr(Apu* apu) {
	//bool result = ((apu->lfsr & 2) >> 1) != (apu->lfsr & 1);
	u8 res = apu->lfsr & 1;
	res ^= (apu->lfsr >> 1) & 1;
	apu->lfsr >>= 1;
	apu->lfsr |= (res << 14);
	
	if (apu->lfsr_width) {
		apu->lfsr &= ~(1 << 6);	
		apu->lfsr |= (res << 6);
	}
}

void channel_3_step(Apu* apu, u8 cycles) {
	if (apu->channel[2].enabled) {
		apu->channel[2].frequency_timer -= cycles;
		if (apu->channel[2].length_enabled) {
			if (apu->channel[2].length_timer == 256) {
				apu->channel[2].enabled = false;
			}
		}
		if (apu->channel[2].frequency_timer < 0) {
			apu->channel[2].frequency_timer = apu->channel[2].frequency;
			++apu->channel[2].wave_index;
			if (apu->channel[2].wave_index > 31) {
				apu->channel[2].wave_index = 0;
			}
		}
	}
}

u8 read_wave_ram(Apu* apu, u8 position) {
	u8 index = position / 2;
	u8 val = 0;
	if (position % 2 == 0) {
		val = (apu->wave_pattern_ram[index] & 0b11110000) >> 4;
	}
	else {
		val = apu->wave_pattern_ram[index] & 0b00001111;
	}
	return val;
}

float channel_3_sample(Apu* apu) {
	if (apu->channel[2].enabled) {
		u8 shift = (apu->channel[2].volume == 0) ? 4 : (apu->channel[2].volume - 1);
		return (read_wave_ram(apu, apu->channel[2].wave_index) >> shift) / (float)0xf;
	}
	else {
		return 0.0f;
	}
}

void channel_4_step(Apu* apu, u8 cycles) {
	if (apu->channel[3].enabled) {
		apu->channel[3].frequency_timer -= cycles;
		if (apu->channel[3].length_enabled) {
			if (apu->channel[3].length_timer == 64) {
				apu->channel[3].enabled = false;
			}
		}
		if (apu->channel[3].frequency_timer < 0) {
			apu->channel[3].frequency_timer = apu->channel[3].frequency;
			step_lfsr(apu);
		}
	}
}

float channel_4_sample(Apu* apu) {
	if (apu->channel[3].enabled) {
		if (apu->channel[3].volume > 0xF || apu->channel[3].volume < 0) {
			apu->channel[3].volume = 0x0;
		}
		return ((apu->lfsr & 0x1) ? (0.0) : (1.0)) * ((float)apu->channel[3].volume / 0xF);
	}
	else {
		return 0.0f;
	}
}

void handle_sample(Apu* apu, u8 cycles) {

}

#define CLAMP(x, upper, lower) (min(upper, max(x, lower)))

static float prev_sample = 0.0f;

// BUFFER FUNCTIONS
void write_channels_to_buffer(Apu* apu) {
	
	// TODO 
	float ch1_sample = channel_1_sample(apu) / 4;
	float ch2_sample = channel_2_sample(apu) / 4;
	float ch3_sample = channel_3_sample(apu) / 4;
	float ch4_sample = channel_4_sample(apu) / 4;

	float sample = ch1_sample + ch2_sample + ch3_sample + ch4_sample;
		
	static float beta = 0.5;
	float filtered = beta * sample + (1 - beta) * prev_sample; // this is a simple low pass filter.
	prev_sample = filtered;

	filtered = CLAMP((filtered * 2), 1.0, 0.0);

	apu->buffer[apu->buffer_position * 2] = filtered;
	apu->buffer[(apu->buffer_position * 2) + 1] = filtered;
	++apu->buffer_position;
}

void handle_buffers(Apu* apu, u8 cycles) {
	apu->sample_counter += (apu->sample_rate * (cycles));
	if (apu->sample_counter > 1048576 * 4) {
		apu->sample_counter -= 1048576 * 4;
		write_channels_to_buffer(apu);
	}

	if (apu->buffer_position >= apu->buffer_size) {
		apu->buffer_position = 0;
		apu->buffer_full = true;
	}
}

float* get_buffer(Apu* apu) {
	return apu->buffer;
}

void apu_step(Apu* apu, u8 cycles) {
	if (apu->nr52 & 0b10000000) { // audio enabled
		apu->buffer_full = false;
		div_apu_step(apu, cycles);

		channel_1_step(apu, cycles);
		channel_2_step(apu, cycles);
		channel_3_step(apu, cycles);
		channel_4_step(apu, cycles);

		handle_buffers(apu, cycles);
		apu->clock += cycles;

	}
}


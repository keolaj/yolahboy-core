#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Mmu.h"
#include "../apu/apu.h"
#include "../controller/controller.h"
#include "./cartridge.h"

void init_mmu(Mmu* mem) {
	memset(mem, 0, sizeof(Mmu));
	mem->cartridge.rom = NULL;
	mem->cartridge.ram = NULL;
	mem->cartridge.rom_bank = 1;
	mem->cartridge.ram_bank = 0;
	mem->cartridge.banking_mode = BANKMODESIMPLE;
	mem->cartridge.ram_enabled = false;

	mem->memory = (u8*)calloc(0x10000, sizeof(u8));
	if (mem->memory == NULL) {
		return;
	}
	mem->bios = (u8*)calloc(0x100, sizeof(u8));
	if (mem->bios == NULL) {
		return;
	}
	mem->in_bios = true;
}

u8 read8(Emulator* emu, u16 address) {
	Mmu* mem = &emu->mmu;
	switch (address & 0xF000) {
	case 0x0000:
		if (mem->in_bios && address < 0x100) {
			return mem->bios[address];
		}
		// fall through
	case 0x1000:
	case 0x2000:
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		return cart_read8(&mem->cartridge, address);

	case 0xA000:
	case 0xB000:
		return cart_read8(&mem->cartridge, address);

	case 0xE000:
		return mem->memory[address - 0x2000];

	case 0xF000:
		if (address >= 0xFF00 && address <= 0xFFFE) {
			if (address == 0xFF00) {
				return joypad_return(emu->controller, mem->memory[address]);
			}

			// GPU registers
			if (address == STAT) return emu->gpu.stat;
			if (address == LCDC) return emu->gpu.lcdc;
			if (address == LY) return emu->gpu.ly;
			if (address == LYC) return emu->gpu.lyc;
			if (address == SCY) return emu->gpu.scy;
			if (address == SCX) return emu->gpu.scx;
			if (address == WY) return emu->gpu.wy;
			if (address == WX) return emu->gpu.wx;

			// APU registers
			if (address == NR52) return emu->apu.nr52;
		}
		// fall through
	default:
		return mem->memory[address];
	}
}
u16 read16(Emulator* emu, u16 address) {
	u16 ret = 0;
	ret |= read8(emu, address);
	ret |= read8(emu, address + 1) << 8;
	return ret;
}

void write16(Emulator* emu, u16 address, u16 value) {
	write8(emu, address, value & 0xFF);
	write8(emu, address + 1, value >> 8);
}

void write8(Emulator* emu, u16 address, u8 data) {
	Mmu* mem = &emu->mmu;
	switch (address & 0xF000) {
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		// cartridge rom
		cart_write8(&mem->cartridge, address, data);
		return;

	case 0x8000:
	case 0x9000:
		// vram
		if (emu->gpu.mode != 3) {
			mem->memory[address] = data;
		}
		return;

	case 0xA000:
	case 0xB000:
		// cartridge ram
		cart_write8(&mem->cartridge, address, data);
		return;

	case 0xC000:
	case 0xD000:
		// wram
		mem->memory[address] = data;
		return;

	case 0xE000:
		// ECHO
		mem->memory[address] = data;
		return;

	case 0xF000:
		if (address >= 0xFE00 && address <= 0xFE9F) {
			// oam
			mem->memory[address] = data; // TODO implement proper oam writes and reads
			return;
		}
		else if (address >= 0xFEA0 && address < 0xFEFF) {
			// prohibited
			mem->memory[address] = data;
			return;
		}
		else if (address >= 0xFF00 && address <= 0xFF7F) {
			// IO Registers
			if (address == DMA) {
				data &= 0xDF;
				u16 src_addr = data << 8;
				for (int i = 0; i < 0x9F; ++i) {
					mem->memory[0xFE00 + i] = mem->memory[src_addr + i];
				}
				return;
			}

			if (address == 0xFF02 && data == 0x81) {
			}

			if (address == DIV) {
				mem->memory[address] = 0;
				emu->timer.clock = 0;
				return;
			}

			// GPU registers
			if (address == LCDC) {
				if ((data & (1 << 7)) == 0) {
					emu->gpu.stat &= 0b11111000;
					emu->gpu.ly = 0;
					emu->gpu.mode = 0;
					emu->gpu.clock = 0;
				}
				emu->gpu.lcdc = data;
				return;
			}
			if (address == STAT) {
				emu->gpu.stat = data & 0b11111100;
				return;
			}
			if (address == LYC) emu->gpu.lyc = data;
			if (address == LY) return;
			if (address == SCY) emu->gpu.scy = data;
			if (address == SCX) emu->gpu.scx = data;
			if (address == WY) emu->gpu.wy = data;
			if (address == WX) emu->gpu.wx = data;
			if (address == BGP) {
				// Handle BGP register
			}

			// APU registers
			if (address == NR52) {
				emu->apu.nr52 = data & 0b10000000;
				return;
			}
			if (emu->apu.nr52 & 0b10000000) {
				if (address == NR51) {
					emu->apu.nr51 = data;
					return;
				}
				if (address == NR11) {
					emu->apu.channel[0].wave_select = (data & 0b11000000) >> 6;
					emu->apu.channel[0].length = data & 0b0011111;
					return;
				}
				if (address == NR12) {
					emu->apu.nr12 = data;
					emu->apu.channel[0].env_initial_volume = (data & 0b11110000) >> 4;
					emu->apu.channel[0].env_dir = data & 0b00001000;
					emu->apu.channel[0].env_sweep_pace = data & 0b00000111;
					emu->apu.channel[0].dac_enable = data & 0b11111000;
					return;
				}
				if (address == NR13) {
					emu->apu.nr13 = data;
					int frequency = (emu->apu.nr13) | ((emu->apu.nr14 & 0b00000111) << 8);
					frequency = (2048 - frequency) * 4;
					emu->apu.channel[0].frequency = frequency;
					return;
				}
				if (address == NR14) {
					emu->apu.nr14 = data;
					int frequency = (emu->apu.nr13) | ((emu->apu.nr14 & 0b00000111) << 8);
					frequency = (2048 - frequency) * 4;
					emu->apu.channel[0].frequency = frequency;
					emu->apu.channel[0].length_enabled = data & 0b01000000;
					if (data & 0b10000000) {
						emu->apu.channel[0].wave_index = 0;
						trigger_channel(&emu->apu.channel[0]);
					}
					return;
				}
				if (address == NR21) {
					emu->apu.nr21 = data;
					emu->apu.channel[1].wave_select = (data & 0b11000000) >> 6;
					emu->apu.channel[1].length = data & 0b0011111;
					return;
				}
				if (address == NR22) {
					emu->apu.nr22 = data;
					emu->apu.channel[1].env_initial_volume = (data & 0b11110000) >> 4;
					emu->apu.channel[1].env_dir = data & 0b00001000;
					emu->apu.channel[1].env_sweep_pace = data & 0b00000111;
					emu->apu.channel[1].dac_enable = data & 0b11111000;
					return;
				}
				if (address == NR23) {
					emu->apu.nr23 = data;
					int frequency = (emu->apu.nr23) | ((emu->apu.nr24 & 0b00000111) << 8);
					frequency = (2048 - frequency) * 4;
					emu->apu.channel[1].frequency = frequency;
					return;
				}
				if (address == NR24) {
					emu->apu.nr24 = data;
					int frequency = (emu->apu.nr23) | ((emu->apu.nr24 & 0b00000111) << 8);
					frequency = (2048 - frequency) * 4;
					emu->apu.channel[1].frequency = frequency;
					emu->apu.channel[1].length_enabled = data & 0b01000000;
					if (data & 0b10000000) {
						emu->apu.channel[1].wave_index = 0;
						trigger_channel(&emu->apu.channel[1]);
					}
					return;
				}
				if (address == NR30) {
					bool enabled = data & 0x80;
					emu->apu.nr30 = data & 0x80;
					emu->apu.channel[2].dac_enable = enabled;
					return;
				}
				if (address == NR31) {
					emu->apu.nr31 = data;
					emu->apu.channel[2].length = data;
					return;
				}
				if (address == NR32) {
					emu->apu.nr32 = data;
					emu->apu.channel[2].volume = (data & 0b01100000) >> 5;
					return;
				}
				if (address == NR33) {
					emu->apu.nr33 = data;
					int frequency = (emu->apu.nr33) | ((emu->apu.nr34 & 0b00000111) << 8);
					frequency = (2048 - frequency) * 2;
					emu->apu.channel[2].frequency = frequency;
					return;
				}
				if (address == NR34) {
					emu->apu.nr34 = data;
					int frequency = (emu->apu.nr33) | ((emu->apu.nr34 & 0b00000111) << 8);
					frequency = (2048 - frequency) * 2;
					emu->apu.channel[2].frequency = frequency;
					emu->apu.channel[2].length_enabled = data & 0b01000000;
					if (data & 0b10000000) {
						emu->apu.channel[2].wave_index = 1;
						emu->apu.channel[2].env_dir = true;
						trigger_channel(&emu->apu.channel[2]);
					}
					return;
				}
				if (address >= 0xFF30 && address <= 0xFF3F) {
					emu->apu.wave_pattern_ram[address - 0xFF30] = data;
					return;
				}
				if (address == NR41) {
					emu->apu.nr41 = data;
					emu->apu.channel[3].length = data & 0b0011111;
				}
				if (address == NR42) {
					emu->apu.nr42 = data;
					emu->apu.channel[3].env_initial_volume = (data & 0b11110000) >> 4;
					emu->apu.channel[3].env_dir = data & 0b00001000;
					emu->apu.channel[3].env_sweep_pace = data & 0b00000111;
					emu->apu.channel[3].dac_enable = data & 0b11111000;
					return;
				}
				if (address == NR43) {
					emu->apu.nr43 = data;
					u8 clock_shift = ((data & 0xF0) >> 4);
					bool width = data & 0x8;
					u8 divider = data & 0x7;
					emu->apu.lfsr_clock_shift = clock_shift;
					emu->apu.lfsr_width = width;
					emu->apu.lfsr_clock_divider = divider;
					u8 div_value;
					switch (emu->apu.lfsr_clock_divider) {
					case 0: div_value = 8; break;
					case 1: div_value = 16; break;
					case 2: div_value = 32; break;
					case 3: div_value = 48; break;
					case 4: div_value = 64; break;
					case 5: div_value = 80; break;
					case 6: div_value = 96; break;
					case 7: div_value = 112; break;
					default: div_value = 8;
					}
					emu->apu.channel[3].frequency = (divider > 0 ? (divider << 4) : 8) << clock_shift;
					return;
				}
				if (address == NR44) {
					emu->apu.nr44 = data;
					emu->apu.channel[3].length_enabled = data & 0b01000000;
					if (data & 0b10000000) {
						emu->apu.lfsr = 0xFFFF;
						trigger_channel(&emu->apu.channel[3]);
					}
					return;
				}
			}
			mem->memory[address] = data;
			return;
		}
		else if (address >= 0xFF80 && address <= 0xFFFE) {
			// hram
			mem->memory[address] = data;
			return;
		}
		else if (address == 0xFFFF) {
			mem->memory[address] = data;
			return;
		}
		break;
	}
}

int load_bootrom(Mmu* mem, const char* path) {
	FILE* fp;
	fp = fopen(path, "rb");
	if (fp == NULL) {
		return -1;
	}
	fread(mem->bios, sizeof(u8), 0x100, fp);
	return 0;
}

int load_rom(Mmu* mem, const char* path) { // I believe this is working
	FILE* fp;
	fp = fopen(path, "rb");
	if (fp == NULL) {
		return -1;
	}
	memset(mem->memory, 0, sizeof(mem->memory));

	u8 cart_type_val;
	if (fseek(fp, 0x147, SEEK_SET) != 0) {
		fclose(fp);
		return -1;
	}
	fread(&cart_type_val, sizeof(u8), 1, fp);
	rewind(fp);

	mem->cartridge.type = cart_type_val;

	u8 rom_size_val; // read rom file into mem->cartridge.rom
	if (fseek(fp, 0x148, SEEK_SET) != 0) {
		fclose(fp);
		return -1;
	}
	if (fread(&rom_size_val, sizeof(u8), 1, fp) != 1) {
		fclose(fp);
		return -1;
	}
	rewind(fp);

	int rom_size = (BANKSIZE * 2) * (1 << rom_size_val);
	mem->cartridge.rom_size = rom_size;
	mem->cartridge.num_rom_banks = rom_size / BANKSIZE;
	mem->cartridge.rom = (u8*)malloc(sizeof(u8) * rom_size);
	if (mem->cartridge.rom == NULL) {
		return -1;
	}
	fread(mem->cartridge.rom, sizeof(u8), rom_size, fp);
	rewind(fp);

	u8 ram_size_val;
	fseek(fp, 0x149, SEEK_SET);
	fread(&ram_size_val, sizeof(u8), 1, fp);
	rewind(fp);
	int actual_ram_size;
	switch (ram_size_val) {
	case 0:
		actual_ram_size = 0;
		break;
	case 1:
		actual_ram_size = 0;
		break;
	case 2:
		actual_ram_size = 0x2000;
		break;
	case 3:
		actual_ram_size = 0x2000 * 4;
		break;
	case 4:
		actual_ram_size = 0x2000 * 16;
		break;
	case 5:
		actual_ram_size = 0x2000 * 8;
		break;
	default:
		actual_ram_size = 0;
		break;
	}
	mem->cartridge.ram_size = actual_ram_size;
	if (actual_ram_size) {
		mem->cartridge.ram = (u8*)malloc(sizeof(u8) * actual_ram_size);
		if (mem->cartridge.ram == NULL) {
			return -1;
		}
		memset(mem->cartridge.ram, 0, actual_ram_size);
	}
	fclose(fp);

	if (mem->cartridge.type == MBC1_RAM_BATTERY) {
		load_save(mem, path);
	}

	return 0;
}

int load_save(Mmu* mem, const char* path) {
	FILE* fp;
	fp = fopen("game.sav", "rb");

	if (fp == NULL) {
		return -1;
	}
	if (fread(mem->cartridge.ram, sizeof(u8), mem->cartridge.ram_size, fp) == mem->cartridge.ram_size) {
		return 0;
	}
	else {
		return -1;
	}
}

void destroy_mmu(Mmu* mem) {
	if (mem == NULL) return;

	if (mem->cartridge.type == MBC1_RAM_BATTERY) {
		FILE* fp = fopen("game.sav", "wb");
		if (fp == NULL) {
		}
		else {
			size_t save_size = mem->cartridge.ram_size;
			size_t written = fwrite(mem->cartridge.ram, sizeof(u8), save_size, fp);

			if (written != save_size) {
				fclose(fp);
			}
			else {
				fclose(fp);
			}
		}
	}
	if (mem->cartridge.rom) free(mem->cartridge.rom);
	if (mem->cartridge.ram) free(mem->cartridge.ram);
	if (mem->memory) free(mem->memory);
	if (mem->bios) free(mem->bios);
}
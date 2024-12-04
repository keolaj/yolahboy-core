#include "gpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mmu/mmu.h"

int init_gpu(Gpu* gpu) {
	memset(gpu, 0, sizeof(Gpu));

	gpu->framebuffer = (u32*)calloc(23040, sizeof(u32));
	if (gpu->framebuffer == NULL) return -1;

	return 0;
}

u8 read_tile(Emulator* emu, int tile_index, u8 x, u8 y) { 
	u16 address = 0x8000;
	address += (tile_index * 16);
	address += (y * 2);

	x = (7 - x);

	u8 id = (emu->mmu.memory[address] & (1 << x)) ? 1 : 0;
	id += (emu->mmu.memory[address + 1] & (1 << x)) ? 2 : 0;

	return id;
}

u32 pixel_from_palette(u8 palette, u8 id) {
	u8 value = 0;
	switch (id) { // read palette and assign value for id
	case 0:
		value = (palette & 0b00000011);
		break;
	case 1:
		value = (palette & 0b00001100) >> 2;
		break;
	case 2:
		value = (palette & 0b00110000) >> 4;
		break;
	case 3:
		value = (palette & 0b11000000) >> 6;
		break;
	default:
		value = 0;
	}
	switch (value) {
	case 0:
		return WHITE;
	case 1:
		return LIGHT;
	case 2:
		return DARK;
	case 3:
		return BLACK;
	default:
		return 0;
	}
}

void draw_line(Emulator* emu) {

	if (emu->gpu.lcdc & 1) { // if BG enabled
		bool BGTileMapArea = (emu->gpu.lcdc & (1 << 3));
		bool BGTileAddressMode = !(emu->gpu.lcdc & (1 << 4));
		int mapAddress = (BGTileMapArea) ? 0x9C00 : 0x9800; // if bit 3 of the LCD Control registers is set we use the tilemap at 0x9C00, else use tile map at 0x9800
		mapAddress += (((emu->gpu.ly + emu->gpu.scy) & 0xFF) >> 3) << 5;

		int lineOffset = (emu->gpu.scx >> 3);
		int tileX = emu->gpu.scx & 7;
		int tileY = (emu->gpu.ly + emu->gpu.scy) & 7;
		int tile = read8(emu, mapAddress + lineOffset);

		if (BGTileAddressMode && tile < 128) tile += 256;

		if (emu->gpu.lcdc & 1) {
			for (int i = 0; i < SCREEN_WIDTH; ++i) {
				int fb_index = emu->gpu.ly * SCREEN_WIDTH + i;
				if (fb_index < 23040 && fb_index >= 0) {
					emu->gpu.framebuffer[fb_index] = pixel_from_palette(read8(emu, BGP), read_tile(emu, tile, tileX, tileY));
				}
				++tileX;
				if (tileX == 8) {
					tileX = 0;
					lineOffset = (lineOffset + 1) & 31;
					tile = read8(emu, mapAddress + lineOffset);
					if (BGTileAddressMode && tile < 128) tile += 256;
				}
			}
		}
	}

	// draw window
	int windowx = emu->gpu.wx - 7;
	u8 windowy = emu->gpu.wy;
	if (emu->gpu.lcdc & (1 << 5) && emu->gpu.ly >= windowy) { // if window enabled
		bool BGTileMapArea = (emu->gpu.lcdc & (1 << 6));
		bool BGTileAddressMode = !(emu->gpu.lcdc & (1 << 4));
		int mapAddress = (BGTileMapArea) ? 0x9C00 : 0x9800; // if bit 3 of the LCD Control registers is set we use the tilemap at 0x9C00, else use tile map at 0x9800
		mapAddress += (((emu->gpu.ly + windowy) & 0xFF) >> 3) << 5;

		int lineOffset = (windowx >> 3);
		int tileX = windowx & 7;
		int tileY = (emu->gpu.ly + windowy) & 7;
		int tile = read8(emu, mapAddress + lineOffset);

		if (BGTileAddressMode && tile < 128) tile += 256;

		for (int i = windowx; i < SCREEN_WIDTH; ++i) {
			if (i >= 0) {
				u32 window_pixel = pixel_from_palette(read8(emu, BGP), read_tile(emu, tile, tileX, tileY));
				int fb_index = emu->gpu.ly * SCREEN_WIDTH + i;
				if (fb_index < 23040 && fb_index >= 0) {
					emu->gpu.framebuffer[fb_index] = window_pixel;
				}
			}
			++tileX;
			if (tileX == 8) {
				tileX = 0;
				lineOffset = (lineOffset + 1) & 31;
				tile = read8(emu, mapAddress + lineOffset);
				if (BGTileAddressMode && tile < 128) tile += 256;
			}
		}
	}

	// draw sprites
	if (emu->gpu.lcdc & (1 << 1)) { // if sprites enabled in LDC Control
		bool eight_by_16_mode = emu->gpu.lcdc & (1 << 2);
		u16 current_OAM_address = 0xFE00;
		int sprite_count_for_line = 0;

		while (current_OAM_address <= 0xFE9F && sprite_count_for_line < 10) {
			if (sprite_count_for_line > 10) break;
			u8 sprite_pos_y = read8(emu, current_OAM_address);
			u8 sprite_pos_x = read8(emu, current_OAM_address + 1);
			u8 tile_index = read8(emu, current_OAM_address + 2);
			u8 attributes = read8(emu, current_OAM_address + 3);
			bool palette_mode = attributes & (1 << 4);
			bool x_flip = attributes & (1 << 5);
			bool y_flip = attributes & (1 << 6);
			bool priority = attributes & (1 << 7);


			int sprite_bottom = sprite_pos_y - 16 + (eight_by_16_mode ? 16 : 8);
			int sprite_top = sprite_pos_y - 16;
			if (emu->gpu.ly < sprite_bottom && emu->gpu.ly >= sprite_top) { // if current line within sprite 
				++sprite_count_for_line; // 10 sprites a line
				if (eight_by_16_mode) tile_index &= 0xFE; // make sure we aren't indexing out of where we should be
				u8 sprite_line = emu->gpu.ly - sprite_top; // get current line of sprite
				if (y_flip) {
					if (eight_by_16_mode) {
						if (sprite_line >= 8) { // address next tile if we are in 16 bit mode
							sprite_line = 15 - sprite_line;
						}
						else {
							sprite_line = 7 - sprite_line;
							tile_index++;
						}
					}
					else {
						sprite_line = 7 - sprite_line;
					}
				}
				else {
					if (sprite_line >= 8) {
						sprite_line -= 8;
						tile_index++;
					}
				}
				u8 palette = read8(emu, (palette_mode ? OBP1 : OBP0));
				for (int i = 0; i < 8; ++i) {
					if ((int)sprite_pos_x - 8 + i < 0) continue;
					u8 id = read_tile(emu, tile_index, i, sprite_line);
					if (id == 0) continue;
					u32 pixel = pixel_from_palette(palette, id);
					int x = x_flip ? (7 - i) : i;
					int fb_index = emu->gpu.ly * SCREEN_WIDTH + sprite_pos_x - 8 + x;
					if (fb_index >= 23040 || fb_index < 0) continue;
					emu->gpu.framebuffer[fb_index] = pixel;
				}
			}
			current_OAM_address += 4;
		}
	}
}

void destroy_gpu(Gpu* gpu) {
	if (gpu->framebuffer) free(gpu->framebuffer);
}

void handle_oam(Emulator* emu) {
	if (emu->gpu.should_stat_interrupt && emu->gpu.clock > 4) {
		write8(emu, IF, read8(emu, IF) | STAT_INTERRUPT);
		emu->gpu.should_stat_interrupt = false;
	}
	if (emu->gpu.clock >= 80) {
		emu->gpu.clock -= 80;
		emu->gpu.mode = VRAM_ACCESS;
		write8(emu, IF, (read8(emu, IF) & ~(STAT_INTERRUPT))); // reset STAT interrupt on mode 3 enter. There is no stat interrupt for mode 3
	}
}

void handle_vram(Emulator* emu) {
	if (emu->gpu.clock >= 172) {
		emu->gpu.clock -= 172;
		emu->gpu.mode = HBLANK;
		draw_line(emu);
		if (emu->gpu.stat & (1 << 3)) {
			emu->gpu.should_stat_interrupt = true;
		}
	}
}

void handle_hblank(Emulator* emu) {
	if (emu->gpu.should_stat_interrupt && emu->gpu.clock > 4) {
		write8(emu, IF, read8(emu, IF) | STAT_INTERRUPT);
		emu->gpu.should_stat_interrupt = false;
	}
	if (emu->gpu.clock >= 204) {
		emu->gpu.clock -= 204;
		++emu->gpu.ly;
		if (emu->gpu.lyc == emu->gpu.ly) {
			emu->gpu.stat |= (1 << 2);
			if (emu->gpu.stat & (1 << 6)) {
				emu->gpu.should_stat_interrupt = true;
			}
		}
		else {
			emu->gpu.stat &= ~(1 << 2);
		}
		if (emu->gpu.ly == 143) {
			u8 interrupt = read8(emu, IF);
			write8(emu, IF, interrupt | VBLANK_INTERRUPT);
			emu->gpu.mode = VBLANK;
			if (emu->gpu.stat & (1 << 4)) {
				emu->gpu.should_stat_interrupt = true;
			}

		}
		else {
			emu->gpu.mode = OAM_ACCESS;
			if (read8(emu, STAT) & (1 << 5)) {
				emu->gpu.should_stat_interrupt = true;
			}
		}
	}
}

void handle_vblank(Emulator* emu) {
	if (emu->gpu.should_stat_interrupt && emu->gpu.clock > 4) {
		write8(emu, IF, read8(emu, IF) | STAT_INTERRUPT);
		emu->gpu.should_stat_interrupt = false;
	}
	if (emu->gpu.clock >= 456) {
		emu->gpu.clock -= 456;
		++emu->gpu.ly;
		emu->mmu.memory[LY] = emu->gpu.ly;
		if (emu->mmu.memory[LYC] == emu->gpu.ly) {
			emu->gpu.stat |= (1 << 2);
			if (read8(emu, STAT) & (1 << 6)) {
				emu->mmu.memory[IF] |= STAT_INTERRUPT;
			}
		}
		else {
			emu->gpu.stat &= ~(1 << 2);
		}


		if (emu->gpu.ly > 153) {
			emu->gpu.should_draw = true;
			emu->gpu.mode = OAM_ACCESS;
			emu->gpu.ly = 0;
			if (emu->gpu.lyc == emu->gpu.ly) {
				emu->gpu.stat |= (1 << 2);
				if (read8(emu, STAT) & (1 << 6)) {
					emu->gpu.should_stat_interrupt = true;
					// emu->mmu.memory[IF] |= STAT_INTERRUPT;
				}
			}
			else {
				emu->gpu.stat &= ~(1 << 2);
			}

			if (read8(emu, STAT) & (1 << 5)) { // mode 2 interrupt select
				write8(emu, IF, read8(emu, IF) | STAT_INTERRUPT);
			}
		}
	}

}

void gpu_step(Emulator* emu, u8 cycles) {

	bool lcd_enabled = emu->gpu.lcdc & (1 << 7);
	emu->gpu.should_draw = false;
	if (!lcd_enabled) {
		return;
	}
	emu->gpu.clock += cycles;
	switch (emu->gpu.mode) {
	case OAM_ACCESS:
		handle_oam(emu);
		break;
	case VRAM_ACCESS:
		handle_vram(emu);
		break;
	case HBLANK:
		handle_hblank(emu);
		break;
	case VBLANK:
		handle_vblank(emu);
		break;
	}
	emu->gpu.stat = (emu->gpu.stat & 0b11111100) | (emu->gpu.mode & 0b00000011);
}

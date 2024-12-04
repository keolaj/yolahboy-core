#include <stdlib.h>
#include "cartridge.h"
#include "../../debugger/imgui_custom_widget_wrapper.h"

#define BANK_SELECT_HIGH 0x7FFF
#define BANK_SELECT_LOW 0x6000
#define RAM_BANK_HIGH 0x5FFF
#define RAM_BANK_LOW 0x4000
#define RAM_ENABLE_HIGH 0x1FFF
#define RAM_ENABLE_LOW 0x0000
#define ROM_BANK_HIGH 0x3FFF
#define ROM_BANK_LOW 0x2000

u8 cart_read8(Cartridge* cart, u16 address) {
	switch (cart->type) {
	case ROM_ONLY: {
		if (address < 0x8000) {
			return cart->rom[address];
		}
		return 0xFF;
	}
	case MBC1:
	case MBC1_RAM:
	case MBC1_RAM_BATTERY: {
		if (address < 0x4000) { // bank 0
			if (cart->banking_mode == BANKMODESIMPLE) {
				return cart->rom[address];
			}
			else {
				u8 current_bank = (cart->ram_bank << 5);
				u8 num_banks = (cart->rom_size / BANKSIZE);
				current_bank &= (num_banks - 1);
				int offset = current_bank * BANKSIZE;
				return cart->rom[offset + address];
			}
		}
		if (address < 0x8000) { // selectable rom bank (this is working so far)
			u8 current_bank = cart->rom_bank;
			u8 num_banks = (cart->rom_size / BANKSIZE);
			current_bank |= (cart->ram_bank << 5);
			current_bank &= (num_banks - 1);

			int offset = current_bank * BANKSIZE;
			u16 newaddr = address - 0x4000;
			return cart->rom[offset + newaddr];
		}
		if (address >= 0xA000 && address < 0xC000) { // ram read
			if (cart->ram_enabled && cart->ram != NULL) {
				int newaddr = address - 0xA000;
				if (cart->banking_mode == BANKMODEADVANCED) {
					int num_banks = cart->ram_size / 0x2000;
					int current_bank = cart->ram_bank & (num_banks - 1);
					int offset = (current_bank * 0x2000);
					newaddr += offset;
					return cart->ram[newaddr];
				}
				else {
					return cart->ram[newaddr];
				}
			}
			else { // if ram disabled return 0xFF
				return 0xFF;
			}
		}

	}
	}
}

void cart_write8(Cartridge* cart, u16 address, u8 data) {
	if (address <= 0x1FFF) { // ram enable register
		cart->ram_enabled = (data & 0x0F) == 0xA;
		return;
	}
	if (address >= 0x2000 && address <= 0x3FFF) { // rom bank number
		switch (cart->type) {
		case ROM_ONLY:
			return;
		case MBC1:
		case MBC1_RAM:
		case MBC1_RAM_BATTERY:
			data = data & 0b00011111;
			if (data == 0 || data == 0x20 || data == 0x40 || data == 0x60) data += 1;
			cart->rom_bank = data;
			return;
		default:
			return;
		}
	}
	if (address >= 0x4000 && address <= 0x5FFF) { // ram bank number or upper bits of rom bank number
		if (cart->type == ROM_ONLY) return;
		switch (cart->type) {
		case ROM_ONLY:
			return;
		case MBC1:
		case MBC1_RAM:
		case MBC1_RAM_BATTERY: {
			cart->ram_bank = (data & 0b00000011);
			return;
		}
		}
	}
	if (address >= 0x6000 && address <= 0x7FFF) { // bank mode select
		if (cart->type == ROM_ONLY) return;
		switch (cart->type) {
		case MBC1: 
		case MBC1_RAM:
		case MBC1_RAM_BATTERY: {
			if ((data & 0b00000001) == 1) {
				cart->banking_mode = BANKMODEADVANCED;
			}
			else {
				cart->banking_mode = BANKMODESIMPLE;
			}
			return;

		}
		}
	}
	if (address >= 0xA000 && address <= 0xBFFF) { // ram write
		switch (cart->type) {
		case ROM_ONLY:
			return;
		case MBC1:
		case MBC1_RAM:
		case MBC1_RAM_BATTERY: {
			if (cart->ram_enabled && cart->ram != NULL) {
				if (cart->banking_mode == BANKMODESIMPLE) {
					int newaddr = address - 0xA000;
					cart->ram[newaddr] = data;
				}
				else {
					int num_banks = cart->ram_size / 0x2000;
					int current_bank = cart->ram_bank & (num_banks - 1);
					int offset = current_bank * 0x2000;
					int newaddr = address - 0xA000;
					cart->ram[offset + newaddr] = data;
				}
			}

		}
		}
	}
}


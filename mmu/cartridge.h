#pragma once
#include "../global_definitions.h"

u8 cart_read8(Cartridge* cart, u16 address);
void cart_write8(Cartridge* cart, u16 address, u8 data);
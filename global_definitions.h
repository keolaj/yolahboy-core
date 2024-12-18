#pragma once
#include <stdbool.h>

typedef unsigned char u8;
typedef char i8;
typedef short i16;
typedef unsigned short u16;
typedef unsigned int u32;

#define MAX_BREAKPOINTS 0x100

#define BANKSIZE 0x4000
#define TITLE 0x134
#define TITLE_SIZE 0x10
#define CARTRIDGE_TYPE 0x147

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

#define LCDC 0xFF40 // LCD control address
#define STAT 0xFF41 // LCD status address
#define LY 0xFF44 // Current horizontal line address
#define LYC 0xFF45 // Current horizonal line compare address
#define SCY 0xFF42 // Scroll Y address
#define SCX 0xFF43 // Scroll X address
#define WY 0xFF4A // Window Y address
#define WX 0xFF4B // Window X address
#define BGP 0xFF47 // Background Palette address
#define NUM_TILES 384
#define TILES_Y 24
#define TILES_X 16
#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define OBP0 0xFF48
#define OBP1 0xFF49

// #define IF 0xFF0F // Interrupt flag

#define BLACK 0x000000FF
#define DARK 0x382843FF
#define LIGHT 0x7c6d80FF
#define WHITE 0xc7c6c6FF

#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07

#define IF 0xFF0F
#define IE 0xFFFF

#define DMA 0xFF46

#define NR52 0xFF26
#define NR51 0xFF25
#define NR50 0xFF24
#define NR10 0xFF10
#define NR11 0xFF11
#define NR12 0xFF12
#define NR13 0xFF13
#define NR14 0xFF14
#define NR21 0xFF16
#define NR22 0xFF17
#define NR23 0xFF18
#define NR24 0xFF19
#define NR30 0xFF1A
#define NR31 0xFF1B
#define NR32 0xFF1C
#define NR33 0xFF1D
#define NR34 0xFF1E
#define NR41 0xFF20
#define NR42 0xFF21
#define NR43 0xFF22
#define NR44 0xFF23


#define VBLANK_INTERRUPT 1
#define VBLANK_ADDRESS 0x40

#define STAT_INTERRUPT 1 << 1
#define LCDSTAT_ADDRESS 0x48

#define TIMER_INTERRUPT 1 << 2
#define TIMER_ADDRESS 0x50

#define SERIAL_INTERRUPT 1 << 3
#define SERIAL_ADDRESS 0x58

#define JOYPAD_INTERRUPT 1 << 4
#define JOYPAD_ADDRESS 0x60

#define FLAG_ZERO 0b10000000
#define FLAG_SUB 0b01000000
#define FLAG_HALFCARRY 0b00100000
#define FLAG_CARRY 0b00010000

typedef struct {
	bool up;
	bool down;
	bool left;
	bool right;

	bool start;
	bool select;

	bool a;
	bool b;
} Controller;

typedef struct _channel {
	bool enabled;
	u8 wave_select;
	u8 wave_index;
	int frequency;
	int frequency_timer;
	int divider;
	i8 volume;

	bool length_enabled;
	u8 length;
	u8 length_timer;

	bool dac_enable;

	// frequency sweep control
	u8 freq_sweep_timer;

	// envelope
	u8 env_initial_volume;
	bool env_dir;
	u8 env_sweep_pace;
	u8 env_timer;
} Channel;

typedef struct _apu {
	u16 clock;
	u8 nr52; // FF26 Master control
	u8 nr51; // FF25 Sound panning
	u8 nr50; // Master volume and VIN panning
	u8 nr10; // Channel 1 sweep
	u8 nr11; // Channel 1 length timer and duty cycle
	u8 nr12; // Channel 1 volume and envelope
	u8 nr13; // Channel 1 period low
	u8 nr14; // Channel 1 period high and control
	u8 nr21; // Channel 2 length timer and duty cycle
	u8 nr22; // Channel 2 volume and envelope
	u8 nr23; // Channel 2 period low
	u8 nr24; // Channel 2 period high and control
	u8 nr30; // Channel 3 DAC enable
	u8 nr31; // Channel 3 length timer
	u8 nr32; // Channel 3 output level (write only)
	u8 nr33; // Channel 3 period low (write only)
	u8 nr34; // Channel 3 period high and control
	u8 nr41; // Channel 4 length timer (write only)
	u8 nr42; // Channel 4 volume and envelope
	u8 nr43; // Channel 4 frequency and randomness
	u8 nr44; // Channel 4 control
	u8 wave_pattern_ram[0x10]; // FF30-FF3F

	bool buffer_full;

	// DIV APU

	int div_apu_internal;
	int div_apu_counter;

	bool sweep_enabled;
	u16 sweep_freq_shadow;
	int sweep_timer;

	int sample_rate;
	int sample_counter;

	int buffer_size;
	int buffer_position;

	u16 lfsr;
	u8 lfsr_clock_shift;
	bool lfsr_width;
	u8 lfsr_clock_divider;

	Channel channel[4];

	float* buffer;

} Apu;

#define ROM_ONLY 0
#define MBC1 1
#define MBC1_RAM 2
#define MBC1_RAM_BATTERY 3
#define MBC2 4
#define MBC3_TIMER_RAM_BATTERY 0x10
#define MBC3 0x11
#define RTC_REGISTER 4

typedef enum {
	MODE256,
	MODE4,
	MODE16,
	MODE64
} TimerMode;

typedef struct {
	u16 clock;
	TimerMode mode;
	bool old_and;
} Timer;

typedef enum {
	RAM_NONE,
	RAM_UNUSED,
	RAM_8KB,
	RAM_32KB,
	RAM128KB,
	RAM_64KB
} RAM_TYPE;

#define BANKMODESIMPLE false
#define BANKMODEADVANCED true

typedef struct {
	u8* rom;
	u8* ram;
	u8 type;
	u8 rom_bank;
	u8 ram_bank;
	int rom_size;
	int ram_size;
	u8 num_rom_banks;
	bool banking_mode;
	bool ram_enabled;
	u8 cgb_flag;
} Cartridge;

typedef struct _Mmu {
	u8* bios;
	u8* memory;
	Cartridge cartridge;
	bool in_bios;
} Mmu;

typedef struct {
	union {
		struct {
			u8 f;
			u8 a;
		};
		u16 af;
	};
	union {
		struct {
			u8 c;
			u8 b;
		};
		u16 bc;
	};
	union {
		struct {
			u8 e;
			u8 d;
		};
		u16 de;
	};
	union {
		struct {
			u8 l;
			u8 h;
		};
		u16 hl;
	};
	u16 sp;
	u16 pc;
} Registers;


typedef struct {
	Registers registers;
	bool halted;
	bool IME;
	bool should_update_IME;
	bool update_IME_value;
	int update_IME_counter;
} Cpu;

typedef struct {
	int m_cycles;
	int t_cycles;
} Cycles;

typedef enum {
	HBLANK,
	VBLANK,
	OAM_ACCESS,
	VRAM_ACCESS
} gpu_mode;

typedef struct _gpu {

	u8 stat;
	u8 lcdc;
	u8 ly;
	u8 lyc;
	u8 scy;
	u8 scx;
	u8 wy;
	u8 wx;

	int clock;
	u32* framebuffer;

	gpu_mode mode;
	bool should_stat_interrupt;
	bool drawline;
	bool should_draw;
	bool drawtile;
} Gpu;

typedef struct {
	Cpu cpu;
	Mmu mmu;
	Gpu gpu;
	Timer timer;
	Apu apu;
	Controller controller;
	
	int clock;	
	bool should_run;
	bool should_draw;
} Emulator;


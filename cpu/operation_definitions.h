#pragma once
#include "../global_definitions.h"

typedef enum {
	OPERAND_NONE,
	A,
	F,
	AF,
	B,
	C,
	BC,
	D,
	E,
	DE,
	H,
	L,
	HL,
	SP,
	SP_ADD_I8,
	PC,
	U8,
	U16,
	I8,
	I16,
} operand_type;


typedef enum {
	ADDR_MODE_NONE,
	REGISTER,			 //	A, B, C, D, E, H, L
	REGISTER16,			 //	BC, DE, HL, SP
	ADDRESS_R16,		 //	(HL)
	ADDRESS_R8_OFFSET,   // (0xFF + C)
	MEM_READ,			 //	u8, i8
	MEM_READ16,			 //	u16
	MEM_READ_ADDR,		 //	(u16)
	MEM_READ_ADDR_OFFSET // (0xFF + u8)
} address_mode;

typedef enum {
	CONDITION_NONE,
	CONDITION_Z,
	CONDITION_C,
	CONDITION_NZ,
	CONDITION_NC,
} condition;

typedef enum {
	SECONDARY_NONE,
	INC_R_1,
	DEC_R_1,
	INC_R_2,
	DEC_R_2,
	ADD_T_4,
	ADD_T_12
} secondary;

typedef enum {
	UNIMPLEMENTED,
	NOP,
	STOP,
	HALT,
	LD,
	INC,
	DEC,
	ADD,
	ADC,
	SUB,
	SBC,
	CP,
	AND,
	OR,
	XOR,
	CPL,
	BIT,
	RR,
	RL,
	RLA,
	RRC,
	RLC,
	SLA,
	SRA,
	SRL,
	SWAP,
	RES,
	SET_OP,
	JP,
	JR,
	RET,
	RETI,
	CALL,
	RST,
	PUSH,
	POP,
	CCF,
	SCF,
	CB,
	DI,
	EI,
	DAA
} instruction_type;

typedef enum {
	_IGNORE,
	SET,
	RESET,
	DEPENDENT,
} flag_action;

typedef struct {
	flag_action zero;
	flag_action sub;
	flag_action halfcarry;
	flag_action carry;
} instruction_flags;

typedef struct {
	u8 result;
	u8 flags;
} alu_return;

typedef struct {
	u16 result;
	u8 flags;
} alu16_return;


typedef struct {
	char* mnemonic;
	instruction_type type;
	address_mode dest_addr_mode;
	address_mode source_addr_mode;
	operand_type dest;
	operand_type source;
	condition condition;
	secondary secondary;
	u8 m_cycles;
	u8 t_cycles;
	instruction_flags flag_actions;
	u8 opcode;
} Operation;

extern Operation operations[0x100];
extern Operation cb_operations[0x100];


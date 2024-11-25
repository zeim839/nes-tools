#ifndef NES_TOOLS_CPU6502_H
#define NES_TOOLS_CPU6502_H

#include "system.h"
#include "bus.h"

#define STACK_START       0x100
#define NIL_OP            {NOP, NONE}

enum
{
	BRANCH_STATE      = 1,
	INTERRUPT_PENDING = 1 << 1
};

// Enumerates the possible states of the CPU status register.
enum cpu_flag
{
	NEGATIVE  = 1 << 7,
	OVERFLW   = 1 << 6,
	BREAK     = 1 << 4,
	DECIMAL_  = 1 << 3,
	INTERRUPT = 1 << 2,
	ZERO      = 1 << 1,
	CARRY     = 1
};

// CPU 6502 opcodes, including unofficial/undocumented.
enum cpu_opcode
{
	// Official opcodes.

	ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL,
	BRK, BVC, BVS, CLC, CLD, CLI, CLV, CMP, CPX, CPY,
	DEC, DEX, DEY, EOR, INC, INX, INY, JMP, JSR, LDA,
	LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL,
	ROR, RTI, RTS, SBC, SEC, SED, SEI, STA, STX, STY,
	TAX, TAY, TSX, TXA, TXS, TYA,

	// Unofficial opcodes.

	ALR, ANC, ARR, AXS, LAX, LAS, SAX, SHY, SHX, DCP,
	ISB, RLA, RRA, SLO, SRE, SKB, IGN,
};

// 6502 instruction addressing modes.
enum cpu_addr_mode
{
	NONE, IMPL, ACC, REL, IMT, ZPG, ZPG_X, ZPG_Y,
	ABS, ABS_X, ABS_Y, IND, IND_IDX, IDX_IND,
};

// A CPU instruction combines an opcode and an addressing mode.
struct cpu_instr
{
	enum cpu_opcode    opcode;
	enum cpu_addr_mode mode;
};

// 6502 instruction lookup table.
static const struct cpu_instr cpu_instr_lookup[256] =
{
//  HI\LO        0x0          0x1          0x2             0x3           0x4             0x5         0x6           0x7           0x8           0x9           0xA        0xB            0xC             0xD          0xE           0xF
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	/*  0x0  */  {BRK, IMPL},{ORA, IDX_IND}, NIL_OP,     {SLO, IDX_IND}, {NOP, ZPG},   {ORA, ZPG},   {ASL, ZPG},   {SLO, ZPG},   {PHP, IMPL}, {ORA, IMT},   {ASL, ACC},  {ANC, IMT},   {NOP, ABS},   {ORA, ABS},   {ASL, ABS},   {SLO, ABS},
	/*  0x1  */  {BPL, REL}, {ORA, IND_IDX}, NIL_OP,     {SLO, IND_IDX}, {NOP, ZPG_X}, {ORA, ZPG_X}, {ASL, ZPG_X}, {SLO, ZPG_X}, {CLC, IMPL}, {ORA, ABS_Y}, {NOP, IMPL}, {SLO, ABS_Y}, {NOP, ABS_X}, {ORA, ABS_X}, {ASL, ABS_X}, {SLO, ABS_X},
	/*  0x2  */  {JSR, ABS}, {AND, IDX_IND}, NIL_OP,     {RLA, IDX_IND}, {BIT, ZPG},   {AND, ZPG},   {ROL, ZPG},   {RLA, ZPG},   {PLP, IMPL}, {AND, IMT},   {ROL, ACC},  {ANC, IMT},   {BIT, ABS},   {AND, ABS},   {ROL, ABS},   {RLA, ABS},
	/*  0x3  */  {BMI, REL}, {AND, IND_IDX}, NIL_OP,     {RLA, IND_IDX}, {NOP, ZPG_X}, {AND, ZPG_X}, {ROL, ZPG_X}, {RLA, ZPG_X}, {SEC, IMPL}, {AND, ABS_Y}, {NOP, IMPL}, {RLA, ABS_Y}, {NOP, ABS_X}, {AND, ABS_X}, {ROL, ABS_X}, {RLA, ABS_X},
	/*  0x4  */  {RTI, IMPL},{EOR, IDX_IND}, NIL_OP,     {SRE, IDX_IND}, {NOP, ZPG},   {EOR, ZPG},   {LSR, ZPG},   {SRE, ZPG},   {PHA, IMPL}, {EOR, IMT},   {LSR, ACC},  {ALR, IMT},   {JMP, ABS},   {EOR, ABS},   {LSR, ABS},   {SRE, ABS},
	/*  0x5  */  {BVC, REL}, {EOR, IND_IDX}, NIL_OP,     {SRE, IND_IDX}, {NOP, ZPG_X}, {EOR, ZPG_X}, {LSR, ZPG_X}, {SRE, ZPG_X}, {CLI, IMPL}, {EOR, ABS_Y}, {NOP, IMPL}, {SRE, ABS_Y}, {NOP, ABS_X}, {EOR, ABS_X}, {LSR, ABS_X}, {SRE, ABS_X},
	/*  0x6  */  {RTS, IMPL},{ADC, IDX_IND}, NIL_OP,     {RRA, IDX_IND}, {NOP, ZPG},   {ADC, ZPG},   {ROR, ZPG},   {RRA, ZPG},   {PLA, IMPL}, {ADC, IMT},   {ROR, ACC},  {ARR, IMT},   {JMP, IND},   {ADC, ABS},   {ROR, ABS},   {RRA, ABS},
	/*  0x7  */  {BVS, REL}, {ADC, IND_IDX}, NIL_OP,     {RRA, IND_IDX}, {NOP, ZPG_X}, {ADC, ZPG_X}, {ROR, ZPG_X}, {RRA, ZPG_X}, {SEI, IMPL}, {ADC, ABS_Y}, {NOP, IMPL}, {RRA, ABS_Y}, {NOP, ABS_X}, {ADC, ABS_X}, {ROR, ABS_X}, {RRA, ABS_X},
	/*  0x8  */  {NOP, IMT}, {STA, IDX_IND}, {NOP, IMT}, {SAX, IDX_IND}, {STY, ZPG},   {STA, ZPG},   {STX, ZPG},   {SAX, ZPG},   {DEY, IMPL}, {NOP, IMT},   {TXA, IMPL}, {NOP, IMT},   {STY, ABS},   {STA, ABS},   {STX, ABS},   {SAX, ABS},
	/*  0x9  */  {BCC, REL}, {STA, IND_IDX}, NIL_OP,     {NOP, IND_IDX}, {STY, ZPG_X}, {STA, ZPG_X}, {STX, ZPG_Y}, {SAX, ZPG_Y}, {TYA, IMPL}, {STA, ABS_Y}, {TXS, IMPL}, {NOP, ABS_Y}, {SHY, ABS_X}, {STA, ABS_X}, {SHX, ABS_Y}, {NOP, ABS_Y},
	/*  0xA  */  {LDY, IMT}, {LDA, IDX_IND}, {LDX, IMT}, {LAX, IDX_IND}, {LDY, ZPG},   {LDA, ZPG},   {LDX, ZPG},   {LAX, ZPG},   {TAY, IMPL}, {LDA, IMT},   {TAX, IMPL}, {LAX, IMT},   {LDY, ABS},   {LDA, ABS},   {LDX, ABS},   {LAX, ABS},
	/*  0xB  */  {BCS, REL}, {LDA, IND_IDX}, NIL_OP,     {LAX, IND_IDX}, {LDY, ZPG_X}, {LDA, ZPG_X}, {LDX, ZPG_Y}, {LAX, ZPG_Y}, {CLV, IMPL}, {LDA, ABS_Y}, {TSX, IMPL}, {LAS, ABS_Y}, {LDY, ABS_X}, {LDA, ABS_X}, {LDX, ABS_Y}, {LAX, ABS_Y},
	/*  0xC  */  {CPY, IMT}, {CMP, IDX_IND}, {NOP, IMT}, {DCP, IDX_IND}, {CPY, ZPG},   {CMP, ZPG},   {DEC, ZPG},   {DCP, ZPG},   {INY, IMPL}, {CMP, IMT},   {DEX, IMPL}, {AXS, IMT},   {CPY, ABS},   {CMP, ABS},   {DEC, ABS},   {DCP, ABS},
	/*  0xD  */  {BNE, REL}, {CMP, IND_IDX}, NIL_OP,     {DCP, IND_IDX}, {NOP, ZPG_X}, {CMP, ZPG_X}, {DEC, ZPG_X}, {DCP, ZPG_X}, {CLD, IMPL}, {CMP, ABS_Y}, {NOP, IMPL}, {DCP, ABS_Y}, {NOP, ABS_X}, {CMP, ABS_X}, {DEC, ABS_X}, {DCP, ABS_X},
	/*  0xE  */  {CPX, IMT}, {SBC, IDX_IND}, {NOP, IMT}, {ISB, IDX_IND}, {CPX, ZPG},   {SBC, ZPG},   {INC, ZPG},   {ISB, ZPG},   {INX, IMPL}, {SBC, IMT},   NIL_OP,      {SBC, IMT},   {CPX, ABS},   {SBC, ABS},   {INC, ABS},   {ISB, ABS},
	/*  0xF  */  {BEQ, REL}, {SBC, IND_IDX}, NIL_OP,     {ISB, IND_IDX}, {NOP, ZPG_X}, {SBC, ZPG_X}, {INC, ZPG_X}, {ISB, ZPG_X}, {SED, IMPL}, {SBC, ABS_Y}, {NOP, IMPL}, {ISB, ABS_Y}, {NOP, ABS_X}, {SBC, ABS_X}, {INC, ABS_X}, {ISB, ABS_X}
};

// cycle times corresponding to each instruction in cpu_instr_lookup.
static const uint8_t cpu_cycle_lookup[256] =
{
        // HI/LO 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
        // -----------------------------------------------------
	/* 0 */  7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
	/* 1 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	/* 2 */  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
	/* 3 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	/* 4 */  6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
	/* 5 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	/* 6 */  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
	/* 7 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	/* 8 */  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	/* 9 */  2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
	/* A */  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	/* B */  2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
	/* C */  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	/* D */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	/* E */  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	/* F */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
};

enum cpu_interrupt
{
	NOI = 0,
	NMI,
	RSI,
	IRQ
};

// cpu6502_t emulates a 6502 microprocessor.
typedef struct cpu6502_t
{
	uint8_t  state;
	uint16_t addr;
	bus_t*   bus;

	// Cycle state.
	uint8_t  cycles;
	size_t   t_cycles;
	uint16_t dma_cycles;
	uint8_t  odd_cycle;

	// Registers.
	uint16_t pc;
	uint8_t  ac;
	uint8_t  x;
	uint8_t  y;
	uint8_t  sr;
	uint8_t  sp;

	// Interrupt flag.
	enum cpu_interrupt interrupt;

	const struct cpu_instr* instr;

} cpu6502_t;

cpu6502_t* cpu_create(bus_t* bus);
void cpu_destroy(cpu6502_t* cpu);

// cpu_reset performs a soft-reset.
void cpu_reset(cpu6502_t* cpu);

// cpu_exec executes a single CPU cycle.
void cpu_exec(cpu6502_t* cpu);

// cpu_dma_suspend suspends the CPU for 513 cycles for DMA transfer.
void cpu_dma_suspend(cpu6502_t* cpu);

// cpu_interrupt queues an interrupt.
void cpu_interrupt(cpu6502_t* cpu, enum cpu_interrupt interrupt);

#endif // NES_TOOLS_CPU6502_H

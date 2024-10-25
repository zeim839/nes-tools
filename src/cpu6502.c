#include "cpu6502.h"

#define DMA_CYCLES 513

static uint16_t read_abs_addr(bus_t* bus, uint16_t offset)
{
	// 16 bit address is little endian so read lo then hi
	uint16_t lo = (uint16_t)bus_read(bus, offset);
	uint16_t hi = (uint16_t)bus_read(bus, offset + 1);
	return (hi << 8) | lo;
}

static uint8_t has_page_break(uint16_t addr1, uint16_t addr2)
{ return (addr1 & 0xFF00) != (addr2 & 0xFF00); }

static uint16_t get_address(cpu6502_t* cpu)
{
	uint16_t addr, hi, lo;
	switch (cpu->instr->mode) {
        case IMPL:
        case ACC:
		bus_read(cpu->bus, cpu->pc);
        case NONE:
		return 0;
        case REL: {
		int8_t offset = (int8_t)bus_read(cpu->bus, cpu->pc++);
		return cpu->pc + offset;
        }
        case IMT:
		return cpu->pc++;
        case ZPG:
		return bus_read(cpu->bus, cpu->pc++) & 0xFF;
        case ZPG_X:
		addr = bus_read(cpu->bus, cpu->pc++);
		return (addr + cpu->x) & 0xFF;
        case ZPG_Y:
		addr = bus_read(cpu->bus, cpu->pc++);
		return (addr + cpu->y) & 0xFF;
        case ABS:
		addr = read_abs_addr(cpu->bus, cpu->pc);
		cpu->pc += 2;
		return addr;
        case ABS_X:
		addr = read_abs_addr(cpu->bus, cpu->pc);
		cpu->pc += 2;
		switch (cpu->instr->opcode) {
                case STA: case ASL: case DEC: case INC:
		case LSR: case ROL: case ROR: case SLO:
		case RLA: case SRE: case RRA: case DCP:
		case ISB: case SHY:
			bus_read(cpu->bus, (addr & 0xff00) | ((addr + cpu->x) & 0xff));
			break;
                default:
			if (has_page_break(addr, addr + cpu->x)) {
				bus_read(cpu->bus, (addr & 0xff00) | ((addr + cpu->x) & 0xff));
				cpu->cycles++;
			}
		}
		return addr + cpu->x;
        case ABS_Y:
		addr = read_abs_addr(cpu->bus, cpu->pc);
		cpu->pc += 2;
		switch (cpu->instr->opcode) {
                case STA: case SLO: case RLA: case SRE:
		case RRA: case DCP: case ISB: case NOP:
			bus_read(cpu->bus, (addr & 0xff00) | ((addr + cpu->y) & 0xff));
			break;
                default:
			if (has_page_break(addr, addr + cpu->y)) {
				bus_read(cpu->bus, (addr & 0xff00) | ((addr + cpu->y) & 0xff));
				cpu->cycles++;
			}
		}
		return addr + cpu->y;
        case IND:
		addr = read_abs_addr(cpu->bus, cpu->pc);
		cpu->pc += 2;
		lo = bus_read(cpu->bus, addr);
		hi = bus_read(cpu->bus, (addr & 0xFF00) | ((addr + 1) & 0xFF));
		return (hi << 8) | lo;
        case IDX_IND:
		addr = (bus_read(cpu->bus, cpu->pc++) + cpu->x) & 0xFF;
		hi = bus_read(cpu->bus, (addr + 1) & 0xFF);
		lo = bus_read(cpu->bus, addr & 0xFF);
		return (hi << 8) | lo;
        case IND_IDX:
		addr = bus_read(cpu->bus, cpu->pc++);
		hi = bus_read(cpu->bus, (addr + 1) & 0xFF);
		lo = bus_read(cpu->bus, addr & 0xFF);
		addr = (hi << 8) | lo;
		switch (cpu->instr->opcode) {
                case STA:case SLO:case RLA:case SRE:case RRA:case DCP:case ISB: case NOP:
			bus_read(cpu->bus, (addr & 0xff00) | ((addr + cpu->y) & 0xff));
			break;
                default:
			if (has_page_break(addr, addr + cpu->y)) {
				bus_read(cpu->bus, (addr & 0xff00) | ((addr + cpu->y) & 0xff));
				cpu->cycles++;
			}
		}
		return addr + cpu->y;
	}
	return 0;
}

static void set_zn(cpu6502_t* cpu, uint8_t value)
{
	cpu->sr &= ~(NEGATIVE | ZERO);
	cpu->sr |= ((!value)? ZERO: 0);
	cpu->sr |= (value & NEGATIVE);
}

static void fast_set_zn(cpu6502_t* cpu, uint8_t value)
{
	// this assumes the necessary flags (Z & N) have been cleared
	cpu->sr |= ((!value)? ZERO: 0);
	cpu->sr |= (value & NEGATIVE);
}

static void stack_push(cpu6502_t* cpu, uint8_t value)
{ bus_write(cpu->bus, STACK_START + cpu->sp--, value); }

static void stack_push_addr(cpu6502_t* cpu, uint16_t addr)
{
	bus_write(cpu->bus, STACK_START + cpu->sp--, addr >> 8);
	bus_write(cpu->bus, STACK_START + cpu->sp--, addr & 0xFF);
}

static uint8_t stack_pop(cpu6502_t* cpu)
{ return bus_read(cpu->bus, STACK_START + ++cpu->sp); }

static uint16_t stack_pop_addr(cpu6502_t* cpu)
{
	uint16_t addr = bus_read(cpu->bus, STACK_START + ++cpu->sp);
	return addr | ((uint16_t)bus_read(cpu->bus, STACK_START + ++cpu->sp)) << 8;
}

cpu6502_t* cpu_create(bus_t* bus)
{
	cpu6502_t* cpu  = malloc(sizeof(cpu6502_t));
	cpu->interrupt  = NOI;
	cpu->bus        = bus;
	cpu->ac         = 0;
	cpu->x          = 0;
	cpu->y          = 0;
	cpu->state      = 0;
	cpu->cycles     = 0;
	cpu->dma_cycles = 0;
	cpu->odd_cycle  = 0;
	cpu->t_cycles   = 0;
	cpu->sr         = 0x24;
	cpu->sp         = 0xfd;
	cpu->pc         = read_abs_addr(cpu->bus, RESET_ADDRESS);
	return cpu;
}

void cpu_destroy(cpu6502_t* cpu)
{ free(cpu); }

void cpu_reset(cpu6502_t* cpu)
{
	cpu->sr        |= INTERRUPT;
	cpu->sp        -= 3;
	cpu->pc         = read_abs_addr(cpu->bus, RESET_ADDRESS);
	cpu->cycles     = 0;
	cpu->dma_cycles = 0;
}

static void branch(cpu6502_t* cpu, uint8_t mask, uint8_t predicate)
{
	if (((cpu->sr & mask) > 0) == predicate) {
		cpu->cycles += has_page_break(cpu->pc, cpu->addr);
		cpu->cycles++;
		cpu->state |= BRANCH_STATE;
		return;
	}
	cpu->state &= ~BRANCH_STATE;
}

static void prep_branch(cpu6502_t* cpu)
{
	switch(cpu->instr->opcode){
        case BCC:
		branch(cpu, CARRY, 0);
		break;
        case BCS:
		branch(cpu, CARRY, 1);
		break;
        case BEQ:
		branch(cpu, ZERO, 1);
		break;
        case BMI:
		branch(cpu, NEGATIVE, 1);
		break;
        case BNE:
		branch(cpu, ZERO, 0);
		break;
        case BPL:
		branch(cpu, NEGATIVE, 0);
		break;
        case BVC:
		branch(cpu, OVERFLW, 0);
		break;
        case BVS:
		branch(cpu, OVERFLW, 1);
		break;
        default:
		cpu->state &= ~BRANCH_STATE;
	}
}

static uint8_t shift_l(cpu6502_t* cpu, uint8_t val)
{
	cpu->sr &= ~(CARRY | ZERO | NEGATIVE);
	cpu->sr |= (val & NEGATIVE) ? CARRY: 0;
	val <<= 1;
	fast_set_zn(cpu, val);
	return val;
}

static uint8_t shift_r(cpu6502_t* cpu, uint8_t val)
{
	cpu->sr &= ~(CARRY | ZERO | NEGATIVE);
	cpu->sr |= (val & 0x1) ? CARRY: 0;
	val >>= 1;
	fast_set_zn(cpu, val);
	return val;
}

static uint8_t rot_l(cpu6502_t* cpu, uint8_t val)
{
	uint8_t rotated = val << 1;
	rotated |= cpu->sr & CARRY;
	cpu->sr &= ~(CARRY | ZERO | NEGATIVE);
	cpu->sr |= val & NEGATIVE ? CARRY: 0;
	fast_set_zn(cpu, rotated);
	return rotated;
}

static uint8_t rot_r(cpu6502_t* cpu, uint8_t val)
{
	uint8_t rotated = val >> 1;
	rotated |= (cpu->sr &  CARRY) << 7;
	cpu->sr &= ~(CARRY | ZERO | NEGATIVE);
	cpu->sr |= val & CARRY;
	fast_set_zn(cpu, rotated);
	return rotated;
}

static void interrupt_(cpu6502_t* cpu)
{
	if ((cpu->sr & INTERRUPT) && cpu->interrupt != NMI) {
		cpu->interrupt = NOI;
		return;
	}

	uint16_t addr;
	switch (cpu->interrupt) {
        case NMI:
		addr = NMI_ADDRESS;
		break;
        case IRQ:
		addr = IRQ_ADDRESS;
		cpu->sr |= BREAK;
		break;
        case RSI:
		addr = RESET_ADDRESS;
		break;
        case NOI:
		LOG(ERROR, "No interrupt set");
		return;
        default:
		LOG(ERROR, "Unknown interrupt");
		return;
	}

	stack_push_addr(cpu, cpu->pc);
	stack_push(cpu, cpu->sr);
	cpu->sr &= ~INTERRUPT;
	cpu->sr |= INTERRUPT;
	cpu->pc = read_abs_addr(cpu->bus, addr);
	cpu->interrupt = NOI;
}

void cpu_dma_suspend(cpu6502_t* cpu)
{
	if (!cpu) return;

	// Extra cycle on odd cycles.
	cpu->dma_cycles += DMA_CYCLES + cpu->odd_cycle;
}

void cpu_exec(cpu6502_t* cpu)
{
	cpu->odd_cycle ^= 1;
	cpu->t_cycles++;

	// Handle DMA suspended cycles.
	if (cpu->dma_cycles != 0) {
		cpu->dma_cycles--;
		return;
	}

	// Poll for pending interrupts.
	if (cpu->cycles == 0 && cpu->interrupt != NOI) {
		cpu->state |= INTERRUPT_PENDING;

		// Takes 7 cycles and this is one of them.
		cpu->cycles = 7 - 1;

		return;
	}

	// Fetch new instruction.
	if (cpu->cycles == 0) {
		uint8_t opcode = bus_read(cpu->bus, cpu->pc++);
		cpu->instr = &cpu_instr_lookup[opcode];
		cpu->addr = get_address(cpu);
		cpu->cycles += cpu_cycle_lookup[opcode];

		// Prepare for branching and adjust cycles accordingly
		prep_branch(cpu);
		cpu->cycles--;
		return;
	}

	if (cpu->cycles == 1)
		cpu->cycles--;

	// Process current instruction.
	if (cpu->cycles > 1) {
		cpu->cycles--;
		return;
	}

	// Handle pending interrupts.
	if (cpu->state & INTERRUPT_PENDING) {
		interrupt_(cpu);
		cpu->state &= ~INTERRUPT_PENDING;
		return;
	}

	// Execute current instruction.
	uint16_t address = cpu->addr;
	switch (cpu->instr->opcode) {
        case LDA:
		cpu->ac = bus_read(cpu->bus, address);
		set_zn(cpu, cpu->ac);
		break;
        case LDX:
		cpu->x = bus_read(cpu->bus, address);
		set_zn(cpu, cpu->x);
		break;
        case LDY:
		cpu->y = bus_read(cpu->bus, address);
		set_zn(cpu, cpu->y);
		break;
        case STA:
		bus_write(cpu->bus, address, cpu->ac);
		break;
        case STY:
		bus_write(cpu->bus, address, cpu->y);
		break;
        case STX:
		bus_write(cpu->bus, address, cpu->x);
		break;
        case TAX:
		cpu->x = cpu->ac;
		set_zn(cpu, cpu->x);
		break;
        case TAY:
		cpu->y = cpu->ac;
		set_zn(cpu, cpu->y);
		break;
        case TXA:
		cpu->ac = cpu->x;
		set_zn(cpu, cpu->ac);
		break;
        case TYA:
		cpu->ac = cpu->y;
		set_zn(cpu, cpu->ac);
		break;
        case TSX:
		cpu->x = cpu->sp;
		set_zn(cpu, cpu->x);
		break;
        case TXS:
		cpu->sp = cpu->x;
		break;
        case PHA:
		stack_push(cpu, cpu->ac);
		break;
        case PHP:
		stack_push(cpu, cpu->sr | BIT_4 | BIT_5);
		break;
        case PLA:
		cpu->ac = stack_pop(cpu);
		set_zn(cpu, cpu->ac);
		break;
        case PLP:
		cpu->sr &= (BIT_4 |BIT_5);
		cpu->sr |= stack_pop(cpu) & ~(BIT_4 | BIT_5);
		break;
        case AND:
		cpu->ac &= bus_read(cpu->bus, address);
		set_zn(cpu, cpu->ac);
		break;
        case EOR:
		cpu->ac ^= bus_read(cpu->bus, address);
		set_zn(cpu, cpu->ac);
		break;
        case ORA:
		cpu->ac |= bus_read(cpu->bus, address);
		set_zn(cpu, cpu->ac);
		break;
        case BIT: {
		uint8_t opr = bus_read(cpu->bus, address);
		cpu->sr &= ~(NEGATIVE | OVERFLW | ZERO);
		cpu->sr |= (!(opr & cpu->ac) ? ZERO: 0);
		cpu->sr |= (opr & (NEGATIVE | OVERFLW));
		break;
        }
        case ADC: {
		uint8_t opr = bus_read(cpu->bus, address);
		uint16_t sum = cpu->ac + opr + ((cpu->sr & CARRY) != 0);
		cpu->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
		cpu->sr |= (sum & 0xFF00 ? CARRY: 0);
		cpu->sr |= ((cpu->ac ^ sum) & (opr ^ sum) & 0x80) ? OVERFLW: 0;
		cpu->ac = sum;
		fast_set_zn(cpu, cpu->ac);
		break;
        }
        case SBC: {
		uint8_t opr = bus_read(cpu->bus, address);
		uint16_t diff = cpu->ac - opr - ((cpu->sr & CARRY) == 0);
		cpu->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
		cpu->sr |= (!(diff & 0xFF00)) ? CARRY : 0;
		cpu->sr |= ((cpu->ac ^ diff) & (~opr ^ diff) & 0x80) ? OVERFLW: 0;
		cpu->ac = diff;
		fast_set_zn(cpu, cpu->ac);
		break;
        }
        case CMP: {
		uint16_t diff = cpu->ac - bus_read(cpu->bus, address);
		cpu->sr &= ~(CARRY | NEGATIVE | ZERO);
		cpu->sr |= !(diff & 0xFF00) ? CARRY: 0;
		fast_set_zn(cpu, diff);
		break;
        }
        case CPX: {
		uint16_t diff = cpu->x - bus_read(cpu->bus, address);
		cpu->sr &= ~(CARRY | NEGATIVE | ZERO);
		cpu->sr |= !(diff & 0x100) ? CARRY: 0;
		fast_set_zn(cpu, diff);
		break;
        }
        case CPY: {
		uint16_t diff = cpu->y - bus_read(cpu->bus, address);
		cpu->sr &= ~(CARRY | NEGATIVE | ZERO);
		cpu->sr |= !(diff & 0xFF00) ? CARRY: 0;
		fast_set_zn(cpu, diff);
		break;
        }
        case DEC: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m--);
		bus_write(cpu->bus, address, m);
		set_zn(cpu, m);
		break;
        }
        case DEX:
		cpu->x--;
		set_zn(cpu, cpu->x);
		break;
        case DEY:
		cpu->y--;
		set_zn(cpu, cpu->y);
		break;
        case INC: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m++);
		bus_write(cpu->bus, address, m);
		set_zn(cpu, m);
		break;
        }
        case INX:
		cpu->x++;
		set_zn(cpu, cpu->x);
		break;
        case INY:
		cpu->y++;
		set_zn(cpu, cpu->y);
		break;
        case ASL:
		if (cpu->instr->mode == ACC) {
			cpu->ac = shift_l(cpu, cpu->ac);
			break;
		}
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		bus_write(cpu->bus, address, shift_l(cpu, m));
		break;
        case LSR: {
		if (cpu->instr->mode == ACC) {
			cpu->ac = shift_r(cpu, cpu->ac);
			break;
		}
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		bus_write(cpu->bus, address, shift_r(cpu, m));
		break;
	}
        case ROL: {
		if (cpu->instr->mode == ACC) {
			cpu->ac = rot_l(cpu, cpu->ac);
			break;
		}
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		bus_write(cpu->bus, address, rot_l(cpu, m));
		break;
	}
        case ROR: {
		if (cpu->instr->mode == ACC) {
			cpu->ac = rot_r(cpu, cpu->ac);
			break;
		}
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		bus_write(cpu->bus, address, rot_r(cpu, m));
		break;
	}
        case JMP:
		cpu->pc = address;
		break;
        case JSR:
		stack_push_addr(cpu, cpu->pc - 1);
		cpu->pc = address;
		break;
        case RTS:
		cpu->pc = stack_pop_addr(cpu) + 1;
		break;
        case BCC:
	case BCS:
	case BEQ:
	case BMI:
	case BNE:
	case BPL:
	case BVC:
	case BVS:
		if (cpu->state & BRANCH_STATE) {
			cpu->pc = cpu->addr;
			cpu->state &= ~BRANCH_STATE;
		}
		break;
        case CLC:
		cpu->sr &= ~CARRY;
		break;
        case CLD:
		cpu->sr &= ~DECIMAL_;
		break;
        case CLI:
		cpu->sr &= ~INTERRUPT;
		break;
        case CLV:
		cpu->sr &= ~OVERFLW;
		break;
        case SEC:
		cpu->sr |= CARRY;
		break;
        case SED:
		cpu->sr |= DECIMAL_;
		break;
        case SEI:
		cpu->sr |= INTERRUPT;
		break;
        case BRK:
		cpu->pc++;
		stack_push_addr(cpu, cpu->pc);
		stack_push(cpu, cpu->sr | BIT_5 | BIT_4);
		cpu->pc = read_abs_addr(cpu->bus, IRQ_ADDRESS);
		cpu->sr |= INTERRUPT;
		break;
        case RTI:
		cpu->sr &= (BIT_4 | BIT_5);
		cpu->sr |= stack_pop(cpu) & ~(BIT_4 | BIT_5);
		cpu->pc = stack_pop_addr(cpu);
		break;
        case NOP:
		break;
        case ALR:
		cpu->ac &= bus_read(cpu->bus, address);
		cpu->ac = shift_r(cpu, cpu->ac);
		break;
        case ANC:
		cpu->ac = cpu->ac & bus_read(cpu->bus, address);
		cpu->sr &= ~(CARRY | ZERO | NEGATIVE);
		cpu->sr |= (cpu->ac & NEGATIVE) ? (CARRY | NEGATIVE): 0;
		cpu->sr |= ((!cpu->ac)? ZERO: 0);
		break;
        case ARR: {
		uint8_t val = cpu->ac & bus_read(cpu->bus, address);
		uint8_t rotated = val >> 1;
		rotated |= (cpu->sr & CARRY) << 7;
		cpu->sr &= ~(CARRY | ZERO | NEGATIVE | OVERFLW);
		cpu->sr |= (rotated & BIT_6) ? CARRY: 0;
		cpu->sr |= (((rotated & BIT_6) >> 1) ^ (rotated & BIT_5)) ? OVERFLW: 0;
		fast_set_zn(cpu, rotated);
		cpu->ac = rotated;
		break;
        }
        case AXS: {
		uint8_t opr = bus_read(cpu->bus, address);
		cpu->x = cpu->x & cpu->ac;
		uint16_t diff = cpu->x - opr;
		cpu->sr &= ~(CARRY | NEGATIVE | ZERO);
		cpu->sr |= (!(diff & 0xFF00)) ? CARRY : 0;
		cpu->x = diff;
		fast_set_zn(cpu, cpu->x);
		break;
        }
        case LAX:
		cpu->ac = bus_read(cpu->bus, address);
		cpu->x = cpu->ac;
		set_zn(cpu, cpu->ac);
		break;
        case SAX: {
		bus_write(cpu->bus, address, cpu->ac & cpu->x);
		break;
        }
        case DCP: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m--);
		bus_write(cpu->bus, address, m);
		uint16_t diff = cpu->ac - bus_read(cpu->bus, address);
		cpu->sr &= ~(CARRY | NEGATIVE | ZERO);
		cpu->sr |= !(diff & 0xFF00) ? CARRY: 0;
		fast_set_zn(cpu, diff);
		break;
        }
        case ISB: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m++);
		bus_write(cpu->bus, address, m);
		uint16_t diff = cpu->ac - m - ((cpu->sr & CARRY) == 0);
		cpu->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
		cpu->sr |= (!(diff & 0xFF00)) ? CARRY : 0;
		cpu->sr |= ((cpu->ac ^ diff) & (~m ^ diff) & 0x80) ? OVERFLW: 0;
		cpu->ac = diff;
		fast_set_zn(cpu, cpu->ac);
		break;
        }
        case RLA: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		m = rot_l(cpu, m);
		bus_write(cpu->bus, address, m);
		cpu->ac &= m;
		set_zn(cpu, cpu->ac);
		break;
        }
        case RRA: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		m = rot_r(cpu, m);
		bus_write(cpu->bus, address, m);
		uint16_t sum = cpu->ac + m + ((cpu->sr & CARRY) != 0);
		cpu->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
		cpu->sr |= (sum & 0xFF00 ? CARRY : 0);
		cpu->sr |= ((cpu->ac ^ sum) & (m ^ sum) & 0x80) ? OVERFLW : 0;
		cpu->ac = sum;
		fast_set_zn(cpu, sum);
		break;
        }
        case SLO: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		m = shift_l(cpu, m);
		bus_write(cpu->bus, address, m);
		cpu->ac |= m;
		set_zn(cpu, cpu->ac);
		break;
        }
        case SRE: {
		uint8_t m = bus_read(cpu->bus, address);
		bus_write(cpu->bus, address, m);
		m = shift_r(cpu, m);
		bus_write(cpu->bus, address, m);
		cpu->ac ^= m;
		set_zn(cpu, cpu->ac);
		break;
        }
        case SHY: {
		uint8_t H = address >> 8, L = address & 0xff;
		bus_write(cpu->bus, ((cpu->y & (H + 1)) << 8) | L, cpu->y & (H + 1));
		break;
        }
        case SHX: {
		uint8_t H = address >> 8, L = address & 0xff;
		bus_write(cpu->bus, ((cpu->x & (H + 1)) << 8) | L, cpu->x & (H + 1));
		break;
        }
        default:
		break;
	}
}

void cpu_interrupt(cpu6502_t* cpu, enum cpu_interrupt interrupt)
{ cpu->interrupt = interrupt; }

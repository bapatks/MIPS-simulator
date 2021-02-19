#pragma once

#define PC_START_ADDRESS 256

#define OPCODE_TYPE_BITMASK 0b11000000000000000000000000000000
#define OPCODE_BITMASK      0b00111100000000000000000000000000
#define RS_BITMASK          0b00000011111000000000000000000000
#define RT_BITMASK          0b00000000000111110000000000000000
#define RD_BITMASK          0b00000000000000001111100000000000
#define IMM_BITMASK         0b00000000000000001111111111111111
#define CHECK_16_BIT        0b00000000000000001000000000000000
#define CHECK_INSTR         0b11111100000000000000000000000000

struct Memory
{
	int               dataMemory[512];
	unsigned long int instructionMemory[512];
};

struct RegisterTypes
{
	int                rs;
	int                rt;
	int                rd;
	short int          imm; //stores 16 bit signed immediate data
	unsigned short int imm_addr; //stores 16 bit offset address
};

void DetermineRegister(unsigned long int instruction, int immFlag, struct RegisterTypes* reg);

class MipsProcessor
{
public:
	MipsProcessor();

	void* GetInstructionMemoryPtr() {
		return &(m_memory.instructionMemory);
	}

	void* GetDataMemoryPtr() {
		return &(m_memory.dataMemory);
	}

private:
	int    m_registers[32];
	int    m_programCounter;
	Memory m_memory;

};

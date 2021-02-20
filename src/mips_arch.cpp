#include "mips_arch.h"

MipsProcessor::MipsProcessor()
	:
	m_registers{0},
	m_programCounter(PC_START_ADDRESS)
{
	for (int i = 0; i < 512; i++)
	{
		// Clear the instruction and data memory
		m_memory.dataMemory[i] = 0;
		m_memory.instructionMemory[i] = 0;
	}
}

void DetermineRegister(long unsigned int instruction, int immFlag, struct RegisterTypes* reg)
{
	if (immFlag == 0)
	{
		reg->rd = (instruction & REG_RD_BITMASK) >> 11;
	}
	else if (immFlag == 1)
	{
		reg->imm = (instruction & REG_IMM_BITMASK);
		//printf("%d\n", reg->imm);
	}
	else if (immFlag == 2)
	{
		reg->imm_addr = (instruction & REG_IMM_BITMASK);
	}

	reg->rs = (instruction & REG_RS_BITMASK) >> 21;
	reg->rt = (instruction & REG_RT_BITMASK) >> 16;
}
#include "mips_arch.h"

// =====================================================================================================================
FetchStage::FetchStage()
	:
	m_t_trackDirtyRegisters{false}
{

}

// =====================================================================================================================
ControlUnit::ControlUnit()
	:
	breakFlag(false),
	trackDirtyRegisters{false}
{

}

// =====================================================================================================================
IssueStage::IssueStage()
	:
	m_t_prissue{0}
{

}

// =====================================================================================================================
AluStage::AluStage()
	:
	m_t_pralu{ 0 }
{

}
// =====================================================================================================================
MipsProcessor::MipsProcessor()
	:
	m_registerFile{0},
	m_programCounter(PC_START_ADDRESS),
	m_buffers{0}
{
	for (int i = 0; i < 512; i++)
	{
		// Clear the instruction and data memory
		m_processorMemory.dataMemory[i] = 0;
		m_processorMemory.instructionMemory[i] = 0;
	}
}

// =====================================================================================================================
void DetermineRegister(unsigned long int instruction, int immFlag, struct RegisterTypes* reg)
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
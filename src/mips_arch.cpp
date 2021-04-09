#include <iostream>
#include "utils.h"
#include "mips_arch.h"

// =====================================================================================================================
void DetermineRegister(unsigned long int instruction, int immFlag, RegisterTypes* pReg)
{
	if (immFlag == 0)
	{
		pReg->rd = (instruction & REG_RD_BITMASK) >> 11;
	}
	else if (immFlag == 1)
	{
		pReg->imm = (instruction & REG_IMM_BITMASK);
		//printf("%d\n", pReg->imm);
	}
	else if (immFlag == 2)
	{
		pReg->imm_addr = (instruction & REG_IMM_BITMASK);
	}

	pReg->rs = (instruction & REG_RS_BITMASK) >> 21;
	pReg->rt = (instruction & REG_RT_BITMASK) >> 16;
}

// =====================================================================================================================
int DetermineDestinationRegister(unsigned long int prissueInstruction, RegisterTypes* pReg)
{
	long unsigned int check;
	check = (prissueInstruction & CHECK_INSTR_BITMASK) >> 26;

	switch (check)
	{
	case 0x17: //LW
		DetermineRegister(prissueInstruction, 2, pReg);
		return pReg->rt;
		break;
	case 0x18: //SLL
	case 0x19: //SRL
	case 0x1A: //SRA
	case 0x30: //ADD
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		DetermineRegister(prissueInstruction, 0, pReg);
		return pReg->rd;
		break;
	case 0x38: //ADDI
	case 0x39:
	case 0x3A:
	case 0x3B:
		DetermineRegister(prissueInstruction, 1, pReg);
		return pReg->rt;
		break;
	default:
		return -1;
		break;
	}
}

// =====================================================================================================================
FetchStage::FetchStage()
	:
	m_t_trackDirtyRegisters{ false },
	m_re1(0),
	m_re2(0)
{

}

// =====================================================================================================================
ControlUnit::ControlUnit()
	:
	m_breakFlag(false),
	m_trackDirtyRegisters{ false },
	m_haltExec{0},
	m_reg{ 0 },
	m_tempBuffers{0}
{

}

// =====================================================================================================================
IssueStage::IssueStage()
	:
	m_prissue{0}
{

}

// =====================================================================================================================
AluStage::AluStage()
	:
	pralu{0}
{

}

// =====================================================================================================================
MemoryStage::MemoryStage()
	:
	premem{0},
	postmem{0}
{

}

// =====================================================================================================================
MipsProcessor::MipsProcessor()
	:
	m_registerFile{0},
	m_programCounter(PC_START_ADDRESS)
{
	for (int i = 0; i < 512; i++)
	{
		// Clear the instruction and data memory
		m_processorMemory.dataMemory[i] = 0;
		m_processorMemory.instructionMemory[i] = 0;
	}
}
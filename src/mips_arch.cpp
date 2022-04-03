#include <iostream>
#include <string>
#include "mips_arch.h"

// =====================================================================================================================
void DetermineRegister(unsigned long int instruction, IntructionFormat immFlag, RegisterTypes* pReg)
{
	if (immFlag == InstrReg)
	{
		pReg->rd = (instruction & REG_RD_BITMASK) >> 11;
	}
	else if (immFlag == InstrImmData)
	{
		pReg->imm = (instruction & REG_IMM_BITMASK);
		//printf("%d\n", pReg->imm);
	}
	else if (immFlag == InstrImmAddress)
	{
		pReg->imm_addr = (instruction & REG_IMM_BITMASK);
	}

	pReg->rs = (instruction & REG_RS_BITMASK) >> 21;
	pReg->rt = (instruction & REG_RT_BITMASK) >> 16;
}

// =====================================================================================================================
int DetermineDestinationRegister(unsigned long int prissueInstruction, RegisterTypes* pReg)
{
	unsigned int instr = (prissueInstruction & CHECK_INSTR_BITMASK) >> 26;

	switch (instr)
	{
	case 0x17: //LW
		DetermineRegister(prissueInstruction, InstrImmAddress, pReg);
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
		DetermineRegister(prissueInstruction, InstrReg, pReg);
		return pReg->rd;
		break;
	case 0x38: //ADDI
	case 0x39:
	case 0x3A:
	case 0x3B:
		DetermineRegister(prissueInstruction, InstrImmData, pReg);
		return pReg->rt;
		break;
	default:
		return -1;
		break;
	}
}

InputHandler::InputHandler()
	:
	m_instructionCount(0),
	m_breakPosition(0)
{
	// Do nothing special
}

void InputHandler::LoadMipsCodeFromFile(MipsProcessor* processor, std::ifstream* pCodeFile)
{
	int  breakFlag = 0;
	std::string mipsText;

	unsigned long int* pInstructionMemory = (unsigned long int*)processor->GetInstructionMemoryPtr();
	int* pDataMemory = (int*)processor->GetDataMemoryPtr();

	while (std::getline(*pCodeFile, mipsText))
	{
		if ((strcmp(mipsText.c_str(), "\n") == 0) || (strcmp(mipsText.c_str(), "\r\n") == 0)) continue;

		// Convert the binary string to decimal equivalent
		unsigned long int readValue = std::strtoul(mipsText.c_str(), nullptr, 2);

		if (breakFlag == 0)
		{
			pInstructionMemory[m_instructionCount] = readValue;
#if DEBUG_LOG
			std::cout << "Instruction : " << pInstructionMemory[m_instructionCount] << "\n";
#endif
			if ((pInstructionMemory[m_instructionCount] & 0b01010100000000000000000000001101) == 0b01010100000000000000000000001101)
			{
				breakFlag = 1;
				m_breakPosition = m_instructionCount;
			}
		}
		else if (breakFlag == 1)
		{
			pDataMemory[m_instructionCount - m_breakPosition - 1] = (int)readValue;
#if DEBUG_LOG
			std::cout << "Data : " << pDataMemory[m_instructionCount - m_breakPosition - 1] << "\n";
#endif
		}
		m_instructionCount += 1;
	}
}

// =====================================================================================================================
ControlUnit::ControlUnit()
	:
	m_breakFlag(false),
	m_trackDirtyRegisters{ false },
	m_haltExec{0},
	m_reg{ 0 }
{
	m_tempBuffers.pPrissueQ = new Queue(4);
    m_tempBuffers.pralu1Q   = new Queue(2);
    m_tempBuffers.pralu2Q   = new Queue(2);
}

ControlUnit::~ControlUnit()
{
    delete m_tempBuffers.pPrissueQ;
    delete m_tempBuffers.pralu1Q;
    delete m_tempBuffers.pralu2Q;
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
	m_programCounter(PC_START_ADDRESS),
    m_alu1(true),
    m_alu2(false)
{
	for (int i = 0; i < 512; i++)
	{
		// Clear the instruction and data memory
		m_processorMemory.dataMemory[i] = 0;
		m_processorMemory.instructionMemory[i] = 0;
	}
}
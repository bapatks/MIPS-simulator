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
	m_t_trackDirtyRegisters{false}
{

}

void FetchStage::SimulateStage(
	unsigned long int*  pInstructionMemory,
	int*                pRegisterFile,
	unsigned short int* pPC,
	ControlUnit*        pControlUnit)
{
	int i, requiredReg;
	int targ_addr, opcode, type, arr;

	ProcessorBuffers* pTempBuffers = &(pControlUnit->m_tempBuffers);
	RegisterTypes* pReg = &(pControlUnit->m_reg);
	bool* pDirtyRegisters = pControlUnit->m_trackDirtyRegisters;
	unsigned long int* pHaltExec = pControlUnit->m_haltExec;

#if DEBUG_LOG
	std::cout<<"\n----------Entered fetch stage";
#endif
	i = (*pPC - 256) / 4;

	//printf("\npHaltExec[0] = %u", pHaltExec[0]);
	//printf("\npHaltExec[1] = %u", pHaltExec[1]);
	for (int j = 0; j < 4; j++)
	{
		requiredReg = DetermineDestinationRegister(pTempBuffers->t_prissue[j], pReg);
		if (requiredReg != -1)
		{
			m_t_trackDirtyRegisters[requiredReg] = 1;
		}
	}

	for (int j = 0; j < 2; j++)
	{
		requiredReg = DetermineDestinationRegister(pInstructionMemory[i + j], pReg);
		if (requiredReg != -1)
		{
			m_t_trackDirtyRegisters[requiredReg] = 1;
		}
	}

	if (m_instructionFetchStall == 1)
	{
		//printf("\nFetch currently stalled");
		if (pHaltExec[0] != 0)
		{
			if (m_re1 != -1)
			{
				//printf("\ncame inside condition for m_re1");

				if (pDirtyRegisters[m_re1] == 0)
				{
					//printf("\ncame inside condition for pDirtyRegisters");
					m_instructionFetchStall = 0;
					m_re1 = -1;
				}
			}

			if (m_re2 != -1)
			{
				if (pDirtyRegisters[m_re2] == 0)
				{
					m_instructionFetchStall = 0;
					m_re2 = -1;
				}
			}
		}

		arr = CheckBuff4Empty(pTempBuffers->t_prissue);
		if (arr != -1)
		{
			m_instructionFetchStall = 0;
		}

	}

	/*printf("\nm_t_trackDirtyRegisters = [");
	for(int j=0; j<32; j++)
	{
		printf("%d, ",m_t_trackDirtyRegisters[j]);
	}
	printf("]");*/

	if (m_instructionFetchStall == 0)
	{
		for (int counter = 0; counter < 2; counter++)
		{
			if (pHaltExec[0] == 0)
			{
				type = (pInstructionMemory[i + counter] & OPCODE_TYPE_BITMASK) >> 30; //check first 2 bits
				//printf("\ntype = %d",type);
				opcode = (pInstructionMemory[i + counter] & OPCODE_BITMASK) >> 26;
				//printf("\nopcode = %d",opcode);
				//printf("\nchecking at start: %u", pInstructionMemory[i+counter]);
			}
			else if (pHaltExec[0] != 0)
			{
				type = (pHaltExec[0] & OPCODE_TYPE_BITMASK) >> 30; //check first 2 bits
				opcode = (pHaltExec[0] & OPCODE_BITMASK) >> 26;
				//printf("\nif instr= %u is waiting; type = %d; opcode = %d",pHaltExec[0],type,opcode);
			}


			/* For branch instructions, when register is not ready the IF stage is stalled i.e. m_instructionFetchStall = 1,
			 * the m_instructionFetchStall will be changed back to zero when register is available*/
			if (type == 0x01) //check if the instruction is type 1
			{
				if (opcode == 0x00) //Jump instruction
				{
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						targ_addr = (pInstructionMemory[i + counter] & 0b00000011111111111111111111111111) << 2;
						pHaltExec[1] = pInstructionMemory[i + counter];
						*pPC = targ_addr;
						//printf("-------------PC = %d", *pPC);
						break;
					}
					else
					{
						m_instructionFetchStall = 1;
					}

				}
				else if (opcode == 0x01) //JR instruction
				{
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						DetermineRegister(pInstructionMemory[i + counter], 0, pReg);
						//printf("\npInstructionMemory[%d] = %u", i+counter, pInstructionMemory[i+counter]);
						targ_addr = pRegisterFile[pReg->rs];
						//printf("\nhere m_t_trackDirtyRegisters[%d] = %d", pReg->rs, m_t_trackDirtyRegisters[pReg->rs]);
						if ((pDirtyRegisters[pReg->rs] == 0) && (m_t_trackDirtyRegisters[pReg->rs] == 0))
						{
							pHaltExec[1] = pInstructionMemory[i + counter];
							pHaltExec[0] = 0;
							*pPC = targ_addr;
							//printf("\n----------here PC is %d",*pPC);
							break;
						}
						else
						{
							pHaltExec[0] = pInstructionMemory[i + counter];
							m_t_trackDirtyRegisters[pReg->rs] = 0;
							if (pDirtyRegisters[pReg->rs] == 1)
							{
								m_re1 = pReg->rs;
							}
							m_instructionFetchStall = 1; //
							if (counter == 1)
							{
								*pPC = *pPC + 4;
							}
							break;
						}
					}
					else
					{
						m_instructionFetchStall = 1;
					}
				}

				else if (opcode == 0x02) //BEQ instruction
				{
					DetermineRegister(pInstructionMemory[i + counter], 2, pReg);
					targ_addr = ((pReg->imm_addr) * 4); //same as shifting a binary number left by 2
					//printf("\nNext I will try to check pDirtyRegisters condition");
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						if (((pDirtyRegisters[pReg->rs] == 0) && (pDirtyRegisters[pReg->rt] == 0)) && (m_t_trackDirtyRegisters[pReg->rs] == 0) && (m_t_trackDirtyRegisters[pReg->rt] == 0))
						{
							//printf("\npDirtyRegisters condition checked successfully!!!");
							if (pRegisterFile[pReg->rs] == pRegisterFile[pReg->rt])
							{
								pHaltExec[1] = pInstructionMemory[i + counter];
								pHaltExec[0] = 0;
								*pPC = *pPC + targ_addr;
								break;
							}

							else if (pRegisterFile[pReg->rs] != pRegisterFile[pReg->rt])
							{
								pHaltExec[1] = pInstructionMemory[i + counter];
								pHaltExec[0] = 0;
								break;
							}
						}
						else
						{
							//printf("\npDirtyRegisters condition check FAILED!!!");
							pHaltExec[0] = pInstructionMemory[i + counter];
							m_t_trackDirtyRegisters[pReg->rs] = 0;
							m_t_trackDirtyRegisters[pReg->rt] = 0;
							if (pDirtyRegisters[pReg->rs] == 1)
							{
								m_re1 = pReg->rs;
							}
							if (pDirtyRegisters[pReg->rt] == 1)
							{
								m_re2 = pReg->rt;
							}
							m_instructionFetchStall = 1;
							break;
						}
					}
					else
					{
						m_instructionFetchStall = 1;
					}
				}

				else if (opcode == 0x03) //BLTZ instruction
				{
					DetermineRegister(pInstructionMemory[i + counter], 2, pReg);
					targ_addr = (pReg->imm_addr) * 4; //same as shifting a binary number left by 2
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						if (pDirtyRegisters[pReg->rs] == 0 && (m_t_trackDirtyRegisters[pReg->rs] == 0))
						{
							if (pRegisterFile[pReg->rs] < 0)
							{
								pHaltExec[1] = pInstructionMemory[i + counter];
								pHaltExec[0] = 0;
								*pPC = *pPC + targ_addr;
								break;
							}
							else
							{
								pHaltExec[1] = pInstructionMemory[i + counter];
								pHaltExec[0] = 0;
								break;
							}
						}
						else
						{
							pHaltExec[0] = pInstructionMemory[i + counter];
							m_t_trackDirtyRegisters[pReg->rs] = 0;
							if (pDirtyRegisters[pReg->rs] == 1)
							{
								m_re1 = pReg->rs;
							}
							m_instructionFetchStall = 1;
							break;
						}
					}
					else
					{
						m_instructionFetchStall = 1;
					}

				}

				else if (opcode == 0x04) //BGTZ instruction
				{
					DetermineRegister(pInstructionMemory[i + counter], 2, pReg);
					targ_addr = (pReg->imm_addr) * 4; //same as shifting a binary number left by 2
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						if ((pDirtyRegisters[pReg->rs] == 0) && (m_t_trackDirtyRegisters[pReg->rs] == 0))
						{
							if (pRegisterFile[pReg->rs] > 0)
							{
								pHaltExec[1] = pInstructionMemory[i + counter];
								pHaltExec[0] = 0;
								*pPC = *pPC + targ_addr;
								break;
							}
							else
							{
								pHaltExec[1] = pInstructionMemory[i + counter];
								pHaltExec[0] = 0;
								break;
							}

						}
						else
						{
							pHaltExec[0] = pInstructionMemory[i + counter];
							m_t_trackDirtyRegisters[pReg->rs] = 0;
							if (pDirtyRegisters[pReg->rs] == 1)
							{
								m_re1 = pReg->rs;
							}
							m_instructionFetchStall = 1;
							break;
						}
					}
					else
					{
						m_instructionFetchStall = 1;
					}

				}

				else if (opcode == 0x05) //BREAK instruction
				{
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						pHaltExec[1] = pInstructionMemory[i + counter];
						m_instructionFetchStall = 1;
						pControlUnit->m_breakFlag = 1;
						break;
					}
					else if (arr == -1)
					{
						m_instructionFetchStall = 1;
					}

				}

				else if (opcode == 0x0B) //NOP instruction
				{
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						pHaltExec[1] = pInstructionMemory[i + counter];
					}
					else if (arr == -1)
					{
						m_instructionFetchStall = 1;
					}
					//break;
				}


				else if (opcode == 0x06 || 0x07 || 0x08 || 0x09 || 0x0A) //SW,LW,SLL,SRL,SRA instruction
				{
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						pTempBuffers->t_prissue[arr] = pInstructionMemory[i + counter];
					}
					else if (arr == -1)
					{
						m_instructionFetchStall = 1;
					}

				}


			}

			else if (type == 0x03) //check if instruction is type 2
			{
				if (opcode == 0x00 || 0x01 || 0x02 || 0x03 || 0x04 || 0x05 || 0x06 || 0x07 || 0x08 || 0x09 || 0x0A || 0x0B)
				{
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					//printf("arr = %d",arr);
					if (arr != -1)
					{
						//printf("I am in add code: %u", pInstructionMemory[i+counter]);
						//printf("\ni+counter = %d\n",i+counter);
						pTempBuffers->t_prissue[arr] = pInstructionMemory[i + counter];
					}
					else if (arr == -1)
					{
						m_instructionFetchStall = 1;
					}
				}
			}



			if (counter == 1)
			{
				if (m_instructionFetchStall == 0)
				{
					*pPC = *pPC + 8;
				}
				else if (m_instructionFetchStall == 1)
				{
					*pPC = *pPC + 4;
				}
			}

		}

	}
}
// =====================================================================================================================
ControlUnit::ControlUnit()
	:
	m_breakFlag(false),
	m_trackDirtyRegisters{ false },
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
#include <iostream>
#include "simulator.h"

Simulator::Simulator(InputHandler* pInputParser)
	:
	m_pInputParser(pInputParser)
{
	// Just create the file and close it
	m_simFile.open(SIMULATION_FILE);
	if (!(m_simFile.is_open()))
	{
		std::cout << "Cannot write to file";
	}
	else
	{
		m_simFile.close();
	}
}

int Simulator::RunSimulation(
	MipsProcessor* processor)
{
	unsigned long int* pInstructionMemory = (unsigned long int*)processor->GetInstructionMemoryPtr();
	int* pDataMemory = (int*)processor->GetDataMemoryPtr();

	unsigned short int* PC = (unsigned short int*)processor->GetProgramCounterPtr();

	int breakPosition    = m_pInputParser->m_breakPosition;
	int instructionCount = m_pInputParser->m_instructionCount;

	int dataStartAddress = (*PC) + (breakPosition * 4);

	while (((*PC) >= PC_START_ADDRESS) &&
		   ((*PC) < dataStartAddress))
	{
		processor->Fetch();
	}
	return 0;
}

// =====================================================================================================================
// Simulate Fetch Stage
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
	std::cout << "\n----------Entered fetch stage";
#endif
	i = (*pPC - 256) / 4;

	//printf("\npHaltExec[0] = %u", pHaltExec[0]);
	//printf("\npHaltExec[1] = %u", pHaltExec[1]);
	for (int j = 0; j < 4; j++)
	{
		requiredReg = DetermineDestinationRegister(pTempBuffers->t_prissue[j], pReg);
		if (requiredReg != -1)
		{
			m_t_trackDirtyRegisters[requiredReg] = true;
		}
	}

	for (int j = 0; j < 2; j++)
	{
		requiredReg = DetermineDestinationRegister(pInstructionMemory[i + j], pReg);
		if (requiredReg != -1)
		{
			m_t_trackDirtyRegisters[requiredReg] = true;
		}
	}

	if (m_instructionFetchStall)
	{
		//printf("\nFetch currently stalled");
		if (pHaltExec[0] != 0)
		{
			if (m_re1 != -1)
			{
				//printf("\ncame inside condition for m_re1");

				if (pDirtyRegisters[m_re1] == false)
				{
					//printf("\ncame inside condition for pDirtyRegisters");
					m_instructionFetchStall = 0;
					m_re1 = -1;
				}
			}

			if (m_re2 != -1)
			{
				if (pDirtyRegisters[m_re2] == false)
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

	if (!m_instructionFetchStall)
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
			else
			{
				type = (pHaltExec[0] & OPCODE_TYPE_BITMASK) >> 30; //check first 2 bits
				opcode = (pHaltExec[0] & OPCODE_BITMASK) >> 26;
				//printf("\nif instr= %u is waiting; type = %d; opcode = %d",pHaltExec[0],type,opcode);
			}

			//check if the instruction is type 1
			if (type == 0x01)
			{
				// For branch instructions, when register is not ready the IF stage is stalled i.e. m_instructionFetchStall = 1,
				// the m_instructionFetchStall will be changed back to zero when register is available

				//Jump instruction
				if (opcode == 0x00)
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
				//JR instruction
				else if (opcode == 0x01)
				{
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						DetermineRegister(pInstructionMemory[i + counter], 0, pReg);
						//printf("\npInstructionMemory[%d] = %u", i+counter, pInstructionMemory[i+counter]);
						targ_addr = pRegisterFile[pReg->rs];
						//printf("\nhere m_t_trackDirtyRegisters[%d] = %d", pReg->rs, m_t_trackDirtyRegisters[pReg->rs]);
						if ((pDirtyRegisters[pReg->rs] == false) && (m_t_trackDirtyRegisters[pReg->rs] == false))
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
							m_t_trackDirtyRegisters[pReg->rs] = false;
							if (pDirtyRegisters[pReg->rs] == true)
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
				//BEQ instruction
				else if (opcode == 0x02)
				{
					DetermineRegister(pInstructionMemory[i + counter], 2, pReg);
					targ_addr = ((pReg->imm_addr) * 4); //same as shifting a binary number left by 2
					//printf("\nNext I will try to check pDirtyRegisters condition");
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						if (((pDirtyRegisters[pReg->rs] == false) &&
							(pDirtyRegisters[pReg->rt] == false)) &&
							(m_t_trackDirtyRegisters[pReg->rs] == false) &&
							(m_t_trackDirtyRegisters[pReg->rt] == false))
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
							m_t_trackDirtyRegisters[pReg->rs] = false;
							m_t_trackDirtyRegisters[pReg->rt] = false;
							if (pDirtyRegisters[pReg->rs] == true)
							{
								m_re1 = pReg->rs;
							}
							if (pDirtyRegisters[pReg->rt] == true)
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
				//BLTZ instruction
				else if (opcode == 0x03)
				{
					DetermineRegister(pInstructionMemory[i + counter], 2, pReg);
					targ_addr = (pReg->imm_addr) * 4; //same as shifting a binary number left by 2
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						if (pDirtyRegisters[pReg->rs] == false && (m_t_trackDirtyRegisters[pReg->rs] == false))
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
							m_t_trackDirtyRegisters[pReg->rs] = false;
							if (pDirtyRegisters[pReg->rs] == true)
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
				//BGTZ instruction
				else if (opcode == 0x04)
				{
					DetermineRegister(pInstructionMemory[i + counter], 2, pReg);
					targ_addr = (pReg->imm_addr) * 4; //same as shifting a binary number left by 2
					arr = CheckBuff4Empty(pTempBuffers->t_prissue);
					if (arr != -1)
					{
						if ((pDirtyRegisters[pReg->rs] == false) && (m_t_trackDirtyRegisters[pReg->rs] == false))
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
							m_t_trackDirtyRegisters[pReg->rs] = false;
							if (pDirtyRegisters[pReg->rs] == true)
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
				//BREAK instruction
				else if (opcode == 0x05)
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
				//NOP instruction
				else if (opcode == 0x0B)
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
				//SW,LW,SLL,SRL,SRA instruction
				else if ((opcode == 0x06) ||
				         (opcode == 0x07) ||
						 (opcode == 0x08) ||
						 (opcode == 0x09) ||
						 (opcode == 0x0A))
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

			//check if instruction is type 2
			else if (type == 0x03)
			{
				if ((opcode >= 0x00) &&
					(opcode <= 0x0B))
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
	std::cout << "\nPC: " << (*pPC);
}

// =====================================================================================================================
// Simulate Issue Stage
void IssueStage::SimulateStage()
{

}
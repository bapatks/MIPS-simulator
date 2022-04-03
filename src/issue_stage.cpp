#include "utils.h"
#include "mips_arch.h"

// =====================================================================================================================
IssueStage::IssueStage()
	:
	m_prissueQ(4)
{

}

// =====================================================================================================================
// Simulate Issue Stage
void IssueStage::SimulateStage(
	ControlUnit* pControlUnit)
{
    unsigned long int instruction;

    unsigned int instr;

    //int requiredReg;

    // This temporary buffer tracks which registers are required by the instructions currently in the pre-issue Q, in
    // order to prevent any data hazards.
    bool someXReg[32]{ false };

    bool canIssueLwSwInstruction{ true }, canIssueNonLwSwInstruction{ true };

    ProcessorBuffers* pTempBuffers = pControlUnit->GetTempBuffersPtr();
    RegisterTypes*    pReg         = pControlUnit->GetRegistertypesPtr();

    bool* pDirtyRegisters = pControlUnit->GetDirtyRegistersPtr();

    size_t prissueQSize = pTempBuffers->pPrissueQ->size();

    for (int i = 0; i < prissueQSize; ++i)
    {
        instruction = pTempBuffers->pPrissueQ->front();
        instr       = (instruction & CHECK_INSTR_BITMASK) >> 26;

        if (canIssueLwSwInstruction)
        {
            bool isStorePending = false;

            if (instr == 0x16) //SW
            {
                if (!(pTempBuffers->pralu1Q->isFull()))
                {
                    DetermineRegister(instruction, InstrImmAddress, pReg);

                    //printf("\nHere R->rs = %d, R->rt = %d, rstatus[%d] = %d, rstatus[%d] = %d",R->rs, R->rt,R->rs, rstatus[R->rs], R->rt, rstatus[R->rt]);
                    if ((pDirtyRegisters[pReg->rs] == false) &&
                        (pDirtyRegisters[pReg->rt] == false) &&
                        (someXReg[pReg->rs] == false)        &&
                        (someXReg[pReg->rt] == false))
                    {
                        if (!isStorePending)
                        {
                            bool status = pTempBuffers->pralu1Q->push(instruction);

                            if (status)
                            {
                                pTempBuffers->pPrissueQ->pop();

                                canIssueLwSwInstruction   = false;
                                someXReg[pReg->rs]        = true;
                                pDirtyRegisters[pReg->rt] = true;
                            }
#if DEBUG_LOG
                            else
                            {
                                std::cout << "ALERT: Push to Pre-ALU1 Queue failed" << std::endl;
                            }
#endif
                        }
                        else
                        {
                            someXReg[pReg->rt] = true;
                            someXReg[pReg->rs] = true;
                        }
                    }
                    else
                    {
                        isStorePending     = true;
                        someXReg[pReg->rt] = true;
                        someXReg[pReg->rs] = true;
                    }
                }
                else
                {
                    isStorePending = true;
                    someXReg[pReg->rt] = true;
                    someXReg[pReg->rs] = true;
                }
            }
            else if (instr == 0x17) //LW
            {
                if (!(pTempBuffers->pralu1Q->isFull()))
                {
                    DetermineRegister(instruction, InstrImmAddress, pReg);

                    if ((pDirtyRegisters[pReg->rt] == false) &&
                        (pDirtyRegisters[pReg->rs] == false) &&
                        (someXReg[pReg->rs] == false))
                    {
                        if (!isStorePending)
                        {
                            bool status = pTempBuffers->pralu1Q->push(instruction);

                            if (status)
                            {
                                pTempBuffers->pPrissueQ->pop();

                                canIssueLwSwInstruction   = false;
                                pDirtyRegisters[pReg->rt] = true;
                            }
#if DEBUG_LOG
                            else
                            {
                                std::cout << "ALERT: Push to Pre-ALU1 Queue failed" << std::endl;
                            }
#endif
                        }
                        else
                        {
                            someXReg[pReg->rt] = true;
                        }
                    }
                    else
                    {
                        someXReg[pReg->rt] = true;
                    }
                }
                else
                {
                    someXReg[pReg->rt] = true;
                }
            }
        }

        if (canIssueNonLwSwInstruction)
        {
            switch (instr)
            {
            case 0x18:
            case 0x19:
            case 0x1A:
                if (!pTempBuffers->pralu2Q->isFull())
                {
                    DetermineRegister(instruction, InstrReg, pReg);

                    //printf("R->rt = %d, R->rd = %d, value of a is %d, value of b is %d",R->rt,R->rd,a,b);
                    if ((pDirtyRegisters[pReg->rd] == false) &&
                        (pDirtyRegisters[pReg->rt] == false) &&
                        (someXReg[pReg->rt] == false) &&
                        (someXReg[pReg->rd] == false))
                    {
                        bool status = pTempBuffers->pralu2Q->push(instruction);

                        if (status)
                        {
                            pTempBuffers->pPrissueQ->pop();

                            canIssueNonLwSwInstruction = false;
                            pDirtyRegisters[pReg->rd]  = true;
                        }
#if DEBUG_LOG
                        else
                        {
                            std::cout << "ALERT: Push to Pre-ALU2 Queue failed" << std::endl;
                        }
#endif
                    }
                    else
                    {
                        someXReg[pReg->rd] = true;
                    }
                }
                else
                {
                    someXReg[pReg->rd] = true;
                }
                break;
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
                if (!pTempBuffers->pralu2Q->isFull())
                {
                    DetermineRegister(instruction, InstrReg, pReg);

                    if ((pDirtyRegisters[pReg->rd] == false) &&
                        (pDirtyRegisters[pReg->rs] == false) &&
                        (pDirtyRegisters[pReg->rt] == false))
                    {
                        if (someXReg[pReg->rs] == false &&
                            someXReg[pReg->rt] == false)
                        {
                            bool status = pTempBuffers->pralu2Q->push(instruction);

                            if (status)
                            {
                                pTempBuffers->pPrissueQ->pop();

                                canIssueNonLwSwInstruction = false;
                                pDirtyRegisters[pReg->rd]  = true;
                            }
#if DEBUG_LOG
                            else
                            {
                                std::cout << "ALERT: Push to Pre-ALU2 Queue failed" << std::endl;
                            }
#endif

                        }

                    }
                    else
                    {
                        someXReg[pReg->rd] = true;
                    }
                }
                else
                {
                    someXReg[pReg->rd] = true;
                }
                break;
            case 0x38:
            case 0x39:
            case 0x3A:
            case 0x3B:
                if (!pTempBuffers->pralu2Q->isFull())
                {
                    DetermineRegister(instruction, InstrImmData, pReg);

                    if ((pDirtyRegisters[pReg->rt] == false) &&
                        (pDirtyRegisters[pReg->rs] == false) &&
                        (someXReg[pReg->rs] == false))
                    {
                        bool status = pTempBuffers->pralu2Q->push(instruction);

                        if (status)
                        {
                            pTempBuffers->pPrissueQ->pop();

                            canIssueNonLwSwInstruction = false;
                            pDirtyRegisters[pReg->rt] = true;
                        }
#if DEBUG_LOG
                        else
                        {
                            std::cout << "ALERT: Push to Pre-ALU2 Queue failed" << std::endl;
                        }
#endif
                    }
                    else
                    {
                        someXReg[pReg->rd] = true;
                    }
                }
                else
                {
                    someXReg[pReg->rd] = true;
                }
                break;
            default:
#if DEBUG_LOG
                std::cout << "Weird instruction found!!" << std::endl;
#endif
                break;
            }
        }

        //TODO: Is this wrong??
        // This is required so as to keep the queue unchanged at the end of the loop
        /*pTempBuffers->pPrissueQ->pop();
        pTempBuffers->pPrissueQ->push(instruction);*/
    }
}
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

	unsigned short int* pPC = (unsigned short int*)processor->GetProgramCounterPtr();

	int breakPosition    = m_pInputParser->m_breakPosition;
	int instructionCount = m_pInputParser->m_instructionCount;

	int dataStartAddress = (*pPC) + (breakPosition * 4);

	while (((*pPC) >= PC_START_ADDRESS) &&
		   ((*pPC) < dataStartAddress))
	{
		processor->Fetch();
        processor->Issue();
	}
	return 0;
}
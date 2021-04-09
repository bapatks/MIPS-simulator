#include <iostream>
#include <string>

#include "utils.h"

int CheckBuff4Empty(unsigned long int* buffer)
{
	int i;
	//printf("buffer size: %d",sizeof(buffer));
	for (i = 0; i < 4; i++)
	{
		if (buffer[i] == 0x0000)
		{
			//printf("\n%d\n",i);
			return i;
		}
	}
	return -1;

}

int CheckBuff2Empty(unsigned long int* buffer)
{
	int i;
	//printf("buffer size: %d",sizeof(buffer));
	for (i = 0; i < 2; i++)
	{
		if (buffer[i] == 0x0000)
		{
			//printf("\n%d\n",i);
			return i;
		}
	}
	return -1;

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
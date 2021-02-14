#include "input_handler.h"

InputHandler::InputHandler()
	:
	m_instructionCount(0),
	m_breakPosition(0),
	m_mipsInstructions{0},
	m_dataMemory{0}
{
	// Do nothing special
}

int InputHandler::convertBinaryStringToDecimal(const char* instruction)
{
	int num = 0;
	for (int i = 0; i < 32; i++)
	{
		num = num + ((instruction[i] - 48) * pow(2, (31 - i)));
	}
	return num;
}

void InputHandler::loadMipsCodeFromFile(FILE* pCodeFile)
{
	int  breakFlag = 0;
	char mipsText[33];

	while (fgets(mipsText, 33, pCodeFile) != NULL)
	{
		if (strcmp(mipsText, "\n") == 0 || strcmp(mipsText, "\r\n") == 0)
		{
			continue;
		}
		mipsText[32] = '\0';

		long unsigned int readValue = convertBinaryStringToDecimal(mipsText);

		if (breakFlag == 0)
		{
			m_mipsInstructions[m_instructionCount] = readValue;
#if DEBUG_LOG
			std::cout << "Instruction : " << m_mipsInstructions[m_instructionCount] << "\n";
#endif
			if ((m_mipsInstructions[m_instructionCount] & 0b01010100000000000000000000001101) == 0b01010100000000000000000000001101)
			{
				breakFlag = 1;
				m_breakPosition = m_instructionCount;
			}
		}
		else if (breakFlag == 1)
		{
			m_dataMemory[m_instructionCount - m_breakPosition - 1] = (int)readValue;
#if DEBUG_LOG
			std::cout << "Data : " << m_dataMemory[m_instructionCount-m_breakPosition-1] << "\n";
#endif
		}
		m_instructionCount += 1;
	}
}
#include "input_handler.h"

int InputHandler::convertBinaryStringToDecimal(const char* instruction)
{
	int num = 0;
	int digit;
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
			(*pMipsInstructions)[m_instructionCount] = readValue;
#ifdef DEBUG_LOG
			cout << "%u\n", (*pMipsInstructions)[m_instructionCount];
#endif
			if (((*pMipsInstructions)[m_instructionCount] & 0b01010100000000000000000000001101) == 0b01010100000000000000000000001101)
			{
				breakFlag = 1;
				m_breakPosition = m_instructionCount;
			}
		}
		else if (breakFlag == 1)
		{
			(*pDataMemory)[m_instructionCount - m_breakPosition - 1] = (int)readValue;
#ifdef DEBUG_LOG
			cout<<"Data : %d\n", (*pDataMemory)[m_instructionCount-m_breakPosition-1];
#endif
		}
		m_instructionCount += 1;
	}
}
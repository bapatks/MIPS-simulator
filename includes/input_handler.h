#include <iostream>

class InputHandler
{
public:
	void loadMipsCodeFromFile(FILE* pCodeFile);

private:
	int                m_instructionCount;
	int                m_breakPosition;
	int*               pDataMemory[1000];
	long unsigned int* pMipsInstructions[1000];

	int convertBinaryStringToDecimal(const char* ins);
};
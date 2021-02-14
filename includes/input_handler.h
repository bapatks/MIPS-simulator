#pragma once
#include <iostream>

#define DEBUG_LOG 1

class InputHandler
{
public:
	InputHandler();
	void loadMipsCodeFromFile(FILE* pCodeFile);

private:
	int               m_instructionCount;
	int               m_breakPosition;
	int               m_dataMemory[1000];
	long unsigned int m_mipsInstructions[1000];

	int convertBinaryStringToDecimal(const char* ins);
};
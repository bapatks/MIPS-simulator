#pragma once
#include <fstream>
#include "mips_arch.h"

#define DEBUG_LOG 1

struct InputHandler
{
public:
	InputHandler();
	void LoadMipsCodeFromFile(MipsProcessor* processor, std::ifstream* pCodeFile);

	int m_instructionCount;
	int m_breakPosition;
};
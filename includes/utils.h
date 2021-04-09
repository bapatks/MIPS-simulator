#pragma once
#include <fstream>
#include <iostream>
#include "mips_arch.h"

#define DEBUG_LOG 1

int CheckBuff4Empty(unsigned long int* buffer);

int CheckBuff2Empty(unsigned long int* buffer);

struct InputHandler
{
public:
	InputHandler();
	void LoadMipsCodeFromFile(MipsProcessor* processor, std::ifstream* pCodeFile);

	// This might seem a misnomer: m_instructionCount is in fact the count of all the lines
	// read from the input file, which consists of 32 bit instructions + data words
	int m_instructionCount;
	int m_breakPosition;
};
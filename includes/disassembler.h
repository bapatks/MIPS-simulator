#pragma once
#include <iostream>
#include <fstream>
#include "mips_arch.h"

#define DISASSEMBLY_FILE "C:\\Users\\kaust\\Desktop\\disasm.txt"

class Disassembler
{
public:
	Disassembler();
	void GenerateDisassembly(MipsProcessor* processor, int breakPosition, int instructionCount);

private:
	RegisterTypes reg;
	std::ofstream* pDisasmFile;
};
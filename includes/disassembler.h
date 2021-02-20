#pragma once
#include <iostream>
#include <fstream>
#include "input_handler.h"
#include "mips_arch.h"

#define DISASSEMBLY_FILE "disasm.txt"

class Disassembler
{
public:
	Disassembler(InputHandler* pInputParser);
	void GenerateDisassembly(MipsProcessor* processor);

private:
	RegisterTypes reg;
	InputHandler* m_pInputParser;
	std::ofstream m_disasmFile;
};
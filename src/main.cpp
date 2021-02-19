#include <iostream>
#include <fstream>
#include <string>
#include "input_handler.h"
#include "mips_arch.h"
#include "disassembler.h"

int main(int argc, char *argv[])
{
	std::string inputFileName;

	if (argc == 2)
	{
		// assuming the length of filename/path will not exceed 256 characters
		inputFileName = argv[1];
	}
	else
	{
		std::cout<<"Usage : MIPSsim <input file name>";
		exit(-1);
	}

	std::ifstream file(inputFileName);

	if (!file.is_open())
	{
		std::cout<<"File not found";
		return -1;
	}

	MipsProcessor processor;
	InputHandler inputParser;
	Disassembler dis;

	inputParser.LoadMipsCodeFromFile(&processor, &file);

	dis.GenerateDisassembly(&processor, inputParser.m_breakPosition, inputParser.m_instructionCount);
	//simulator(instr_fetch, data_mem, break_pos, count);
	return 0;
}
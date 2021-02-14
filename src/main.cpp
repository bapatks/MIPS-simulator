#include <stdio.h>
#include <iostream>
#include "input_handler.h"

int main(int argc, char *argv[])
{
	char  fileName[15];
	FILE* pFile;

	if (argc >= 2)
	{
		strcpy(fileName, argv[1]);
	}
	else
	{
		std::cout<<"Usage : MIPSsim <input fileName>";
		exit(1);
	}

	pFile = fopen(fileName, "r");

	if (pFile == NULL)
	{
		std::cout<<"File not found";
		return -1;
	}

	InputHandler inputParser;

	inputParser.loadMipsCodeFromFile(pFile);

	//disassembler(instr_fetch, data_mem, break_pos, count);
	//simulator(instr_fetch, data_mem, break_pos, count);

}
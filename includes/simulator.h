#pragma once
#include <fstream>
#include "input_handler.h"
#include "mips_arch.h"

#define SIMULATION_FILE "sim.txt"

class Simulator
{
public:
	Simulator(InputHandler* pInputParser);
	int RunSimulation(MipsProcessor* processor);

private:
	InputHandler* m_pInputParser;
	std::ofstream m_simFile;
};
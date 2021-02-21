#include <bitset>
#include <iostream>
#include "disassembler.h"

Disassembler::Disassembler(InputHandler* pInputParser)
	:
	reg{0},
	m_pInputParser(pInputParser)
{
	// Just create the file and close it
	m_disasmFile.open(DISASSEMBLY_FILE);
	if (!(m_disasmFile.is_open()))
	{
		std::cout << "Cannot write to file";
	}
	else
	{
		m_disasmFile.close();
	}
}

void Disassembler::GenerateDisassembly(
	MipsProcessor* processor)
{
	unsigned long int* pInstructionMemory = (unsigned long int*)processor->GetInstructionMemoryPtr();
	int* pDataMemory = (int*)processor->GetDataMemoryPtr();

	unsigned short int PC = *((unsigned short int*)processor->GetProgramCounterPtr());

	int breakPosition    = m_pInputParser->m_breakPosition;
	int instructionCount = m_pInputParser->m_instructionCount;

	int data;
	unsigned long int instruction;
	unsigned int type, opcode, sourceAddress, targetAddress;

	RegisterTypes reg{ 0 };

	m_disasmFile.open(DISASSEMBLY_FILE);

	for (int i = 0; i < (breakPosition + 1); i++)
	{
		instruction = pInstructionMemory[i];

		type   = (instruction & OPCODE_TYPE_BITMASK) >> 30; //check first 2 bits
		opcode = (instruction & OPCODE_BITMASK) >> 26;

		// Convert the instruction word back into string to facilitate writing disassembly
		std::string instructionString = std::bitset<32>(instruction).to_string();

		// Check for type 1 instructions
		if (type == 0x01)
		{
			switch (opcode)
			{
			// Jump instruction
			case 0x00:
				targetAddress = (instruction & ~(CHECK_INSTR_BITMASK)) << 2;
				m_disasmFile << instructionString << "\t" << PC << "\tJ #" << targetAddress << "\n";
				break;

			// JR instruction
			case 0x01:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tJR R" << reg.rs << "\n";
				break;

			case 0x02:
				DetermineRegister(instruction, 2, &reg);
				targetAddress = reg.imm_addr << 2; // same as multiplying by 4
				m_disasmFile << instructionString << "\t" << PC << "\tBEQ R" << reg.rs << ", R" << reg.rt << ", #" << targetAddress << "\n";
				break;

			case 0x03:
				DetermineRegister(instruction, 2, &reg);
				targetAddress = reg.imm_addr << 2; // same as multiplying by 4
				m_disasmFile << instructionString << "\t" << PC << "\tBLTZ R" << reg.rs << ", #" << targetAddress << "\n";
				break;

			case 0x04:
				DetermineRegister(instruction, 2, &reg);
				targetAddress = reg.imm_addr << 2; // same as multiplying by 4
				m_disasmFile << instructionString<<"\t" << PC << "\tBGTZ R" << reg.rs << ", #" << targetAddress << "\n";
				break;

			case 0x05:
				m_disasmFile << instructionString << "\t" << PC << "\tBREAK\n";
				break;

			case 0x06:
				DetermineRegister(instruction, 2, &reg);
				// here, rs => base and imm_addr => offset
				m_disasmFile << instructionString << "\t" << PC << "\tSW R" << reg.rt << ", " << reg.imm_addr << "(R" << reg.rs << ")\n";
				break;

			case 0x07:
				DetermineRegister(instruction, 2, &reg);
				// here, rs => base and imm_addr => offset
				m_disasmFile << instructionString << "\t" << PC << "\tLW R" << reg.rt << ", " << reg.imm_addr << "(R" << reg.rs << ")\n";
				break;

			case 0x08:
				DetermineRegister(instruction, 0, &reg);
				sourceAddress = (instruction & 0b00000000000000000000011111000000) >> 6;
				m_disasmFile << instructionString << "\t" << PC << "\tSLL R" << reg.rd << ", R" << reg.rt << ", #" << sourceAddress << "\n";
				break;

			case 0x09:
				DetermineRegister(instruction, 0, &reg);
				sourceAddress = (instruction & 0b00000000000000000000011111000000) >> 6;
				m_disasmFile << instructionString << "\t" << PC << "\tSRL R" << reg.rd << ", R" << reg.rt << ", #" << sourceAddress << "\n";
				break;

			case 0x0A:
				DetermineRegister(instruction, 0, &reg);
				sourceAddress = (instruction & 0b00000000000000000000011111000000) >> 6;
				m_disasmFile << instructionString << "\t" << PC << "\tSRA R" << reg.rd << ", R" << reg.rt << ", #" << sourceAddress << "\n";
				break;

			case 0x0B:
				m_disasmFile << instructionString << "\t" << PC << "\tNOP\n";
				break;

			default:
				std::cout << "Unknown instruction read. Was a valid input file provided?";
#if DEBUG_LOG
				std::cout << "Invalid instruction found : " << instructionString;
#endif
				break;
			}
		}
		// Check for type 2 instructions
		else if (type == 0x03)
		{
			switch (opcode)
			{
			case 0x00:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tADD R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x01:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tSUB R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x02:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tMUL R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x03:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tAND R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x04:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tOR R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x05:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tXOR R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x06:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tNOR R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x07:
				DetermineRegister(instruction, 0, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tSLT R" << reg.rd << ", R" << reg.rs << ", R" << reg.rt << "\n";
				break;

			case 0x08:
				DetermineRegister(instruction, 1, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tADDI R" << reg.rt << ", R" << reg.rs << ", #" << reg.imm << "\n";
				break;

			case 0x09:
				DetermineRegister(instruction, 1, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tANDI R" << reg.rt << ", R" << reg.rs << ", #" << reg.imm << "\n";
				break;

			case 0x0A:
				DetermineRegister(instruction, 1, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tORI R" << reg.rt << ", R" << reg.rs << ", #" << reg.imm << "\n";
				break;

			case 0x0B:
				DetermineRegister(instruction, 1, &reg);
				m_disasmFile << instructionString << "\t" << PC << "\tXORI R" << reg.rt << ", R" << reg.rs << ", #" << reg.imm << "\n";
				break;

			default:
				std::cout << "Unknown instruction read. Was a valid input file provided?";
#if DEBUG_LOG
				std::cout << "Invalid instruction found : " << instructionString;
#endif
				break;
			}
		}
		PC = PC + 4;
	}

	for (int j = 0; j < (instructionCount - breakPosition - 1); j++)
	{
		data = pDataMemory[j];

		// Convert the instruction word back into string to facilitate writing disassembly
		std::string dataString = std::bitset<32>(data).to_string();

		m_disasmFile << dataString << "\t" << PC << "\t" << data << "\n";
		PC = PC + 4;
	}

	m_disasmFile.close();
}
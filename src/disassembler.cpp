#include <bitset>
#include "disassembler.h"

Disassembler::Disassembler()
	:
	reg{0}
{
	/*pDisasmFile->open("E:\\disasm.txt", ios::write);
	if (!(pDisasmFile->is_open()))
	{
		std::cout << "Cannot write to file";
	}*/
}

void Disassembler::GenerateDisassembly(
	MipsProcessor* processor,
	int            breakPosition,
	int            instructionCount)
{
	unsigned long int* pInstructionMemory = (unsigned long int*)processor->GetInstructionMemoryPtr();
	int* pDataMemory = (int*)processor->GetDataMemoryPtr();

	int data;
	unsigned long int instruction;
	unsigned int type, opcode;

	unsigned short int PC = PC_START_ADDRESS;

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
//			switch (opcode)
//			{
//			// Jump instruction
//			case 0x00:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				targ_addr = (instruction & 0b00000011111111111111111111111111) << 2;
//				fprintf(disasmfp, "%s\t%d\tJ #%d \n", instr, PC, targ_addr);
//				break;
//
//			// JR instruction
//			case 0x01:
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tJR R%d\n", instr, PC, RG.rs);
//				break;
//
//			case 0x02:
//				determine_register(instruction, 2, &RG);
//				targ_addr = (RG.imm_addr * 4); //same as shifting a binary number left by 2
//				fprintf(disasmfp, "%s\t%d\tBEQ R%d, R%d, #%d\n", instr, PC, RG.rs, RG.rt, targ_addr);
//				break;
//
//			case 0x03:
//				determine_register(instruction, 2, &RG);
//				targ_addr = RG.imm_addr * 4; //same as shifting a binary number left by 2
//				fprintf(disasmfp, "%s\t%d\tBLTZ R%d, #%d\n", instr, PC, RG.rs, targ_addr);
//				break;
//
//			case 0x04:
//				determine_register(instruction, 2, &RG);
//				targ_addr = RG.imm_addr * 4; //same as shifting a binary number left by 2
//				fprintf(disasmfp, "%s\t%d\tBGTZ R%d, #%d\n", instr, PC, RG.rs, targ_addr);
//				break;
//
//			case 0x05:
//				fprintf(disasmfp, "%s\t%d\tBREAK\n", instr, PC);
//				break;
//
//			case 0x06:
//				determine_register(instruction, 2, &RG);
//				// here, rs => base and imm_addr => offset
//				fprintf(disasmfp, "%s\t%d\tSW R%d, %d(R%d)\n", instr, PC, RG.rt, RG.imm_addr, RG.rs);
//				break;
//
//			case 0x07:
//				determine_register(instruction, 2, &RG);
//				// here, rs => base and imm_addr => offset
//				fprintf(disasmfp, "%s\t%d\tLW R%d, %d(R%d)\n", instr, PC, RG.rt, RG.imm_addr, RG.rs);
//				break;
//
//			case 0x08:
//				determine_register(instruction, 0, &RG);
//				sa = (instruction & 0b00000000000000000000011111000000) >> 6;
//				fprintf(disasmfp, "%s\t%d\tSLL R%d, R%d, #%d\n", instr, PC, RG.rd, RG.rt, sa);
//				break;
//
//			case 0x09:
//				determine_register(instruction, 0, &RG);
//				sa = (instruction & 0b00000000000000000000011111000000) >> 6;
//				fprintf(disasmfp, "%s\t%d\tSRL R%d, R%d, #%d\n", instr, PC, RG.rd, RG.rt, sa);
//				break;
//
//			case 0x0A:
//				determine_register(instruction, 0, &RG);
//				sa = (instruction & 0b00000000000000000000011111000000) >> 6;
//				fprintf(disasmfp, "%s\t%d\tSRA R%d, R%d, #%d\n", instr, PC, RG.rd, RG.rt, sa);
//				break;
//
//			case 0x0B:
//				fprintf(disasmfp, "%s\t%d\tNOP\n", instr, PC);
//				break;
//
//			default:
//				std::cout << "Unknown instruction read. Was a valid input file provided?";
//#if DEBUG_LOG
//				std::cout << "Invalid instruction found : " << instructionString;
//#endif
//				break;
//			}
		}
		// Check for type 2 instructions
		else if (type == 0x03)
		{
#if DEBUG_LOG
			//printf("%s\t%d\t%d\n",instr,PC,opcode);
#endif
//			switch (opcode)
//			{
//			case 0x00:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tADD R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x01:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tSUB R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x02:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tMUL R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x03:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tAND R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x04:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tOR R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x05:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tXOR R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x06:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tNOR R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x07:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 0, &RG);
//				fprintf(disasmfp, "%s\t%d\tSLT R%d, R%d, R%d \n", instr, PC, RG.rd, RG.rs, RG.rt);
//				break;
//
//			case 0x08:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 1, &RG);
//				fprintf(disasmfp, "%s\t%d\tADDI R%d, R%d, #%d \n", instr, PC, RG.rt, RG.rs, RG.imm);
//				break;
//
//			case 0x09:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 1, &RG);
//				fprintf(disasmfp, "%s\t%d\tANDI R%d, R%d, #%d \n", instr, PC, RG.rt, RG.rs, RG.imm);
//				break;
//
//			case 0x0A:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 1, &RG);
//				fprintf(disasmfp, "%s\t%d\tORI R%d, R%d, #%d \n", instr, PC, RG.rt, RG.rs, RG.imm);
//				break;
//
//			case 0x0B:
//				//printf("%s\t%d\t%d\n", instr,PC,opcode);
//				determine_register(instruction, 1, &RG);
//				fprintf(disasmfp, "%s\t%d\tXORI R%d, R%d, #%d \n", instr, PC, RG.rt, RG.rs, RG.imm);
//				break;
//
//			default:
//				std::cout << "Unknown instruction read. Was a valid input file provided?";
//#if DEBUG_LOG
//				std::cout << "Invalid instruction found : " << instructionString;
//#endif
//			}
		}
		PC = PC + 4;
	}

	for (int j = 0; j < (instructionCount - breakPosition - 1); j++)
	{
		data = pDataMemory[j];

		// Convert the instruction word back into string to facilitate writing disassembly
		std::string dataString = std::bitset<32>(data).to_string();

		//fprintf(disasmfp, "%s\t%d\t%d\n", instr, PC, data_mem[j]);
		PC = PC + 4;
	}

	pDisasmFile->close();
}
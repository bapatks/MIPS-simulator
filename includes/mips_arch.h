#pragma once

#define PC_START_ADDRESS 256

#define OPCODE_TYPE_BITMASK 0b11000000000000000000000000000000
#define OPCODE_BITMASK      0b00111100000000000000000000000000
#define REG_RS_BITMASK      0b00000011111000000000000000000000
#define REG_RT_BITMASK      0b00000000000111110000000000000000
#define REG_RD_BITMASK      0b00000000000000001111100000000000
#define REG_IMM_BITMASK     0b00000000000000001111111111111111
#define CHECK_16_BIT        0b00000000000000001000000000000000
#define CHECK_INSTR_BITMASK 0b11111100000000000000000000000000

struct Memory
{
	int               dataMemory[512];
	unsigned long int instructionMemory[512];
};

struct RegisterTypes
{
	int                rs;       // source register 1
	int                rt;       // source register 2
	int                rd;       // destination register
	short int          imm;      // stores 16 bit signed immediate data
	unsigned short int imm_addr; // stores 16 bit offset address
};

void DetermineRegister(unsigned long int instruction, int immFlag, RegisterTypes* pReg);
int DetermineDestinationRegister(unsigned long int prissueInstruction, RegisterTypes* pReg);

struct ProcessorBuffers
{
	unsigned long int t_prissue[4]; // temp pre issue buffer
	unsigned long int t_pralu1[2];  // temp pre alu1 queue
	unsigned long int t_pralu2[2];  // temp pre alu2 queue
	unsigned long int t_premem;     //pre mem buffer
	unsigned long int t_postalu2;   //post alu2 buffer
	unsigned long int t_postmem;    //post mem buffer
	unsigned long int dum;          //dummy buffer, needed?
	int calc;
	int mem_addr;
	int mem_val;
};

class ControlUnit
{
public:
	ControlUnit();
	bool m_breakFlag; // used by fetch stage and simulator
	bool m_trackDirtyRegisters[32]; // rstatus - Used by fetch, issue, mem stage

	//TODO: consider renaming to haltExec
	unsigned long int m_haltExec[2]; //used by fetch stage and simulator

	RegisterTypes m_reg;

	// Intermediate buffers that are used to move data between pipeline stages
	ProcessorBuffers m_tempBuffers;
};

// Each stage will read a permanent buffer and write to a temporary buffer
class FetchStage
{
public:
	FetchStage();
	void SimulateStage(
		unsigned long int*  pInstructionMemory,
		int*                pRegisterFile,
		unsigned short int* pPC,
		ControlUnit*        pControlUnit);

private:
	// These are used for checking if a particular register is free for use.
	// TODO: change the names, what is "re"?
	int m_re1, m_re2;
	bool m_instructionFetchStall;
	// array of temp values - temp_rstatus
	bool m_t_trackDirtyRegisters[32];
};

class IssueStage
{
public:
	IssueStage();
	void SimulateStage();

private:
	unsigned long int m_prissue[4];   // pre issue buffer
};

class AluStage
{
public:
	AluStage();

private:
	unsigned long int pralu[2]; // pre alu queue or FIFO buffer
	unsigned long int postalu; // post alu buffer

};

class MemoryStage
{
public:
	MemoryStage();

private:
	unsigned long int premem;     //pre mem buffer
	unsigned long int postmem;    //post mem buffer
};

class WriteBackStage
{

};

class MipsProcessor
{
public:
	MipsProcessor();

	void* GetInstructionMemoryPtr() {
		return &(m_processorMemory.instructionMemory);
	}

	void* GetDataMemoryPtr() {
		return &(m_processorMemory.dataMemory);
	}

	void* GetControlUnitPtr() {
		return &(m_controlUnit);
	}

	void* GetProgramCounterPtr() {
		return &(m_programCounter);
	}

	void Fetch() {
		m_fetch.SimulateStage(m_processorMemory.instructionMemory, m_registerFile, &m_programCounter, &m_controlUnit);
	};

	void Issue() {
		m_issue.SimulateStage();
	}

private:
	int                m_registerFile[32];
	unsigned short int m_programCounter;
	Memory             m_processorMemory;

	// Pipeline stages
	FetchStage     m_fetch;
	IssueStage     m_issue;
	AluStage       m_alu1;
	AluStage       m_alu2;
	MemoryStage    m_memory;
	WriteBackStage m_writeBack;

	ControlUnit    m_controlUnit;
};

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

void DetermineRegister(unsigned long int instruction, int immFlag, struct RegisterTypes* reg);

struct ProcessorBuffers
{
	unsigned long int pralu1[2];  //pre alu1 queue
	unsigned long int pralu2[2];  //pre alu2 queue
	unsigned long int premem;     //pre mem buffer
	unsigned long int postalu2;   //post alu2 buffer
	unsigned long int postmem;    //post mem buffer
	unsigned long int dum;        //dummy buffer, needed?
	int calc;
	int mem_addr;
	int mem_val;
};

struct ControlUnit
{
public:
	ControlUnit();
	bool breakFlag; // used by fetch stage and simulator
	bool trackDirtyRegisters[32]; // rstatus - Used by fetch, issue, mem stage

	//TODO: consider renaming to haltExecution
	unsigned long int wait_exec[2]; //used by fetch stage and simulator
};

// Each stage will read a permanent buffer and write to a temporary buffer
class FetchStage
{
public:
	FetchStage();

private:
	bool m_instructionFetchStall;
	// array of temp values - temp_rstatus
	bool m_t_trackDirtyRegisters[32];
};

class IssueStage
{
public:
	IssueStage();

private:
	unsigned long int m_prissue[4];   // pre issue buffer
	unsigned long int m_t_prissue[4]; // temp pre issue buffer
};

class AluStage
{
public:
	AluStage();

private:
	unsigned long int pralu[2];     // pre alu queue or FIFO buffer
	unsigned long int m_t_pralu[2]; // temp pre alu queue
};

// Do we need this?
class MemoryStage
{

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

private:
	int                m_registerFile[32];
	unsigned short int m_programCounter;
	Memory             m_processorMemory;

	// Intermediate buffers that are used to move data between pipeline stages
	// Each stage should be exposed to only a subset of buffers relevant to it
	ProcessorBuffers m_buffers;

	// Pipeline stages
	FetchStage     m_fetch;
	IssueStage     m_issue;
	AluStage       m_alu1;
	AluStage       m_alu2;
	MemoryStage    m_memory;
	WriteBackStage m_writeBack;

	ControlUnit    m_controlUnit;
};

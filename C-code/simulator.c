/*
 ============================================================================
 Name        : Project_spartacus.c
 Author      : Kaustubh Bapat
 Version     : 8.0
 Description : Pipelined processor simulation
 ============================================================================
 */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

#define OPCODE_TYPE_BITMASK 0b11000000000000000000000000000000
#define OPCODE_BITMASK 0b00111100000000000000000000000000
#define RS_BITMASK 0b00000011111000000000000000000000
#define RT_BITMASK 0b00000000000111110000000000000000
#define RD_BITMASK 0b00000000000000001111100000000000
#define IMM_BITMASK 0b00000000000000001111111111111111
#define CHECK_16_BIT 0b00000000000000001000000000000000
#define CHECK_INSTR 0b11111100000000000000000000000000

int simulator(long unsigned int *, int *, int, int);
int binstr2dec(const char *);
void bit2str(long unsigned int, char *);
int disassembler(long unsigned int *, int *, int, int);
void signbit2str(int, char*);

int reg[32], rstatus[32], temp_rstatus[32], IF_stall = 0, BREAK_flag = 0, temp_mem_addr = 0, temp_calc = 0, temp_reg = 0;
long unsigned int wait_exec[2], temp_prissue[4], temp_pralu1[2],temp_pralu2[2],temp_premem=0, temp_postmem=0, temp_postalu2=0;
long unsigned int dum_postalu2=0;

int re1 = -1, re2 = -1; //re1 and re2 initialized to a number not in 0-31 range

struct registers
{
  int rs;
  int rt;
  int rd;
  short int imm; //stores 16 bit signed immediate data
  unsigned short int imm_addr; //stores 16 bit offset address
};

struct buff
{
	long unsigned int prissue[4]; //pre issue buffer
	long unsigned int pralu1[2]; //pre alu1 queue
	long unsigned int pralu2[2]; //pre alu2 queue
	long unsigned int premem; //pre mem buffer
	long unsigned int postalu2; //post alu2 buffer
	long unsigned int postmem; //post mem buffer
	long unsigned int dum; //dummy buffer
	int calc;
	int mem_addr;
	int mem_val;

};

void determine_register(long unsigned int, int, struct registers *rg);
void fetch(long unsigned int *, int *PC, struct registers *R, struct buff *B);
int check_buff4_empty(long unsigned int* buffer);
int check_buff2_empty(long unsigned int* buffer);
void put(int*, int);
void print_prissue(long unsigned int* , struct registers *R, FILE* simfp);
void if_wait_exe(struct registers *R, FILE* simfp, int *targ_addr);
void print_pralu1(long unsigned int* , struct registers *R, FILE* simfp);
void print_pralu2(long unsigned int* , struct registers *R, FILE* simfp);
void print_premem(long unsigned int , struct registers *R, FILE* simfp);
void print_postmem(long unsigned int , struct registers *R, FILE* simfp);
void print_postalu2(long unsigned int , struct registers *R, FILE* simfp);
void issue(struct registers *R, struct buff *B);
void alu1(struct registers *R, struct buff *B);
void alu2(struct registers *R, struct buff *B);
void mem(struct registers *R, struct buff *B, int, int*);
void dummy(struct buff *B);
void writeback(struct registers *R, struct buff *B);
int check_val(int *, int);
int reg_dest(long unsigned int, struct registers *R);
void adjust2Q(long unsigned int*);
void adjust4Q(long unsigned int*);

void adjust4Q(long unsigned int* buffin)
{
	int k=0;
	long unsigned int temp[4];

	//printf("\nbuffin = [");
	for(int k=0; k<4; k++)
	{
		temp[k] = 0;
		//printf("%u, ",buffin[k]);
	}
	//printf("]\n");

	for(int i=0; i<4; i++)
	{
		if(buffin[i]!=0)
		{
			temp[k] = buffin[i];
			k++;
		}
	}
	if(k<4)
	{
		for(int x = k; x<4; x++)
		{
			temp[k] = 0;
		}
	}

	/*printf("\ntemp = [");
	for(int k=0; k<4; k++)
	{
		printf("%u, ",temp[k]);
	}
	printf("]\n");*/

	for(int z=0; z<4;z++)
	{
		buffin[z] = temp[z];
	}
}

void adjust2Q(long unsigned int* buffin)
{
	int k=0;
	long unsigned int temp[2];

	//printf("\nbuffin = [");
	for(int k=0; k<2; k++)
	{
		temp[k] = 0;
		//printf("%u, ",buffin[k]);
	}
	//printf("]\n");

	for(int i=0; i<2; i++)
	{
		if(buffin[i]!=0)
		{
			temp[k] = buffin[i];
			k++;
		}
	}
	if(k<2)
	{
		for(int x = k; x<2; x++)
		{
			temp[k] = 0;
		}
	}

	for(int z=0; z<2;z++)
	{
		buffin[z] = temp[z];
	}
}

void put(int* reg_req, int val)
{
	for(int k=0; k<12; k++)
	{
		if(reg_req[k] == -1)
		{
			reg_req[k] = val;
			break;
		}
	}
}

int check_val(int* reg_req, int val)
{
	for (int j=0; j<12; j++)
	{
		if (reg_req[j] == val)
		{
			return 1;
		}
	}
	return 0;
}

int binstr2dec(const char *ins)
{
  int num = 0;
  int digit;
  for (int i=0;i<32;i++)
  {
    num = num + ((ins[i]-48) * pow(2,(31-i)));
  }
  return num;
}

int reg_dest(long unsigned int prissue, struct registers *R)
{
	long unsigned int check;
	check = (prissue & CHECK_INSTR)>>26;

	if(check == 0x17) //LW
	{
		determine_register(prissue,2,R);
		return R->rt;
	}
	else if (check == 0x18)//SLL
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x19)//SRL
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x1A)//SRA
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x30)//ADD
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x31)
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x32)
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x33)
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x34)
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x35)
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x36)
	{
		determine_register(prissue,0, R);
		return R->rd;
	}
	else if (check == 0x37)
	{
		determine_register(prissue,0, R);
		return R->rd;
	}

	else if (check == 0x38) //ADDI
	{
		determine_register(prissue,1, R);
		return R->rt;
	}
	else if (check == 0x39)
	{
		determine_register(prissue,1, R);
		return R->rt;
	}
	else if (check == 0x3A)
	{
		determine_register(prissue,1, R);
		return R->rt;
	}
	else if (check == 0x3B)
	{
		determine_register(prissue,1, R);
		return R->rt;
	}
	else
	{
		return -1;
	}
}

int check_buff4_empty(long unsigned int* buffer)
{
	int j;
	//printf("buffer size: %d",sizeof(buffer));
	for(j=0; j<4; j++)
	{
		if (buffer[j]==0x0000)
		{
			//printf("\n%d\n",i);
			return j;
		}
	}
	return -1;

}

int check_buff2_empty(long unsigned int* buffer)
{
	int j;
	//printf("buffer size: %d",sizeof(buffer));
	for(j=0; j<2; j++)
	{
		if (buffer[j]==0x0000)
		{
			//printf("\n%d\n",i);
			return j;
		}
	}
	return -1;

}

void if_wait_exe(struct registers *R, FILE* simfp, int *targ_addr)
{
	long unsigned int val0, val1;
	int check0, check1;
	val0 = wait_exec[0];
	val1 = wait_exec[1];
	check0 = (val0 & CHECK_INSTR)>>26; //check first 6 bits
	check1 = (val1 & CHECK_INSTR)>>26; //check first 6 bits
	if (check0 == 0x00)
	{
		fprintf(simfp,"\tWaiting Instruction:\n");
	}
	else if (check0 == 0x10)
	{
		fprintf(simfp,"\tWaiting Instruction:[J #%d]\n",*targ_addr);
	}
	else if (check0 == 0x11) //JR instruction
	{
		determine_register(val0,0,R);
		fprintf(simfp,"\tWaiting Instruction:[JR R%d]\n",R->rs);
	}
	else if(check0 == 0x12)
	{
		determine_register(val0,2,R);
		fprintf(simfp,"\tWaiting Instruction:[BEQ R%d, R%d, #%d]\n",R->rs,R->rt,(R->imm_addr) * 4);
	}
	else if(check0 == 0x13)
	{
		determine_register(val0,2,R);
		fprintf(simfp,"\tWaiting Instruction:[BLTZ R%d, #%d]\n",R->rs,(R->imm_addr) * 4);
	}
	else if(check0 == 0x14)
	{
		determine_register(val0,2,R);
		fprintf(simfp,"\tWaiting Instruction:[BGTZ R%d, #%d]\n",R->rs,(R->imm_addr) * 4);
	}



	//--------------------------------------------------------------------------------------
	if (check1 == 0x00)
	{
		fprintf(simfp,"\tExecuted Instruction:");
	}
	else if (check1 == 0x10)
	{
		fprintf(simfp,"\tExecuted Instruction:[J #%d]",*targ_addr);
	}
	else if (check1 == 0x11) //JR instruction
	{
		determine_register(val1,0,R);
		fprintf(simfp,"\tExecuted Instruction:[JR R%d]\n",R->rs);
	}
	else if(check1 == 0x12)
	{
		determine_register(val1,2,R);
		fprintf(simfp,"\tExecuted Instruction:[BEQ R%d, R%d, #%d]\n",R->rs,R->rt,(R->imm_addr) * 4);
	}
	else if(check1 == 0x13)
	{
		determine_register(val1,2,R);
		fprintf(simfp,"\tExecuted Instruction:[BLTZ R%d, #%d]\n",R->rs,(R->imm_addr) * 4);
	}
	else if(check1 == 0x14)
	{
		determine_register(val1,2,R);
		fprintf(simfp,"\tExecuted Instruction:[BGTZ R%d, #%d]\n",R->rs,(R->imm_addr) * 4);
	}
	else if(check1 == 0x15)
	{
		fprintf(simfp,"\tExecuted Instruction:[BREAK]\n");
	}
	else if(check1 == 0x1B)
	{
		fprintf(simfp,"\tExecuted Instruction:[NOP]\n");
	}

}

void print_prissue(long unsigned int* prissue, struct registers* R, FILE* simfp)
{
	long unsigned int stored;
	int check,sa;
	//fprintf(simfp,"\tWaiting Instruction:\n\tExecuted Instruction:\n");
	fprintf(simfp,"\nPre-Issue Queue:\n");
	for(int k=0;k<4;k++)
	{
		//printf("\nvalue of k is %d",k);
		stored = prissue[k];
		//printf("\n%u\n",stored);
		check = (stored & CHECK_INSTR)>>26; //check first 6 bits
		//printf("value of check is %d",check);
		if (check== 0x0000) //true if the buffer entry is empty
		{
			fprintf(simfp,"\tEntry %d:\n",k);
		}
		else if (check == 0x16)
		{
			determine_register(stored,2, R);
			fprintf(simfp,"\tEntry %d: [SW R%d, %d(R%d)]\n",k,R->rt,R->imm_addr, R->rs);
		}
		else if(check == 0x17)
		{
			determine_register(stored,2, R);
			fprintf(simfp,"\tEntry %d: [LW R%d, %d(R%d)]\n",k,R->rt,R->imm_addr, R->rs);
		}
		else if(check == 0x18)
		{
			determine_register(stored,0, R);
			sa = (stored & 0b00000000000000000000011111000000)>>6;
			fprintf(simfp,"\tEntry %d: [SLL R%d, R%d, #%d]\n",k,R->rd,R->rt,sa);
		}
		else if(check == 0x19)
		{
			determine_register(stored,0, R);
			sa = (stored & 0b00000000000000000000011111000000)>>6;
			fprintf(simfp,"\tEntry %d: [SRL R%d, R%d, #%d]\n",k,R->rd,R->rt,sa);
		}
		else if(check == 0x1A)
		{
			determine_register(stored,0, R);
			sa = (stored & 0b00000000000000000000011111000000)>>6;
			fprintf(simfp,"\tEntry %d: [SRA R%d, R%d, #%d]\n",k,R->rd,R->rt,sa);
		}
		else if (check == 0x30)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [ADD R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x31)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [SUB R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x32)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [MUL R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x33)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [AND R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x34)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [OR R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x35)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [XOR R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x36)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [NOR R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x37)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [SLT R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x38)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [ADDI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}
		else if (check == 0x39)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [ANDI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}
		else if (check == 0x3A)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [ORI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}
		else if (check == 0x3B)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [XORI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}

	}
}

void print_pralu1(long unsigned int* pralu, struct registers* R, FILE* simfp)
{
	long unsigned int stored;
	int check;

	for(int k=0;k<2;k++)
	{
		//printf("\nvalue of k is %d",k);
		stored = pralu[k];
		//printf("\n%u\n",stored);
		check = (stored & CHECK_INSTR)>>26; //check first 6 bits
		//printf("value of check is %d",check);
		if (check== 0x0000) //true if the buffer entry is empty
		{
			fprintf(simfp,"\tEntry %d:\n",k);
		}
		else if (check == 0x16)
		{
			determine_register(stored,2, R);
			fprintf(simfp,"\tEntry %d: [SW R%d, %d(R%d)]\n",k,R->rt,R->imm_addr, R->rs);
		}
		else if(check == 0x17)
		{
			determine_register(stored,2, R);
			fprintf(simfp,"\tEntry %d: [LW R%d, %d(R%d)]\n",k,R->rt,R->imm_addr, R->rs);
		}

	}
}

void print_pralu2(long unsigned int* pralu, struct registers* R, FILE* simfp)
{
	long unsigned int stored;
	int check,sa;

	for(int k=0;k<2;k++)
	{
		//printf("\nvalue of k is %d",k);
		stored = pralu[k];
		//printf("\n%u\n",stored);
		check = (stored & CHECK_INSTR)>>26; //check first 6 bits
		//printf("value of check is %d",check);
		if (check== 0x0000) //true if the buffer entry is empty
		{
			fprintf(simfp,"\tEntry %d:\n",k);
		}
		else if(check == 0x18)
		{
			determine_register(stored,0, R);
			sa = (stored & 0b00000000000000000000011111000000)>>6;
			fprintf(simfp,"\tEntry %d: [SLL R%d, R%d, #%d]\n",k,R->rd,R->rt,sa);
		}
		else if(check == 0x19)
		{
			determine_register(stored,0, R);
			sa = (stored & 0b00000000000000000000011111000000)>>6;
			fprintf(simfp,"\tEntry %d: [SRL R%d, R%d, #%d]\n",k,R->rd,R->rt,sa);
		}
		else if(check == 0x1A)
		{
			determine_register(stored,0, R);
			sa = (stored & 0b00000000000000000000011111000000)>>6;
			fprintf(simfp,"\tEntry %d: [SRA R%d, R%d, #%d]\n",k,R->rd,R->rt,sa);
		}
		else if (check == 0x30)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [ADD R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x31)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [SUB R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x32)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [MUL R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x33)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [AND R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x34)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [OR R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x35)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [XOR R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x36)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [NOR R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x37)
		{
			determine_register(stored,0, R);
			fprintf(simfp,"\tEntry %d: [SLT R%d, R%d, R%d]\n",k,R->rd,R->rs, R->rt);
		}
		else if (check == 0x38)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [ADDI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}
		else if (check == 0x39)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [ANDI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}
		else if (check == 0x3A)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [ORI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}
		else if (check == 0x3B)
		{
			determine_register(stored,1, R);
			fprintf(simfp,"\tEntry %d: [XORI R%d, R%d, #%d]\n",k,R->rt,R->rs, R->imm);
		}

	}
}

void print_postmem(long unsigned int postmem, struct registers* R, FILE* simfp)
{
	int check;

	//printf("\n%u\n",stored);
	check = (postmem & CHECK_INSTR)>>26; //check first 6 bits
	//printf("value of check is %d",check);
	if (check== 0x0000) //true if the buffer entry is empty
	{
		fprintf(simfp,"Post-MEM Queue:\n");
	}
	else if (check == 0x16)
	{
		determine_register(postmem,2, R);
		fprintf(simfp,"Post-MEM Queue: [SW R%d, %d(R%d)]\n",R->rt,R->imm_addr, R->rs);
	}
	else if(check == 0x17)
	{
		determine_register(postmem,2, R);
		fprintf(simfp,"Post-MEM Queue: [LW R%d, %d(R%d)]\n",R->rt,R->imm_addr, R->rs);
	}
}

void print_postalu2(long unsigned int postalu2, struct registers* R, FILE* simfp)
{
	long unsigned int stored;
	int check,sa;

	stored = postalu2;
	//printf("\n%u\n",stored);
	check = (stored & CHECK_INSTR)>>26; //check first 6 bits
	//printf("value of check is %d",check);
	if (check== 0x0000) //true if the buffer entry is empty
	{
		fprintf(simfp,"Post-ALU2 Queue:\n");
	}
	else if(check == 0x18)
	{
		determine_register(stored,0, R);
		sa = (stored & 0b00000000000000000000011111000000)>>6;
		fprintf(simfp,"Post-ALU2 Queue: [SLL R%d, R%d, #%d]\n",R->rd,R->rt,sa);
	}
	else if(check == 0x19)
	{
		determine_register(stored,0, R);
		sa = (stored & 0b00000000000000000000011111000000)>>6;
		fprintf(simfp,"Post-ALU2 Queue: [SRL R%d, R%d, #%d]\n",R->rd,R->rt,sa);
	}
	else if(check == 0x1A)
	{
		determine_register(stored,0, R);
		sa = (stored & 0b00000000000000000000011111000000)>>6;
		fprintf(simfp,"Post-ALU2 Queue: [SRA R%d, R%d, #%d]\n",R->rd,R->rt,sa);
	}
	else if (check == 0x30)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"Post-ALU2 Queue: [ADD R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x31)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"Post-ALU2 Queue: [SUB R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x32)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"Post-ALU2 Queue: [MUL R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x33)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"Post-ALU2 Queue: [AND R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x34)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"Post-ALU2 Queue: [OR R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x35)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"Post-ALU2 Queue: [XOR R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x36)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"Post-ALU2 Queue: [NOR R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x37)
	{
		determine_register(stored,0, R);
		fprintf(simfp,"\tPost-ALU2 Queue: [SLT R%d, R%d, R%d]\n",R->rd,R->rs, R->rt);
	}
	else if (check == 0x38)
	{
		determine_register(stored,1, R);
		fprintf(simfp,"Post-ALU2 Queue: [ADDI R%d, R%d, #%d]\n",R->rt,R->rs, R->imm);
	}
	else if (check == 0x39)
	{
		determine_register(stored,1, R);
		fprintf(simfp,"Post-ALU2 Queue: [ANDI R%d, R%d, #%d]\n",R->rt,R->rs, R->imm);
	}
	else if (check == 0x3A)
	{
		determine_register(stored,1, R);
		fprintf(simfp,"Post-ALU2 Queue: [ORI R%d, R%d, #%d]\n",R->rt,R->rs, R->imm);
	}
	else if (check == 0x3B)
	{
		determine_register(stored,1, R);
		fprintf(simfp,"Post-ALU2 Queue: [XORI R%d, R%d, #%d]\n",R->rt,R->rs, R->imm);
	}
}

void determine_register(long unsigned int ins, int imm_flag, struct registers *R)
{
  if (imm_flag==0)
  {
    R->rd = (ins & RD_BITMASK)>>11;
  }
  else if (imm_flag == 1)
  {
    R->imm = (ins & IMM_BITMASK);
    //printf("%d\n", R->imm);
  }
  else if (imm_flag == 2)
  {
    R->imm_addr = (ins & IMM_BITMASK);
  }
  R->rs = (ins & RS_BITMASK)>>21;
  R->rt = (ins & RT_BITMASK)>>16;

}

void bit2str(long unsigned int ins, char *instr)
{
  //printf("entered");
  for(int j=31; j>=0; j--)
  {
    instr[j] = (ins & 0b00000000000000000000000000000001)+48;
    ins = ins >> 1;
  }
  instr[32]='\0';
}

void signbit2str(int data, char *instr)
{
  //printf("entered");
  for(int j=31; j>=0; j--)
  {
    instr[j] = (data & 0b00000000000000000000000000000001)+48;
    data = data >> 1;
  }
  instr[32]='\0';
}

void print_premem(long unsigned int premem, struct registers* R, FILE* simfp)
{
	long unsigned int stored;
	int check;
	stored = premem;
	//printf("\n%u\n",stored);
	check = (stored & CHECK_INSTR)>>26; //check first 6 bits
	//printf("value of check is %d",check);
	if (check== 0x0000) //true if the buffer entry is empty
	{
		fprintf(simfp,"Pre-MEM Queue:\n");
	}
	else if (check == 0x16)
	{
		determine_register(stored,2, R);
		fprintf(simfp,"Pre-MEM Queue: [SW R%d, %d(R%d)]\n",R->rt,R->imm_addr, R->rs);
	}
	else if(check == 0x17)
	{
		determine_register(stored,2, R);
		fprintf(simfp,"Pre-MEM Queue: [LW R%d, %d(R%d)]\n",R->rt,R->imm_addr, R->rs);
	}
}

/*----------------------------------Pipeline stages are defined starting from here-------------------------------------*/
void fetch(long unsigned int *instr_fetch, int *PC, struct registers *R, struct buff *B)
{
	int i;
	int targ_addr, opcode,type, sa, arr, required;
	char instr[32];

	//printf("\n----------Entered fetch stage");
	i = (*PC - 256)/4;

	//printf("\nwait_exec[0] = %u", wait_exec[0]);
	//printf("\nwait_exec[1] = %u", wait_exec[1]);
	for(int j=0; j<4; j++)
	{
		required = reg_dest(B->prissue[j], R);
		if (required != -1)
		{
			temp_rstatus[required] = 1;
		}
	}

	for(int j=0; j<2; j++)
	{
		required = reg_dest(instr_fetch[i+j],R);
		if(required != -1)
		{
			temp_rstatus[required] = 1;
		}
	}


	if(IF_stall == 1)
	{
		//printf("\nFetch currently stalled");
		if (wait_exec[0] != 0)
		{
			if (re1 != -1)
			{
				//printf("\ncame inside condition for re1");

				if (rstatus[re1] == 0)
				{
					//printf("\ncame inside condition for rstatus");
					IF_stall = 0;
					re1 = -1;
				}
			}

			if(re2 != -1)
			{
				if(rstatus[re2] == 0)
				{
					IF_stall = 0;
					re2 = -1;
				}
			}
		}

		arr = check_buff4_empty(temp_prissue);
		if (arr != -1)
		{
			IF_stall = 0;
		}

	}

	/*printf("\ntemp_rstatus = [");
	for(int j=0; j<32; j++)
	{
		printf("%d, ",temp_rstatus[j]);
	}
	printf("]");*/

	if (IF_stall == 0)
	{


		for(int counter=0; counter<2; counter++)
			{
				if (wait_exec[0] == 0)
				{
					type = (instr_fetch[i+counter] & OPCODE_TYPE_BITMASK)>>30; //check first 2 bits
					//printf("\ntype = %d",type);
					opcode = (instr_fetch[i+counter] & OPCODE_BITMASK)>>26;
					//printf("\nopcode = %d",opcode);
					//printf("\nchecking at start: %u", instr_fetch[i+counter]);
				}

				else if(wait_exec[0] != 0)
				{
					type = (wait_exec[0] & OPCODE_TYPE_BITMASK)>>30; //check first 2 bits
					opcode = (wait_exec[0] & OPCODE_BITMASK)>>26;
					//printf("\nif instr= %u is waiting; type = %d; opcode = %d",wait_exec[0],type,opcode);
				}


				/* For branch instructions, when register is not ready the IF stage is stalled i.e. IF_stall = 1,
				 * the IF_stall will be changed back to zero when register is available*/
				if (type == 0x01) //check if the instruction is type 1
				    {
				      if (opcode == 0x00) //Jump instruction
				      {
				    	  arr = check_buff4_empty(temp_prissue);
				    	  if(arr != -1)
				    	  {
				    		  targ_addr = (instr_fetch[i+counter] & 0b00000011111111111111111111111111)<<2;
				    		  wait_exec[1] = instr_fetch[i+counter];
				    		  *PC = targ_addr;
				    		  //printf("-------------PC = %d", *PC);
				    		  break;
				    	  }
				    	  else
				    	  {
				    		  IF_stall = 1;
				    	  }

				      }
				      else if (opcode == 0x01) //JR instruction
				      {
				    	  arr = check_buff4_empty(temp_prissue);
				    	  if(arr != -1)
				    	  {
				    		  determine_register(instr_fetch[i+counter], 0, R);
				    		  //printf("\ninstr_fetch[%d] = %u", i+counter, instr_fetch[i+counter]);
				    		  targ_addr = reg[R->rs];
				    		  //printf("\nhere temp_rstatus[%d] = %d", R->rs, temp_rstatus[R->rs]);
				    		  if ((rstatus[R->rs]==0) && (temp_rstatus[R->rs] == 0))
				    		  {
				    			  wait_exec[1] = instr_fetch[i+counter];
				    			  wait_exec[0] = 0;
				    			  *PC = targ_addr;
				    			  //printf("\n----------here PC is %d",*PC);
				    			  break;
				    		  }
				    		  else
				    		  {
				    			  wait_exec[0] = instr_fetch[i+counter];
				    			  temp_rstatus[R->rs] = 0;
				    			  if(rstatus[R->rs] == 1)
				    			  {
				    				  re1 = R->rs;
				    			  }
				    			  IF_stall = 1; //
				    			  if(counter == 1)
				    			  {
				    				  *PC = *PC + 4;
				    			  }
				    			  break;
				    		  }
				    	  }
				    	  else
				    	  {
				    		  IF_stall = 1;
				    	  }
				      }

				      else if (opcode == 0x02) //BEQ instruction
				      {
				    	  determine_register(instr_fetch[i+counter],2,R);
				    	  targ_addr = ((R->imm_addr)*4); //same as shifting a binary number left by 2
				    	  //printf("\nNext I will try to check rstatus condition");
				    	  arr = check_buff4_empty(temp_prissue);
				    	  if(arr != -1)
				    	  {
				    		  if (((rstatus[R->rs] == 0) && (rstatus[R->rt] == 0)) && (temp_rstatus[R->rs] == 0) && (temp_rstatus[R->rt]==0))
				    		  {
				    			  //printf("\nrstatus condition checked successfully!!!");
				    			  if (reg[R->rs] == reg[R->rt])
				    			  {
				    				  wait_exec[1] = instr_fetch[i+counter];
				    				  wait_exec[0] = 0;
				    				  *PC = *PC + targ_addr;
				    				  break;
				    			  }

				    			  else if(reg[R->rs] != reg[R->rt])
				    			  {
				    				  wait_exec[1] = instr_fetch[i+counter];
				    				  wait_exec[0] = 0;
				    				  break;
				    			  }
				    		  }
				    		  else
				    		  {
				    			  //printf("\nrstatus condition check FAILED!!!");
				    			  wait_exec[0] = instr_fetch[i+counter];
				    			  temp_rstatus[R->rs] = 0;
				    			  temp_rstatus[R->rt] = 0;
				    			  if (rstatus[R->rs] == 1)
				    			  {
				    				  re1 = R->rs;
				    			  }
				    			  if (rstatus[R->rt] == 1)
				    			  {
				    				  re2 = R->rt;
				    			  }
				    			  IF_stall = 1;
				    			  break;
				    		  }
				    	  }
				    	  else
				    	  {
				    		  IF_stall = 1;
				    	  }
				      }

				      else if(opcode == 0x03) //BLTZ instruction
				      {
				    	  determine_register(instr_fetch[i+counter],2, R);
				    	  targ_addr = (R->imm_addr) * 4; //same as shifting a binary number left by 2
				    	  arr = check_buff4_empty(temp_prissue);
				    	  if(arr != -1)
				    	  {
				    		  if (rstatus[R->rs] == 0 && (temp_rstatus[R->rs] == 0))
				    		  {
				    			  if(reg[R->rs] < 0)
				    			  {
				    				  wait_exec[1] = instr_fetch[i+counter];
				    				  wait_exec[0] = 0;
				    				  *PC = *PC + targ_addr;
				    				  break;
				    			  }
				    			  else
				    			  {
				    				  wait_exec[1] = instr_fetch[i+counter];
				    				  wait_exec[0] = 0;
				    				  break;
				    			  }
				    		  }
				    		  else
				    		  {
				    			  wait_exec[0] = instr_fetch[i+counter];
				    			  temp_rstatus[R->rs] = 0;
				    			  if (rstatus[R->rs] == 1)
				    			  {
				    				  re1 = R->rs;
				    			  }
				    			  IF_stall = 1;
				    			  break;
				    		  }
				    	  }
				    	  else
				    	  {
				    		  IF_stall = 1;
				    	  }

				      }

				      else if (opcode == 0x04) //BGTZ instruction
				      {
				    	  determine_register(instr_fetch[i+counter],2, R);
				    	  targ_addr = (R->imm_addr) * 4; //same as shifting a binary number left by 2
				    	  arr = check_buff4_empty(temp_prissue);
				    	  if(arr != -1)
				    	  {
				    		  if ((rstatus[R->rs] == 0) && (temp_rstatus[R->rs] == 0))
				    		  {
				    			  if (reg[R->rs] > 0)
				    			  {
				    				  wait_exec[1] = instr_fetch[i+counter];
				    				  wait_exec[0] = 0;
				    				  *PC = *PC + targ_addr;
				    				  break;
				    			  }
				    			  else
				    			  {
				    				  wait_exec[1] = instr_fetch[i+counter];
				    				  wait_exec[0] = 0;
				    				  break;
				    			  }

				    		  }
				    		  else
				    		  {
				    			  wait_exec[0] = instr_fetch[i+counter];
				    			  temp_rstatus[R->rs] = 0;
				    			  if (rstatus[R->rs] == 1)
				    			  {
				    				  re1 = R->rs;
				    			  }
				    			  IF_stall = 1;
				    			  break;
				    		  }
				    	  }
				    	  else
				    	  {
				    		  IF_stall = 1;
				    	  }

				      }

				      else if (opcode == 0x05) //BREAK instruction
				      {
				    	  arr = check_buff4_empty(temp_prissue);
				    	  if(arr != -1)
				    	  {
				    		  wait_exec[1] = instr_fetch[i+counter];
				    		  IF_stall = 1;
				    		  BREAK_flag = 1;
				    		  break;
				    	  }
				    	  else if(arr == -1)
				    	  {
				    		  IF_stall = 1;
				    	  }

				      }

				      else if (opcode == 0x0B) //NOP instruction
				      {
				    	  arr = check_buff4_empty(temp_prissue);
				    	  if(arr != -1)
				    	  {
				    		  wait_exec[1] = instr_fetch[i+counter];
				    	  }
				    	  else if(arr == -1)
				    	  {
				    		  IF_stall = 1;
				    	  }
				    	  //break;
				      }


				      else if (opcode == 0x06||0x07||0x08||0x09||0x0A) //SW,LW,SLL,SRL,SRA instruction
				      {
				        arr = check_buff4_empty(temp_prissue);
				        if (arr != -1)
				        {
				        	temp_prissue[arr] = instr_fetch[i+counter];
				        }
				        else if(arr == -1)
				        {
				        	IF_stall = 1;
				        }

				      }


				    }

				else if (type == 0x03) //check if instruction is type 2
				    {
				      if (opcode == 0x00||0x01||0x02||0x03||0x04||0x05||0x06||0x07||0x08||0x09||0x0A||0x0B)
				      {
				        arr = check_buff4_empty(temp_prissue);
				        //printf("arr = %d",arr);
				        if (arr != -1)
				        {
				        	//printf("I am in add code: %u", instr_fetch[i+counter]);
				        	//printf("\ni+counter = %d\n",i+counter);
				        	temp_prissue[arr] = instr_fetch[i+counter];
				        }
				        else if (arr == -1)
				        {
				        	IF_stall = 1;
				        }
				      }
				    }



				if (counter == 1)
				{
					if(IF_stall == 0)
					{
						*PC = *PC + 8;
					}
					else if(IF_stall == 1)
					{
						*PC = *PC + 4;
					}
				}

			}

	}
}


void issue(struct registers *R, struct buff *B)
{
	long unsigned int stored;
	int check, arr, temp, store_pending = 0, issue_1count = 0, issue_2count = 0;
	int reg_req[12], a,b;

	//printf("\n----------Entered issue stage");

	//printf("\nPre-issue = [");
	for(int k=0; k<12; k++)
	{
		reg_req[k] = -1; //some value not in range in 0-31
	}
	//printf("]\n");
	/*
	printf("\nPre-alu1 = [");
	for(int k=0; k<2; k++)
	{
		printf("%u, ",B->pralu1[k]);
	}
	printf("]\n");*/

	for(int k=0; k<4; k++) // 4 entries in pre issue buffer
	{
		//printf("\n 1st statement inside loop");
		stored = B->prissue[k];
		//printf("\n now stored is %u",stored);
		check = (stored & CHECK_INSTR)>>26;
		//printf("\nhere the check is %d", check);
		//printf("\ni am here @ val of k is %d; issue_1count = %d; issue_2count = %d",k,issue_1count, issue_2count);
		if(issue_1count<1)
		{
			if (check == 0x16) //SW
			{
				arr = check_buff2_empty(temp_pralu1);
				if(arr != -1)
				{
					determine_register(stored,2, R);
					//printf("\nHere R->rs = %d, R->rt = %d, rstatus[%d] = %d, rstatus[%d] = %d",R->rs, R->rt,R->rs, rstatus[R->rs], R->rt, rstatus[R->rt]);
					if((rstatus[R->rs]==0) && (rstatus[R->rt] == 0) && !check_val(reg_req, R->rs) && !check_val(reg_req, R->rt))
					{
						if (store_pending == 0)
						{

							temp_pralu1[arr] = B->prissue[k];
							temp_prissue[k] = 0; //remove instruction here, queue adjusted at the end of function
							issue_1count = issue_1count + 1;
							put(reg_req, R->rs);
							rstatus[R->rt] = 1;
						}
						else
						{
							put(reg_req, R->rt);
							put(reg_req, R->rs);
						}
					}
					else
					{
						store_pending = 1;
						put(reg_req, R->rt);
						put(reg_req, R->rs);
					}
				}
				else
				{
					store_pending = 1;
					put(reg_req, R->rt);
					put(reg_req, R->rs);
				}

			}

			else if(check == 0x17) //LW
			{
				arr = check_buff2_empty(temp_pralu1);
				//printf("\n arr for LW came %d",arr);
				if(arr != -1)
				{
					determine_register(stored,2, R);

					if((rstatus[R->rt] == 0) && (rstatus[R->rs]==0) && !check_val(reg_req, R->rs))
					{
						if (store_pending == 0)
						{
							//printf("\nI have come inside store_pending check of LW");
							temp_pralu1[arr] = (B->prissue[k]);
							//printf("\npre-issue[k] here is %u and pre-issue[k+1] is %u", B->prissue[k], B->prissue[k+1]);
							(temp_prissue[k]) = 0; //remove instruction here, queue adjusted at the end of function
							//printf("\npre-issue[k] and pre-issue[k+1] after making 0 is %u and %u", B->prissue[k], B->prissue[k+1]);
							issue_1count = issue_1count + 1;
							rstatus[R->rt] = 1;
							//printf("\nrstatus[%d] = %d",R->rt, rstatus[R->rt]);
						}
						else
						{
							put(reg_req, R->rt);
						}
					}
					else
					{
						put(reg_req, R->rt);
					}
				}
				else
				{
					put(reg_req, R->rt);
				}
			}
		}

		//printf("\n I am at line after condition for issue1_count");
		if(issue_2count<1)
		{
			if(check == 0x18 || check == 0x19 || check == 0x1A)
			{
				//printf("\n-------------------I got here");
				arr = check_buff2_empty(temp_pralu2);
				if (arr != -1)
				{
					determine_register(stored,0, R);
					a = check_val(reg_req, R->rt);
					b = check_val(reg_req, R->rd);
					//printf("R->rt = %d, R->rd = %d, value of a is %d, value of b is %d",R->rt,R->rd,a,b);
					if ((rstatus[R->rd]==0) && (rstatus[R->rt]==0) && (a == 0) && (b==0))
					{
						temp_pralu2[arr] = B->prissue[k];
						temp_prissue[k] = 0; //remove instruction here, queue adjusted at the end of function
						issue_2count++;
						rstatus[R->rd] = 1;
					}
					else
					{
						put(reg_req, R->rd);
					}
				}
				else
				{
					put(reg_req, R->rd);
				}
			}

			else if(check == 0x30 || check == 0x31 || check == 0x32 || check == 0x33 || check == 0x34 || check == 0x35 || check == 0x36 || check == 0x37)
			{
				//printf("\n----------------I got here-----");
				arr = check_buff2_empty(temp_pralu2);
				//printf("\narr here is found to be %d", arr);
				if (arr != -1)
				{
					determine_register(stored,0, R);
					//printf("\nI came inside the arr condition");
					if ((rstatus[R->rd]==0) && (rstatus[R->rs]==0) && (rstatus[R->rt]==0))
					{
						if(!check_val(reg_req, R->rs) && !check_val(reg_req, R->rt))
						{
							//printf("\nI came inside the rstatus condition");
							temp_pralu2[arr] = B->prissue[k];
							//printf("\nHere pre-alu2[%d] = %u", k, B->pralu2[arr]);
							temp_prissue[k] = 0; //remove instruction here, queue adjusted at the end of function
							issue_2count = issue_2count + 1;
							rstatus[R->rd] = 1;
						}

					}
					else
					{
						put(reg_req, R->rd);
					}
				}
				else
				{
					put(reg_req, R->rd);
				}
			}

			else if(check == 0x38 || check == 0x39 || check == 0x3A || check == 0x3B)
			{
				//printf("\n----------------I got here----------------i got here");
				arr = check_buff2_empty(temp_pralu2);
				if (arr != -1)
				{
					determine_register(stored,1, R);
					if ((rstatus[R->rt]==0) && (rstatus[R->rs]==0) && !check_val(reg_req, R->rs))
					{
						temp_pralu2[arr] = B->prissue[k];

						temp_prissue[k] = 0; //remove instruction here, queue adjusted at the end of function

						issue_2count++;
						rstatus[R->rt] = 1;
					}
					else
					{
						put(reg_req, R->rd);
					}
				}
				else
				{
					put(reg_req, R->rd);
				}
			}
		}
		/*printf("\nreg_req = [");
		for(int k=0; k<12; k++)
		{
			printf("%d, ",reg_req[k]);
		}
		printf("]\n");*/
	}



	adjust4Q(temp_prissue);

	//printf("\n------------EXIT issue stage");
	/*
	printf("\nTemp Pre-issue after adjust = [");
	for(int k=0; k<4; k++)
	{
		printf("%u, ",temp_prissue[k]);
	}
	printf("]\n");

	printf("\nTemp Pre-alu1 @ end of issue= [");
	for(int k=0; k<2; k++)
	{
		printf("%u, ",temp_pralu1[k]);
	}
	printf("]\n");*/
}

void alu1(struct registers *R, struct buff *B)
{
	long unsigned int stored;
	int check;

	//printf("\n----------Entered ALU1 stage");
	stored = B->pralu1[0];
	check = (stored & CHECK_INSTR)>>26;
	if (check == 0x16)//SW
	{
		determine_register(stored,2,R);
		temp_mem_addr = reg[R->rs] + R->imm_addr;
		temp_pralu1[0] = 0;
		temp_premem = stored;
		//printf("\ntemp_mem_addr = %d", temp_mem_addr);
	}
	else if (check == 0x17)//LW
	{
		determine_register(stored,2,R);
		temp_mem_addr = reg[R->rs] + R->imm_addr;
		temp_pralu1[0] = 0;
		temp_premem = stored;
		//printf("\ntemp_mem_addr = %d", temp_mem_addr);
	}
	adjust2Q(temp_pralu1);
	//printf("\n------------EXIT ALU1 stage");
}

void alu2(struct registers *R, struct buff *B)
{
	long unsigned int stored;
	int check,sa,temp;

	//printf("\n----------Entered ALU2 stage");
	stored = B->pralu2[0];
	check = (stored & CHECK_INSTR)>>26;

	//printf("\ninstruction: %u",stored);
	if (check == 0x18)//SLL
	{
		determine_register(stored,0, R);
		sa = (stored & 0b00000000000000000000011111000000)>>6;
		temp_calc = (reg[R->rt])<<sa;
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}

	else if (check == 0x19)//SRL
	{
		determine_register(stored,0, R);
		sa = (stored & 0b00000000000000000000011111000000)>>6;
		temp_calc = (reg[R->rt])>>sa;
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x1A)//SRA
	{
		determine_register(stored,0, R);
		sa = (stored & 0b00000000000000000000011111000000)>>6;
		if (((reg[R->rt] & 0x80000000)>>31) == 1)
		{
			temp = reg[R->rt];
			for(int k = 0;k<sa;k++)
			{
				temp = (temp>>1) | (0x80000000);
			}
			temp_calc = temp;
		}
		else if (((reg[R->rt] & 0x80000000)>>31) == 0)
		{
			temp_calc = reg[R->rt]>>sa;
		}

		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x30)// ADD
	{
		determine_register(stored,0, R);
		temp_calc = reg[R->rs] + reg[R->rt];
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x31)
	{
		determine_register(stored,0, R);
		temp_calc = reg[R->rs] - reg[R->rt];
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x32)
	{
		determine_register(stored,0, R);
		temp_calc = reg[R->rs] * reg[R->rt];
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x33)
	{
		determine_register(stored,0, R);
		temp_calc = reg[R->rs] & reg[R->rt];
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x34)
	{
		determine_register(stored,0, R);
		temp_calc = reg[R->rs] | reg[R->rt];
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x35)
	{
		determine_register(stored,0, R);
		temp_calc = reg[R->rs] ^ reg[R->rt];
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x36)
	{
		determine_register(stored,0, R);
		temp_calc = ~(reg[R->rs] | reg[R->rt]);
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x37) //SLT
	{
		determine_register(stored,0, R);
		if (reg[R->rs]<reg[R->rt])
		{
			temp_calc = 1;
		}
		else if (reg[R->rs] > reg[R->rt])
		{
			temp_calc = 0;
	    }
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if(check == 0x38) //ADDI
	{
		determine_register(stored,1, R);
		temp_calc = reg[R->rs] + (R->imm);
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x39)
	{
		determine_register(stored,1, R);
		temp_calc = reg[R->rs] & (R->imm);
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x3A)
	{
		determine_register(stored,1, R);
		temp_calc = reg[R->rs] | (R->imm);
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}
	else if (check == 0x3B) //XORI
	{
		determine_register(stored,1, R);
		temp_calc = reg[R->rs] ^ (R->imm);
		temp_pralu2[0] = 0;
		temp_postalu2 = stored;
	}

	adjust2Q(temp_pralu2);
	//printf("\nValue of temp_calc = %d",temp_calc);
	//printf("\n------------EXIT ALU2 stage");
}


void mem(struct registers *R, struct buff *B, int data_pc, int *data_mem)
{
	long unsigned int stored;
	int check;

	//printf("\n----------Entered mem stage");
	stored = B->premem;
	check = (stored & CHECK_INSTR)>>26;
	if (check == 0x16) //SW
	{
		determine_register(stored,2,R);
		data_mem[((B->mem_addr)-data_pc-1)/4] = reg[R->rt];
		//temp_premem = 0;
		//rstatus[R->rs] = 0;
		rstatus[R->rt] = 0;
	}
	else if(check == 0x17)//LW
	{
		determine_register(stored,2,R);
		temp_reg = data_mem[((B->mem_addr)-data_pc-1)/4];
		temp_postmem = stored;
		//printf("\ntemp_postmem = %u and B->mem_addr = %d",temp_postmem,B->mem_addr);
		//printf("\nWorked on LW instruction, (mem_addr-data_pc-1)/4 = %d temp_reg = %d",((B->mem_addr)-data_pc-1)/4, temp_reg);
	}
	//printf("\n------------EXIT MEM stage");
}

void dummy(struct buff* B)
{
	dum_postalu2 = B->dum;

}
void writeback(struct registers *R, struct buff *B)
{
	long unsigned int mem_store, alu_store;
	int mem_check, alu_check;

	//printf("\n----------Entered write-back stage");
	mem_store = B->postmem;
	alu_store = B->postalu2;

	//printf("\nWorking on %u and %u", mem_store, alu_store);
	mem_check = (mem_store & CHECK_INSTR)>>26;
	alu_check = (alu_store & CHECK_INSTR)>>26;

	if(mem_check == 0x17) //LW
	{
		determine_register(mem_store,2,R);
		reg[R->rt] = B->mem_val;
		rstatus[R->rt] = 0;
	}

	if (alu_check == 0x18)//SLL
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
		//printf("\n rstatus[%d] = %d", R->rd, rstatus[R->rd]);
	}
	else if (alu_check == 0x19)//SRL
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;

	}
	else if (alu_check == 0x1A)//SRA
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}
	else if (alu_check == 0x30)//ADD
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
		//printf("\nI have made rstatus[%d] = %d", R->rd, rstatus[R->rd]);
	}
	else if (alu_check == 0x31)
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}
	else if (alu_check == 0x32)
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}
	else if (alu_check == 0x33)
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}
	else if (alu_check == 0x34)
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}
	else if (alu_check == 0x35)
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}
	else if (alu_check == 0x36)
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}
	else if (alu_check == 0x37)
	{
		determine_register(alu_store,0, R);
		reg[R->rd] = B->calc;
		rstatus[R->rd] = 0;
	}

	else if (alu_check == 0x38) //ADDI
	{
		//printf("\nI got here");
		determine_register(alu_store,1, R);
		reg[R->rt] = B->calc;
		rstatus[R->rt] = 0;
	}
	else if (alu_check == 0x39)
	{
		//printf("I got here");
		determine_register(alu_store,1, R);
		reg[R->rt] = B->calc;
		rstatus[R->rt] = 0;
	}
	else if (alu_check == 0x3A)
	{
		//printf("I got here");
		determine_register(alu_store,1, R);
		reg[R->rt] = B->calc;
		rstatus[R->rt] = 0;
	}
	else if (alu_check == 0x3B)
	{
		//printf("I got here");
		determine_register(alu_store,1, R);
		reg[R->rt] = B->calc;
		rstatus[R->rt] = 0;
	}
	else if(alu_check == 0)
	{
		//do nothing
	}

	//printf("\n instruction: %u , rt = %d, temp_calc = %d",alu_store, R->rt, temp_calc);
	//printf("\n rstatus[%d] = %d", R->rt, rstatus[R->rt]);
	//printf("\n------------EXIT write-back stage\n");
}
/*-------------------------------------------------end of definition---------------------------------------------*/



int disassembler(long unsigned int *fetch, int *data_mem, int break_pos, int lines)
{
  int PC = 256, opcode, type, targ_addr, sa, brk_flg = 0, temp_imm;
  char instr[33];
  struct registers RG;
  RG.rs = 0;
  RG.rt = 0;
  RG.rd = 0;
  RG.imm = 0;
  FILE *disasmfp;
  disasmfp = fopen("disassembly.txt","w");
  if (disasmfp == NULL)
  {
    printf("File not found");
    return -1;
  }
  for(int i=0;i<(break_pos+1);i++)
  {
    type = (fetch[i] & OPCODE_TYPE_BITMASK)>>30; //check first 2 bits
    opcode = (fetch[i] & OPCODE_BITMASK)>>26;
    bit2str(fetch[i], instr);
    if (type == 0x01) //check if the instruction is type 1
    {
      if (opcode == 0x00) //Jump instruction
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        targ_addr = (fetch[i] & 0b00000011111111111111111111111111)<<2;
        fprintf(disasmfp,"%s\t%d\tJ #%d \n",instr,PC,targ_addr);
      }
      else if (opcode == 0x01) //JR instruction
      {
        determine_register(fetch[i], 0, &RG);
        fprintf(disasmfp,"%s\t%d\tJR R%d\n",instr,PC,RG.rs);
      }
      else if (opcode == 0x02)
      {
        determine_register(fetch[i],2,&RG);
        targ_addr = (RG.imm_addr*4); //same as shifting a binary number left by 2
        fprintf(disasmfp,"%s\t%d\tBEQ R%d, R%d, #%d\n",instr,PC,RG.rs,RG.rt,targ_addr);
      }
      else if (opcode == 0x03)
      {
        determine_register(fetch[i],2, &RG);
        targ_addr = RG.imm_addr * 4; //same as shifting a binary number left by 2
        fprintf(disasmfp,"%s\t%d\tBLTZ R%d, #%d\n",instr,PC,RG.rs,targ_addr);
      }
      else if (opcode == 0x04)
      {
        determine_register(fetch[i],2, &RG);
        targ_addr = RG.imm_addr * 4; //same as shifting a binary number left by 2
        fprintf(disasmfp,"%s\t%d\tBGTZ R%d, #%d\n",instr,PC,RG.rs,targ_addr);
      }
      else if (opcode == 0x05)
      {
        fprintf(disasmfp,"%s\t%d\tBREAK\n",instr,PC);
      }
      else if (opcode == 0x06)
      {
        determine_register(fetch[i],2, &RG);
        fprintf(disasmfp,"%s\t%d\tSW R%d, %d(R%d)\n",instr,PC,RG.rt,RG.imm_addr, RG.rs); //here, rs => base and imm_addr => offset
      }
      else if (opcode == 0x07)
      {
        determine_register(fetch[i],2,&RG);
        fprintf(disasmfp,"%s\t%d\tLW R%d, %d(R%d)\n",instr,PC,RG.rt,RG.imm_addr, RG.rs); //here, rs => base and imm_addr => offset
      }
      else if (opcode == 0x08)
      {
        determine_register(fetch[i],0, &RG);
        sa = (fetch[i] & 0b00000000000000000000011111000000)>>6;
        fprintf(disasmfp,"%s\t%d\tSLL R%d, R%d, #%d\n",instr,PC,RG.rd,RG.rt,sa);
      }
      else if (opcode == 0x09)
      {
        determine_register(fetch[i],0, &RG);
        sa = (fetch[i] & 0b00000000000000000000011111000000)>>6;
        fprintf(disasmfp,"%s\t%d\tSRL R%d, R%d, #%d\n",instr,PC,RG.rd,RG.rt,sa);
      }
      else if (opcode == 0x0A)
      {
        determine_register(fetch[i],0, &RG);
        sa = (fetch[i] & 0b00000000000000000000011111000000)>>6;
        fprintf(disasmfp,"%s\t%d\tSRA R%d, R%d, #%d\n",instr,PC,RG.rd,RG.rt,sa);
      }
      else if (opcode == 0x0B)
      {
        fprintf(disasmfp,"%s\t%d\tNOP\n",instr,PC);
      }
    }

    else if (type == 0x03) //check if instruction is type 2
    {
      //printf("%s\t%d\t%d\n",instr,PC,opcode);
      if (opcode == 0x00)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tADD R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x01)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tSUB R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x02)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tMUL R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x03)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tAND R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x04)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tOR R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x05)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tXOR R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x06)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tNOR R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x07)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],0, &RG);
        fprintf(disasmfp,"%s\t%d\tSLT R%d, R%d, R%d \n",instr,PC,RG.rd, RG.rs, RG.rt);
      }
      else if (opcode == 0x08)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],1, &RG);
        fprintf(disasmfp,"%s\t%d\tADDI R%d, R%d, #%d \n",instr,PC,RG.rt, RG.rs, RG.imm);
      }
      else if (opcode == 0x09)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],1, &RG);
        fprintf(disasmfp,"%s\t%d\tANDI R%d, R%d, #%d \n",instr,PC,RG.rt, RG.rs, RG.imm);
      }
      else if (opcode == 0x0A)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],1, &RG);
        fprintf(disasmfp,"%s\t%d\tORI R%d, R%d, #%d \n",instr,PC,RG.rt, RG.rs, RG.imm);
      }
      else if (opcode == 0x0B)
      {
        //printf("%s\t%d\t%d\n", instr,PC,opcode);
        determine_register(fetch[i],1, &RG);
        fprintf(disasmfp,"%s\t%d\tXORI R%d, R%d, #%d \n",instr,PC,RG.rt, RG.rs, RG.imm);
      }
    }
    PC = PC+4;
  }
  for(int j=0;j< (lines - break_pos-1); j++)
  {
    signbit2str(data_mem[j],instr);
    fprintf(disasmfp, "%s\t%d\t%d\n",instr, PC, data_mem[j]);
    PC = PC + 4;
  }
  fclose(disasmfp);
  return 0;
}

int simulator(long unsigned int *instr_fetch, int *data_mem, int break_pos, int total_lines)
{
  int PC = 256, opcode, type, targ_addr,sa, count = 1,temp, op;
  char instr[33];
  struct registers R;
  struct buff B;
  /* initialize registers to 0*/
  for(int i=0;i<32;i++)
  {
    reg[i] = 0;
  }

  //reg[2] = 5; //JUST FOR CHECKING PURPOSES, needs to be commented
  //reg[5] = 280;
  for(int j=0;j<32;j++)
  {
	  rstatus[j]=0;
	  temp_rstatus[j] = 0;
  }

  R.rs = 0;
  R.rt = 0;
  R.rd = 0;
  R.imm = 0;

  for(int i=0; i<4; i++)
  {
	  B.prissue[i] = 0; //initialize as empty buffer
	  temp_prissue[i] = 0;
  }
  for(int k=0;k<2;k++)
  {
	  B.pralu1[k] = 0; //initialize empty buffer
	  B.pralu2[k] = 0; //initialize empty buffer
	  temp_pralu1[k] = 0;
	  temp_pralu2[k] = 0;
	  wait_exec[k] = 0; //initialize as empty array
  }
  B.premem = 0;
  B.postalu2 = 0;
  B.dum = 0;
  B.mem_val = 0;

  /*-----------*/

  int data_pc = 256 + (break_pos*4);
  /* open/create files */
  FILE *simfp;
  simfp = fopen("simulation.txt","w");

  if (simfp == NULL)
  {
    printf("File not found");
    return -1;
  }

  /* -------- */

  while(PC>=256)
  {

	  /*--------------------------Fetch Stage--------------------------*/

	  //printf("\n************** Cycle: %d PC: %d", count, PC);
	  fetch(instr_fetch, &PC, &R, &B);

	  /*--------------------------End of Fetch Stage--------------------------*/
	  /*--------------------------Issue Stage--------------------------*/
	  issue(&R,&B);

	  /*--------------------------End of Issue Stage--------------------------*/
	  /*--------------------------ALU Stage--------------------------*/
	  alu1(&R, &B);
	  alu2(&R, &B);
	  /*--------------------------End of ALU Stage--------------------------*/
	  /*--------------------------MEM Stage--------------------------*/
	  mem(&R, &B, data_pc, data_mem);
	  //dummy(&B);
	  /*--------------------------End of MEM Stage--------------------------*/
	  /*--------------------------writeback Stage--------------------------*/
	  writeback(&R, &B);
	  /*--------------------------End of writeback Stage--------------------------*/

	  /*-------------------------Print starts here--------------------------*/
	  fprintf(simfp,"--------------------\nCycle:%d\n\n",count);
	  fprintf(simfp,"IF Unit:\n");
	  if_wait_exe(&R,simfp,&PC);
	  print_prissue(temp_prissue, &R, simfp);
	  if (wait_exec[1] != 0)
	  {
		  op = (wait_exec[1] & CHECK_INSTR)>>26;
		  if(op==0x10 || op==0x11)
		  {
			  wait_exec[1] = 0;
		  }
		  else if(op==0x1B)
		  {
			  wait_exec[1] = 0;
		  }
		  else
		  {
			  wait_exec[1] = 0;
			  PC = PC + 4;
		  }

	  }

	  fprintf(simfp,"Pre-ALU1 Queue:\n");
	  print_pralu1(temp_pralu1, &R, simfp);

	  print_premem(temp_premem, &R, simfp);
	  print_postmem(temp_postmem, &R, simfp);

	  fprintf(simfp,"Pre-ALU2 Queue:\n");
	  print_pralu2(temp_pralu2, &R, simfp);

	  print_postalu2(temp_postalu2, &R, simfp);


	  fprintf(simfp,"\nRegisters\nR00:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",reg[0],reg[1],reg[2],reg[3],reg[4],reg[5],reg[6],reg[7]);
	  fprintf(simfp,"R08:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",reg[8],reg[9],reg[10],reg[11],reg[12],reg[13],reg[14],reg[15]);
	  fprintf(simfp,"R16:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",reg[16],reg[17],reg[18],reg[19],reg[20],reg[21],reg[22],reg[23]);
	  fprintf(simfp,"R24:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n\n",reg[24],reg[25],reg[26],reg[27],reg[28],reg[29],reg[30],reg[31]);


	  fprintf(simfp,"Data\n");
	  if ((total_lines - break_pos-1)%8 == 0)
	  {
		  for(int k=0; k<(total_lines - break_pos-1)/8;k++)
		  {
			  fprintf(simfp,"%d:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",(data_pc+4+ 32*k),data_mem[8*k],data_mem[8*k+1],data_mem[8*k+2],data_mem[8*k+3],data_mem[8*k+4],data_mem[8*k+5],data_mem[8*k+6],data_mem[8*k+7]);
	      }
	  }
	  else if ((total_lines - break_pos-1)%8 != 0)
	  {
		  for(int k=0; k<(total_lines - break_pos-1)/8;k++)
	      {
			  fprintf(simfp,"%d:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",(data_pc+4+ 32*k),data_mem[8*k],data_mem[8*k+1],data_mem[8*k+2],data_mem[8*k+3],data_mem[8*k+4],data_mem[8*k+5],data_mem[8*k+6],data_mem[8*k+7]);
	      }
		  int k = (total_lines - break_pos-1)/8;
		  fprintf(simfp, "%d:", data_pc+4 + 32*k);
		  for (int j = 0; j < (total_lines - break_pos-1)%8; j++)
		  {
			  fprintf(simfp, "\t%d", data_mem[8*k + j] );
	      }
		  fprintf(simfp,"\n" );
	  }

	  /*-------------------------Print ends here--------------------------*/

	  for(int j=0; j<4; j++)
	  {
		  B.prissue[j] = temp_prissue[j];
	  }

	  for(int k=0; k<2; k++)
	  {
		  B.pralu1[k] = temp_pralu1[k];
		  B.pralu2[k] = temp_pralu2[k];
	  }

	  B.premem = temp_premem;
	  //B.dum = temp_postalu2;
	  B.postalu2 = temp_postalu2;
	  B.postmem = temp_postmem;
	  B.calc = temp_calc;
	  B.mem_addr = temp_mem_addr;
	  B.mem_val = temp_reg;

	  temp_premem = 0;
	  temp_postalu2 = 0;
	  temp_postmem = 0;

	  count = count + 1;

	  /*if (count == 10)
	  {
		  break;
	  }*/

	  //printf("\n---------Count now is %d and PC is %d\n",count, PC);
	  if(BREAK_flag == 1)
	  {
		  break;
	  }
  }

  fclose(simfp);

  return 0;
}


int main(int argc, char *argv[])
{
  char str[33], filename[15];
  long unsigned int instr_fetch[1000];
  int count = 0, break_flag=0, break_pos, data_mem[1000];
  FILE *infp;

  if (argc == 2)
  {
    strcpy(filename,argv[1]);
  }
  else
  {
    printf("Usage : MIPSsim <input filename>");
    exit(1);
  }

  infp = fopen(filename,"r");

  if (infp == NULL)
  {
    printf("File not found");
    return -1;
  }

  while (fgets(str,33,infp)!= NULL)
  {
	if(strcmp(str,"\n")==0 || strcmp(str,"\r\n")==0)
    {
      continue;
    }
    str[32]='\0';

    if (break_flag == 0)
    {
      instr_fetch[count] = binstr2dec(str);
      //printf("%u\n", instr_fetch[count]);
      if ((instr_fetch[count] & 0b01010100000000000000000000001101) == 0b01010100000000000000000000001101)
      {
        break_flag = 1;
        break_pos = count;
      }
    }
    else if (break_flag == 1)
    {
      data_mem[count - break_pos - 1] = binstr2dec(str);
      //printf("Data : %d\n", data_mem[count-break_pos-1]);
    }
    count = count+1;
  }

  disassembler(instr_fetch, data_mem, break_pos, count);
  simulator(instr_fetch, data_mem, break_pos, count);

}
/* Code Ends */
/*
int main()
{
  char str[33];
  long unsigned int instr_fetch[1000];
  int count = 0, break_flag=0, break_pos, data_mem[1000];
  FILE *infp;
  infp = fopen("sample.txt","r");

  if (infp == NULL)
  {
    printf("File not found");
    return -1;
  }

  /* Preprocessor block*/
/*
  while (fgets(str,33,infp)!= NULL)
  {
    if(strcmp(str,"\n")==0 || strcmp(str,"\r\n")==0)
    {
      continue;
    }
    str[32]='\0';

    if (break_flag == 0)
    {
      instr_fetch[count] = binstr2dec(str);
      //printf("%u\n", instr_fetch[count]);
      if ((instr_fetch[count] & 0b01010100000000000000000000001101) == 0b01010100000000000000000000001101)
      {
        break_flag = 1;
        break_pos = count;
      }
    }
    else if (break_flag == 1)
    {
      data_mem[count - break_pos - 1] = binstr2dec(str);
      //printf("Data : %d\n", data_mem[count-break_pos-1]);
    }
    count = count+1;
  }
  printf("break pos: %d, count: %d\n",break_pos+1, count);
  printf("instr = [");
  for(int i=0; i<(break_pos+1); i++)
  {
    printf("%u, ", instr_fetch[i]);
  }
  printf("]\n");
  printf("data_mem = [");
  for(int i=0; i<(count-break_pos+1); i++)
  {
    printf("%d, ", data_mem[i]);
  }
  printf("]\n");
  /* End of preprocessor block */
/*
  disassembler(instr_fetch, data_mem, break_pos, count);
  simulator(instr_fetch, data_mem, break_pos, count);
  //printf("here : %d\n",data_mem[3]);

}*/

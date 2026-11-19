#include "sim.h"

PC_t pc = {0, 0};
IF_DR_t if_dr = {0, 0};
DR_EX_t dr_ex = {0, 0, 0};
EX1_EX2_t ex1_ex2 = {0, 0, 0};
EX2_MEM_t ex2_mem = {0, 0, 0};
MEM_WB_t mem_wb = {0, 0, 0};

const int ALU_OP_NOP = 0x0000;
const int ALU_OP_ADD = 0x0001;
const int ALU_OP_SUB = 0x0010;
const int ALU_OP_MUL = 0x0011;
const int ALU_OP_DIV = 0x0100;

const int BRANCH_TYPE_B = 0;
const int BRANCH_TYPE_CBZ = 1;
const int BRANCH_TYPE_CBNZ = 2;
const int BRANCH_TYPE_CBNEG = 3;

const int COUNT_ID_NOP = 0;
const int COUNT_ID_ADD = 1;
const int COUNT_ID_SUB = 2;
const int COUNT_ID_MUL = 3;
const int COUNT_ID_DIV = 4;
const int COUNT_ID_LD = 5;
const int COUNT_ID_ST = 6;
const int COUNT_ID_EXIT = 7;
const int COUNT_ID_MOV = 8;
const int COUNT_ID_B = 9;
const int COUNT_ID_CBZ = 10;
const int COUNT_ID_CBNZ = 11;
const int COUNT_ID_CBNEG = 12;

const int MEMORY_STEP = 4;
int mainMemory[0x2000] = {0};
int register_file[32] = {0};
int cycle_count = 1;
int total_instructions = 1;
bool exit_program = false;
int *InstructionCounts[12] = {};

void pulse()
{
	pc.currentVal = pc.nextVal;
	if_dr.currentVal = if_dr.nextVal;
	dr_ex.currentVal = dr_ex.nextVal;
	ex1_ex2.currentVal = ex1_ex2.nextVal;
	ex2_mem.currentVal = ex2_mem.nextVal;
	mem_wb.currentVal = mem_wb.nextVal;
	cycle_count++;
}

void fetch()
{
	if_dr.nextVal = mainMemory[pc.currentVal / MEMORY_STEP];
	pc.nextVal = pc.currentVal + MEMORY_STEP;
	if_dr.nextPCInc = pc.nextVal;
}

void decode()
{
	int code = if_dr.currentVal;
	int op = (code & 0xff000000) >> (4 * 6);
	int b1 = (code & 0x00ff0000) >> (4 * 4);
	int b2 = (code & 0x0000ff00) >> (4 * 2);
	int b3 = (code & 0x000000ff);

	// R format
	if (op <= 0x09 && ((op % 2) == 0))
	{
		dr_ex.Reg2Loc = 0;
		dr_ex.ALUSrc = 0;
		dr_ex.MemRead = 0;
		dr_ex.MemWrite = 0;
		dr_ex.Mem2Reg = 0;
		dr_ex.PCSrc = 0;
		dr_ex.RegWrite = 1;
		dr_ex.Rd = b1;
		dr_ex.operand1 = register_file[b2];
		dr_ex.operand2 = register_file[b3];

		switch (op)
		{
		case 0x02:
			dr_ex.Op = ALU_OP_ADD;
			dr_ex.CountId = COUNT_ID_ADD;
			break;
		case 0x04:
			dr_ex.Op = ALU_OP_SUB;
			dr_ex.CountId = COUNT_ID_SUB;
			break;
		case 0x06:
			dr_ex.Op = ALU_OP_MUL;
			dr_ex.CountId = COUNT_ID_MUL;
			break;
		case 0x08:
			dr_ex.Op = ALU_OP_DIV;
			dr_ex.CountId = COUNT_ID_DIV;
			break;
		default:
			dr_ex.Op = ALU_OP_NOP;
			break;
		}
	}

	// I format
	if (op <= 0x09 && ((op % 2) == 1))
	{
		dr_ex.Reg2Loc = 0;
		dr_ex.ALUSrc = 1;
		dr_ex.MemRead = 0;
		dr_ex.MemWrite = 0;
		dr_ex.Mem2Reg = 0;
		dr_ex.PCSrc = 0;
		dr_ex.RegWrite = 1;
		dr_ex.Rd = b1;
		dr_ex.operand1 = register_file[b2];
		dr_ex.operand2 = b3;

		switch (op)
		{
		case 0x03:
			dr_ex.Op = ALU_OP_ADD;
			dr_ex.CountId = COUNT_ID_ADD;
			break;
		case 0x05:
			dr_ex.Op = ALU_OP_SUB;
			dr_ex.CountId = COUNT_ID_SUB;
			break;
		case 0x07:
			dr_ex.Op = ALU_OP_MUL;
			dr_ex.CountId = COUNT_ID_MUL;
			break;
		case 0x09:
			dr_ex.Op = ALU_OP_DIV;
			dr_ex.CountId = COUNT_ID_DIV;
			break;
		default:
			dr_ex.Op = ALU_OP_NOP;
			break;
		}
	}

	// Load (D)
	if ((op == 0x0B))
	{
		dr_ex.Reg2Loc = 0;
		dr_ex.ALUSrc = 1;
		dr_ex.Op = ALU_OP_ADD;
		dr_ex.MemRead = 1;
		dr_ex.MemWrite = 0;
		dr_ex.Mem2Reg = 1;
		dr_ex.PCSrc = 0;
		dr_ex.RegWrite = 1;
		dr_ex.Rd = b1;
		dr_ex.operand1 = register_file[b2];
		dr_ex.operand2 = b3;
		dr_ex.CountId = COUNT_ID_LD;
	}

	// Store (D)
	if (op == 0x0D)
	{
		dr_ex.Reg2Loc = 1;
		dr_ex.ALUSrc = 1;
		dr_ex.Op = ALU_OP_ADD;
		dr_ex.MemRead = 0;
		dr_ex.MemWrite = 1;
		dr_ex.Mem2Reg = 1;
		dr_ex.PCSrc = 0;
		dr_ex.RegWrite = 0;
		dr_ex.Rt = register_file[b1];
		dr_ex.operand1 = register_file[b2];
		dr_ex.operand2 = b3;
		dr_ex.CountId = COUNT_ID_ST;
	}

	// Branch (B/CB)
	if ((op >= 0x13) && (op <= 0x19))
	{
		dr_ex.Reg2Loc = 0;
		dr_ex.ALUSrc = 1;
		dr_ex.Op = ALU_OP_NOP;
		dr_ex.MemRead = 0;
		dr_ex.MemWrite = 0;
		dr_ex.Mem2Reg = 0;
		dr_ex.PCSrc = 1;
		dr_ex.RegWrite = 0;
		dr_ex.Branch = 1;
		dr_ex.Rt = register_file[b1];
		dr_ex.operand2 = b2 << (4 * 2) | b3;

		switch (op)
		{
		case 0x13:
			dr_ex.BranchType = BRANCH_TYPE_B;
			dr_ex.CountId = COUNT_ID_B;
			break;
		case 0x15:
			dr_ex.BranchType = BRANCH_TYPE_CBZ;
			dr_ex.CountId = COUNT_ID_CBZ;
			break;
		case 0x17:
			dr_ex.BranchType = BRANCH_TYPE_CBNZ;
			dr_ex.CountId = COUNT_ID_CBNZ;
			break;
		case 0x19:
			dr_ex.BranchType = BRANCH_TYPE_CBNEG;
			dr_ex.CountId = COUNT_ID_CBNEG;
			break;
		default:
			printf("ERROR: Branch with no branch type");
			break;
		}
	}

	// Mov (IW)
	if (op == 0x11)
	{
		dr_ex.Reg2Loc = 0;
		dr_ex.ALUSrc = 0;
		dr_ex.Op = ALU_OP_NOP;
		dr_ex.MemRead = 0;
		dr_ex.MemWrite = 1;
		dr_ex.Mem2Reg = 0;
		dr_ex.PCSrc = 0;
		dr_ex.RegWrite = 1;
		dr_ex.Rd = b1;
		dr_ex.operand2 = b2 << (4 * 2) | b3;
		dr_ex.Mov = 1;
		dr_ex.CountId = COUNT_ID_MOV;
	}

	// NOP
	if (op == 0x00)
	{
		dr_ex.Reg2Loc = 0;
		dr_ex.ALUSrc = 0;
		dr_ex.Op = ALU_OP_NOP;
		dr_ex.MemRead = 0;
		dr_ex.MemWrite = 0;
		dr_ex.Mem2Reg = 0;
		dr_ex.PCSrc = 0;
		dr_ex.RegWrite = 0;
		dr_ex.CountId = COUNT_ID_NOP;
	}

	// EXIT
	if (op == 0x0E)
	{
		dr_ex.Exit = 1;
		dr_ex.CountId = COUNT_ID_EXIT;
	}
}

void exec1()
{
	ex1_ex2.MemRead = dr_ex.MemRead;
	ex1_ex2.MemWrite = dr_ex.MemWrite;
	ex1_ex2.Mem2Reg = dr_ex.Mem2Reg;
	ex1_ex2.RegWrite = dr_ex.RegWrite;
	ex1_ex2.Branch = dr_ex.Branch;
	ex1_ex2.operand1 = dr_ex.operand1;
	ex1_ex2.operand2 = dr_ex.operand2;
	ex1_ex2.Exit = dr_ex.Exit;
	ex1_ex2.CountId = dr_ex.CountId;

	int operand1 = dr_ex.operand1;
	int operand2 = (dr_ex.ALUSrc) ? dr_ex.operand2 : dr_ex.operand2;

	if (dr_ex.Mov)
		ex1_ex2.Rd = dr_ex.operand2;

	if (dr_ex.Branch)
		ex1_ex2.BranchAddress = pc.nextVal = pc.currentVal + (MEMORY_STEP * dr_ex.operand2);

	if (dr_ex.Op == ALU_OP_ADD)
		ex1_ex2.nextVal = operand1 + operand2;

	if (dr_ex.Op == ALU_OP_SUB)
		ex1_ex2.nextVal = operand1 - operand2;
}

void exec2()
{
	ex2_mem.MemRead = ex1_ex2.MemRead;
	ex2_mem.MemWrite = ex1_ex2.MemWrite;
	ex2_mem.Mem2Reg = ex1_ex2.Mem2Reg;
	ex2_mem.RegWrite = ex1_ex2.RegWrite;
	ex2_mem.Rd = ex1_ex2.Rd;
	ex2_mem.Exit = ex1_ex2.Exit;
	ex2_mem.CountId = ex1_ex2.CountId;

	if (ex1_ex2.Branch)
	{
		if ((dr_ex.BranchType == BRANCH_TYPE_B) ||
			(dr_ex.BranchType == BRANCH_TYPE_CBZ && dr_ex.Rt == 0) ||
			(dr_ex.BranchType == BRANCH_TYPE_CBNZ && dr_ex.Rt != 0) ||
			(dr_ex.BranchType == BRANCH_TYPE_CBNEG && dr_ex.Rt < 0))
			squashFetchAndDecode();
		else
			printf("ERROR: Branch condition code without proper branch type");
	}

	if (ex1_ex2.Op == ALU_OP_MUL)
		ex2_mem.nextVal = ex1_ex2.operand1 * ex1_ex2.operand2;

	if (ex1_ex2.Op == ALU_OP_DIV)
		ex2_mem.nextVal = ex1_ex2.operand1 / ex1_ex2.operand2;
}

void memory()
{
	mem_wb.Exit = ex2_mem.Exit;
	mem_wb.RegWrite = ex2_mem.RegWrite;
	mem_wb.nextVal = ex2_mem.currentVal;
	mem_wb.CountId = ex2_mem.CountId;

	if (ex2_mem.MemWrite)
		mainMemory[ex2_mem.currentVal] = ex2_mem.Rt;
	if (ex2_mem.MemRead)
		mem_wb.Rd = mainMemory[ex2_mem.currentVal];
}

void writeBack()
{
	if (mem_wb.RegWrite)
		register_file[mem_wb.Rd] = mem_wb.currentVal;
	if (mem_wb.Exit)
	{
		exit_program = true;
		printf(" ===================== Exiting program =====================\n");
	}
}

void putMachineCodeIntoMemory(char **argv)
{
	FILE *fptr;
	fptr = fopen(argv[1], "r");

	while (!feof(fptr))
	{
		int address;
		int value;
		fscanf(fptr, "%x %x", &address, &value);
		mainMemory[address / MEMORY_STEP] = value;
		// printf("%08x\n", mainMemory[address / MEMORY_STEP]);
	}
	fclose(fptr);
}

void squashFetchAndDecode()
{
	if_dr.currentVal = 0;
	if_dr.nextVal = 0;
	// if_dr.nextPCInc

	dr_ex.currentVal = 0;
	dr_ex.nextVal = 0;
	dr_ex.operand2 = 0;
	dr_ex.operand1 = 0;
	dr_ex.Rt = 0;
	dr_ex.Rd = 0;
	dr_ex.Reg2Loc = 0;
	dr_ex.ALUSrc = 0;
	dr_ex.Op = 0;
	dr_ex.MemRead = 0;
	dr_ex.MemWrite = 0;
	dr_ex.Mem2Reg = 0;
	dr_ex.PCSrc = 0;
	dr_ex.RegWrite = 0;
	dr_ex.Branch = 0;
	dr_ex.BranchType = 0;
}

/*------------------------------------------------------------------------
	Main Function Definition
------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	// TODO: Code your simulator here
	// copy the input file to a vector
	putMachineCodeIntoMemory(argv);

	while (!exit_program)
	{
		fetch();
		decode();
		exec1();
		exec2();
		memory();
		writeBack();
		pulse();
	}

	// printf("== cycle |    fetch |   decode |  exec1   |  exec2   |   memory | writeBack|\n");
	// printf("== ------+----------+----------+----------+----------+----------+----------|\n");

	printf("=== Simulation Reports ===\n");
	printf("Total Instructions=%d Cycles=%d IPC= %.2f\n", total_instructions, cycle_count, (double)total_instructions / cycle_count);
	return 0;
}

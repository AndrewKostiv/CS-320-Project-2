#include "sim.h"

PC_t pc = {0, 0};
IF_DR_t if_dr;
DR_EX_t dr_ex;
EX1_EX2_t ex1_ex2;
EX2_MEM_t ex2_mem;
MEM_WB_t mem_wb;

FetchStage_t fetchStage;
DecodeStage_t decodeStage;

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
int register_file[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int cycle_count = 0;
int total_instructions = 0;
bool exit_program = false;
int instructionCounts[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char *instructionCountsLabel[13] = {"NOP", "ADD,", "SUB", "MUL", "DIV", "LD", "ST", "EXIT", "MOV", "B", "CBZ", "CBNZ", "CBNEG"};

void pulse()
{
	pc.currentVal = pc.nextVal;
	if_dr.currentData = if_dr.nextData;
	dr_ex.currentData = dr_ex.nextData;
	ex1_ex2.currentData = ex1_ex2.nextData;
	ex2_mem.currentData = ex2_mem.nextData;
	mem_wb.currentData = mem_wb.nextData;
}

void fetch()
{
	fetchStage.instruction = mainMemory[pc.currentVal / MEMORY_STEP];
	fetchStage.pcNextVal = pc.currentVal + MEMORY_STEP;

	if_dr.nextData.value = fetchStage.instruction;

	if_dr.nextData.value = mainMemory[pc.currentVal / MEMORY_STEP];
	pc.nextVal = pc.currentVal + MEMORY_STEP;
	if_dr.nextData.MachineAddress = pc.currentVal;
}

void decode()
{

	int code = if_dr.currentData.value;
	int op = (code & 0xff000000) >> (4 * 6);
	int b1 = (code & 0x00ff0000) >> (4 * 4);
	int b2 = (code & 0x0000ff00) >> (4 * 2);
	int b3 = (code & 0x000000ff);

	// R format
	if (op <= 0x09 && ((op % 2) == 0))
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.RegWrite = 1;
		dr_ex.nextData.Rd = b1;
		dr_ex.nextData.operand1 = register_file[b2];
		dr_ex.nextData.operand2 = register_file[b3];

		switch (op)
		{
		case 0x02:
			dr_ex.nextData.Op = ALU_OP_ADD;
			dr_ex.nextData.CountId = COUNT_ID_ADD;
			break;
		case 0x04:
			dr_ex.nextData.Op = ALU_OP_SUB;
			dr_ex.nextData.CountId = COUNT_ID_SUB;
			break;
		case 0x06:
			dr_ex.nextData.Op = ALU_OP_MUL;
			dr_ex.nextData.CountId = COUNT_ID_MUL;
			break;
		case 0x08:
			dr_ex.nextData.Op = ALU_OP_DIV;
			dr_ex.nextData.CountId = COUNT_ID_DIV;
			break;
		default:
			dr_ex.nextData.Op = ALU_OP_NOP;
			break;
		}
	}

	// I format
	if (op <= 0x09 && ((op % 2) == 1))
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.ALUSrc = 1;
		dr_ex.nextData.RegWrite = 1;
		dr_ex.nextData.Rd = b1;
		dr_ex.nextData.operand1 = register_file[b2];
		dr_ex.nextData.operand2 = b3;

		switch (op)
		{
		case 0x03:
			dr_ex.nextData.Op = ALU_OP_ADD;
			dr_ex.nextData.CountId = COUNT_ID_ADD;
			break;
		case 0x05:
			dr_ex.nextData.Op = ALU_OP_SUB;
			dr_ex.nextData.CountId = COUNT_ID_SUB;
			break;
		case 0x07:
			dr_ex.nextData.Op = ALU_OP_MUL;
			dr_ex.nextData.CountId = COUNT_ID_MUL;
			break;
		case 0x09:
			dr_ex.nextData.Op = ALU_OP_DIV;
			dr_ex.nextData.CountId = COUNT_ID_DIV;
			break;
		default:
			dr_ex.nextData.Op = ALU_OP_NOP;
			break;
		}
	}

	// Load (D)
	if ((op == 0x0B))
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.ALUSrc = 1;
		dr_ex.nextData.Op = ALU_OP_ADD;
		dr_ex.nextData.MemRead = 1;
		dr_ex.nextData.Mem2Reg = 1;
		dr_ex.nextData.RegWrite = 1;
		dr_ex.nextData.Rd = b1;
		dr_ex.nextData.operand1 = register_file[b2];
		dr_ex.nextData.operand2 = b3;
		dr_ex.nextData.CountId = COUNT_ID_LD;
	}

	// Store (D)
	if (op == 0x0D)
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.Reg2Loc = 1;
		dr_ex.nextData.ALUSrc = 1;
		dr_ex.nextData.Op = ALU_OP_ADD;
		dr_ex.nextData.MemWrite = 1;
		dr_ex.nextData.Mem2Reg = 1;
		dr_ex.nextData.Rt = register_file[b1];
		dr_ex.nextData.operand1 = register_file[b2];
		dr_ex.nextData.operand2 = b3;
		dr_ex.nextData.CountId = COUNT_ID_ST;
	}

	// Branch (B/CB)
	if ((op >= 0x13) && (op <= 0x19))
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.ALUSrc = 1;
		dr_ex.nextData.Op = ALU_OP_ADD;
		dr_ex.nextData.PCSrc = 1;
		dr_ex.nextData.Branch = 1;
		dr_ex.nextData.Rt = register_file[b1];
		dr_ex.nextData.operand2 = b2 << (4 * 2) | b3;

		switch (op)
		{
		case 0x13:
			dr_ex.currentData.BranchType = BRANCH_TYPE_B;
			dr_ex.currentData.CountId = COUNT_ID_B;
			break;
		case 0x15:
			dr_ex.currentData.BranchType = BRANCH_TYPE_CBZ;
			dr_ex.currentData.CountId = COUNT_ID_CBZ;
			break;
		case 0x17:
			dr_ex.currentData.BranchType = BRANCH_TYPE_CBNZ;
			dr_ex.currentData.CountId = COUNT_ID_CBNZ;
			break;
		case 0x19:
			dr_ex.currentData.BranchType = BRANCH_TYPE_CBNEG;
			dr_ex.currentData.CountId = COUNT_ID_CBNEG;
			break;
		default:
			printf("ERROR: Branch with no branch type");
			break;
		}
	}

	// Mov (IW)
	if (op == 0x11)
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.Op = ALU_OP_NOP; // TODO: Fix this
		dr_ex.nextData.MemWrite = 1;
		dr_ex.nextData.RegWrite = 1;
		dr_ex.nextData.Rd = b1;
		dr_ex.nextData.operand2 = b2 << (4 * 2) | b3;
		dr_ex.nextData.Mov = 1;
		dr_ex.nextData.CountId = COUNT_ID_MOV;
	}

	// NOP
	if (op == 0x00)
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.Op = ALU_OP_NOP;
		dr_ex.nextData.CountId = COUNT_ID_NOP;
	}

	// EXIT
	if (op == 0x0E)
	{
		clearFlags(dr_ex.nextData);
		dr_ex.nextData.Exit = 1;
		dr_ex.nextData.CountId = COUNT_ID_EXIT;
	}
}

void exec1()
{
	if (dr_ex.currentData.Mov)
		ex1_ex2.nextData.Rd = dr_ex.currentData.operand2;

	if (dr_ex.currentData.Branch)
		ex1_ex2.nextData.BranchAddress = pc.currentVal = pc.currentVal + (MEMORY_STEP * dr_ex.currentData.operand2);

	if (dr_ex.currentData.Op == ALU_OP_ADD)
		ex1_ex2.nextData.value = dr_ex.currentData.operand1 + dr_ex.currentData.operand2;

	if (dr_ex.currentData.Op == ALU_OP_SUB)
		ex1_ex2.nextData.value = dr_ex.currentData.operand1 - dr_ex.currentData.operand2;
}

void exec2()
{

	// Handle Branches
	if (ex1_ex2.currentData.Branch)
	{
		if ((ex1_ex2.currentData.BranchType == BRANCH_TYPE_B) ||
			(ex1_ex2.currentData.BranchType == BRANCH_TYPE_CBZ && ex1_ex2.currentData.Rt == 0) ||
			(ex1_ex2.currentData.BranchType == BRANCH_TYPE_CBNZ && ex1_ex2.currentData.Rt != 0) ||
			(ex1_ex2.currentData.BranchType == BRANCH_TYPE_CBNEG && ex1_ex2.currentData.Rt < 0))
		{
			squashFetchAndDecode();
			pc.nextVal = dr_ex.currentData.BranchAddress;
		}
		else
			printf("ERROR: Branch condition code without proper branch type");
	}

	// Handle MUL
	if (ex1_ex2.currentData.Op == ALU_OP_MUL)
		ex2_mem.nextData.value = ex1_ex2.currentData.operand1 * ex1_ex2.currentData.operand2;

	// Handle DIV
	if (ex1_ex2.currentData.Op == ALU_OP_DIV)
		ex2_mem.nextData.value = ex1_ex2.currentData.operand1 / ex1_ex2.currentData.operand2;
}

void memory()
{
	mem_wb.nextData = ex2_mem.currentData;

	if (ex2_mem.currentData.MemWrite)
		mainMemory[ex2_mem.currentData.value] = ex2_mem.currentData.Rt;
	if (ex2_mem.currentData.MemRead)
		mem_wb.currentData.Rd = mainMemory[ex2_mem.currentData.value];
}

void writeBack()
{
	instructionCounts[mem_wb.currentData.CountId] += 1;

	if (mem_wb.currentData.RegWrite)
		register_file[mem_wb.currentData.Rd] = mem_wb.currentData.value;

	if (mem_wb.currentData.Exit)
		exit_program = true;

	if (mem_wb.currentData.CountId != COUNT_ID_NOP)
		total_instructions++;
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
	clearFlags(if_dr.currentData);
	clearFlags(if_dr.nextData);
	clearFlags(dr_ex.currentData);
	clearFlags(dr_ex.nextData);
	clearFlags(ex1_ex2.currentData);
	clearFlags(ex1_ex2.nextData);
}

void clearFlags(Flags_t src)
{
	src.value = 0;
	src.value = 0;
	src.operand2 = 0;
	src.operand1 = 0;
	src.Rt = 0;
	src.Rd = 0;
	src.Reg2Loc = 0;
	src.ALUSrc = 0;
	src.Op = 0;
	src.MemRead = 0;
	src.MemWrite = 0;
	src.Mem2Reg = 0;
	src.PCSrc = 0;
	src.RegWrite = 0;
	src.Branch = 0;
	src.BranchType = 0;
	src.Mov = 0;
	src.CountId = 0;
	src.Exit = 0;
	src.MachineAddress = 0;
}
/*------------------------------------------------------------------------
	Main Function Definition
------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	// TODO: Code your simulator here
	// copy the input file to a vector
	putMachineCodeIntoMemory(argv);

	printf("== cycle |    fetch |   decode |  exec1   |  exec2   |   memory | writeBack|\n");
	printf("== ------+----------+----------+----------+----------+----------+----------|\n");

	while (!exit_program)
	{
		fetch();
		decode();
		exec1();
		exec2();
		memory();
		writeBack();
		pulse();
		printf("== %5x | %8x | %8x | %8x | %8x | %8x | %8x |\n", cycle_count, if_dr.currentData.MachineAddress, dr_ex.currentData.MachineAddress, ex1_ex2.currentData.MachineAddress, ex1_ex2.currentData.MachineAddress, ex2_mem.currentData.MachineAddress, mem_wb.currentData.MachineAddress);
		cycle_count++;
		if (cycle_count > 100)
		{
			break;
		}
	}

	printf(" === Simulation Reports ===\n");
	printf("Non-Zero Registers....\n");
	for (int i = 0; i < sizeof(register_file); i++)
	{
		if (register_file[i])
			printf("  X%d, = %d\n", i, register_file[i]);
	}
	printf("Instruction Count Report...\n");
	printf("OPCODE - CNT\n");
	for (int i = 1; i < sizeof(instructionCounts); i++)
	{
		if (instructionCounts[i])
			printf("%s - %d\n", instructionCountsLabel[i], instructionCounts[i]);
	}
	printf("Total Instructions=%d Cycles=%d IPC= %.2f\n", total_instructions, cycle_count, (double)total_instructions / cycle_count);
	printf("\n");

	return 0;
}

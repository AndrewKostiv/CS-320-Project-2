#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct PC
{
	long currentVal;
	long nextVal;
} PC_t;

typedef struct IFDR
{
	int currentVal;
	int nextVal;
	int nextPCInc;

} IFDR_t;

typedef struct DREX
{
	int currentVal;
	int nextVal;
	int Rm;
	int Rn;
	int Rd;
	int imm;
	int Rt;
	int Rb;
	bool Reg2Loc;
	bool ALUSrc;
	int Op;
	bool MemRead;
	bool MemWrite;
	bool Mem2Reg;
	bool PCSrc;
	bool RegWrite;
} DREX_t;

typedef struct EXMEM
{
	int currentVal;
	int nextVal;
	int Rd;
	bool MemRead;
	bool MemWrite;
	bool Mem2Reg;
	bool RegWrite;
} EXMEM_t;

typedef struct MEMWB
{
	int currentVal;
	int nextVal;
	int Rd;
	bool RegWrite;
} MEMWB_t;

PC_t pc = {0, 0};
IFDR_t ifdr = {0, 0};
DREX_t drex = {0, 0, 0};
EXMEM_t exmem = {0, 0, 0};
MEMWB_t memwb = {0, 0, 0};

const int ALU_OP_NOP = 0x0000;
const int ALU_OP_ADD = 0x0001;
const int ALU_OP_SUB = 0x0010;
const int ALU_OP_MUL = 0x0011;
const int ALU_OP_DIV = 0x0100;

const int MEMORY_STEP = 4;
int mainMemory[2048] = {0};
int register_file[32] = {0};
int cycle_count = 1;
int total_instructions = 1;

void pulse()
{
	pc.currentVal = pc.nextVal;
	ifdr.currentVal = ifdr.nextVal;
	drex.currentVal = drex.nextVal;
	exmem.currentVal = exmem.nextVal;
	memwb.currentVal = memwb.nextVal;
	cycle_count++;
}

void fetch()
{
	ifdr.nextVal = mainMemory[pc.currentVal / MEMORY_STEP];
	pc.nextVal = pc.currentVal + MEMORY_STEP;
	ifdr.nextPCInc = pc.nextVal;
}

void decode()
{
	int code = ifdr.currentVal;
	int op = (code & 0xff000000) >> (4 * 6);
	int b1 = (code & 0x00ff0000) >> (4 * 4);
	int b2 = (code & 0x0000ff00) >> (4 * 2);
	int b3 = (code & 0x000000ff);

	// R format
	if (op <= 0x09 && ((op % 2) == 0))
	{
		drex.Reg2Loc = 0;
		drex.ALUSrc = 0;
		drex.MemRead = 0;
		drex.MemWrite = 0;
		drex.Mem2Reg = 0;
		drex.PCSrc = 0;
		drex.RegWrite = 1;
		drex.Rd = b1;
		drex.Rn = register_file[b2];
		drex.Rm = register_file[b3];
		if (op == 0x02)
			drex.Op = ALU_OP_ADD;
		else if (op == 0x04)
			drex.Op = ALU_OP_SUB;
		else if (op == 0x06)
			drex.Op = ALU_OP_MUL;
		else if (op == 0x08)
			drex.Op = ALU_OP_DIV;
		else
			drex.Op = ALU_OP_NOP;
	}

	// I format
	if (op <= 0x09 && ((op % 2) == 1))
	{
		drex.Reg2Loc = 0;
		drex.ALUSrc = 1;
		drex.MemRead = 0;
		drex.MemWrite = 0;
		drex.Mem2Reg = 0;
		drex.PCSrc = 0;
		drex.RegWrite = 1;
		drex.Rd = b1;
		drex.Rn = register_file[b2];
		drex.imm = b3;
		if (op == 0x03)
			drex.Op = ALU_OP_ADD;
		else if (op == 0x05)
			drex.Op = ALU_OP_SUB;
		else if (op == 0x07)
			drex.Op = ALU_OP_MUL;
		else if (op == 0x09)
			drex.Op = ALU_OP_DIV;
		else
			drex.Op = ALU_OP_NOP;
	}

	// Load (D)
	if ((op == 0x0B))
	{
		drex.Reg2Loc = 0;
		drex.ALUSrc = 1;
		drex.Op = ALU_OP_ADD;
		drex.MemRead = 1;
		drex.MemWrite = 0;
		drex.Mem2Reg = 1;
		drex.PCSrc = 0;
		drex.RegWrite = 1;
		drex.Rd = b1;
		drex.Rb = register_file[b2];
		drex.imm = b3;
	}

	// Store (D)
	if ((op == 0x0D))
	{
		drex.Reg2Loc = 1;
		drex.ALUSrc = 1;
		drex.Op = ALU_OP_ADD;
		drex.MemRead = 0;
		drex.MemWrite = 1;
		drex.Mem2Reg = 1;
		drex.PCSrc = 0;
		drex.RegWrite = 0;
		drex.Rt = b1;
		drex.Rb = register_file[b2];
		drex.imm = b3;
	}

	// Branch (B)
	if (op == 0x13)
	{
		drex.imm = b2 << (4 * 2) | b3;
	}

	// Branch (CB)
	if ((op >= 0x13) && op <= 0x19)
	{
		drex.Reg2Loc = 0;
		drex.ALUSrc = 1;
		drex.Op = ALU_OP_NOP;
		drex.MemRead = 0;
		drex.MemWrite = 0;
		drex.Mem2Reg = 0;
		drex.PCSrc = 1;
		drex.RegWrite = 0;
		drex.Rt = b1;
		drex.imm = b2 << (4 * 2) | b3;
	}

	// Mov (IW)
	if ((op == 0x11))
	{
		drex.Reg2Loc = 0;
		drex.ALUSrc = 0;
		drex.Op = ALU_OP_NOP;
		drex.MemRead = 0;
		drex.MemWrite = 1;
		drex.Mem2Reg = 0;
		drex.PCSrc = 0;
		drex.RegWrite = 1;
		drex.Rd = b1;
		drex.imm = b2 << (4 * 2) | b3;
	}

	// NOP
	if ((op >> 6) == 0x00)
	{
		drex.Reg2Loc = 0;
		drex.ALUSrc = 0;
		drex.Op = ALU_OP_NOP;
		drex.MemRead = 0;
		drex.MemWrite = 0;
		drex.Mem2Reg = 0;
		drex.PCSrc = 0;
		drex.RegWrite = 0;
	}
}

void exec1()
{

	exmem.MemRead = drex.MemRead;
	exmem.MemWrite = drex.MemWrite;
	exmem.Mem2Reg = drex.Mem2Reg;
	exmem.RegWrite = drex.RegWrite;
	exmem.Rd = drex.Rd;

	int b2 = drex.Rn;
	int b3 = (drex.ALUSrc) ? drex.imm : register_file[drex.Rm];

	if (drex.Op = ALU_OP_ADD)
		exmem.nextVal = b2 + b3;
	if (drex.Op = ALU_OP_SUB)
		exmem.nextVal = b2 - b3;
	if (drex.Op = ALU_OP_MUL)
		exmem.nextVal = b2 * b3;
	if (drex.Op = ALU_OP_DIV)
		exmem.nextVal = b2 / b3;

	// // add
	// if (drex.Op == 0x02)
	// 	exmem.nextVal = drex.Rn + drex.Rm;
	// if (drex.Op == 0x03)
	// 	exmem.nextVal = drex.Rn + drex.imm;

	// // sub
	// if (drex.Op == 0x04)
	// 	exmem.nextVal = drex.Rn - drex.Rm;
	// if (drex.Op == 0x05)
	// 	exmem.nextVal = drex.Rn - drex.imm;

	// // mul
	// if (drex.Op == 0x06)
	// 	exmem.nextVal = drex.Rn * drex.Rm;
	// if (drex.Op == 0x07)
	// 	exmem.nextVal = drex.Rn * drex.imm;

	// // div
	// if (drex.Op == 0x08)
	// 	exmem.nextVal = drex.Rn / drex.Rm;
	// if (drex.Op == 0x09)
	// 	exmem.nextVal = drex.Rn / drex.imm;

	// // ld
	// if (drex.Op == 0x08)
	// 	exmem.nextVal = mainMemory[register_file[drex.Rb]] + drex.imm;

	// // st
	// if (drex.Op == 0x09)
	// 	// register_file[drex.rd]

	// 	exmem.nextVal = register_file[drex.Rb + drex.imm] = register_file[drex.Rt];

	// // mov
	// if (drex.Op == 0x11)
	// 	exmem.nextVal = drex.imm;

	// // br
	// if (drex.Op == 0x13)
	// 	pc.nextVal = pc.currentVal + (MEMORY_STEP * drex.imm);

	// // cd br
	// if (drex.Op == 0x15 && (drex.Rt == 0))
	// 	exmem.nextVal = pc.currentVal + (MEMORY_STEP * drex.imm);
	// if (drex.Op == 0x17 && (drex.Rt != 0))
	// 	exmem.nextVal = pc.currentVal + (MEMORY_STEP * drex.imm);
	// if (drex.Op == 0x19 && (drex.Rt < 0))
	// 	exmem.nextVal = pc.currentVal + (MEMORY_STEP * drex.imm);
}

void exec2()
{
	//
}

void memory()
{
	//
}

void writeBack()
{
	register_file[memwb.Rd] = memwb.currentVal;
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
		printf("%08x\n", mainMemory[address / MEMORY_STEP]);
	}
	fclose(fptr);
}

/*------------------------------------------------------------------------
	Main Function Definition
------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	// TODO: Code your simulator here
	// copy the input file to a vector
	// printf("start of sim.c\n");

	putMachineCodeIntoMemory(argv);

	// fetch();
	// decode();
	// alu();
	// data_memory();
	// writeBack();
	// pulse();

	// printf("== cycle |    fetch |   decode |  exec1   |  exec2   |   memory | writeBack|\n");
	// printf("== ------+----------+----------+----------+----------+----------+----------|\n");

	printf("=== Simulation Reports ===\n");
	printf("Total Instructions=%d Cycles=%d IPC= %.2f\n", total_instructions, cycle_count, (double)total_instructions / cycle_count);
	return 0;
}

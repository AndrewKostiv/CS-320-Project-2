#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct PC
{
	int currentVal;
	int nextVal;
} PC_t;

typedef struct IFDR
{
	int currentVal;
	int nextVal;
} IFDR_t;

typedef struct DREX
{
	int currentVal;
	int nextVal;
	int Rd;
} DREX_t;

typedef struct EXMEM
{
	int currentVal;
	int nextVal;
	int Rd;
} EXMEM_t;

typedef struct MEMWB
{
	int currentVal;
	int nextVal;
	int Rd;
} MEMWB_t;

PC_t pc = {0, 0};
IFDR_t ifdr = {0, 0};
DREX_t drex = {0, 0, 0};
EXMEM_t exmem = {0, 0, 0};
MEMWB_t memwb = {0, 0, 0};
int registers[32];
int memory[255] = {};
const int BUFFER_SIZE = 255;

void pulse()
{
	pc.currentVal = pc.nextVal;
	ifdr.currentVal = ifdr.nextVal;
	drex.currentVal = drex.nextVal;
	exmem.currentVal = exmem.nextVal;
	memwb.currentVal = memwb.nextVal;
}

void instruction_memory()
{
	pc.nextVal = pc.currentVal + 4;
}

void decode_fetch_registers()
{
	//
}

void alu()
{
	//
}

void data_memory()
{
	//
}

void write_to_memory()
{
	//
}

void putMachineCodeIntoMemory(char **argv)
{
	FILE *fptr;
	fptr = fopen(argv[1], "r");
	char buffer[BUFFER_SIZE];

	int i = 0;
	while (!feof(fptr))
	{
		int address;
		int value;
		fscanf(fptr, "%x %x", &address, &value);
		memory[address / 4] = value;
		printf("%08x\n", memory[address / 4]);
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
	printf("start of sim.c\n");

	putMachineCodeIntoMemory(argv);

	// instruction_memory();
	// decode_fetch_registers();
	// alu();
	// data_memory();
	// write_to_memory();
	// pulse();

	// printf("== cycle |    fetch |   decode |  exec1   |  exec2   |   memory | writeBack|\n");
	// printf("== ------+----------+----------+----------+----------+----------+----------|\n");

	printf("=== Simulation Reports ===\n");

	printf("end of sim.c\n");
	return 0;
}

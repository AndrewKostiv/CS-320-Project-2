#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Flags
{
    int value;
    int operand2;
    int operand1;
    int Rt;
    int Rd;
    bool Reg2Loc;
    bool ALUSrc;
    int Op;
    bool MemRead;
    bool MemWrite;
    bool Mem2Reg;
    bool PCSrc;
    bool RegWrite;
    bool Branch;
    int BranchType;
    int BranchAddress;
    bool Mov;
    int CountId;
    bool Exit;
    int MachineAddress;
} Flags_t;

typedef struct PC
{
    int currentVal;
    int nextVal;
} PC_t;

typedef struct IF_DR
{
    Flags_t currentData;
    Flags_t nextData;

} IF_DR_t;

typedef struct DR_EX
{
    Flags_t currentData;
    Flags_t nextData;
} DR_EX_t;

typedef struct EX1_EX2
{
    Flags_t currentData;
    Flags_t nextData;

} EX1_EX2_t;

typedef struct EX2_MEM
{
    Flags_t currentData;
    Flags_t nextData;

} EX2_MEM_t;

typedef struct MEM_WB
{
    Flags_t currentData;
    Flags_t nextData;

} MEM_WB_t;

typedef struct FetchStage
{
    int32_t instruction;
    int pcNextVal;
} FetchStage_t;

typedef struct DecodeStage
{
    int op;
    int b1;
    int b2;
    int b3;
    
} DecodeStage_t;

void pulse();
void fetch();
void decode();
void exec1();
void exec2();
void memory();
void writeBack();
void putMachineCodeIntoMemory(char **argv);
void squashFetchAndDecode();
void clearFlags(Flags_t);
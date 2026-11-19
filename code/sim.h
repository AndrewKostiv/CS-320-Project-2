#pragma once
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

typedef struct IF_DR
{
    int currentVal;
    int nextVal;
    int nextPCInc;

} IF_DR_t;

typedef struct DR_EX
{
    int currentVal;
    int nextVal;
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
    bool Mov;
    int CountId;
    bool Exit;
} DR_EX_t;

typedef struct EX1_EX2
{
    int currentVal;
    int nextVal;
    int operand2;
    int operand1;
    int Rd;
    int Rt;
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

} EX1_EX2_t;

typedef struct EX2_MEM
{
    int currentVal;
    int nextVal;
    int operand2;
    int operand1;
    int Rd;
    int Rt;
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
    int CountId;
    bool Exit;

} EX2_MEM_t;

typedef struct MEM_WB
{
    int currentVal;
    int nextVal;
    int Rd;
    bool RegWrite;
    int CountId;
    bool Exit;
} MEM_WB_t;

void pulse();
void fetch();
void decode();
void exec1();
void exec2();
void memory();
void writeBack();
void putMachineCodeIntoMemory(char **argv);
void squashFetchAndDecode();
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define Kilobytes(Value) ((Value) * 1000LL)
#define Megabytes(Value) (Kilobytes(Value) * 1000LL)
#define Gigabytes(Value) (Megabytes(Value) * 1000LL)
#define Kibibytes(Value) ((Value) * 1024LL)
#define Mebibytes(Value) (Kibibytes(Value) * 1024LL)
#define Gibibytes(Value) (Mebibytes(Value) * 1024LL)

#define internal static
#define global_variable static
#define local_persist static

#define Assert(Expression)			\
  if(!(Expression)) {*(int*)0 = 0;}

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef enum
  {
   BR = 0, // Branch
   ADD,    // ADD
   LD,     // LD
   ST,     // STORE
   JSR,    // JUMP REGISTER
   AND,    // AND
   LDR,    // LOAD REGISTER
   STR,    // STORE REGISTER
   RTI,    // UNUSED
   NOT,    // BITWISE NOT
   LDI,    // LOAD INDIRECT
   STI,    // STORE INDIRECT
   JMP,    // JUMP
   RES,    // RESERVED (UNUSED)
   LEA,    // LOAD EFFECTIVE ADDRESS
   TRAP    // EXECUTE TRAP
  } InstructionSet;

typedef enum
  {
   R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, // GENERAL PURPOSE
   PC,                                         // PROGRAM COUNTER
   COND,                                       // CONDITION BITS
   NumberOfRegisters                           // NUMBER OF REGS
  } Register;

typedef enum
  {
   POS = 1 << 0, // P
   ZRO = 1 << 1, // N
   NEG = 1 << 2  // Z
  } ConditionFlags;

global_variable uint16 Registers[NumberOfRegisters];

global_variable int32* Program;
global_variable int32* Memory;

int main(void)
{

  // NOTE(l4v): Reserve our entire memory to be 128KiB
  size_t MemorySize = Kibibytes(128);
  Memory = (int32*)mmap(0,
		      MemorySize,
		      PROT_READ | PROT_WRITE,
		      MAP_ANONYMOUS | MAP_PRIVATE,
		      -1,
		      0);
  // NOTE(l4v): Assertion to check whether the memory
  // allocation failed
  Assert(Memory != (void*)-1);
  
  bool32 Running = 1;
  while(Running)
    {
      // NOTE(l4v): Fetching the instruction
      uint16 Instruction = ReadMemory(Registers[PC]++);
      uint16 OpCode = Instruction >> 12;

      switch(OpCode)
	{
	case ADD:
	  {
	  }break;
	case AND:
	  {
	  }break;
	case NOT:
	  {
	  }break;
	case BR:
	  {
	  }break;
	case JMP:
	  {
	  }break;
	case JSR:
	  {
	  }break;
	case LD:
	  {
	  }break;
	case LDI:
	  {
	  }break;
	case LDR:
	  {
	  }break;
	case LEA:
	  {
	  }break;
	case ST:
	  {
	  }break;
	case STI:
	  {
	  }break;
	case STR:
	  {
	  }break;
	case TRAP:
	  {
	  }break;
	case RES:
	case RTI:
	default:
	  {
	    // NOTE(l4v): Bad OpCode
	  }break;
	}
    }

  munmap(Memory, MemorySize);
  return 0;
}

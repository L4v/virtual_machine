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
  } ConditionFlag;

typedef enum
  {
   GETC = 0x20,  // Get char from keyboard, don't echo
   OUT = 0x21,   // Output char
   PUTS = 0x22,  // Output a word string
   IN = 0x23,    // Get char from terminal, echo
   PUTSP = 0x24, // Output a byte string
   HALT = 0x25   // Halt the program
  } TrapCode;

global_variable uint16 Registers[NumberOfRegisters];

global_variable int32* Program;
global_variable int32* Memory;

internal uint16
SignExtend(uint16 X, int32 BitCount)
{
  if((X >> (BitCount - 1)) & 1)
    {
      X |= (0xFFFF << BitCount);
    }
  return X;
}

internal void
UpdateFlags(uint16 Register)
{
  if(!Registers[Register])
    {
      Registers[COND] = ZRO;
    }
  else if(Registers[Register] >> 15)
    {
      Registers[COND] = NEG;
    }
  else
    {
      Registers[COND] = POS;
    }
}

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
	    uint16 DestReg = (Instruction & 0x0E00);
	    uint16 Src1Reg = (Instruction & 0x01C0);
	    uint16 Imm5Flag = (Instruction & 0x0020);

	    if(Imm5Flag)
	      {
		// NOTE(l4v): Immediate mode
		uint16 ImmValue = (Instruction & 0x001F);
		ImmValue = SignExtend(ImmValue, 5);
	        Registers[DestReg] = Registers[Src1Reg] + ImmValue; 
	      }
	    else
	      {
		// NOTE(l4v): Register mode
		uint16 Src2Reg = (Instruction & 0x0007);
		Registers[DestReg] = Registers[Src1Reg] +
		  Registers[Src2Reg];
	      }
	    UpdateFlags(Registers[DestReg]);
	  }break;
	case AND:
	  {
	    uint16 DestReg = (Instruction & 0x0E00);
	    uint16 Src1Reg = (Instruction & 0x01C0);
	    uint16 Imm5Flag = (Instruction & 0x0020);

	    if(Imm5Flag)
	      {
		// NOTE(l4v): Immediate mode
		uint16 ImmValue = (Instruction & 0x001F);
		ImmValue = SignExtend(ImmValue, 5);
	        Registers[DestReg] = (Registers[Src1Reg] & ImmValue); 
	      }
	    else
	      {
		// NOTE(l4v): Register mode
		uint16 Src2Reg = (Instruction & 0x0007);
		Registers[DestReg] = (Registers[Src1Reg] &
				      Registers[Src2Reg]);
	      }
	    UpdateFlags(Registers[DestReg]);
	  }break;
	case NOT:
	  {
	    uint16 DestReg = (Instruction & 0x0E00);
	    uint16 SrcReg = (Instruction & 0x01C00);
	    Registers[DestReg] = ~Registers[SrcReg];
	    UpdateFlags(Registers[DestReg]);
	    
	  }break;
	case BR:
	  {
	    uint16 PCOffset = (Instruction & 0X1FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    uint16 CondFlag = (Instruction & 0x0700);
	    
	    if(CondFlag & Registers[COND])
	      {
		Registers[PC] += PCOffset;
	      }
	  }break;
	case JMP:
	  {
	    uint16 BaseReg = (Instruction & 0x01C0);
	    Registers[PC] = Registers[BaseReg];
	  }break;
	case JSR:
	  {
	    // NOTE(l4v): Flag to check whether the base address is
	    // located in the base register or from argument
	    uint16 Flag = (Instruction & 0x0800);
	    Registers[R7] = Registers[PC];
	    if(Flag)
	      {
		// NOTE(l4v): Address loaded from argument
		uint16 PCOffset = (Instruction & 0x07FF);
		Registers[PC] += SignExtend(PCOffset, 11);
	      }
	    else
	      {
		// NOTE(l4v): Address loaded from base register
		uint16 BaseReg = (Instruction & 0x01C0);
		Registers[PC] = BaseReg;
	      }
	    
	  }break;
	case LD:
	  {
	    uint16 DestReg = (Instruction & 0x0E00);
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    Registers[DestReg] = ReadMemory(Registers[PC] + PCOffset);
	    UpdateFlags(Registers[DestReg]);
	  }break;
	case LDI:
	  {
	    uint16 DestReg = (Instruction & 0x0700);
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);

	    Registers[DestReg] = ReadMemory(
					    ReadMemory(Registers[PC] +
						       PCOffset));
	    UpdateFlags(Registers[DestReg]);
	  }break;
	case LDR:
	  {
	    uint16 DestReg = (Instruction & 0x0E00);
	    uint16 BaseReg = (0x01C00);
	    uint16 Offset = (Instruction & 0x003F);
	    Offset = SignExtend(Offset, 6);
	    Registers[DestReg] = ReadMemory(Registers[BaseReg] + Offset);
	    UpdateFlags(Registers[DestReg]);
	  }break;
	case LEA:
	  {
	    uint16 DestReg = (Instruction & 0x0E00);
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    Registers[DestReg] = Registers[PC] + PCOffset;
	    UpdateFlags(Registers[DestReg]);
	  }break;
	case ST:
	  {
	    uint16 SrcReg = (Instruction & 0x0E00);
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    WriteMemory(Registers[PC] + PCOffset, Registers[SrcReg]);
	  }break;
	case STI:
	  {
	    uint16 SrcReg = (Instruction & 0x0E00);
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    WriteMemory(ReadMemory(Registers[PC] + PCOffset), Registers[SrcReg]);
	  }break;
	case STR:
	  {
	    uint16 SrcReg = (Instruction & 0x0E00);
	    uint16 BaseReg = (Instruction & 0x01C0);
	    uint16 PCOffset = (Instruction & 0x001F);
	    PCOffset = SignExtend(PCOffset, 6);
	    WriteMemory(Registers[BaseReg] + PCOffset, Registers[SrcReg]);
	  }break;
	case TRAP:
	  {
	    // TODO(l4v): Implement
	  }break;
	case RES:
	case RTI:
	default:
	  {
	    // NOTE(l4v): Bad OpCode
	    abort();
	  }break;
	}
    }

  munmap(Memory, MemorySize);
  return 0;
}

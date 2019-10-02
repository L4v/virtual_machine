#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/types.h>

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

typedef enum
  {
   KBSR = 0xFE00, // Keyboard status
   KBDR = 0xFE02  // Keyboard data
  } MemoryMappedRegister;

global_variable uint16 Registers[NumberOfRegisters];
global_variable int32* Program;
global_variable uint16* Memory;
struct termios OriginalTerminal;

internal void
DisableInputBuffering()
{
  tcgetattr(STDIN_FILENO, &OriginalTerminal);
  struct termios NewTerminal = OriginalTerminal;
  NewTerminal.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &NewTerminal);
}

internal void
RestoreInputBuffering()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &OriginalTerminal);
}

internal void
HandleInterrupt()
{
  RestoreInputBuffering();
  printf("\n");
  exit(-2);
}

internal uint16
Swap16(uint16 X)
{
  return ((X << 8) | (X >> 8));
}

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

internal uint16
CheckKey()
{
  // NOTE(l4v): Set of file descriptors
  fd_set ReadFDs;
  // NOTE(l4v): Sets the fd_set to 0 and then adds stdin to the set
  FD_ZERO(&ReadFDs);
  FD_SET(STDIN_FILENO, &ReadFDs);

  // NOTE(l4v): Uses select to monitor the file(s)
  struct timeval Timeout;
  Timeout.tv_sec = 0;
  Timeout.tv_usec = 0;
  return select(1, &ReadFDs, 0, 0, &Timeout) != 0;
}

internal void
WriteMemory(uint16 Address, uint16 Value)
{
  Memory[Address] = Value;
}

internal uint16
ReadMemory(uint16 Address)
{
  if(Address == KBSR)
    {
      if(CheckKey())
  	{
  	  Memory[KBSR] = (1 << 15);
  	  Memory[KBDR] = getchar();
  	}
      else
  	{
  	  Memory[KBSR] = 0;
  	}
    }
  uint16 Result;
  Result = Memory[Address];
  return Result;
}

internal void
LoadObjectFile(FILE* File)
{
  // NOTE(l4v): Tells us where to begin reading the file
  uint16 Origin;
  fread(&Origin, sizeof(Origin), 1, File);
  Origin = Swap16(Origin);

  uint16 MaxReadSize = UINT16_MAX - Origin;
  uint16* ReadPtr = Memory + Origin;
  size_t ReadBytes = fread(ReadPtr, sizeof(uint16), MaxReadSize, File);

  // NOTE(l4v): Swap to little endian
  while(ReadBytes-- > 0)
    {
      *ReadPtr = Swap16(*ReadPtr);
      ++ReadPtr;
    }
}

internal int32
LoadObject(const char* Path)
{
  FILE* File = fopen(Path, "rb");
  if(!File)
    {
      return 0;
    }
  LoadObjectFile(File);
  fclose(File);
  return 1;
}

int main(int32 argc, char* argv[])
{

  // NOTE(l4v): Reserve our entire memory to be 128KiB
  size_t MemorySize = Kibibytes(128);
  Memory = (uint16*)mmap(0,
			 MemorySize,
			 PROT_READ | PROT_WRITE,
			 MAP_ANONYMOUS | MAP_PRIVATE,
			 -1,
			 0);
  // NOTE(l4v): Assertion to check whether the memory
  // allocation failed
  Assert(Memory != (void*)-1);

  if(argc < 2)
    {
      // NOTE(l4v): If path was not specified, show usage example
      printf("vm [object-file] ...\n");
      exit(2);
    }

  for(int32 ObjectIndex = 1;
      ObjectIndex < argc;
      ++ObjectIndex)
    {
      if(!LoadObject(argv[ObjectIndex]))
	{
	  printf("Failed to load object file: %s\n", argv[ObjectIndex]);
	  exit(1);
	}
    }

  signal(SIGINT, HandleInterrupt);
  DisableInputBuffering();
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
	    uint16 DestReg = (Instruction >> 9) & 0x7;
	    uint16 Src1Reg = (Instruction >> 6) & 0x7;
	    uint16 Imm5Flag = (Instruction >> 5) & 0x1;

	    if(Imm5Flag)
	      {
		// NOTE(l4v): Immediate mode
		uint16 ImmValue = (Instruction & 0x1F);
		ImmValue = SignExtend(ImmValue, 5);
	        Registers[DestReg] = Registers[Src1Reg] + ImmValue;
	      }
	    else
	      {
		// NOTE(l4v): Register mode
		uint16 Src2Reg = (Instruction & 0x7);
		Registers[DestReg] = Registers[Src1Reg] +
		  Registers[Src2Reg];
	      }
	    UpdateFlags(DestReg);
	  }break;
	case AND:
	  {
	    uint16 DestReg = (Instruction >> 9) & 0x7;
	    uint16 Src1Reg = (Instruction >> 6) & 0x7;
	    uint16 Imm5Flag = (Instruction >> 5) & 0x1;

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
		uint16 Src2Reg = (Instruction & 0x7);
		Registers[DestReg] = (Registers[Src1Reg] &
				      Registers[Src2Reg]);
	      }
	    UpdateFlags(DestReg);
	  }break;
	case NOT:
	  {
	    uint16 DestReg = (Instruction >> 9) & 0x7;
	    uint16 SrcReg = (Instruction >> 6) & 0x7;
	    Registers[DestReg] = ~Registers[SrcReg];
	    UpdateFlags(DestReg);
	  }break;
	case BR:
	  {
	    uint16 PCOffset = (Instruction  & 0X1FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    uint16 CondFlag = (Instruction >> 9) & 0x7;
	    
	    if(CondFlag & Registers[COND])
	      {
		Registers[PC] += PCOffset;
	      }
	    
	  }break;
	case JMP:
	  {
	    uint16 BaseReg = (Instruction >> 6) & 0x7;
	    Registers[PC] = Registers[BaseReg];
	  }break;
	case JSR:
	  {
	    // NOTE(l4v): Flag to check whether the base address is
	    // located in the base register or from argument
	    uint16 Flag = (Instruction >> 11) & 0x1;
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
		uint16 BaseReg = (Instruction >> 6) & 0x7;
		Registers[PC] = BaseReg;
	      }
	    
	  }break;
	case LD:
	  {
	    uint16 DestReg = (Instruction >> 9) & 0x7;
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    Registers[DestReg] = ReadMemory(Registers[PC] + PCOffset);
	    UpdateFlags(DestReg);
	  }break;
	case LDI:
	  {
	    uint16 DestReg = (Instruction >> 9) & 0x7;
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);

	    Registers[DestReg] =
	      ReadMemory(ReadMemory(Registers[PC] + PCOffset));
	    UpdateFlags(DestReg);
	  }break;
	case LDR:
	  {
	    uint16 DestReg = (Instruction >> 9) & 0x7;
	    uint16 BaseReg = (Instruction >> 6) & 0x7;
	    uint16 Offset = (Instruction & 0x003F);
	    Offset = SignExtend(Offset, 6);
	    Registers[DestReg] =
	      ReadMemory(Registers[BaseReg] + Offset);
	    UpdateFlags(DestReg);
	  }break;
	case LEA:
	  {
	    uint16 DestReg = (Instruction >> 9) & 0x7;
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    Registers[DestReg] = Registers[PC] + PCOffset;
	    UpdateFlags(DestReg);
	  }break;
	case ST:
	  {
	    uint16 SrcReg = (Instruction >> 9) & 0x7;
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    WriteMemory(Registers[PC] + PCOffset, Registers[SrcReg]);
	  }break;
	case STI:
	  {
	    uint16 SrcReg = (Instruction >> 9) & 0x7;
	    uint16 PCOffset = (Instruction & 0x01FF);
	    PCOffset = SignExtend(PCOffset, 9);
	    WriteMemory(ReadMemory(Registers[PC] + PCOffset), Registers[SrcReg]);
	  }break;
	case STR:
	  {
	    uint16 SrcReg = (Instruction >> 9) & 0x7;
	    uint16 BaseReg = (Instruction >> 6) & 0x7;
	    uint16 PCOffset = (Instruction & 0x003F);
	    PCOffset = SignExtend(PCOffset, 6);
	    WriteMemory(Registers[BaseReg] + PCOffset, Registers[SrcReg]);
	  }break;
	case TRAP:
	  {
	    switch(Instruction & 0x00FF)
	      {
	      case GETC:
	    	{
	    	  Registers[R0] = (uint16)getchar();
	    	}break;
	      case OUT:
	    	{
	    	  putc((char)(Registers[R0]), stdout);
	    	  fflush(stdout);
	    	}break;
	      case PUTS:
	    	{
	    	  uint16* C = Memory + Registers[R0];
	    	  while(*C)
	    	    {
	    	      putc((char)*C, stdout);
	    	      ++C;
	    	    }
	    	  fflush(stdout);
	    	}break;
	      case IN:
	    	{
	    	  printf("Enter a character:\n");
	    	  char C = getchar();
	    	  putc(C, stdout);
	    	  Registers[R0] = (uint16)C;
	    	}break;
	      case PUTSP:
	    	{
	    	  // NOTE(l4v): Big endian is used
	    	  uint16* C = Memory + Registers[R0];
	    	  while(*C)
	    	    {
	    	      char C1 = (*C) & 0x00FF;
	    	      putc(C1, stdout);
	    	      char C2 = (*C) >> 8;
	    	      if(C2)
	    		{
	    		  putc(C2, stdout);
	    		}
	    	      ++C;
	    	    }
	    	  fflush(stdout);
	    	}break;
	      case HALT:
	    	{
	    	  printf("HALT");
	    	  Running = 0;
	    	}break;
	      }
	         
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
  RestoreInputBuffering();
  return 0;
}

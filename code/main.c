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

#define internal static
#define global_variable static
#define local_persist static

#define Assert(Expression)			\
  if(!(Expression)) {*(int*)0 = 0;}

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define STACK_SIZE 8

typedef enum
  {
   PSH = 0,
   ADD,
   POP,
   SET,
   HLT
  } InstructionSet;

typedef enum
  {
   R0 = 0, R1, R2, R3, R4, R5, PC, SP,
   NumberOfRegisters
  } Register;

global_variable Register Registers[NumberOfRegisters];

#define StackPointer (Registers[SP])
#define ProgramCounter (Registers[PC])

global_variable const int32 Program[] = {
					 PSH, 5,
					 PSH, 6,
					 ADD,
					 POP,
					 HLT
};

global_variable int32* Stack;

internal int32
Fetch()
{
  return Program[ProgramCounter];
}

internal bool32
Eval(int32 Instruction)
{
  bool32 ShouldHalt = 0;
  switch(Instruction)
    {
    case HLT:
      {
        ShouldHalt = 1;
      }break;

    case PSH:
      {
	Stack[++StackPointer] = Program[++ProgramCounter];
      }break;

    case POP:
      {
	int32 Popped = Stack[StackPointer--];
	printf("Popped value:%d\n", Popped);
      }break;

    case ADD:
      {
	int32 A = Stack[StackPointer--];
	int32 B = Stack[StackPointer--] + A;
	Stack[++StackPointer] = B;
      }break;
      
    }
  return ShouldHalt;
}

int main(void)
{
  bool32 Running = 1;
  ProgramCounter = 0;
  StackPointer = -1;
  
  Stack = (int32*)mmap(0,
		      STACK_SIZE,
		      PROT_READ | PROT_WRITE,
		      MAP_ANONYMOUS | MAP_PRIVATE,
		      -1,
		      0);
  
  while(Running)
    {
      if(Eval(Fetch()))
	{
	  Running = 0;
	}
      ProgramCounter++;
    }

  munmap(Stack, STACK_SIZE);
  return 0;
}

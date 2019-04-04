#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DATA_LEN 6

//We'll make use of these helper function later
unsigned char cpu_ram_read(struct cpu *cpu, unsigned char address)
{
  return cpu->ram[address];
}

void cpu_ram_write(struct cpu *cpu, unsigned char address, unsigned char value)
{
  cpu->ram[address] = value;
}

/**
 * Load the binary bytes from a .ls8 source file into a RAM array
 */
void cpu_load(struct cpu *cpu, char *file)
{
  // char data[DATA_LEN] = {
  //     // From print8.ls8
  //     0b10000010, // LDI R0,8
  //     0b00000000,
  //     0b00001000,
  //     0b01000111, // PRN R0
  //     0b00000000,
  //     0b00000001 // HLT
  // };

  FILE *fp;
  char line[1024];

  if ((fp = fopen(file, "r")) == NULL)
  {
    fprintf(stderr, "No file found\n");
    exit(1);
  }

  int address = 0;

  fp = fopen(file, "r");

  while (fgets(line, sizeof(line), fp) != NULL)
  {
    char *endptr;
    unsigned char value;
    value = strtoul(line, &endptr, 2);

    if (endptr == line)
    {
      continue;
    }
    cpu->ram[address++] = value;
  }

  // for (int i = 0; i < DATA_LEN; i++)
  // {
  //   cpu->ram[address++] = data[i];
  // }

  // TODO: Replace this with something less hard-coded
}

/**
 * ALU
 */
void alu(struct cpu *cpu, enum alu_op op, unsigned char regA, unsigned char regB)
{
  switch (op)
  {
  case ALU_ADD:
    cpu->registers[regA] += cpu->registers[regB];
    break;
  case ALU_AND:
    cpu->registers[regA] = cpu->registers[regA] & cpu->registers[regB];
    break;
  case ALU_CMP:
    if (cpu->registers[regA] == cpu->registers[regB])
    {
      cpu->E = 1;
      break;
    }
    else if (cpu->registers[regA] > cpu->registers[regB])
    {
      cpu->G = 1;
      break;
    }
    else
    {
      cpu->L = 1;
      break;
    }
  case ALU_MOD:
    cpu->registers[regA] %= cpu->registers[regB];
    break;
  case ALU_MUL:
    cpu->registers[regA] *= cpu->registers[regB];
    break;
  case ALU_NOT:
    cpu->registers[regA] = ~cpu->registers[regA];
    break;
  case ALU_OR:
    cpu->registers[regA] = cpu->registers[regA] | cpu->registers[regB];
    break;
  case ALU_SHL:
    cpu->registers[regA] = cpu->registers[regA] << cpu->registers[regB];
    break;
  case ALU_SHR:
    cpu->registers[regA] = cpu->registers[regA] >> cpu->registers[regB];
    break;
  case ALU_XOR:
    cpu->registers[regA] = cpu->registers[regA] ^ cpu->registers[regB];
    break;
  }
}

/**
 * Run the CPU
 */
void cpu_run(struct cpu *cpu)
{
  int running = 1; // True until we get a HLT instruction

  while (running)
  {
    // TODO
    // 1. Get the value of the current instruction (in address PC).
    unsigned char IR = cpu_ram_read(cpu, cpu->PC);
    // 2. Figure out how many operands this next instruction requires
    // 3. Get the appropriate value(s) of the operands following this instruction
    unsigned char operandA;
    unsigned char operandB;
    int next_line = 1;

    if (IR & 0X80)
    {
      operandA = cpu->ram[(cpu->PC + 1) & 0xff];
      operandB = cpu->ram[(cpu->PC + 2) & 0xff];
      next_line = 3;
    }
    else if (IR & 0x40)
    {
      operandA = cpu->ram[(cpu->PC + 1) & 0xff];
      next_line = 2;
    }

    // 4. switch() over it to decide on a course of action.
    switch (IR)
    {
    case ADD:
      alu(cpu, ALU_ADD, operandA, operandB);
      break;
    case AND:
      alu(cpu, ALU_AND, operandA, operandB);
      break;
    case CALL:
      cpu->ram[--cpu->registers[7]] = cpu->PC + next_line;
      cpu->PC = cpu->registers[operandA];
      continue;
    case RET:
      cpu->PC = cpu->ram[cpu->registers[7]++];
      continue;
    case CMP:
      alu(cpu, ALU_CMP, operandA, operandB);
      break;
    case HLT:
      running = 0;
      break;
    case JEQ:
      if (cpu->E == 1)
      {
        cpu->PC = cpu->registers[operandA];
        continue;
      }
      break;
    case JMP:
      cpu->PC = cpu->registers[operandA];
      continue;
    case JNE:
      if (cpu->E != 1)
      {
        cpu->PC = cpu->registers[operandA];
        continue;
      }
      break;
    case LDI:
      cpu->registers[operandA] = operandB;
      break;
    case MOD:
      if (cpu->registers[operandB] == 0)
      {
        printf("Dividing by zero is not allowed.\n");
        running = 0;
      }
      else
      {
        alu(cpu, ALU_MOD, operandA, operandB);
      }
      break;
    case MUL:
      alu(cpu, ALU_MUL, operandA, operandB);
      break;
    case NOT:
      alu(cpu, ALU_AND, operandA, operandB);
      break;
    case OR:
      alu(cpu, ALU_AND, operandA, operandB);
      break;
    case POP:
      cpu->registers[operandA] = cpu_ram_read(cpu, cpu->registers[7]);
      cpu->ram[cpu->registers[7]++] = 0x00;
      break;
    case PRN:
      printf("%d\n", cpu->registers[operandA]);
      break;
    case PUSH:
      cpu_ram_write(cpu, --cpu->registers[7], cpu->registers[operandA]);
      break;
    case SHL:
      alu(cpu, ALU_SHL, operandA, operandB);
      break;
    case SHR:
      alu(cpu, ALU_SHR, operandA, operandB);
      break;
    case ST:
      cpu_ram_write(cpu, operandA, cpu->registers[operandB]);
      break;
    case XOR:
      alu(cpu, ALU_AND, operandA, operandB);
      break;
    default:
      break;
    }
    cpu->PC = cpu->PC + next_line;
  }
}

/**
 * Initialize a CPU struct
 */
void cpu_init(struct cpu *cpu)
{
  // TODO: Initialize the PC and other special registers
  cpu->PC = 0;
  cpu->FL = 0;
  cpu->E = 0;
  cpu->L = 0;
  cpu->G = 0;
  memset(cpu->ram, 0, 8 * sizeof(unsigned char));
  memset(cpu->registers, 0, 256 * sizeof(unsigned char));
  cpu->registers[7] = 0XF4;
}

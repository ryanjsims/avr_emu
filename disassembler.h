#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint16_t swapEndianness(uint16_t num);
void unimplementedInstruction();
int disassembleAVROp(uint16_t *code_buffer, uint32_t pc);

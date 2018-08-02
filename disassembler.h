#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint16_t swapEndianness(uint16_t num);
void unimplementedInstruction(int*, uint16_t);
int buildSubroutineTable(uint16_t *codeBuffer, uint32_t pc, uint32_t *subBuffer, long *subs, long maxSubs);
int disassembleAVROp(uint16_t *code_buffer, uint32_t pc, uint32_t *subBuffer, long subs);

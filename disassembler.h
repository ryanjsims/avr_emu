#ifndef __disassembler_h
#define __disassembler_h
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "avr-elf-util.h"
#include "avr-utils.h"

void unimplementedInstruction(int*, uint16_t);
//char** buildIORegMap(FILE* mapFile, int* length);
//int buildSubroutineTable(uint16_t *codeBuffer, uint32_t pc, uint32_t *subBuffer, long *subs, long maxSubs);
int disassembleAVROp(uint16_t *code_buffer, uint32_t pc, 
        uint32_t *subBuffer, long subs, 
        char **ioMap, int mapLen, 
        uint16_t *text, uint16_t *data, 
        uint16_t *bss, uint16_t *rodata);
int printData(uint16_t *codeBuffer, uint32_t pc, uint16_t *data, uint16_t *bss, uint16_t *rodata);
#endif

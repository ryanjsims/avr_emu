#ifndef AVR_UTILS_H
#define AVR_UTILS_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char** buildIORegMap(FILE* mapFile, int* length);
int buildSubroutineTable(uint16_t *codeBuffer, uint32_t pc, uint32_t *subBuffer, long *subs, long maxSubs);

#endif

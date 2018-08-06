#ifndef __avr_utils
#define __avr_utils
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <elf.h>

uint16_t *getBinary(FILE* elfFile, uint16_t **text, uint16_t **data, uint16_t **bss, uint16_t **rodata, uint32_t *wordLen);
void readElfHeader(FILE* elfFile, Elf32_Ehdr *hdr);
void readPgrmHeaders(FILE* elfFile, Elf32_Ehdr ehdr, Elf32_Phdr *phdr);
void readSectHeaders(FILE* elfFile, Elf32_Ehdr ehdr, Elf32_Shdr *shdr);
#endif

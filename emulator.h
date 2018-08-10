#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "avr-elf-util.h"
#include "avr-utils.h"

#define C 0b00000001
#define Z 0b00000010
#define N 0b00000100
#define V 0b00001000
#define S 0b00010000
#define H 0b00100000
#define T 0b01000000
#define I 0b10000000

typedef struct AVRState_struct {
	uint16_t *program;
    uint8_t  *memory, *registers, *ioRegs, *SRAM;
    uint8_t  *SREG, *SPH, *SPL, *RAMPX, *RAMPY, *RAMPZ, *RAMPD, *EIND;
	uint32_t progSize, memSize;
	uint32_t pc;
} AVRState;

AVRState *createAVR(FILE* elfFile, 
        int bytesProgMem, int bytesSRAM, 
        int numRegs, int numIO,
        char **ioMap, int mapLen);

int emulateAVROp(AVRState *state);

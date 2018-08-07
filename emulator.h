#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "avr-elf-utils.h"
#include "avr-utils.h"

typedef struct AVRState_struct {
	uint16_t *program;
    uint8_t  *memory, *registers, *ioRegs, *SRAM;
    uint8_t  *SREG, *SPH, *SPL, *RAMPX, *RAMPY, *EIND;
	uint32_t progSize, memSize;
	uint32_t pc;
} AVRState;

AVRState *createAVR(FILE* elfFile, int bytesProgMem, int bytesSRAM, int numRegs, int numIO);

int emulateAVROp(AVRState *state);

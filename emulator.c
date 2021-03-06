#include "emulator.h"
/*
 * typedef struct AVRState_struct {
 *	   uint16_t *program;
 *     uint8_t  *memory, *registers, *ioRegs, *SRAM;
 *     uint8_t  *SREG, *SPH, *SPL, *RAMPX, *RAMPY, *RAMPZ, *RAMPD, *EIND;
 *     uint32_t progSize, memSize;
 *     uint32_t pc, prevPC;
 *     char ** ioMap;
 *     int ioMapLen;
 * } AVRState;
 */

void unimplementedInstruction(char *name, AVRState *state);
void breakPoint(AVRState *state);
void dumpRegisters(AVRState *state);
void dumpIORegisters(AVRState *state);

uint8_t arithSubSREG(AVRState *state, uint8_t valRd, uint8_t valRr, uint16_t result);
uint8_t arithAddSREG(AVRState *state, uint8_t valRd, uint8_t valRr, uint16_t result);
uint8_t logicSREG(AVRState *state, uint8_t valRd, uint8_t valRr, uint16_t result);

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Usage: avr_emu <compiled-avr-elf-file> [-i IOMapFile]\n");
        exit(1);
    }
    FILE* mapFile = NULL;
    for(int i = 0; i < argc - 1; i++){
        if(strcmp(argv[i], "-i") == 0){
            mapFile = fopen(argv[i+1], "r");
            break;
        }
    }
    int ioMapLen = 0;
    char **ioMap = buildIORegMap(mapFile, &ioMapLen);
    FILE* elfFile = fopen(argv[1], "rb");
    AVRState *attiny85 = createAVR(elfFile, 8192, 512, 32, 64, ioMap, ioMapLen);
    int incr;
    while(1){
        incr = emulateAVROp(attiny85);
        attiny85->pc += incr;
    }
    return 0;
}

AVRState *createAVR(FILE* elfFile, 
        int bytesProgMem, int bytesSRAM, 
        int numRegs, int numIO, 
        char **ioMap, int ioMapLen){
    AVRState *state = malloc(sizeof(AVRState));
    uint16_t *text, *data, *bss, *rodata;
    state->program = getBinary(elfFile, &text, &data, &bss, &rodata, &(state->progSize));
    state->memSize = numRegs + numIO + bytesSRAM;
    state->memory = calloc(state->memSize, 1);
    state->registers = state->memory;
    state->ioRegs = &(state->memory[numRegs]);
    state->SRAM = &(state->memory[numRegs + numIO]);
    state->pc = 0;
    state->RAMPX = NULL;
    state->RAMPY = NULL;
    state->RAMPZ = NULL;
    state->RAMPD = NULL;
    state->EIND = NULL;
    state->SREG = &(state->ioRegs[0x3F]);
    state->SPH = &(state->ioRegs[0x3E]);
    state->SPL = &(state->ioRegs[0x3D]);
    for(int i = 0; i < ioMapLen; i++){
        if(strcmp(ioMap[i], "SREG") == 0){
            state->SREG = &(state->ioRegs[i]);
        } else if(strcmp(ioMap[i], "SPH") == 0){
            state->SPH = &(state->ioRegs[i]);
        } else if(strcmp(ioMap[i], "SPL") == 0){
            state->SPL = &(state->ioRegs[i]);
        } else if(strcmp(ioMap[i], "RAMPX") == 0){
            state->RAMPX = &(state->ioRegs[i]);
        } else if(strcmp(ioMap[i], "RAMPY") == 0){
            state->RAMPY = &(state->ioRegs[i]);   
        } else if(strcmp(ioMap[i], "RAMPZ") == 0){
            state->RAMPZ = &(state->ioRegs[i]);
        } else if(strcmp(ioMap[i], "RAMPD") == 0){
            state->RAMPD = &(state->ioRegs[i]);
        } else if(strcmp(ioMap[i], "EIND") == 0){
            state->EIND = &(state->ioRegs[i]);
        }
    }
    state->ioMap = ioMap;
    state->ioMapLen = ioMapLen;
    return state;
}

uint8_t arithSubSREG(AVRState *state, uint8_t valRd, uint8_t valRr, uint16_t result){
    uint8_t newSREG = *(state->SREG);
    if((result & 0xFF) != 0){
        newSREG &= ~Z;
    } else {
        newSREG |= Z;
    }

    if(result > 0xFF){
        newSREG |= C;
    } else {
        newSREG &= ~C;
    }

    if(result & 0x80){
        newSREG |= N;
    } else {
        newSREG &= ~N;
    }

    if((valRd & ~valRr & ~result & 0x80) |
            (~valRd & valRr & result & 0x80)){
        newSREG |= V;
    } else {
        newSREG &= ~V;
    }

    if((newSREG & N) ^ ((newSREG & V) >> 1)){
        newSREG |= S;
    } else {
        newSREG &= ~S;
    }

    if(((~valRd & valRr) |
            (valRr & result) |
            (result & ~valRd)) & 0x08){
        newSREG |= H;
    } else {
        newSREG &= ~H;
    }
    return newSREG;
}

uint8_t arithAddSREG(AVRState *state, uint8_t valRd, uint8_t valRr, uint16_t result){
    uint8_t newSREG = *(state->SREG);
    if((result & 0xFF) != 0){
        newSREG &= ~Z;
    } else {
        newSREG |= Z;
    }

    if(result > 0xFF){
        newSREG |= C;
    } else {
        newSREG &= ~C;
    }

    if(result & 0x80){
        newSREG |= N;
    } else {
        newSREG &= ~N;
    }

    if((valRd & valRr & ~result & 0x80) |
            (~valRd & ~valRr & result & 0x80)){
        newSREG |= V;
    } else {
        newSREG &= ~V;
    }

    if((newSREG & N) ^ ((newSREG & V) >> 1) ){
        newSREG |= S;
    } else {
        newSREG &= ~S;
    }

    if(((valRd & valRr) |
            (valRr & ~result) |
            (~result & valRd)) & 0x08){
        newSREG |= H;
    } else {
        newSREG &= ~H;
    }
    return newSREG;
}

uint8_t logicSREG(AVRState *state, uint8_t valRd, uint8_t valRr, uint16_t result){
    uint8_t newSREG = *(state->SREG);
    if((result & 0xFF) == 0)
        newSREG |= Z;
    else
        newSREG &= ~Z;
    
    if(result & 0x80)
        newSREG |= N;
    else
        newSREG &= ~N;
    
    newSREG &= ~V;

    if((newSREG & N) ^ ((newSREG & V) >> 1) )
        newSREG |= S;
    else
        newSREG &= ~S;

    return newSREG;
}

void unimplementedInstruction(char *name, AVRState *state){
	fprintf(stderr, "Error: opcode %s (@ 0x%04X) not implemented\n", name, state->pc);
    dumpRegisters(state);
    dumpIORegisters(state);
	exit(1);
}

void breakPoint(AVRState *state){
	fprintf(stderr, "Breakpoint @ 0x%04X hit! Previous pc: 0x%04X\n", state->pc, state->prevPC);
    dumpRegisters(state);
    dumpIORegisters(state);
	exit(1);
}

void dumpRegisters(AVRState *state){
    fprintf(stderr, "\nRegisters:\n");
    fprintf(stderr, "        ITHSVNZC\n");
    fprintf(stderr, "SREG: 0b");
    for(int i = 0; i < 8; i++){
        fprintf(stderr, "%d", (*(state->SREG) & 0x80 >> i) ? 1 : 0);
    }
    fprintf(stderr, "\nSPH: 0x%02X\nSPL: 0x%02X\n", *state->SPH, *state->SPL);
    for(int i = 0; i < 32; i++){
        fprintf(stderr, "R%02d: 0x%02X", i, state->registers[i]);
        if((i + 1) % 4 == 0){
            fprintf(stderr, "\n");
        } else {
            fprintf(stderr, "    ");
        }
    }
}

void dumpIORegisters(AVRState *state){
    fprintf(stderr, "\nIO Registers:\n");
    for(int i = state->ioMapLen - 1; i >= 0; i--){
        fprintf(stderr, "\t%s:\t0b", state->ioMap[i]);
        for(int j = 0; j < 8; j++){
            fprintf(stderr, "%d", (state->ioRegs[i] & 0x80 >> j) ? 1 : 0);
        }
        fprintf(stderr, "\n");
    }
}

int emulateAVROp(AVRState *state){
	char name[64];
    uint16_t *code = &(state->program[state->pc]);
    int incr = 1;
	//Basic instructions
    if((*code & 0xFC00) == 0x0000){
        switch(*code & 0x0300){
            case 0x0000:
                //NOP
                break;
            case 0x0100:{
                //MOVW
                int Rd = ((*code & 0x00F0) >> 4) * 2;
                int Rr = (*code & 0x000F) * 2;
                //sprintf(name, "MOVW R%d:R%d, R%d:R%d", Rd + 1, Rd, Rr + 1, Rr);
				//unimplementedInstruction(name, state);
                uint8_t wordh = state->registers[Rr + 1];
                uint8_t wordl = state->registers[Rr];
                state->registers[Rd + 1] = wordh;
                state->registers[Rd] = wordl;
                break;
            }
            case 0x0200:{
                int Rd = ((*code & 0x00F0) >> 4) + 16;
                int Rr = (*code & 0x000F) + 16;
                sprintf(name, "MULS R%d, R%d", Rd, Rr);
				unimplementedInstruction(name, state);
                break;
            }
            case 0x0300:{
                int Rd = ((*code & 0x0070) >> 4) + 16;
                int Rr = (*code & 0x0007) + 16;
                switch(*code & 0x0088){
                    case 0x0000: 
						sprintf(name, "MULSU R%d, R%d", Rd, Rr); 
						unimplementedInstruction(name, state);
						break;
                    case 0x0008:
						sprintf(name, "FMUL R%d, R%d", Rd, Rr);
						unimplementedInstruction(name, state);
						break;
                    case 0x0080:
						sprintf(name, "FMULS R%d, R%d", Rd, Rr);
						unimplementedInstruction(name, state);
						break;
                    case 0x0088:
						sprintf(name, "FMULSU R%d, R%d", Rd, Rr);
						unimplementedInstruction(name, state);
						break;
                }
                break;
            }
        }
    }
    //Two-operand instructions
    else if((*code & 0xC000) == 0x0000){
        int Rd = (*code & 0x01F0) >> 4;
        int Rr = (*code & 0x000F) + ((*code & 0x0200) >> 5);
        uint16_t result;
        uint8_t newSREG;
        switch(*code & 0x3C00){
            case 0x0000: 
				sprintf(name, "Reserved"); 
				unimplementedInstruction(name, state); 
				break;
            case 0x0400: 
//				sprintf(name, "CPC R%1$d, R%2$d", Rd, Rr); 
                result = (uint16_t)(state->registers[Rd]) - (uint16_t)(state->registers[Rr]);
				result -= (*(state->SREG) & C) ? 1 : 0;
                newSREG = arithSubSREG(state, state->registers[Rd], state->registers[Rr], result);
                if(result == 0){
                    *(state->SREG) = newSREG;
                } else {
                    *(state->SREG) = (newSREG & ~Z) | (*(state->SREG) & Z);
                }
                break;
            case 0x1400: 
//				sprintf(name, "CP R%1$d, R%2$d", Rd, Rr);
                result = (uint16_t)(state->registers[Rd]) - (uint16_t)(state->registers[Rr]);
                *(state->SREG) = arithSubSREG(state, state->registers[Rd], state->registers[Rr], result);
				break;
            case 0x0800: 
//				sprintf(name, "SBC R%1$d, R%2$d", Rd, Rr); 
                result = (uint16_t)(state->registers[Rd]) - (uint16_t)(state->registers[Rr]);
				result -= (*(state->SREG) & C) ? 1 : 0;
                newSREG = arithSubSREG(state, state->registers[Rd], state->registers[Rr], result);
                if(result == 0){
                    *(state->SREG) = newSREG;
                } else {
                    *(state->SREG) = (newSREG & ~Z) | (*(state->SREG) & Z);
                }
                state->registers[Rd] = result & 0xFF;
				break;
            case 0x1800: 
//				sprintf(name, "SUB R%1$d, R%2$d", Rd, Rr);
                result = (uint16_t)(state->registers[Rd]) - (uint16_t)(state->registers[Rr]);
                *(state->SREG) = arithSubSREG(state, state->registers[Rd], state->registers[Rr], result);
                state->registers[Rd] = result & 0xFF;
				break;
            case 0x0C00:
                /*if(Rd == Rr)
                    sprintf(name, "LSL R%1$d", Rd);
                else
                    sprintf(name, "ADD R%1$d, R%2$d", Rd, Rr);*/
                result = (uint16_t)(state->registers[Rd]) + (uint16_t)(state->registers[Rr]);
                *(state->SREG) = arithAddSREG(state, state->registers[Rd], state->registers[Rr], result);
                state->registers[Rd] = result & 0xFF;
                break;
            case 0x1C00:
                /*if(Rd == Rr)
                    sprintf("ROL R%1$d", Rd);
                else
                    sprintf("ADC R%1$d, R%2$d", Rd, Rr);*/
                result = (uint16_t)(state->registers[Rd]) + (uint16_t)(state->registers[Rr]);
				result += (*(state->SREG) & C) ? 1 : 0;
                *(state->SREG) = arithAddSREG(state, state->registers[Rd], state->registers[Rr], result);
                state->registers[Rd] = result & 0xFF;
                break;
            case 0x1000: 
//				sprintf(name, "CPSE R%1$d, R%2$d", Rd, Rr);
                if(state->registers[Rd] == state->registers[Rr]){
                    incr = 1 + instructionWords(code[1]);
                }
				break;
            case 0x2000: 
//				sprintf(name, "AND R%1$d, R%2$d", Rd, Rr);
                result = state->registers[Rd] & state->registers[Rr];
                *(state->SREG) = logicSREG(state, state->registers[Rd], state->registers[Rr], result);
				state->registers[Rd] = result & 0xFF;
                break;
            case 0x2400: 
//				sprintf(name, "EOR R%1$d, R%2$d", Rd, Rr);
                result = state->registers[Rd] ^ state->registers[Rr];
                *(state->SREG) = logicSREG(state, state->registers[Rd], state->registers[Rr], result);
				state->registers[Rd] = result & 0xFF;
				break;
            case 0x2800: 
//				sprintf(name, "OR R%1$d, R%2$d", Rd, Rr);
                result = state->registers[Rd] | state->registers[Rr];
                *(state->SREG) = logicSREG(state, state->registers[Rd], state->registers[Rr], result);
				state->registers[Rd] = result & 0xFF;
				break;
            case 0x2C00: 
//				sprintf(name, "MOV R%1$d, R%2$d", Rd, Rr);
                state->registers[Rd] = state->registers[Rr];
				break;
            default:{
          //case 0x3X00:
                Rd = (Rd & 0x000F) + 16;
                int K = ((*code & 0x0F00) >> 4) + (*code & 0x000F);
//              sprintf(name, "CPI R%d, %d", Rd, K);
                result = (uint16_t)(state->registers[Rd]) - (uint16_t)K;
                *(state->SREG) = arithSubSREG(state, state->registers[Rd], K, result);
                break;
            }
        }
    }
    //Register-immediate instructions
    else if((*code & 0xC000) == 0x4000){
        int Rd = ((*code & 0x00F0) >> 4) + 16;
        int K = ((*code & 0x0F00) >> 4) + (*code & 0x000F);
        uint16_t result;
        uint8_t newSREG;
        switch(*code & 0x3000){
            case 0x0000: 
//				sprintf(name, "SBCI R%1$d, %2$d", Rd, K);
                result = (uint16_t)(state->registers[Rd]) - (uint16_t)K;
				result -= (*(state->SREG) & C) ? 1 : 0;
                newSREG = arithSubSREG(state, state->registers[Rd], K, result);
                if(result == 0){
                    *(state->SREG) = newSREG;
                } else {
                    *(state->SREG) = (newSREG & ~Z) | (*(state->SREG) & Z);
                }
                state->registers[Rd] = result & 0xFF;
				break;
            case 0x1000: 
//				sprintf(name, "SUBI R%1$d, %2$d", Rd, K);
                result = (uint16_t)(state->registers[Rd]) - (uint16_t)K;
                *(state->SREG) = arithSubSREG(state, state->registers[Rd], K, result);
                state->registers[Rd] = result & 0xFF;
				break;
            case 0x2000: 
//              sprintf(name, "ORI R%1$d, %2$d", Rd, K);
                result = state->registers[Rd] | K;
                *(state->SREG) = logicSREG(state, state->registers[Rd], K, result);
				state->registers[Rd] = result & 0xFF;
                break;
            case 0x3000: 
//              sprintf(name, "ANDI R%1$d, %2$d", Rd, K);
                result = state->registers[Rd] & K;
                *(state->SREG) = logicSREG(state, state->registers[Rd], K, result);
				state->registers[Rd] = result & 0xFF;
                break;
        }
    }
    //LDD/STD
    else if((*code & 0xD000) == 0x8000){
        int Rd = (*code & 0x01F0) >> 4;
        int K = ((*code & 0x2000) >> 8) + ((*code & 0x0C00) >> 7) + (*code & 0x0007);
        uint32_t loc = 0;
        if(K != 0){
            switch(*code & 0x0208){
                case 0x0000:
//                  sprintf(name, "LDD R%d, Z + %d", Rd, K); 
                    if(state->RAMPZ)
                        loc = ((*state->RAMPZ) << 16);
                    loc |= (state->registers[31] << 8) | (state->registers[30]);
                    loc += K;
                    state->registers[Rd] = state->memory[loc];
                    break;
                case 0x0008:
//                  sprintf(name, "LDD R%d, Y + %d", Rd, K);
                    if(state->RAMPY)
                        loc = ((*state->RAMPY) << 16);
                    loc |= (state->registers[29] << 8) | (state->registers[28]);
                    loc += K;
                    state->registers[Rd] = state->memory[loc];
                    break;
                case 0x0200:
//                  sprintf(name, "STD Z + %d, R%d", K, Rd);
                    if(state->RAMPZ)
                        loc = ((*state->RAMPZ) << 16);
                    loc |= (state->registers[31] << 8) | (state->registers[30]);
                    loc += K;
                    state->memory[loc] = state->registers[Rd];
                    break;
                case 0x0208:
//                  sprintf(name, "STD Y + %d, R%d", K, Rd);
                    if(state->RAMPY)
                        loc = ((*state->RAMPY) << 16);
                    loc |= (state->registers[29] << 8) | (state->registers[28]);
                    loc += K;
                    state->memory[loc] = state->registers[Rd];
                    break;
            }
        } else {
            switch(*code & 0x0208){
                case 0x0000:
//                  sprintf(name, "LD R%d, Z", Rd);
                    if(state->RAMPZ)
                        loc = ((*state->RAMPZ) << 16);
                    loc |= (state->registers[31] << 8) | (state->registers[30]);
                    state->registers[Rd] = state->memory[loc];
                    break;
                case 0x0008:
//                  sprintf(name, "LD R%d, Y", Rd);
                    if(state->RAMPY)
                        loc = ((*state->RAMPY) << 16);
                    loc |= (state->registers[29] << 8) | (state->registers[28]);
                    state->registers[Rd] = state->memory[loc];
                    break;
                case 0x0200:
//                  sprintf(name, "ST Z, R%d", Rd);
                    if(state->RAMPZ)
                        loc = ((*state->RAMPZ) << 16);
                    loc |= (state->registers[31] << 8) | (state->registers[30]);
                    state->memory[loc] = state->registers[Rd];
                    break;
                case 0x0208:
//                  sprintf(name, "ST Y, R%d", Rd);
                    if(state->RAMPY)
                        loc = ((*state->RAMPY) << 16);
                    loc |= (state->registers[29] << 8) | (state->registers[28]);
                    state->memory[loc] = state->registers[Rd];
                    break;
            }
        }
    }
    //Load-store operations
    else if((*code & 0xFC00) == 0x9000){
        int Rd = (*code & 0x01F0) >> 4;
        uint16_t pX = (((uint16_t)state->registers[27]) << 8) | state->registers[26];
        uint16_t pY = (((uint16_t)state->registers[29]) << 8) | state->registers[28];
        uint16_t pZ = (((uint16_t)state->registers[31]) << 8) | state->registers[30];
        uint16_t sp = (((uint16_t)*state->SPH) << 8) | *state->SPL;
        switch(*code & 0x020F){
            case 0x0000:
                //sprintf(name, "LDS R%d, 0x%04x", Rd, code[1]);
				state->registers[Rd] = state->memory[code[1]];
                incr = 2;
                break;
            case 0x0200:
                //sprintf(name, "STS 0x%04x, R%d", code[1], Rd);
				state->memory[code[1]] = state->registers[Rd];
                incr = 2;
                break;
            case 0x0001:
                //sprintf(name, "LD R%d, Z+", Rd);
				state->registers[Rd] = state->memory[pZ];
                state->registers[31] = ((pZ + 1) & 0xFF00) >> 8;
                state->registers[30] = (pZ + 1) & 0x00FF;
                break;
            case 0x0009:
                //sprintf(name, "LD R%d, Y+", Rd);
				state->registers[Rd] = state->memory[pY];
                state->registers[29] = ((pY + 1) & 0xFF00) >> 8;
                state->registers[28] = (pY + 1) & 0x00FF;
                break;
            case 0x0201:
                //sprintf(name, "ST Z+, R%d", Rd); 
				state->memory[pZ] = state->registers[Rd];
                state->registers[31] = ((pZ + 1) & 0xFF00) >> 8;
                state->registers[30] = (pZ + 1) & 0x00FF;
                break;
            case 0x0209:
                //sprintf(name, "ST Y+, R%d", Rd);
				state->memory[pY] = state->registers[Rd];
                state->registers[29] = ((pY + 1) & 0xFF00) >> 8;
                state->registers[28] = (pY + 1) & 0x00FF;
                break;
            case 0x0002:
                //sprintf(name, "LD R%d, -Z", Rd);
                state->registers[Rd] = state->memory[pZ - 1];
                state->registers[31] = ((pZ - 1) & 0xFF00) >> 8;
                state->registers[30] = (pZ - 1) & 0x00FF;
                break;
            case 0x000A:
                //sprintf(name, "LD R%d, -Y", Rd);
                state->registers[Rd] = state->memory[pY - 1];
                state->registers[29] = ((pY - 1) & 0xFF00) >> 8;
                state->registers[28] = (pY - 1) & 0x00FF;
                break;
            case 0x0202:
                //sprintf(name, "ST -Z, R%d", Rd);
                state->memory[pZ - 1] = state->registers[Rd];
                state->registers[31] = ((pZ - 1) & 0xFF00) >> 8;
                state->registers[30] = (pZ - 1) & 0x00FF;
                break;
            case 0x020A:
                //sprintf(name, "ST -Y, R%d", Rd);
                state->memory[pY - 1] = state->registers[Rd];
                state->registers[29] = ((pY - 1) & 0xFF00) >> 8;
                state->registers[28] = (pY - 1) & 0x00FF;
                break;
            case 0x0004:
//              sprintf(name, "LPM R%d, Z", Rd);
                state->registers[Rd] = (pZ % 2 == 0) ? 
                    (state->program[pZ >> 1] & 0x00FF) : 
                    (state->program[pZ >> 1] & 0xFF00) >> 8;
                break;
            case 0x0006:{
                if(state->RAMPZ == NULL){
                    sprintf(name, "ELPM R%d, Z", Rd);
				    unimplementedInstruction(name, state);
                }
                uint32_t addr = ((*state->RAMPZ) << 16) | pZ;
                state->registers[Rd] = (addr % 2 == 0) ? 
                    (state->program[addr >> 1] & 0x00FF) : 
                    (state->program[addr >> 1] & 0xFF00) >> 8;
                break;
            }
            case 0x0005:
//              sprintf(name, "LPM R%d, Z+", Rd);
                state->registers[Rd] = (pZ % 2 == 0) ? 
                    (state->program[pZ >> 1] & 0x00FF) : 
                    (state->program[pZ >> 1] & 0xFF00) >> 8;
                state->registers[31] = ((pZ + 1) & 0xFF00) >> 8;
                state->registers[30] = (pZ + 1) & 0x00FF;
                break;
            case 0x0007:
                if(state->RAMPZ == NULL){
                    sprintf(name, "ELPM R%d, Z+", Rd);
				    unimplementedInstruction(name, state);
                }
                uint32_t addr = ((*state->RAMPZ) << 16) | pZ;
                state->registers[Rd] = (addr % 2 == 0) ? 
                    (state->program[addr >> 1] & 0x00FF) : 
                    (state->program[addr >> 1] & 0xFF00) >> 8;
                addr += 1;
                *state->RAMPZ = (addr & 0x00FF0000) >> 16;
                state->registers[31] = (addr & 0x0000FF00) >> 8;
                state->registers[30] = (addr & 0x000000FF);
                break;
            case 0x0204:{
                //sprintf(name, "XCH Z, R%d", Rd);
                uint8_t temp = state->memory[pZ];
                state->memory[pZ] = state->registers[Rd];
                state->registers[Rd] = temp;
                break;
            }
            case 0x0205:
                sprintf(name, "LAS Z, R%d", Rd);
				unimplementedInstruction(name, state); 
                break;
            case 0x0206:
                sprintf(name, "LAC Z, R%d", Rd);
				unimplementedInstruction(name, state); 
                break;
            case 0x0207:
                sprintf(name, "LAT Z, R%d", Rd);
				unimplementedInstruction(name, state); 
                break;
            case 0x000C:
                sprintf(name, "LD R%d, X", Rd);
				unimplementedInstruction(name, state); 
                break;
            case 0x020C:
//              sprintf(name, "ST X, R%d", Rd);
                state->memory[pX] = state->registers[Rd];
                break;
            case 0x000D:
                sprintf(name, "LD R%d, X+", Rd);
				unimplementedInstruction(name, state); 
                break;
            case 0x020D:
//              sprintf(name, "ST X+, R%d", Rd);
                state->memory[pX] = state->registers[Rd];
                state->registers[27] = ((pX + 1) & 0xFF00) >> 8;
                state->registers[26] = (pX + 1) & 0x00FF;
                break;
            case 0x000E:
                sprintf(name, "LD R%d, -X", Rd);
				unimplementedInstruction(name, state); 
                break;
            case 0x020E:
                sprintf(name, "ST -X, R%d", Rd);
				unimplementedInstruction(name, state); 
                break;
            case 0x000F:
//              sprintf(name, "POP R%d", Rd);
                state->registers[Rd] = state->memory[sp+1];
                *(state->SPH) = ((sp + 1) & 0xFF00) >> 8;
                *(state->SPL) = (sp + 1) & 0x00FF;
                break;
            case 0x020F:
//              sprintf(name, "PUSH R%d", Rd);
                state->memory[sp] = state->registers[Rd];
                *(state->SPH) = ((sp - 1) & 0xFF00) >> 8;
                *(state->SPL) = (sp - 1) & 0x00FF;
                break;
            default:
                sprintf(name, "LD/ST reserved");
                unimplementedInstruction(name, state);
                break;
        }
    }
    //Zero-operand instructions
    else if((*code & 0xFF0F) == 0x9508){
        switch(*code & 0x00F0){
            case 0x0000:{
                //sprintf(name, "RET");
                uint16_t sp = (uint16_t)(*state->SPH << 8) | (*state->SPH);
                uint32_t newPC;
                if(state->progSize > 65535){
                    sp = sp - 3;
                    newPC = (state->memory[sp - 2] << 16);
                } else {
                    sp = sp - 2;
                }
                newPC |= (state->memory[sp - 1] << 8) | (state->memory[sp]);
                state->pc = newPC;
                *state->SPH = (sp & 0xFF00) >> 8;
                *state->SPL = (sp & 0x00FF);
                break;
            }
            case 0x0010:{
                //sprintf(name, "RETI");
                uint16_t sp = (uint16_t)(*state->SPH << 8) | (*state->SPH);
                uint32_t newPC;
                if(state->progSize > 65535){
                    sp = sp - 3;
                    newPC = (state->memory[sp - 2] << 16);
                } else {
                    sp = sp - 2;
                }
                newPC |= (state->memory[sp - 1] << 8) | (state->memory[sp]);
                state->pc = newPC;
                *state->SPH = (sp & 0xFF00) >> 8;
                *state->SPL = (sp & 0x00FF);
                *state->SREG |= I;
                break; //Return from interrupt - Sets I bit in SREG
            }
            //0x0020 - 0x0070: reserved
            case 0x0080:
                sprintf(name, "SLEEP");
                unimplementedInstruction(name, state);
                break;
            case 0x0090:
                //Break
                //TODO: add lock bits/fuses
                breakPoint(state);
                break;
            case 0x00A0:
                sprintf(name, "WDR");
                unimplementedInstruction(name, state);
                break;
            //0x00B0: reserved
            case 0x00C0:
                sprintf(name, "LPM");
                unimplementedInstruction(name, state);
                break;
            case 0x00D0:
                sprintf(name, "ELPM");
                unimplementedInstruction(name, state);
                break;
            case 0x00E0:
                sprintf(name, "SPM");
                unimplementedInstruction(name, state);
                break;
            case 0x00F0:
                sprintf(name, "SPM Z+");
                unimplementedInstruction(name, state);
                break;
            default: 
                sprintf(name, "Zero Operand");
                unimplementedInstruction(name, state);
                break;
        }
    }
    //One-operand instructions
    else if((*code & 0xFE08) == 0x9400){
        int Rd = (*code & 0x01F0) >> 4;
        switch(*code & 0x0007){
            case 0x0000: 
                sprintf(name, "COM R%1$d", Rd);
                unimplementedInstruction(name, state);
                break;
            case 0x0001:
                sprintf(name, "NEG R%1$d", Rd);
                unimplementedInstruction(name, state);
                break;
            case 0x0002:
                sprintf(name, "SWAP R%1$d", Rd);
                unimplementedInstruction(name, state);
                break;
            case 0x0003:
                sprintf(name, "INC R%1$d", Rd);
                unimplementedInstruction(name, state);
                break;
            case 0x0004:
                sprintf(name, "One operand");
                unimplementedInstruction(name, state);
                break;
            case 0x0005:
                sprintf(name, "ASR R%1$d", Rd);
                unimplementedInstruction(name, state);
                break;
            case 0x0006:
                sprintf(name, "LSR R%1$d", Rd);
                unimplementedInstruction(name, state);
                break;
            case 0x0007:
                sprintf(name, "ROR R%1$d", Rd);
                unimplementedInstruction(name, state);
                break;
        }
    }
    //SE*/CL*: * in {C, Z, N, V, S, H, T, I}
    //Set/clear flags
    else if((*code & 0xFF0F) == 0x9408){
        switch(*code & 0x00F0){
            case 0x0000: 
//              sprintf(name, "SEC"); 
                *(state->SREG) |= C;
                break;
            case 0x0010: 
//              sprintf(name, "SEZ"); 
                *(state->SREG) |= Z;
                break;
            case 0x0020: 
//              sprintf(name, "SEN"); 
                *(state->SREG) |= N;
                break;
            case 0x0030: 
//              sprintf(name, "SEV"); 
                *(state->SREG) |= V;
                break;
            case 0x0040: 
//              sprintf(name, "SES"); 
                *(state->SREG) |= S;
                break;
            case 0x0050: 
//              sprintf(name, "SEH"); 
                *(state->SREG) |= H;
                break;
            case 0x0060: 
//              sprintf(name, "SET"); 
                *(state->SREG) |= T;
                break;
            case 0x0070: 
//              sprintf(name, "SEI"); 
                *(state->SREG) |= I;
                break;
            case 0x0080: 
//              sprintf(name, "CLC"); 
                *(state->SREG) &= ~C;
                break;
            case 0x0090: 
//              sprintf(name, "CLZ"); 
                *(state->SREG) &= ~Z;
                break;
            case 0x00A0: 
//              sprintf(name, "CLN"); 
                *(state->SREG) &= ~N;
                break;
            case 0x00B0: 
//              sprintf(name, "CLV"); 
                *(state->SREG) &= ~V;
                break;
            case 0x00C0: 
//              sprintf(name, "CLS"); 
                *(state->SREG) &= ~S;
                break;
            case 0x00D0: 
//              sprintf(name, "CLH"); 
                *(state->SREG) &= ~H;
                break;
            case 0x00E0: 
//              sprintf(name, "CLT"); 
                *(state->SREG) &= ~T;
                break;
            case 0x00F0: 
//              sprintf(name, "CLI"); 
                *(state->SREG) &= ~I;
                break;
        }
    }
    //EIJMP, IJMP, EICALL, ICALL, DEC, DES, JMP, CALL
    else if((*code & 0xFE08) == 0x9408){
        switch(*code & 0x0007){
            //SE*/CL* block catches 0x0000 case
            case 0x0001:
                switch(*code & 0x0110){
                    case 0x0000: 
                        sprintf(name, "IJMP"); 
                        unimplementedInstruction(name, state);
                        break;
                    case 0x0010: 
                        sprintf(name, "EIJMP"); 
                        unimplementedInstruction(name, state);
                        break;
                    case 0x0100: 
                        sprintf(name, "ICALL"); 
                        unimplementedInstruction(name, state);
                        break;
                    case 0x0110: 
                        sprintf(name, "EICALL"); 
                        unimplementedInstruction(name, state);
                        break;
                }
                break;
            case 0x0002:{
                int Rd = (*code & 0x01F0) >> 4;
                uint8_t result = state->registers[Rd] - 1;
				//sprintf(name, "DEC R%d", Rd);
				if(result == 0x7F)
					*state->SREG |= V;
				else
					*state->SREG &= ~V;
				if(result == 0)
					*state->SREG |= Z;
				else
					*state->SREG &= ~Z;
				if(result & 0x80)
					*state->SREG |= N;
				else
					*state->SREG &= ~N;
				if(((result & V) >> 1) ^ (result & N))
					*state->SREG |= S;
				else
					*state->SREG &= ~S;
				state->registers[Rd] = result;
                break;
            }
            case 0x0003:{
                int K = (*code & 0x00F0) >> 4;
                sprintf(name, "DES 0x%1x", K);
                unimplementedInstruction(name, state);
                break;
            }
            default:{
          //case 0b010x, 0b011x:
                //2 word (4 byte) instruction. 22 bit Parameter K stored as: 
                //0b0000000k kkkk000k kkkkkkkk kkkkkkkk
                uint32_t K = (((uint32_t)((*code & 0x01F0) >> 3) | (*code & 0x0001)) << 16) | ((uint32_t)code[1]);
                switch(*code & 0x0002){
                    case 0x0000: 
                        sprintf(name, "JMP 0x%06x", K); 
                        unimplementedInstruction(name, state);
                        break;
                    case 0x0002: 
                        sprintf(name, "CALL 0x%06x", K); 
                        unimplementedInstruction(name, state);
                        break;
                }
                incr = 2;
                break;
            }
        }
    }
    //ADIW/SBIW
    else if((*code & 0xFE00) == 0x9600){
        uint8_t k = ((*code & 0x00C0) >> 2) | (*code & 0x000F);
        int Rp = ((*code & 0x0030) >> 4) * 2 + 24; //Rp in (24, 26, 28, 30}
        uint16_t result = (((uint16_t)state->registers[Rp + 1]) << 8) | state->registers[Rp];
        switch(*code & 0x0100){
            case 0x0000: 
//              sprintf(name, "ADIW R%d, %d", Rp, k); 
                result += k;
                if(result == 0)
                    *(state->SREG) |= Z;
                else
                    *(state->SREG) &= ~Z;
                
                if(((result >> 8) & ~(state->registers[Rp + 1])) & 0x80)
                    *(state->SREG) |= V;
                else
                    *(state->SREG) &= ~V;

                if((~(result >> 8) & (state->registers[Rp + 1])) & 0x80)
                    *(state->SREG) |= C;
                else
                    *(state->SREG) &= ~C;

                if(result & 0x8000)
                    *(state->SREG) |= N;
                else
                    *(state->SREG) &= ~N;

                if(((*(state->SREG) & V) >> 1) ^ (*(state->SREG) & N))
                    *(state->SREG) |= S;
                else
                    *(state->SREG) &= ~S;

                state->registers[Rp + 1] = result >> 8;
                state->registers[Rp] = result & 0x00FF;
                break;
            case 0x0100: 
//              sprintf(name, "SBIW R%d, %d", Rp, k); 
                result -= k;
                if(result == 0)
                    *(state->SREG) |= Z;
                else
                    *(state->SREG) &= ~Z;
                
                if(((result >> 8) & ~(state->registers[Rp + 1])) & 0x80)
                    *(state->SREG) |= C | V;
                else
                    *(state->SREG) &= ~(C | V);

                if(result & 0x8000)
                    *(state->SREG) |= N;
                else
                    *(state->SREG) &= ~N;

                if(((*(state->SREG) & V) >> 1) ^ (*(state->SREG) & N))
                    *(state->SREG) |= S;
                else
                    *(state->SREG) &= ~S;

                state->registers[Rp + 1] = result >> 8;
                state->registers[Rp] = result & 0x00FF;
                break;
        }
    }
    //CBI, SBI, SBIC, SBIS
    //Set and check bits in IO registers 0-31 (memory addresses 0x20 - 0x3F)
    else if((*code & 0xFC00) == 0x9800){
        int Ra = (*code & 0x00F8) >> 3;
        int bit = (*code & 0x0007);
        switch(*code & 0x0300){
            case 0x0000: 
                sprintf(name, "CBI IO%d, %d", Ra, bit); 
                unimplementedInstruction(name, state);
                break;
            case 0x0200: 
                sprintf(name, "SBI IO%d, %d", Ra, bit); 
                unimplementedInstruction(name, state);
                break;
            case 0x0100: 
                sprintf(name, "SBIC IO%d, %d", Ra, bit); 
                unimplementedInstruction(name, state);
                break;
            case 0x0300: 
                sprintf(name, "SBIS IO%d, %d", Ra, bit); 
                unimplementedInstruction(name, state);
                break;
        }
    }
    //MUL (Unsigned)
    //Multiplies the values in Rd and Rr and stores the 16 bit result in R1:R0
    else if((*code & 0xFC00) == 0x9C00){
        int Rd = (*code & 0x01F0) >> 4;
        int Rr = ((*code & 0x0200) >> 5) | (*code & 0x000F);
        sprintf(name, "MUL R%d, R%d", Rd, Rr);
        unimplementedInstruction(name, state);
    }
    //IN/OUT to I/O space
    //Read/Write to the 64 IO registers (memory addresses 0x20 - 0x5F)
    else if((*code & 0xF000) == 0xB000){
        int Rio = ((*code & 0x0600) >> 5) | (*code & 0x000F);
        int Rd = (*code & 0x01F0) >> 4;
        switch(*code & 0x0800){
            case 0x0000: 
//              sprintf(name, "IN R%d, IO%d", Rd, Rio); 
                state->registers[Rd] = state->ioRegs[Rio];
                break;
            case 0x0800: 
//              sprintf(name, "OUT IO%d, R%d", Rio, Rd); 
                state->ioRegs[Rio] = state->registers[Rd];
                break;
        }
    }
    //RJMP/RCALL to PC + signed 12 bit immediate value
    else if((*code & 0xE000) == 0xC000){
        int16_t offset = *code & 0x0FFF;
        uint16_t sp = ((uint16_t)(*(state->SPH)) << 8) | *(state->SPL);
        //Offset is signed, sign extension required
        if(offset & 0x0800)
            offset |= 0xF000;
        switch(*code & 0x1000){
            case 0x0000: 
//              sprintf(name, "RJMP %d", offset); 
                state->prevPC = state->pc;
                state->pc += offset;
                break;
            case 0x1000: 
//              sprintf(name, "RCALL %d", offset); 
                state->memory[sp] = (state->pc + 1) & 0x000000FF;
                state->memory[sp - 1] = ((state->pc + 1) & 0x0000FF00) >> 8;
                if(state->progSize > 65535){
                    state->memory[sp - 2] = ((state->pc + 1) & 0x001F0000) >> 16;
                    sp = sp - 3;
                } else {
                    sp = sp - 2;
                }
                *(state->SPH) = (sp & 0xFF00) >> 8;
                *(state->SPL) = (sp & 0x00FF);
                state->prevPC = state->pc;
                state->pc += offset;
                break;
        }
    }
    //LDI
    //Load immediate into registers 16 to 31
    else if((*code & 0xF000) == 0xE000){
        uint8_t K = ((*code & 0x0F00) >> 4) | (*code & 0x000F);
        int Rd = ((*code & 0x00F0) >> 4) + 16; //16 <= Rd <= 31
//      sprintf(name, "LDI R%1$d, %2$X", Rd, K);
        state->registers[Rd] = K;
    }
    //SBRC/SBRS, BLD/BST
    else if((*code & 0xF808) == 0xF800){
        int Rd = (*code & 0x01F0) >> 4;
        int bit = (*code & 0x0007);
        switch(*code & 0x0600){
            case 0x0000: 
                sprintf(name, "BLD R%1$d, %2$d", Rd, bit); 
                unimplementedInstruction(name, state);
                break;
            case 0x0200: 
                sprintf(name, "BST R%1$d, %2$d", Rd, bit); 
                unimplementedInstruction(name, state);
                break;
            case 0x0400: 
                sprintf(name, "SBRC R%1$d, %2$d", Rd, bit); 
                unimplementedInstruction(name, state);
                break;
            case 0x0600: 
                sprintf(name, "SBRS R%1$d, %2$d", Rd, bit); 
                unimplementedInstruction(name, state);
                break;
        }
    }
    //Conditional branches
    //Flags: {C, Z, N, V, S, H, T, I}
    else if((*code & 0xF800) == 0xF000){
        int8_t offset = (int8_t)((*code & 0x03F8) >> 3);
        if(offset & 0x40)
            offset |= 0x80;
        state->prevPC = state->pc;
        switch(*code & 0x0407){
            case 0x0000: 
                //sprintf(name, "BRCS %d", offset); 
                if(*state->SREG & C){
                    state->pc += offset;
                }
                break;
            case 0x0400: 
                //sprintf(name, "BRCC %d", offset); 
                if(!(*state->SREG & C)){
                    state->pc += offset;
                }
                break;
            case 0x0001: 
                //sprintf(name, "BREQ %d", offset); 
                if(*state->SREG & Z){
                    state->pc += offset;
                }
                break;
            case 0x0401: 
//              sprintf(name, "BRNE %d", offset); 
                if(!(*state->SREG & Z)){
                    state->pc += offset;
                }
                break;
            case 0x0002: 
                //sprintf(name, "BRMI %d", offset); 
                if(*state->SREG & N){
                    state->pc += offset;
                }
                break;
            case 0x0402: 
                //sprintf(name, "BRPL %d", offset); 
                if(!(*state->SREG & N)){
                    state->pc += offset;
                }
                break;
            case 0x0003: 
                //sprintf(name, "BRVS %d", offset); 
                if(*state->SREG & V){
                    state->pc += offset;
                }
                break;
            case 0x0403: 
                //sprintf(name, "BRVC %d", offset); 
                if(!(*state->SREG & V)){
                    state->pc += offset;
                }
                break;
            case 0x0004: 
                //sprintf(name, "BRLT %d", offset); 
                if(*state->SREG & S){
                    state->pc += offset;
                }
                break;
            case 0x0404: 
                //sprintf(name, "BRGE %d", offset); 
                if(!(*state->SREG & S)){
                    state->pc += offset;
                }
                break;
            case 0x0005: 
                //sprintf(name, "BRHS %d", offset); 
                if(*state->SREG & H){
                    state->pc += offset;
                }
                break;
            case 0x0405: 
                //sprintf(name, "BRHC %d", offset); 
                if(!(*state->SREG & H)){
                    state->pc += offset;
                }
                break;
            case 0x0006: 
                //sprintf(name, "BRTS %d", offset); 
                if(*state->SREG & T){
                    state->pc += offset;
                }
                break;
            case 0x0406: 
                //sprintf(name, "BRTC %d", offset); 
                if(!(*state->SREG & T)){
                    state->pc += offset;
                }
                break;
            case 0x0007: 
                //sprintf(name, "BRIE %d", offset); 
                if(*state->SREG & I){
                    state->pc += offset;
                }
                break;
            case 0x0407: 
                //sprintf(name, "BRID %d", offset); 
                if(!(*state->SREG & I)){
                    state->pc += offset;
                }
                break;
        }
    }
    //Reserved opcodes
    else{
        sprintf(name, "Reserved");
        unimplementedInstruction(name, state);
    }
    return incr;
}

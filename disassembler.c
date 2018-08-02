#include "disassembler.h"

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Please provide a compiled AVR binary file to disassemble.\n");
        return 1;
    }
    FILE* compiledFile = fopen(argv[1], "rb");
    fseek(compiledFile, 0, SEEK_END);
    long flength = ftell(compiledFile);
    fseek(compiledFile, 0, SEEK_SET);
    uint16_t *codeBuffer = calloc(flength / 2, sizeof(uint16_t));
    fread(codeBuffer, sizeof(uint16_t), flength / 2, compiledFile);
    fclose(compiledFile);
	uint16_t *swappedCodeBuffer = calloc(flength / 2, sizeof(uint16_t));
    uint32_t *subBuffer = calloc(flength / 2, sizeof(uint32_t));
    long subs = 0, maxSubs = flength / 2;
	for(int i = 0; i < flength / 2; i++){
		swappedCodeBuffer[i] = swapEndianness(codeBuffer[i]);
	}
    uint32_t pc = 0;
    while(pc < flength / 2){
        int incr = buildSubroutineTable(codeBuffer, pc, subBuffer, &subs, maxSubs);
        pc += incr;
    }
    pc = 0;
    while(pc < flength / 2){
        int incr = disassembleAVROp(codeBuffer, pc, subBuffer, subs);
        if(incr == 0)
            return -1;
        pc += incr;
    }
    return 0;
}

uint16_t swapEndianness(uint16_t num){
    uint16_t b0,b1;

    b0 = (num & 0x00ff) << 8u;
    b1 = (num & 0xff00) >> 8u;

    return b0 | b1;
}


void unimplementedInstruction(int *opWords, uint16_t opCode){
    printf("UNKNOWN OPCODE (data?) -- 0b");
    for(int i = 0; i < 16; i++){
        if(opCode & (0x8000 >> i))
            printf("1");
        else
            printf("0");
    }
    printf(" -- 0x%04x -- %d", opCode, opCode);
    //*opWords = 0;
}

int buildSubroutineTable(uint16_t *codeBuffer, uint32_t pc, uint32_t *subBuffer, long *subs, long maxSubs){
    uint16_t *code = &codeBuffer[pc];
    switch(*code & 0xF000){
        case 0x9000:
            if(*code & 0x0E0E == 0x040E){
                uint32_t cons = (((uint32_t)(((*code & 0x01F0) >> 3) | (*code & 0x0001))) << 16) | (uint32_t)code[1];
                subBuffer[*subs] = cons;
                *subs += 1;
                return 2;
            } else if(*code & 0x0E0E == 0x040C){
                return 2;
            }
        case 0xD000:{
                int16_t offset = (int16_t)(*code & 0x0FFF);
                if(offset & 0x0800) offset |= 0xF000;
                subBuffer[*subs] = pc + offset;
                *subs += 1;
            }
    }
    return 1;
}

int disassembleAVROp(uint16_t *codeBuffer, uint32_t pc, uint32_t *subBuffer, long subs){
    uint16_t *code = &codeBuffer[pc];
    int opWords = 1;
    printf("%06x ", pc);
    int subFound = 0;
    for(long i = 0; i < subs; i++){
        if(pc == subBuffer[i]){
            printf("sub%04lX: ", i);
            subFound = 1;
            break;
        }
    }
    if(!subFound)
        printf("         ");
    //Basic instructions
    if((*code & 0xFC00) == 0x0000){
        switch(*code & 0x0300){
            case 0x0000: printf("NOP"); break;
            case 0x0100:{
                int Rd = ((*code & 0x00F0) >> 4) * 2;
                int Rr = (*code & 0x000F) * 2;
                printf("MOVW   R%d:R%d, R%d:R%d", Rd + 1, Rd, Rr + 1, Rr);
                break;
            }
            case 0x0200:{
                int Rd = ((*code & 0x00F0) >> 4) + 16;
                int Rr = (*code & 0x000F) + 16;
                printf("MULS   R%d, R%d", Rd, Rr);
                break;
            }
            case 0x0300:{
                int Rd = ((*code & 0x0070) >> 4) + 16;
                int Rr = (*code & 0x0007) + 16;
                switch(*code & 0x0088){
                    case 0x0000: printf("MULSU  R%d, R%d", Rd, Rr); break;
                    case 0x0008: printf("FMUL   R%d, R%d", Rd, Rr); break;
                    case 0x0080: printf("FMULS  R%d, R%d", Rd, Rr); break;
                    case 0x0088: printf("FMULSU R%d, R%d", Rd, Rr); break;
                }
                break;
            }
        }
    }
    //Two-operand instructions
    else if((*code & 0xC000) == 0x0000){
        int Rd = (*code & 0x01F0) >> 4;
        int Rr = (*code & 0x000F) + (*code & 0x0200) >> 5;
        switch(*code & 0x3C00){
            case 0x0000: fprintf(stderr, "Two operand\n"); unimplementedInstruction(&opWords, *code); break;
            case 0x0400: printf("CPC    R%d, R%d", Rd, Rr); break;
            case 0x1400: printf("CP     R%d, R%d", Rd, Rr); break;
            case 0x0800: printf("SBC    R%1$d, R%2$d\t; *R%1$d = *R%1$d - *R%2$d - C", Rd, Rr); break;
            case 0x1800: printf("SUB    R%1$d, R%2$d\t; *R%1$d = *R%1$d - *R%2$d", Rd, Rr); break;
            case 0x0C00:
                if(Rd == Rr)
                    printf("LSL    R%1$d\t\t; *R%1$d <<= 1", Rd);
                else
                    printf("ADD    R%1$d, R%2$d\t; *R%1$d = *R%1$d + *R%2$d", Rd, Rr);
                break;
            case 0x1C00:
                if(Rd == Rr)
                    printf("ROL    R%d", Rd);
                else
                    printf("ADC    R%1$d, R%2$d\t; *R%1$d = *R%1$d + *R%2$d + C", Rd, Rr);
                break;
            case 0x1000: printf("CPSE   R%d, R%d", Rd, Rr); break;
            case 0x2000: printf("AND    R%d, R%d", Rd, Rr); break;
            case 0x2400: printf("EOR    R%d, R%d", Rd, Rr); break;
            case 0x2800: printf("OR     R%d, R%d", Rd, Rr); break;
            case 0x2C00: printf("MOV    R%d, R%d", Rd, Rr); break;
            default:{
          //case 0x3X00:
                Rd = (Rd & 0x000F) + 16;
                int K = (*code & 0x0F00) >> 4 + (*code & 0x000F);
                printf("CPI    R%d, %d", Rd, K);
                break;
            }
        }
    }
    //Register-immediate instructions
    else if((*code & 0xC000) == 0x4000){
        int Rd = ((*code & 0x00F0) >> 4) + 16;
        int K = ((*code & 0x0F00) >> 4) + (*code & 0x000F);
        switch(*code & 0x3000){
            case 0x0000: printf("SBCI   R%d, %d", Rd, K); break;
            case 0x1000: printf("SUBI   R%d, %d", Rd, K); break;
            case 0x2000: printf("ORI    R%d, %d", Rd, K); break;
            case 0x3000: printf("ANDI   R%d, %d", Rd, K); break;
        }
    }
    //LDD/STD
    else if((*code & 0xD000) == 0x8000){
        int Rd = (*code & 0x01F0) >> 4;
        int K = ((*code & 0x2000) >> 8) + ((*code & 0x0C00) >> 7) + (*code & 0x0007);
        if(K != 0){
            switch(*code & 0x0208){
                case 0x0000: printf("LDD    R%d, Z + %d", Rd, K); break;
                case 0x0008: printf("LDD    R%d, Y + %d", Rd, K); break;
                case 0x0200: printf("STD    Z + %d, R%d", K, Rd); break;
                case 0x0208: printf("STD    Y + %d, R%d", K, Rd); break;
            }
        } else {
            switch(*code & 0x0208){
                case 0x0000: printf("LD     R%d, Z", Rd); break;
                case 0x0008: printf("LD     R%d, Y", Rd); break;
                case 0x0200: printf("ST     Z, R%d", Rd); break;
                case 0x0208: printf("ST     Y, R%d", Rd); break;
            }
        }
    }
    //Load-store operations
    else if((*code & 0xFC00) == 0x9000){
        int Rd = (*code & 0x01F0) >> 4;
        switch(*code & 0x020F){
            case 0x0000: printf("LDS    R%d, 0x%04x", Rd, code[1]); opWords = 2; break;
            case 0x0200: printf("STS    0x%04x, R%d", code[1], Rd); opWords = 2; break;
            case 0x0001: printf("LD     R%d, Z+", Rd); break;
            case 0x0009: printf("LD     R%d, Y+", Rd); break;
            case 0x0201: printf("ST     Z+, R%d", Rd); break;
            case 0x0209: printf("ST     Y+, R%d", Rd); break;
            case 0x0002: printf("LD     R%d, -Z", Rd); break;
            case 0x000A: printf("LD     R%d, -Y", Rd); break;
            case 0x0202: printf("ST     -Z, R%d", Rd); break;
            case 0x020A: printf("ST     -Y, R%d", Rd); break;
            case 0x0004: printf("LPM    R%d, Z", Rd); break;
            case 0x0006: printf("ELPM   R%d, Z", Rd); break;
            case 0x0005: printf("LPM    R%d, Z+", Rd); break;
            case 0x0007: printf("ELPM   R%d, Z+", Rd); break;
            case 0x0204: printf("XCH    Z, R%d", Rd); break;
            case 0x0205: printf("LAS    Z, R%d", Rd); break;
            case 0x0206: printf("LAC    Z, R%d", Rd); break;
            case 0x0207: printf("LAT    Z, R%d", Rd); break;
            case 0x000C: printf("LD     R%d, X", Rd); break;
            case 0x020C: printf("ST     X, R%d", Rd); break;
            case 0x000D: printf("LD     R%d, X+", Rd); break;
            case 0x020D: printf("ST     X+, R%d", Rd); break;
            case 0x000E: printf("LD     R%d, -X", Rd); break;
            case 0x020E: printf("ST     -X, R%d", Rd); break;
            case 0x000F: printf("POP    R%d", Rd); break;
            case 0x020F: printf("PUSH   R%d", Rd); break;
            default: fprintf(stderr, "Load/Store\n"); unimplementedInstruction(&opWords, *code); break;
        }
    }
    //Zero-operand instructions
    else if((*code & 0xFF0F) == 0x9508){
        switch(*code & 0x00F0){
            case 0x0000: printf("RET");  break;
            case 0x0010: printf("RETI"); break; //Return from interrupt - Sets I bit in SREG
            //0x0020 - 0x0070: reserved
            case 0x0080: printf("SLEEP"); break;
            case 0x0090: printf("BREAK"); break;
            case 0x00A0: printf("WDR"); break;
            //0x00B0: reserved
            case 0x00C0: printf("LPM"); break;
            case 0x00D0: printf("ELPM"); break;
            case 0x00E0: printf("SPM"); break;
            case 0x00F0: printf("SPM    Z+"); break;
            default: fprintf(stderr, "Zero operand\n"); unimplementedInstruction(&opWords, *code); break;
        }
    }
    //One-operand instructions
    else if((*code & 0xFE08) == 0x9400){
        int Rd = (*code & 0x01F0) >> 4;
        switch(*code & 0x0007){
            case 0x0000: printf("COM    R%d", Rd); break;
            case 0x0001: printf("NEG    R%d", Rd); break;
            case 0x0002: printf("SWAP   R%d", Rd); break;
            case 0x0003: printf("INC    R%d", Rd); break;
            case 0x0004: fprintf(stderr, "One operand\n"); unimplementedInstruction(&opWords, *code); break;
            case 0x0005: printf("ASR    R%d", Rd); break;
            case 0x0006: printf("LSR    R%d", Rd); break;
            case 0x0007: printf("ROR    R%d", Rd); break;
        }
    }
    //SE*/CL*: * in {C, Z, N, V, S, H, T, I}
    //Set/clear flags
    else if((*code & 0xFF0F) == 0x9408){
        switch(*code & 0x00F0){
            case 0x0000: printf("SEC"); break;
            case 0x0010: printf("SEZ"); break;
            case 0x0020: printf("SEN"); break;
            case 0x0030: printf("SEV"); break;
            case 0x0040: printf("SES"); break;
            case 0x0050: printf("SEH"); break;
            case 0x0060: printf("SET"); break;
            case 0x0070: printf("SEI"); break;
            case 0x0080: printf("CLC"); break;
            case 0x0090: printf("CLZ"); break;
            case 0x00A0: printf("CLN"); break;
            case 0x00B0: printf("CLV"); break;
            case 0x00C0: printf("CLS"); break;
            case 0x00D0: printf("CLH"); break;
            case 0x00E0: printf("CLT"); break;
            case 0x00F0: printf("CLI"); break;
        }
    }
    //EIJMP, IJMP, EICALL, ICALL, DEC, DES, JMP, CALL
    else if((*code & 0xFE08) == 0x9408){
        switch(*code & 0x0007){
            //SE*/CL* block catches 0x0000 case
            case 0x0001:
                switch(*code & 0x0110){
                    case 0x0000: printf("IJMP"); break;
                    case 0x0010: printf("EIJMP"); break;
                    case 0x0100: printf("ICALL"); break;
                    case 0x0110: printf("EICALL"); break;
                }
                break;
            case 0x0002:{
                int Rd = (*code & 0x01F0) >> 4;
                printf("DEC    R%d", Rd);
                break;
            }
            case 0x0003:{
                int K = (*code & 0x00F0) >> 4;
                printf("DES    0x%1x", K);
                break;
            }
            default:{
          //case 0b010x, 0b011x:
                //2 word (4 byte) instruction. 22 bit Parameter K stored as: 
                //0b0000000k kkkk000k kkkkkkkk kkkkkkkk
                uint32_t K = (((uint32_t)((*code & 0x01F0) >> 3) | (*code & 0x0001)) << 16) | ((uint32_t)code[1]);
                switch(*code & 0x0002){
                    case 0x0000: printf("JMP    0x%06x", K); break;
                    case 0x0002: printf("CALL   0x%06x", K); break;
                }
                opWords = 2;
                break;
            }
        }
    }
    //ADIW/SBIW
    else if((*code & 0xFE00) == 0x9600){
        int k = ((*code & 0x00C0) >> 2) | (*code & 0x000F);
        int Rp = ((*code & 0x0030) >> 4) * 2 + 24; //Rp in (24, 26, 28, 30}
        switch(*code & 0x0100){
            case 0x0000: printf("ADIW   R%d, %d", Rp, k); break;
            case 0x0100: printf("SBIW   R%d, %d", Rp, k); break;
        }
    }
    //CBI, SBI, SBIC, SBIS
    //Set and check bits in IO registers 0-31 (memory addresses 0x20 - 0x3F)
    else if((*code & 0xFC00) == 0x9800){
        int Ra = (*code & 0x00F8) >> 3;
        int bit = (*code & 0x0007);
        switch(*code & 0x0300){
            case 0x0000: printf("CBI    %d, %d", Ra, bit); break;
            case 0x0200: printf("SBI    %d, %d", Ra, bit); break;
            case 0x0100: printf("SBIC   %d, %d", Ra, bit); break;
            case 0x0300: printf("SBIS   %d, %d", Ra, bit); break;
        }
    }
    //MUL (Unsigned)
    //Multiplies the values in Rd and Rr and stores the 16 bit result in R1:R0
    else if((*code & 0xFC00) == 0x9C00){
        int Rd = (*code & 0x01F0) >> 4;
        int Rr = ((*code & 0x0200) >> 5) | (*code & 0x000F);
        printf("MUL    R%d, R%d", Rd, Rr);
    }
    //IN/OUT to I/O space
    //Read/Write to the 64 IO registers (memory addresses 0x20 - 0x5F)
    else if((*code & 0xF000) == 0xB000){
        int Rio = ((*code & 0x0600) >> 5) | (*code & 0x000F);
        int Rd = (*code & 0x01F0) >> 4;
        switch(*code & 0x0800){
            case 0x0000: printf("IN     R%d, %d", Rd, Rio); break;
            case 0x0800: printf("OUT    %d, R%d", Rio, Rd); break;
        }
    }
    //RJMP/RCALL to PC + signed 12 bit immediate value
    else if((*code & 0xE000) == 0xC000){
        int16_t offset = *code & 0x0FFF;
        //Offset is signed, sign extension required
        if(offset & 0x0800)
            offset |= 0xF000;
        switch(*code & 0x1000){
            case 0x0000: printf("RJMP   %d", offset); break;
            case 0x1000: printf("RCALL  %d", offset); break;
        }
    }
    //LDI
    //Load immediate into registers 16 to 31
    else if((*code & 0xF000) == 0xE000){
        int K = ((*code & 0x0F00) >> 4) | (*code & 0x000F);
        int Rd = ((*code & 0x00F0) >> 4) + 16; //16 <= Rd <= 31
        printf("LDI    R%d, %d", Rd, K);
    }
    //SBRC/SBRS, BLD/BST
    else if((*code & 0xF808) == 0xF800){
        int Rd = (*code & 0x01F0) >> 4;
        int bit = (*code & 0x0007);
        switch(*code & 0x0600){
            case 0x0000: printf("BLD    R%d, %d", Rd, bit); break;
            case 0x0200: printf("BST    R%d, %d", Rd, bit); break;
            case 0x0400: printf("SBRC   R%d, %d", Rd, bit); break;
            case 0x0600: printf("SBRS   R%d, %d", Rd, bit); break;
        }
    }
    //Conditional branches
    //Flags: {C, Z, N, V, S, H, T, I}
    else if((*code & 0xF800) == 0xF000){
        int8_t offset = (int8_t)((*code & 0x03F8) >> 3);
        if(offset & 0x40)
            offset |= 0x80;
        switch(*code & 0x0407){
            case 0x0000: printf("BRCS   %d", offset); break;
            case 0x0400: printf("BRCC   %d", offset); break;
            case 0x0001: printf("BREQ   %d", offset); break;
            case 0x0401: printf("BRNE   %d", offset); break;
            case 0x0002: printf("BRMI   %d", offset); break;
            case 0x0402: printf("BRPL   %d", offset); break;
            case 0x0003: printf("BRVS   %d", offset); break;
            case 0x0403: printf("BRVC   %d", offset); break;
            case 0x0004: printf("BRLT   %d", offset); break;
            case 0x0404: printf("BRGE   %d", offset); break;
            case 0x0005: printf("BRHS   %d", offset); break;
            case 0x0405: printf("BRHC   %d", offset); break;
            case 0x0006: printf("BRTS   %d", offset); break;
            case 0x0406: printf("BRTC   %d", offset); break;
            case 0x0007: printf("BRIE   %d", offset); break;
            case 0x0407: printf("BRID   %d", offset); break;
        }
    }
    //Reserved opcodes
    else{
        fprintf(stderr, "else\n");
        unimplementedInstruction(&opWords, *code);
    }
    printf("\n");
    return opWords;
}

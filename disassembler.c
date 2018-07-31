#include <stdio.h>
#include <stdint.h>

int disassembleAVROp(uint16_t *codeBuffer, int pc){
    uint16_t *code = &codeBuffer[pc];
    int opWords = 1;
    printf("%04x ", pc);
    //Basic instructions
    if((*code & 0xFC00) == 0x0000){
        switch(*code & 0x0300){
            case 0x0000: printf("NOP"); break;
            case 0x0100:
                int Rd = ((*code & 0x00F0) >> 4) * 2;
                int Rr = (*code & 0x000F) * 2;
                printf("MOVW   R%d:R%d, R%d:R%d", Rd + 1, Rd, Rr + 1, Rr);
                break;
            case 0x0200:
                int Rd = ((*code & 0x00F0) >> 4) + 16;
                int Rr = (*code & 0x000F) + 16;
                printf("MULS   R%d, R%d", Rd, Rr);
                break;
            case 0x0300:
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
    //Two-operand instructions
    else if((*code & 0xC000) == 0x0000){
        int Rd = (*code & 0x01F0) >> 4;
        int Rr = (*code & 0x000F) + (*code & 0x0200) >> 5;
        switch(*code & 0x3C00){
            case 0x0000: printf("ERROR: UNKNOWN OPCODE");   break;
            case 0x0400: printf("CPC    R%d, R%d", Rd, Rr); break;
            case 0x1400: printf("CP     R%d, R%d", Rd, Rr); break;
            case 0x0800: printf("SBC    R%d, R%d", Rd, Rr); break;
            case 0x1800: printf("SUB    R%d, R%d", Rd, Rr); break;
            case 0x0C00:
                if(Rd == Rr)
                    printf("LSL    R%d", Rd);
                else
                    printf("ADD    R%d, R%d", Rd, Rr);
                break;
            case 0x1C00:
                if(Rd == Rr)
                    printf("ROL    R%d", Rd);
                else
                    printf("ADC    R%d, R%d", Rd, Rr);
                break;
            case 0x1000: printf("CPSE   R%d, R%d", Rd, Rr); break;
            case 0x2000: printf("AND    R%d, R%d", Rd, Rr); break;
            case 0x2400: printf("EOR    R%d, R%d", Rd, Rr); break;
            case 0x2800: printf("OR     R%d, R%d", Rd, Rr); break;
            case 0x2C00: printf("MOV    R%d, R%d", Rd, Rr); break;
            default:
          //case 0x3X00:
                Rd = (Rd & 0x000F) + 16;
                int K = (*code & 0x0F00) >> 4 + (*code & 0x000F);
                printf("CPI    R%d, %d", Rd, K);
                break;
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
            case 0x0201: printf("ST     R%d, Z+", Rd); break;
            //TODO: Rest of stores and loads
        }
    }
    //Zero-operand instructions
    else if((*code & 0xFF0F) == 0x9508){

    }
    //One-operand instructions
    else if((*code & 0xFE08) == 0x9400){

    }
    //EIJMP, IJMP, EICALL, ICALL, DEC, DES, JMP, CALL
    else if((*code & 0xFE08) == 0x9408){

    }
    //ADIW/SBIW
    else if((*code & 0xFE00) == 0x9600){

    }
    //CBI, SBI, SBIC, SBIS
    else if((*code & 0xFC00) == 0x9800){

    }
    //MUL
    else if((*code & 0xFC00) == 0x9C00){
        
    }
    //IN/OUT to I/O space
    else if((*code & 0xF000) == 0xB000){

    }
    //RJMP/RCALL to PC + simm12
    else if((*code & 0xE000) == 0xC000){

    }
    //LDI
    else if((*code & 0xF000) == 0xE000){
        
    }
    //SBRC/SBRS
    else if((*code & 0xFC08) == 0xFC00){

    }
    //BLD/BST
    else if((*code & 0xFC08) == 0xF800){

    }
    //Conditional branches
    else if((*code & 0xF800) == 0xF000){

    }
    //Reserved opcodes
    else{

    }
    return opWords;
}

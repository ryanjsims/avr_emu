#include "avr-utils.h"

char **buildIORegMap(FILE *ioMapFile, int *mapLen){
    *mapLen = 64;
    char **toReturn = malloc(*mapLen * sizeof(char*));
    int i = 0;

    while(ioMapFile != NULL && feof(ioMapFile) != EOF){
        char *readStr = malloc(16 * sizeof(char));
        int n = 16;
        fgets(readStr, n, ioMapFile);
        char *toFree = readStr;
        char *newline = strchr(readStr, '\n');
        if(newline != NULL){
            *newline = '\0';
        }
        readStr = strchr(readStr, ' ');
        if(readStr != NULL){
            readStr = readStr + 1;
            if(strcmp(readStr, "Reserved") != 0){
                toReturn[i] = malloc((strlen(readStr) + 1) * sizeof(char));
                strcpy(toReturn[i], readStr);
            } else {
                toReturn[i] = malloc((strlen(readStr) + strlen(" (0x00)") + 1) * sizeof(char));
                sprintf(toReturn[i], "%s (0x%02X)", readStr, i + 0x20);
            }
            i++;
        }
        free(toFree);
        if(readStr == NULL)
            break;
    }
    if(ioMapFile == NULL){
        for(i = 0; i < *mapLen; i++){
            toReturn[i] = malloc(3 * sizeof(char));
            sprintf(toReturn[i], "%d", i);
        }
    }
    if(i > *mapLen)
        *mapLen = i;
    return toReturn;
}

int buildSubroutineTable(uint16_t *codeBuffer, uint32_t pc, uint32_t *subBuffer, long *subs, long maxSubs){
    uint16_t *code = &codeBuffer[pc];
    switch(*code & 0xF000){
        case 0x9000:
            if((*code & 0x0E0E) == 0x040E){
                uint32_t cons = (((uint32_t)(((*code & 0x01F0) >> 3) | (*code & 0x0001))) << 16) | (uint32_t)code[1];
                subBuffer[*subs] = cons;
                *subs += 1;
                fprintf(stderr, "Found CALL instruction at addr 0x%06X\n", pc);
                return 2;
            } else if((*code & 0x0E0E) == 0x040C){
                fprintf(stderr, "Found JMP instruction at addr 0x%06X\n", pc);
                return 2;
            }
            break;
        case 0xD000:{
                int16_t offset = (int16_t)(*code & 0x0FFF);
                if(offset & 0x0800) offset |= 0xF000;
                subBuffer[*subs] = pc + offset + 1;
                *subs += 1;
                fprintf(stderr, "Found RCALL instruction at addr 0x%06X\n", pc);
                break;
            }
        case 0xF000:{
                if(*code & 0x0800) break;
                int8_t offset = (int8_t)((*code & 0x03F8) >> 3);
                if(offset & 0x40)
                    offset |= 0x80; //Sign extend offset
                subBuffer[*subs] = (pc + offset + 1) | 0x80000000;
                *subs += 1;
                fprintf(stderr, "Found BRXX instruction at addr 0x%06X\n", pc);
                break;
            }
        default:
                break;
    }
    return 1;
}

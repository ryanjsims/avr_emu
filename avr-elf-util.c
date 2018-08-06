#include "avr-elf-util.h"

char *getStrings(FILE* elfFile, Elf32_Ehdr ehdr, Elf32_Shdr *shdrs, int *strings);
void readBinaries(FILE* elfFile, uint16_t *buffer, Elf32_Shdr **shdrs, 
        int shdrLen, uint16_t ***binPtrs, int binPtrsLen, char *strTab);
        //Elf32_Shdr *sText, Elf32_Shdr *sData, Elf32_Shdr *sBss,
        //uint16_t **text, uint16_t **data, uint16_t **bss);

uint16_t *getBinary(FILE* elfFile, uint16_t **text, uint16_t **data, uint16_t **bss, uint16_t **rodata, 
        uint32_t *wordLen){
    Elf32_Ehdr elfHeader;
    readElfHeader(elfFile, &elfHeader);
    Elf32_Phdr *pgrmHeaders = malloc(sizeof(Elf32_Phdr) * elfHeader.e_phnum);
    Elf32_Shdr *sectHeaders = malloc(sizeof(Elf32_Shdr) * elfHeader.e_shnum);

    readPgrmHeaders(elfFile, elfHeader, pgrmHeaders);
    readSectHeaders(elfFile, elfHeader, sectHeaders);

    int strings = 0;
    char *stringTable = getStrings(elfFile, elfHeader, sectHeaders, &strings);

    Elf32_Shdr *sText = NULL, *sData = NULL, *sBss = NULL, *sROData = NULL;
    for(int i = 0; i < elfHeader.e_shnum; i++){
        uint32_t nameIdx = sectHeaders[i].sh_name;
        char *name = &stringTable[nameIdx];
        if(strcmp(name, ".text") == 0){
            sText = &sectHeaders[i];
        } else if(strcmp(name, ".data") == 0){
            sData = &sectHeaders[i];
        } else if(strcmp(name, ".bss") == 0){
            sBss = &sectHeaders[i];
        } else if(strcmp(name, ".rodata") == 0){
            sROData = &sectHeaders[i];
        }
    }

    uint32_t binSize = 0;
    if(sText == NULL){
        fprintf(stderr, "ERROR: Could not find .text code section!\n");
        exit(1);
    }
    binSize += sText->sh_size;
    if(sData != NULL){
        binSize += sData->sh_size;
    }
    if(sBss != NULL){
        binSize += sBss->sh_size;
    }
    if(sROData != NULL){
        binSize += sROData->sh_size;
    }
    uint32_t wordSize = binSize / 2 + binSize % 2;
    uint16_t *codeBuffer = malloc(wordSize * sizeof(uint16_t));
    *wordLen = wordSize;
    Elf32_Shdr *sHdrs[4] = {
        sText,
        sData,
        sBss,
        sROData
    };
    uint16_t **binPtrs[4] = {
        text,
        data,
        bss,
        rodata
    };
    readBinaries(elfFile, codeBuffer, sHdrs, 4, binPtrs, 4, stringTable);
    return codeBuffer;
}

void readBinaries(FILE* elfFile, uint16_t *buffer, Elf32_Shdr **shdrs,
        int shdrLen, uint16_t ***binPtrs, int binPtrsLen, char *stringTable){
        //Elf32_Shdr *sText, Elf32_Shdr *sData, Elf32_Shdr *sBss,
        //uint16_t **text, uint16_t **data, uint16_t **bss){
    uint32_t bufferOffset = 0;
    long offset = ftell(elfFile);
    for(int i = 0; i < shdrLen; i++){
        if(shdrs[i] == NULL)
            continue;
        char *name = &stringTable[shdrs[i]->sh_name];
        *(binPtrs[i]) = &buffer[bufferOffset];
        if(strcmp(name, ".bss") == 0){
            memset(*(binPtrs[i]), 0, shdrs[i]->sh_size);
            bufferOffset += shdrs[i]->sh_size / sizeof(uint16_t) + shdrs[i]->sh_size % sizeof(uint16_t);
        } else {
            fseek(elfFile, (long)shdrs[i]->sh_offset, SEEK_SET);
            bufferOffset += fread(&buffer[bufferOffset], sizeof(uint16_t), 
                shdrs[i]->sh_size / sizeof(uint16_t) 
                + shdrs[i]->sh_size % sizeof(uint16_t), elfFile);
    
        }
    }
    fseek(elfFile, offset, SEEK_SET);
}

char *getStrings(FILE* elfFile, Elf32_Ehdr ehdr, Elf32_Shdr *shdrs, int *strings){
    Elf32_Shdr strHeader = shdrs[ehdr.e_shstrndx];
    char *rawStrings = malloc(strHeader.sh_size * sizeof(char));

    long offset = ftell(elfFile);
    if(offset != (long)strHeader.sh_offset)
        fseek(elfFile, (long)strHeader.sh_offset, SEEK_SET);
    fread(rawStrings, sizeof(char), strHeader.sh_size, elfFile);
    if(offset != (long)strHeader.sh_offset)
        fseek(elfFile, offset, SEEK_SET);

    /*char *currNull = rawStrings;
    *strings = 0;
    while(currNull - rawStrings < strHeader.sh_size){
        currNull = strchr(currNull + 1, '\0');
        (*strings)++;
    }

    char **toReturn = malloc(sizeof(char*) * (*strings));
    currNull = strchr(rawStrings, '\0');
    char *strStart = rawStrings;
    int i = 0;
    while(i < *strings){
        toReturn[i] = malloc(sizeof(char) * (strlen(strStart) + 1));
        strcpy(toReturn[i], strStart);
        strStart = currNull + 1;
        currNull = strchr(strStart, '\0');
        i++;
    }
    free(rawStrings);*/
    return rawStrings;
}

void readElfHeader(FILE* elfFile, Elf32_Ehdr *hdr){
    long offset = ftell(elfFile);
    if(offset != 0l)
        fseek(elfFile, 0, SEEK_SET);
    fread(hdr, sizeof(Elf32_Ehdr), 1, elfFile);
    if(offset != 0l)
        fseek(elfFile, offset, SEEK_SET);
}

void readPgrmHeaders(FILE* elfFile, Elf32_Ehdr ehdr, Elf32_Phdr *phdr){
    long offset = ftell(elfFile);
    if(offset != ehdr.e_phoff)
        fseek(elfFile, (long)ehdr.e_phoff, SEEK_SET);
    fread(phdr, sizeof(Elf32_Phdr), ehdr.e_phnum, elfFile);
    if(offset != ehdr.e_phoff)
        fseek(elfFile, offset, SEEK_SET);
}

void readSectHeaders(FILE* elfFile, Elf32_Ehdr ehdr, Elf32_Shdr *shdr){
    long offset = ftell(elfFile);
    if(offset != ehdr.e_shoff)
        fseek(elfFile, (long)ehdr.e_shoff, SEEK_SET);
    fread(shdr, sizeof(Elf32_Shdr), ehdr.e_shnum, elfFile);
    if(offset != ehdr.e_shoff)
        fseek(elfFile, offset, SEEK_SET);
}

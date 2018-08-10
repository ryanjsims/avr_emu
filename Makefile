avr_dis: disassembler.* avr-elf-util.* avr-utils.*
	gcc -std=c99 -Wall disassembler.c avr-elf-util.c avr-utils.c -o avr_dis

avr_dis_dbg: disassembler.* avr-elf-util.* avr-utils.*
	gcc -std=c99 -g -Wall disassembler.c avr-elf-util.c avr-utils.c -o avr_dis

avr_emu: emulator.* avr-elf-util.* avr-utils.*
	gcc -std=c99 -Wall emulator.c avr-elf-util.c avr-utils.c -o avr_emu

avr_emu_dbg: emulator.* avr-elf-util.* avr-utils.*
	gcc -std=c99 -g -Wall emulator.c avr-elf-util.c avr-utils.c -o avr_emu

clean:
	rm -f avr_dis avr_emu

avr_dis:
	gcc -std=c99 -Wall disassembler.c avr-elf-util.c -o avr_dis

avr_dis_dbg:
	gcc -std=c99 -g -Wall disassembler.c avr-elf-util.c -o avr_dis

clean:
	rm avr_dis

avr_dis:
	gcc -std=c99 disassembler.c avr-elf-util.c -o avr_dis

avr_dis_dbg:
	gcc -std=c99 -g disassembler.c avr-elf-util.c -o avr_dis

clean:
	rm avr_dis

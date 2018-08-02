avr_dis:
	gcc -std=c99 disassembler.c -o avr_dis

clean:
	rm avr_dis

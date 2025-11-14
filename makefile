all:
	gcc -o vcd2svg -Wall *.c

debug:
	gcc -o vcd2svg -Wall -g *.c
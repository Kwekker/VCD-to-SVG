FILES=simple.v
TESTBENCH=simple_tb.v

simulate: signals.vcd

signals.vcd: sim.vvp
	vvp sim.vvp

sim.vvp: $(TESTBENCH) $(FILES)
	iverilog -o sim.vvp $(TESTBENCH)


syntax: $(FILES)
	yosys $(FILES) -p ''

wave:
	gtkwave signals.vcd display.gtkw
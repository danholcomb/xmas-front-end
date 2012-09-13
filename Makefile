


TARGET = dump

SRC = 	main.cpp \
	main.h

.PHONY: all clean sim veriabc doc



# use *.v instead of *.aig for BMC because we can simulate the counterexample
bmc: $(TARGET).v
	abc-bmc.sh $(TARGET).v

pdr: $(TARGET).aig
	abc-pdr.sh $(TARGET).aig

kind: $(TARGET).aig
	abc-kind.sh $(TARGET).aig

$(TARGET).aig: $(TARGET).v
	v2aig.sh $(TARGET).v

$(TARGET).out: $(SRC)
	./colorgcc.pl main.cpp -O0 -g3 -x -fexceptions -Wall -o $(TARGET).out
#	g++ main.cpp -O0 -g3 -x -fexceptions -Wall -o $(TARGET).out

$(TARGET).vcd: $(SRC) $(TARGET).v tb_rand.v
	iverilog -Wall -Winfloop -o dump.iv -D SIMULATION_ONLY dump.v tb_rand.v
	vvp dump.iv 

$(TARGET).v: $(TARGET).out
	./$(TARGET).out

run: $(TARGET).out  
	./$(TARGET).out

all: run dump.vcd doc

doc: $(SRC) Doxyfile
	doxygen Doxyfile

clean:
	rm -f *.out



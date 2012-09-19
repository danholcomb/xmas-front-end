


TARGET = dump

SRC = 	main.cpp \
	main.h \
        expressions.h \
        expressions.cpp \
	primitives.cpp \
	primitives.h \
	channel.cpp \
	channel.h

.PHONY: all clean sim doc


# use *.v instead of *.aig for BMC because that simulates cex
bmc: $(TARGET).v
	abc-bmc.sh $(TARGET).v

pdr: $(TARGET).aig
	abc-pdr.sh $(TARGET).aig

kind: $(TARGET).aig
	abc-kind.sh $(TARGET).aig

$(TARGET).aig: $(TARGET).v
	v2aig.sh $(TARGET).v

$(TARGET).out: $(SRC)
	clang++ main.cpp expressions.cpp primitives.cpp channel.cpp -ferror-limit=10 -Wall -o $(TARGET).out

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
	rm -R dump.out.dSYM
	rm -f *.out dump.* verific.script



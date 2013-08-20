


TARGET = dump

SRC = 	main.cpp \
	main.h \
        expressions.h \
        expressions.cpp \
	primitives.cpp \
	primitives.h \
	channel.cpp \
	channel.h \
	network.cpp \
	network.h

GPP = g++
GARGS =  -Wall -Wno-unused-variable 

# GPP = clang++
# GARGS = -ferror-limit=50 -Wall -Wno-unused-variable


.PHONY: all clean sim doc

$(TARGET).out: $(SRC)
	$(GPP) main.cpp expressions.cpp primitives.cpp channel.cpp network.cpp $(GARGS) -g -o $(TARGET).out

# $(TARGET).vcd: $(SRC) $(TARGET).v tb_rand.v
# 	iverilog -Wall -Winfloop -o dump.iv -D SIMULATION_ONLY dump.v tb_rand.v
# 	vvp dump.iv 


#VOPTIONS = --network ex_queue_chain1 --t_max 13 --enable_phil --enable_psi
#VOPTIONS = --network ex_queue_chain1 --t_max 13 --enable_tag all
#VOPTIONS = --network ex_queue_chain1 --t_max 13 --enable_tag top__ch0__inv1 --enable_tag top__ch0__inv2
#VOPTIONS = --network ex_queue_chain1 --t_max 11	--enable_tag all
#VOPTIONS = --network ex_queue_chain2 --t_range 20 --t_bound 18 --enable_tag all
#VOPTIONS = --network ex_queue_chain1 --t_range 50 --enable_tag all

# $(TARGET).v: $(TARGET).out
# 	./$(TARGET).out $(VOPTIONS)

run: $(TARGET).out  
	./$(TARGET).out

all: run dump.vcd doc

doc: $(SRC) Doxyfile
	doxygen Doxyfile

clean:
	rm -Rf dump.out.dSYM
	rm -f *.out dump.* verific.script
	rm -f sweep*.eps sweep*.gp sweep*.dat sweep*.pdf dump.* verific.script
	rm -f exp_results.txt current_results.txt abc_runs.log
	rm -f foo.cex
	rm -f *.tcex.tex
	rm -rf log


#!/bin/bash


if [ $# -eq 1 ]
    then
    VFILE=$1
    TBFILE=tb_rand.v
elif [ $# -eq 2 ]
    then
    VFILE=$1
    TBFILE=$2
elif [ $# -eq 3 ]
    then
    VFILE=$1
    TBFILE=$2
    VARGS=$3
else
    echo "wrong input args s/b $0 file.v <tb.v> <args>"
    exit
fi




echo ; 
echo "=========================================================================="
echo "| "$0 $*
echo "| VFILE:     "$VFILE
echo "| TBFILE:    "$TBFILE
echo "| VARGS:     "$VARGS
echo "=========================================================================="
echo ;

rm -f dump.iv
#iverilog -o dump.iv -D DUT=$(TARGET) -D SIMULATION_ONLY $(TARGET).v tb.v  
#iverilog -o dump.iv -s tb -D DUT=$DUT  -D CYCLES=5 -D SIMULATION_ONLY $VFILE tb.v  
#iverilog -o dump.iv -D DUT=$DUT ${IVARGS// / -D } -D SIMULATION_ONLY  $VFILE tb.v -s tb
#iverilog -o dump.iv -D DUT=$DUT ${IVARGS// / -D } -D SIMULATION_ONLY $VFILE tb.v -s tb
#iverilog -o dump.iv ${VARGS// / -D } -D SIMULATION_ONLY $VFILE tb.v -s tb
# iverilog -o dump.iv ${VARGS// / -D } -D SIMULATION_ONLY $VFILE $TBFILE -s $TB
iverilog -o dump.iv ${VARGS// / -D } -D SIMULATION_ONLY $VFILE $TBFILE
# iverilog -v -o dump.iv ${VARGS// / -D } -D SIMULATION_ONLY $VFILE $TBFILE -s tb
#iverilog -o dump.iv ${IVARGS// / -D } -D SIMULATION_ONLY $VFILE tb.v
# iverilog -o dump.iv -s tb -D DUT=ex1 -D SIMULATION_ONLY ex1.v tb.v  
vvp dump.iv -v




# #exit;


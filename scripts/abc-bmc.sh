#!/bin/bash


if [ $# -eq 0 ]
    then
	FILE=dump.aig
	FRAMES=30
elif [ $# -eq 1 ]
    then
	FILE=$1
	FRAMES=30
elif [ $# -eq 2 ]
    then
    FILE=$1
    FRAMES=$2
else
    echo "wrong input args"
    exit
fi

echo ; echo ;
echo "========================================================="
echo $0
echo "frames:    "$FRAMES 
echo "file:      "$FILE
echo "========================================================="
echo ; echo ;

CONFLICTS=10000000
BMC="bmc3 -F $FRAMES -C $CONFLICTS -v;"

# if the input file is verilog, create/simulate a testbench from the cex to allow analysis
if [ "${FILE%.aig}.aig" == "$FILE" ]; then
    echo "found aig $FILE"
    AIG=$FILE
    abc -c "version; r $AIG; $BMC; write_counter -n foo.cex;" | tee dump.abc

elif [ "${FILE%.v}.v" == "$FILE" ]; then
    AIG=dump.aig;
    echo "found verilog $FILE, creating aig dump.aig";    
    v2aig.sh $FILE
    rm -f foo.cex
    abc -c "version; r $AIG; pio -f; $BMC; write_counter -n foo.cex;" | tee dump.abc
    cex2v.pl

    # simulate the cex if there is one
    if [ $? == 0 ]; then
	iverilog-sim.sh $FILE tb_cex.v
    else
	echo "no cex to simulate"
    fi

else 
    echo "found unknown format"
    exit
fi

exit;

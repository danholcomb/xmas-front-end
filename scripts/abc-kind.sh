#!/bin/bash

if [ $# -eq 0 ]
    then
    FILE=dump.aig
    FRAMES=40
elif [ $# -eq 1 ]
    then
	FILE=$1
	FRAMES=100
elif [ $# -eq 2 ]
    then
	FILE=$1
	FRAMES=$2
else
    echo "wrong input args"
    exit
fi

echo ; 
echo "===================================================================================="
echo "| "$0
echo "| file:      "$FILE
echo "| frames:    "$FRAMES 
echo "===================================================================================="
echo ;

if [ "${FILE%.aig}.aig" == "$FILE" ] 
    then
    echo "found aig $FILE"
    AIG=$FILE
elif [ "${FILE%.v}.v" == "$FILE" ]
    then
    AIG=dump.aig;
    echo "found verilog $FILE, creating aig dump.aig";    
    v2aig.sh $FILE
else 
    echo "found unknown format"
    exit
fi


CONFLICTS=10000000
#NODES=10000000

# using ind -vw makes the output ugly... but the ind output doesn't show up using just -v
# need the output to show up to learn number of frames when abc times out

#BMC="bmc3 -F $FRAMES -C $CONFLICTS -v;"
BMC="bmc3 -v -F $FRAMES -C $CONFLICTS;"
#IND="ind -vw -F $FRAMES -C $CONFLICTS;"
IND="ind -avw -F $FRAMES -C $CONFLICTS;"
abc -c "version; read_aiger $AIG;  ps; $BMC; orpos; $IND;" | tee -i dump.abc
dos2unix dump.abc

#abc -c "version; read_aiger $AIG;  ps; $BMC; orpos; ind -vw -F $FRAMES -C 100000000;" | tee -i dump.abc

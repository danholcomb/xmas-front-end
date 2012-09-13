#!/bin/bash

if [ $# -eq 1 ]
    then
	FILE=$1
else
    echo "wrong input args"
    exit
fi

echo ; 
echo "===================================================================================="
echo "| "$0
echo "| file:      "$FILE
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

#abc -c "version; read_aiger $AIG; pdr -v;" | tee -i dump.abc;
abc -c "version; read_aiger $AIG; pdr -vg;" | tee -i dump.abc;
dos2unix dump.abc



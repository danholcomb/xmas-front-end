#!/bin/bash

if [ $# -eq 1 ]; then
    VFILE=$1
else
    echo $0" has wrong input args: "$*
    exit
fi


if [ "${VFILE%.v}.v" != "$VFILE" ]; then
    echo "found unknown format"
    exit
fi

echo ; echo ;
echo "========================================================="
echo "| "$0
echo "| vfile:     "$VFILE
echo "========================================================="
echo ; echo ;


MIMA_DIR="/home/holcomb/temp_v2aig"
MIMA_TCLMAIN="/hd/common/jiang/veriabc-alpha-installation/bin/tclmain"

echo  "analyze -sysv $VFILE; elaborate; veriabc_analyze -zaiger; veriabc_set_reset .*reset -all -run -phase 1; veriabc_write -zaiger dump.aig" > verific.script

scp $VFILE verific.script holcomb@mima.eecs.berkeley.edu:$MIMA_DIR/

ssh holcomb@mima.eecs.berkeley.edu "cd $MIMA_DIR; rm dump.aig; $MIMA_TCLMAIN -s verific.script";

scp holcomb@mima.eecs.berkeley.edu:$MIMA_DIR/dump.aig .


echo "finishing v2aig.sh ";
echo "========================================================="
exit;


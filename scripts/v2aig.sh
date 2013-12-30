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


# UDIR is a unique working directory to ensure uniqueness on
# mima. Otherwise collisions occur if running multiple copies of
# script

RANDOM_DIR=$RANDOM$RANDOM$RANDOM
MIMA_DIR="/home/holcomb/temp_v2aig"
MIMA_UDIR=$MIMA_DIR/$RANDOM_DIR
MIMA_TCLMAIN="/hd/common/jiang/veriabc-alpha-installation/bin/tclmain"



# otherwise might not notice failure
rm dump.aig

ssh holcomb@mima.eecs.berkeley.edu "mkdir -p $MIMA_UDIR; cd $MIMA_UDIR; rm * ";

echo  "analyze -sysv $VFILE; elaborate; veriabc_analyze -zaiger; veriabc_set_reset .*reset -all -run -phase 1; veriabc_write -zaiger dump.aig" > verific.script

scp $VFILE verific.script holcomb@mima.eecs.berkeley.edu:$MIMA_UDIR/

ssh holcomb@mima.eecs.berkeley.edu "cd $MIMA_UDIR; rm dump.aig; $MIMA_TCLMAIN -s verific.script; rm $VFILE";

scp holcomb@mima.eecs.berkeley.edu:$MIMA_UDIR/dump.aig .

#clean up 
ssh holcomb@mima.eecs.berkeley.edu "rm -rf $MIMA_UDIR;"; 

echo "finishing v2aig.sh ";
echo "========================================================="
exit;


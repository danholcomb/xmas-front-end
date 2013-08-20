#!/bin/bash


# make clean
make dump.out
TIMEOUT=300;
FRAMEOUT=200;

# TIMEOUT=1800;
# FRAMEOUT=100;

T_STYLE1=" --t_style binary_fixed ";
T_STYLE2=" --t_style binary_incrementing ";
T_STYLE3=" --t_style unary_incrementing ";

ASSERTIONS1_SWEEP_DEPTH=" --enable_tag psi --enable_tag phig_tl "
ASSERTIONS2_SWEEP_DEPTH=" --enable_tag psi --enable_tag phig_tl  --enable_tag phil --enable_tag psil "

ASSERTIONS1_SWEEP_BOUND=" --enable_tag psi --enable_tag phig_x "
ASSERTIONS2_SWEEP_BOUND=" --enable_tag psi --enable_tag phig_x  --enable_tag phil --enable_tag psil "


# settings for the 8 experiments in the paper
BUILD_ARGS_CREDIT_LOOP_5CASES=" --network tb_credit_loop --queueDepth 6 --sinkBound 5 "
BUILD_ARGS_CREDIT_LOOP_SWEEP_BOUND=$BUILD_ARGS_CREDIT_LOOP_5CASES
BUILD_ARGS_VIRTUAL_CHANNEL_5CASES=" --network tb_virtual_channel --sinkBound1 3 --sinkBound2 6 --depth1 5 --depth2 5 "
BUILD_ARGS_VIRTUAL_CHANNEL_SWEEP_BOUND=$BUILD_ARGS_VIRTUAL_CHANNEL_5CASES
BUILD_ARGS_TOKEN_BUCKET_SWEEP_BOUND=" --network tb_token_bucket --queueDepth 5 --sinkBound 4 --sigma 2 --rho 10 "
BUILD_ARGS_QUEUE_SWEEP_DEPTH=" --network tb_queue --sinkBound 3 "
BUILD_ARGS_QUEUE_CHAIN_SWEEP_STAGES=" --network tb_queue_chain --queueDepth 2 --sinkBound 1 "
BUILD_ARGS_CREDIT_LOOP_SWEEP_DEPTH=" --network tb_credit_loop --sinkBound 5 " 
BUILD_ARGS_QUEUE_SWEEP_TIME_DOMAIN=" --network tb_queue --queueDepth 6 --sinkBound 5 "
    


# # toy settings for smaller experiments to test things
# BUILD_ARGS_CREDIT_LOOP_5CASES=" --network tb_credit_loop --queueDepth 3 --sinkBound 3 "
# BUILD_ARGS_CREDIT_LOOP_SWEEP_BOUND=$BUILD_ARGS_CREDIT_LOOP_5CASES
# BUILD_ARGS_VIRTUAL_CHANNEL_5CASES=" --network tb_virtual_channel --sinkBound1 2 --sinkBound2 4 --depth1 3 --depth2 3 "
# BUILD_ARGS_VIRTUAL_CHANNEL_SWEEP_BOUND=$BUILD_ARGS_VIRTUAL_CHANNEL_5CASES
# BUILD_ARGS_TOKEN_BUCKET_SWEEP_BOUND=" --network tb_token_bucket --queueDepth 4 --sinkBound 4 --sigma 2 --rho 10 "
# BUILD_ARGS_QUEUE_SWEEP_DEPTH=" --network tb_queue --sinkBound 2 "
# BUILD_ARGS_QUEUE_CHAIN_SWEEP_STAGES=" --network tb_queue_chain --queueDepth 2 --sinkBound 1 "
# BUILD_ARGS_CREDIT_LOOP_SWEEP_DEPTH=" --network tb_credit_loop --sinkBound 3 " 
# BUILD_ARGS_QUEUE_SWEEP_TIME_DOMAIN=" --network tb_queue --queueDepth 4 --sinkBound 5 "
# TIMEOUT=60;
# FRAMEOUT=80;


RUN_RING3=0;
RUN_RING8=0;
RUN_QUEUE_SWEEP_TIME_DOMAIN=0;

RUN_CREDIT_LOOP_SWEEP_BOUND=0;
RUN_CREDIT_LOOP_SWEEP_DEPTH=0;
RUN_CREDIT_LOOP_5CASES=0; 
RUN_VIRTUAL_CHANNEL_5CASES=0; 
RUN_VIRTUAL_CHANNEL_SWEEP_BOUND=0; 
RUN_TOKEN_BUCKET_SWEEP_BOUND=0; 
RUN_QUEUE_SWEEP_DEPTH=0; 
RUN_QUEUE_CHAIN_SWEEP_STAGES=0;


# choose an experiment to run from command line
if [ $# -eq 1 ]; then
    echo $1;
    if   [ "$1" = "credit_loop_sweep_bound" ];      then RUN_CREDIT_LOOP_SWEEP_BOUND=1;
    elif [ "$1" = "credit_loop_sweep_depth" ];      then RUN_CREDIT_LOOP_SWEEP_DEPTH=1;
    elif [ "$1" = "credit_loop_5cases" ];           then RUN_CREDIT_LOOP_5CASES=1; 
    elif [ "$1" = "virtual_channel_5cases" ];       then RUN_VIRTUAL_CHANNEL_5CASES=1; 
    elif [ "$1" = "virtual_channel_sweep_bound" ];  then RUN_VIRTUAL_CHANNEL_SWEEP_BOUND=1; 
    elif [ "$1" = "token_bucket_sweep_bound" ];     then RUN_TOKEN_BUCKET_SWEEP_BOUND=1; 
    elif [ "$1" = "queue_sweep_depth" ];            then RUN_QUEUE_SWEEP_DEPTH=1; 
    elif [ "$1" = "queue_chain_sweep_stages" ];     then RUN_QUEUE_CHAIN_SWEEP_STAGES=1;
    else echo "bad input: $1"; exit; fi
fi







if [ $RUN_RING3 -eq "1" ]; then
    BUILD_ARGS=" --network tb_ring --queueDepth 2 --sinkBound 2 --numStages 3 "
    run_batch_5cases.pl \
	--exp_name "ring3" \
	--build_args " $BUILD_ARGS $T_STYLE2 " \
	--timeout $TIMEOUT \
	
fi


if [ $RUN_RING8 -eq "1" ]; then
    BUILD_ARGS=" --network tb_ring --queueDepth 2 --sinkBound 2 --numStages 8 "
    run_batch_5cases.pl \
	--exp_name "ring8" \
	--build_args " $BUILD_ARGS $T_STYLE2 " \
	--timeout $TIMEOUT \
	
fi


###################################################################
# experiments to compare the different encodings for packet ages. #
###################################################################
if [ $RUN_QUEUE_SWEEP_TIME_DOMAIN -eq "1" ]; then

    # run bmc to verify that the two time domain encodings are equivalent
    find_t_cex.pl \
	--exp_name "bmc_encoding1" \
	--t_min 3 --t_max 50 \
	--build_args " $BUILD_ARGS_QUEUE_SWEEP_TIME_DOMAIN --enable_tag phig_x $T_STYLE1 " \
	--timeout $TIMEOUT --max_frames $FRAMEOUT \

    find_t_cex.pl \
	--exp_name "bmc_encoding2" \
	--t_min 3 --t_max 50 \
	--build_args " $BUILD_ARGS_QUEUE_SWEEP_TIME_DOMAIN --enable_tag phig_x $T_STYLE2 " \
	--timeout $TIMEOUT --max_frames $FRAMEOUT \
	
    T_CEX=$?
    T_TIGHT=$(($T_CEX+1))
    T_RANGE=$(($T_TIGHT+1))
    
    echo $T_TIGHT
    echo $T_RANGE
    
    sweep_param_and_prove_tl.pl \
	--exp_name "queue_sweep_time_domain"  \
	--build_args " $BUILD_ARGS_QUEUE_SWEEP_TIME_DOMAIN --dont_build_lemmas "  \
	--timeout $TIMEOUT \
	--sweep_param "t_range" --p_min $T_RANGE --p_max 1000000 \
	--log --exponent 1.4 \
	--engine "kind" --assertions " --enable_tag phig_x --t_bound $T_TIGHT --enable_tag psi $T_STYLE1"  \
	--engine "kind" --assertions " --enable_tag phig_x --t_bound $T_TIGHT --enable_tag psi $T_STYLE2"  \
	--terminate_on_fail \

fi


######################################################
# sweep the size of the problem to check scalability #
######################################################


if [ $RUN_QUEUE_SWEEP_DEPTH -eq "1" ]; then
    sweep_param_and_prove_tl.pl \
	--exp_name "queue_sweep_depth" \
	--build_args " $BUILD_ARGS_QUEUE_SWEEP_DEPTH $T_STYLE2 " \
	--timeout $TIMEOUT \
	--sweep_param "queueDepth" --p_min 2 --p_max 10 \
	--engine "kind" --assertions "$ASSERTIONS1_SWEEP_DEPTH $T_STYLE2"  \
	--engine "kind" --assertions "$ASSERTIONS2_SWEEP_DEPTH $T_STYLE2"  \
	--bmc \
	--terminate_on_fail \
	
fi


if [ $RUN_QUEUE_CHAIN_SWEEP_STAGES -eq "1" ]; then
    sweep_param_and_prove_tl.pl \
	--exp_name "queue_chain_sweep_stages" \
	--build_args " $BUILD_ARGS_QUEUE_CHAIN_SWEEP_STAGES $T_STYLE2" \
	--timeout $TIMEOUT \
	--sweep_param "numStages" --p_min 2 --p_max 10 \
	--engine "kind" --assertions "$ASSERTIONS1_SWEEP_DEPTH $T_STYLE2"  \
	--engine "kind" --assertions "$ASSERTIONS2_SWEEP_DEPTH $T_STYLE2"  \
	--bmc \
	--terminate_on_fail \

fi


if [ $RUN_CREDIT_LOOP_SWEEP_DEPTH -eq "1" ]; then
    sweep_param_and_prove_tl.pl \
	--exp_name "credit_loop_sweep_depth" \
	--build_args " $BUILD_ARGS_CREDIT_LOOP_SWEEP_DEPTH $T_STYLE2 "  \
	--timeout $TIMEOUT \
	--sweep_param "queueDepth" --p_min 2 --p_max 10 \
	--engine "kind" --assertions "$ASSERTIONS1_SWEEP_DEPTH $T_STYLE2"  \
	--engine "kind" --assertions "$ASSERTIONS2_SWEEP_DEPTH $T_STYLE2"  \
	--bmc \
	--terminate_on_fail \

fi












########################################
# try different engines on the problem #
########################################

if [ $RUN_CREDIT_LOOP_5CASES -eq "1" ]; then
    run_batch_5cases.pl \
	--exp_name "credit_loop_5cases" \
	--build_args " $BUILD_ARGS_CREDIT_LOOP_5CASES $T_STYLE2 " \
	--timeout $TIMEOUT \
	
fi

if [ $RUN_VIRTUAL_CHANNEL_5CASES -eq "1" ]; then
    run_batch_5cases.pl \
	--exp_name "virtual_channel_5cases" \
	--build_args " $BUILD_ARGS_VIRTUAL_CHANNEL_5CASES $T_STYLE2 " \
	--timeout $TIMEOUT \
	
fi


######################################################
# sweep bounds using global latency and with lemmas  #
######################################################

if [ $RUN_CREDIT_LOOP_SWEEP_BOUND -eq "1" ]; then
    sweep_bound.pl \
	--exp_name "credit_loop_sweep_bound" \
	--build_args " $BUILD_ARGS_CREDIT_LOOP_SWEEP_BOUND $T_STYLE2 " \
	--timeout $TIMEOUT \
	--assertions "$ASSERTIONS1_SWEEP_BOUND" \
	--assertions "$ASSERTIONS2_SWEEP_BOUND" \

fi


if [ $RUN_VIRTUAL_CHANNEL_SWEEP_BOUND -eq "1" ]; then
    sweep_bound.pl \
	--exp_name "virtual_channel_sweep_bound" \
	--build_args " $BUILD_ARGS_VIRTUAL_CHANNEL_SWEEP_BOUND $T_STYLE2" \
	--timeout $TIMEOUT \
	--assertions "$ASSERTIONS1_SWEEP_BOUND" \
	--assertions "$ASSERTIONS2_SWEEP_BOUND" \
	
fi


if [ $RUN_TOKEN_BUCKET_SWEEP_BOUND -eq "1" ]; then
    sweep_bound.pl \
	--exp_name "token_bucket_sweep_bound" \
	--build_args " $BUILD_ARGS_TOKEN_BUCKET_SWEEP_BOUND $T_STYLE2" \
	--timeout $TIMEOUT \
	--assertions "$ASSERTIONS1_SWEEP_BOUND" \
	--assertions "$ASSERTIONS2_SWEEP_BOUND" \
	
fi

exit;









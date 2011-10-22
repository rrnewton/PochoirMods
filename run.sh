#!/bin/bash

STEP_BASE=200
STEP_BOUND=4000
STEP_STEP=200

SIZE_BASE=200
SIZE_BOUND=4000
SIZE_STEP=200

LOOP_PATH=.
OBASE_PATH=.

EXIT_STAT=0

if [ -e $LOOP_PATH/loop ] && [ -e $OBASE_PATH/obase ]
then   

    for STEP in {$STEP_BASE..$STEP_BOUND..$STEP_STEP} 
    do
	for SIZE in {$SIZE_BASE..$SIZE_BOUND..$SIZE_STEP}
	do
	    echo "$LOOP_PATH/loop $SIZE $STEP"
	    $LOOP_PATH/loop $SIZE $STEP
	    echo "$OBASE_PATH/obase $SIZE $STEP"
	    $OBASE_PATH/obase $SIZE $STEP
	done
    done

fi

if [ ! -e $LOOP_PATH/loop ]
then
    echo "$LOOP_PATH/loop doesn't exist"
    EXIT_STAT=1
fi

if [ ! -e $OBASE_PATH/obase ]
then
    echo "$OBASE_PATH/obase doesn't exist"
    EXIT_STAT=1
fi

exit $EXIT_STAT
    

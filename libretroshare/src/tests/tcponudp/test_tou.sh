#!/bin/sh 
# Script to Test the udp_server code.

EXEC=./test_tou
TMPOUTPUT=tmpoutput$$
EXPECTEDPERIOD=10
# launches one instance in server mode.
$EXEC 127.0.0.1 4001 127.0.0.1 4002 > $TMPOUTPUT &
#2> udp_server1.cerr &

# launch a second in connect mode.
$EXEC -c 127.0.0.1 4002 127.0.0.1 4001 < $EXEC
# 2> udp_server2.cerr
# pipe a bunch of data through.
# make sure the data is the same.

# print success / failure.
sleep $EXPECTEDPERIOD
killall $EXEC

if diff -s $EXEC $TMPOUTPUT
then
	echo "SUCCESS"
else
	echo "FAILURE to accurately transfer DATA"
fi

#rm udp_server1.cerr
#rm udp_server2.cerr
rm $TMPOUTPUT



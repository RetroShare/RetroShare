#!/bin/sh 
# Script to Test the udp_server code.

EXEC=./udpsock_test
TMPOUTPUT=tmpoutput$$
EXPECTEDPERIOD=10
$EXEC 6001 6002 6003 6004 6005 &
#2> udp_server1.cerr &

$EXEC 6002 6001 6003 6004 6005 &
$EXEC 6003 6002 6001 6004 6005 &
$EXEC 6004 6002 6003 6001 6005 &
$EXEC 6005 6002 6003 6004 6001 &

## print success / failure.
sleep $EXPECTEDPERIOD
killall $EXEC

#
#if diff -s $EXEC $TMPOUTPUT
#then
#	echo "SUCCESS"
#else
#	echo "FAILURE to accurately transfer DATA"
#fi
#
##rm udp_server1.cerr
##rm udp_server2.cerr
#rm $TMPOUTPUT
##
###

#!/bin/bash

#test jack_property client
#//tb/1902

#this test needs:
#	-a running jack server
#	-client programs
#		-jack_property
#		-jack_metro
#		-jack_lsp

#uninstall jack, setup jack1, make sure /dev/shm/ is clean, start jackd -ddummy in new terminal, run this script in new terminal
#	time ./jack_property_test.sh > /tmp/jack_property_test_jack1_out.txt 2>&1

#uninstall jack, setup jack2, make sure /dev/shm/ is clean, start jackd -ddummy in new terminal, run this script in new terminal
#	time ./jack_property_test.sh > /tmp/jack_property_test_jack2_out.txt 2>&1

#for stress test: while true; do ./jack_property_test.sh; sleep 1; done
#to inspect running script: start with bash -x ./jack_property_test.sh

set -o pipefail

#any failed test will set this to 1 (error)
FINAL_RETURN_VALUE=0

function expect()
{
	if [ "$1" = "$2" ];
	then
		echo "	OK: $2"
		return 0
	fi
	echo "**	FAILED: $1"
	echo "**	EXP EQ: $2"
	FINAL_RETURN_VALUE=1
	return 1
}

function expect_not()
{
	if [ "$1" != "$2" ];
	then
		echo "	OK: $2"
		return 0
	fi
	echo "**	FAILED: $1"
	echo "**	EXP NE: $2"
	FINAL_RETURN_VALUE=1
	return 1
}

function expect_ok_empty()
{
	expect "$1" 0
	expect "$2" ""
}

TESTPREFIX=""
function tell()
{
	echo "test ${TESTPREFIX}$1: $2"
}

#test using -c, --client
function client_test()
{
client="$1"

cmd="jack_property -D"
tell c1 "$cmd"
res="`$cmd 2>&1`"
	expect $? 0
	expect "$res" "JACK metadata successfully deleted"

cmd="jack_property -l"
tell c2 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -c -l $client"
tell c3 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -c -s $client client_key client_value"
tell c4 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -c -l $client"
tell c5 "$cmd"
res="`$cmd 2>&1`"
	expect $? 0
	expect "$res" "key: client_key value: client_value"

cmd="jack_property -c -l $client client_key"
tell c6 "$cmd"
res="`$cmd 2>&1`"
	expect $? 0
	expect "$res" "client_value"

cmd="jack_property -l" #     |tail -1"
tell c7 "$cmd"
res="`$cmd 2>&1     |tail -1`"
	#18446744073709551615
	#key: client_key value: client_value
	expect $? 0
	expect "$res" "key: client_key value: client_value"

cmd="jack_property -p -l ${client}:non"
tell c8 "$cmd"
res="`$cmd 2>&1`"
	expect_not $? 0
	expect "$res" "cannot find port name ${client}:non"
}

#test using -p, --port
function port_test()
{
port="$1"

cmd="jack_property -D"
tell p1 "$cmd"
res="`$cmd 2>&1`"
	expect $? 0
	expect "$res" "JACK metadata successfully deleted"

cmd="jack_property -l"
tell p2 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -p -l $port"
tell p3 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -p -s $port port_key port_value"
tell p4 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -p -l $port"
tell p5 "$cmd"
res="`$cmd 2>&1`"
	expect $? 0
	expect "$res" "key: port_key value: port_value"

cmd="jack_property -p -l $port port_key"
tell p6 "$cmd"
res="`$cmd 2>&1`"
	expect $? 0
	expect "$res" "port_value"

cmd="jack_property -p -d $port port_key"
tell p7 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -p -l $port port_key"
tell p8 "$cmd"
res="`$cmd 2>&1`"
	expect_not $? 0
	expect "$res" "Value not found for port_key of $port"

cmd="jack_property -p -d $port port_key"
tell p9 "$cmd"
res="`$cmd 2>&1       |tail -1`"
	#Cannot delete key port_key (BDB0073 DB_NOTFOUND: No matching key/data pair found)
	#"port_key" property not removed for system:playback_1
	expect_not $? 0
	expect "$res" "\"port_key\" property not removed for $port"

cmd="jack_property -p -l $port"
tell p10 "$cmd"
res="`$cmd 2>&1`"
	expect_ok_empty $? "$res"

cmd="jack_property -p -l $port non"
tell p11 "$cmd"
res="`$cmd 2>&1`"
	expect_not $? 0
	expect "$res" "Value not found for non of $port"

cmd="jack_property -c -l non"
tell p12 "$cmd"
res="`$cmd 2>&1`"
	expect_not $? 0
	expect "$res" "cannot get UUID for client named non"
}


TESTPREFIX="system_"
client_test system
port_test system:playback_1

#test with any jack client
jack_metro -b120 &
metro_pid=$!

sleep 0.1
jack_lsp|grep metro
#metro:120_bpm

TESTPREFIX="metro_"
client_test metro
port_test metro:120_bpm

jack_property -D
#JACK metadata successfully deleted
jack_property -l

kill -HUP $metro_pid
sleep 0.5
echo "done, exit status is $FINAL_RETURN_VALUE"

###TO DO:
#test short keys, values
#test long keys, values
#test many

exit $FINAL_RETURN_VALUE
#EOF

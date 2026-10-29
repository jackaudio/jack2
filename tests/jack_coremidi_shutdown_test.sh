#!/bin/bash

# Test that a jackd server loaded with the CoreMIDI slave driver can be stopped
# and started again on macOS.
#
# Before the fix this test guards, stopping such a server never finished: the CoreMIDI
# threads did not honour a cancellation request, so jackd stayed blocked while shutting
# down. A server that did start after a quick restart could also register its MIDI ports
# twice.
#
# The test starts "jackd -X coremidi -d dummy", asks it to stop with SIGTERM, checks that
# it exits in time, and repeats that several times without any pause in between.
# No audio device is used.
#
# usage:
#	./jack_coremidi_shutdown_test.sh [path/to/jackd]
#
# environment:
#	JACK_TEST_CYCLES	number of start/stop cycles (default: 5)
#	JACK_TEST_TIMEOUT	seconds to wait for a start or a stop (default: 10)
#
# exit status: 0 on success, 1 on failure, 77 if the test cannot run here (not macOS,
# jackd not found, or the CoreMIDI driver is not available).
#
# Note: jackd keeps a registry of at most 8 server names. A server that had to be killed
# keeps its entry, so this test always uses the same server name, which is reclaimed
# the next time it runs.

set -o pipefail

JACKD=${1:-jackd}
CYCLES=${JACK_TEST_CYCLES:-5}
TIMEOUT=${JACK_TEST_TIMEOUT:-10}
SERVER_NAME=jack_coremidi_shutdown_test

LOG=$(mktemp /tmp/jack_coremidi_shutdown_test.XXXXXX)
JACKD_PID=

#any failed cycle will set this to 1 (error)
FINAL_RETURN_VALUE=0

function cleanup()
{
	if [ -n "$JACKD_PID" ] && kill -0 "$JACKD_PID" 2> /dev/null;
	then
		kill -9 "$JACKD_PID" 2> /dev/null
	fi
	rm -f "$LOG"
}

trap cleanup EXIT

function skip()
{
	echo "SKIPPED: $1"
	exit 77
}

function fail()
{
	echo "**	FAILED: $1"
	FINAL_RETURN_VALUE=1
}

#wait until the process is gone: $1 = pid, $2 = timeout in seconds
#return 0 if it exited in time
function wait_exit()
{
	local ticks=$(( $2 * 10 ))
	while kill -0 "$1" 2> /dev/null;
	do
		if [ $ticks -le 0 ];
		then
			return 1
		fi
		ticks=$(( ticks - 1 ))
		sleep 0.1
	done
	return 0
}

#kill a server that did not stop on its own. It can take a moment for the kill to
#be effective if the process is stuck in a kernel call, so try again a few times
function force_kill()
{
	local tries=0
	while kill -0 "$1" 2> /dev/null && [ $tries -lt 10 ];
	do
		kill -9 "$1" 2> /dev/null
		tries=$(( tries + 1 ))
		sleep 0.5
	done
	#reap the process so that the shell does not report it later
	{ wait "$1"; } 2> /dev/null
}

[ "$(uname)" = "Darwin" ] || skip "this test is for macOS"
command -v "$JACKD" > /dev/null 2>&1 || skip "jackd not found: $JACKD"

for cycle in $(seq 1 "$CYCLES");
do
	echo "cycle $cycle/$CYCLES"
	: > "$LOG"
	"$JACKD" -n "$SERVER_NAME" -X coremidi -d dummy -r 48000 -p 256 > "$LOG" 2>&1 < /dev/null &
	JACKD_PID=$!

	#wait for the CoreMIDI driver to be fully started
	started=0
	ticks=$(( TIMEOUT * 10 ))
	while [ $ticks -gt 0 ];
	do
		if grep -q "JackCoreMidiDriver::Start - Driver started" "$LOG";
		then
			started=1
			break
		fi
		if ! kill -0 "$JACKD_PID" 2> /dev/null;
		then
			break
		fi
		ticks=$(( ticks - 1 ))
		sleep 0.1
	done

	if [ $started -ne 1 ];
	then
		if grep -q -E "Driver \"coremidi\" cannot be loaded|Could not find|No such file" "$LOG" && [ "$cycle" -eq 1 ];
		then
			force_kill "$JACKD_PID"
			skip "the CoreMIDI driver is not available"
		fi
		fail "the server did not start (cycle $cycle)"
		sed 's/^/	| /' "$LOG" | tail -n 5
		force_kill "$JACKD_PID"
		JACKD_PID=
		continue
	fi

	kill -TERM "$JACKD_PID"
	if wait_exit "$JACKD_PID" "$TIMEOUT";
	then
		echo "	OK: stopped"
	else
		fail "the server did not stop within $TIMEOUT s (cycle $cycle)"
		force_kill "$JACKD_PID"
	fi
	JACKD_PID=
done

if [ $FINAL_RETURN_VALUE -eq 0 ];
then
	echo "PASSED"
else
	echo "FAILED"
fi

exit $FINAL_RETURN_VALUE

#!/bin/sh

echo "`date`"
echo "========================================================================="
sudo updatedb
locate jack | grep -e "/usr/bin" -e "/usr/lib" -e "/usr/share/man" -e "/usr/include"
echo "========================================================================="
ls -l /usr/bin/jackd
jackd --version
echo "========================================================================="

#echo "/usr/bin/jackd --verbose -p512 -t5000 -ddummy -r44100"> ~/.jackdrc
#sudo jack_bufsize

locate jack | grep /usr/share/man | grep "\.1" | while read line; do
	man -P cat "$line"; done

echo "========================================================================="
sudo jackd -ddummy &
ret=$!
sleep 10
echo "========================================================================="
sudo jack_lsp
echo "========================================================================="
sudo jack_bufsize
echo "========================================================================="
sudo jack_samplerate
echo "========================================================================="
sudo jack_test
echo "========================================================================="
#sudo jack_test --verbose
#echo "========================================================================="
echo "stopping jackd now"
sudo kill -9 $ret
echo "========================================================================="
echo "`date`"
echo "done"

#!/bin/bash

#stop here if ./waf install wasn't successful
ls -l /usr/bin/jackd || exit

echo "`date`"
echo "$TRAVIS_OS_NAME"
echo "========================================================================="
if [ "$TRAVIS_OS_NAME" == "linux" ]; then
	#find installed files
	sudo updatedb
	locate jack | grep -e "/usr/bin" -e "/usr/lib" -e "/usr/share/man" -e "/usr/include"
	#show man pages
	locate jack | grep /usr/share/man | grep "\.1" | while read line; do
		man -P cat "$line"; done
	#check for unused dependencies
	ls -1 /usr/bin/jack_*|while read line; do
		echo "checking unused dependencies for ${line}:"; ldd -r -u "$line"; done
fi
echo "========================================================================="
jackd --version
echo "========================================================================="
sudo jackd -ddummy &
ret=$!
sleep 10
echo "========================================================================="
sudo jack_lsp
echo "========================================================================="
sudo jack_bufsize
echo "========================================================================="
sudo jack_test #--verbose
echo "========================================================================="
echo "stopping jackd now"
sudo kill -9 $ret
echo "========================================================================="
echo "`date`"
echo "done"
#EOF

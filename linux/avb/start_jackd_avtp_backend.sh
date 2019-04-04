#!/bin/sh

#
# Choose AVB backend:
#   -davb  		
#
# Choose ethernet device name:
#   --eth-dev enp5s0 
#
# Stream ID of the media clock stream:
#   --stream-id 00:22:97:00:41:2c:00:00 
#
# Destination MAC address of the media clock talker
#   --dst-mac 91:e0:f0:11:11:11
#
# Use JACK periods with accumulated reception intervals (1) 
# or with constant 125000ns (0):
#   -a 1 
#
# JACK periods (32, 64 and 128 have been tested): 
#   -p 
#
# JACK sample rate (only 48k has been tested):
#   -r 

jackd -R  -P8 -davb --eth-dev enp5s0 --stream-id 00:22:97:00:41:2c:00:00 --dst-mac 91:e0:f0:11:11:11 -a 1 -r 48000 -p 64

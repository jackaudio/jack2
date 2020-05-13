Experimental Audio Video Bridging Media Clock Listener JACK Backend

For a detailed description refer to http://lac.linuxaudio.org/2019/doc/kuhr.pdf

#####
# Requirements:
#####
- Intel i210 Ethernet Adapter
- Running OpenAvnu installation (igb_avb kernel module, libigb, gPTP daemon, MAAP daemon, MRP daemon, Shaper daemon)
- AVB Media Clock Talker on the AVB network segment must be active and accept any connection (only SRP, no AVDECC ACMP)
- linux/if_packet.h


#####
# Linux build steps:
#####
# Init submodule linux/avbmcl/OpenAvnu
cd linux/avbmcl
git submodule update --init
cd ../../
# Build JACK:
./waf configure
./waf build
sudo ./waf install



#####
# Example parameters: 
#####

# Choose AVB backend:
  -d avbmcl

# Choose ethernet device name:
  --eth-dev enp5s0 

# Stream ID of the media clock stream:
  --stream-id 00:22:97:00:41:2c:00:00 

# Destination MAC address of the media clock talker
  --dst-mac 91:e0:f0:11:11:11

# Use JACK periods with accumulated reception intervals (1) 
# or with constant 125000ns (0):
  -a 1 

# JACK periods (32, 64 and 128 have been tested): 
  -p 

# JACK sample rate (only 48k has been tested):
  -r 


#####
# Example call:
#####

jackd -R  -P8 -davbmcl --eth-dev enp5s0 --stream-id 00:22:97:00:41:2c:00:00 --dst-mac 91:e0:f0:11:11:11 -a 1 -r 48000 -p 64

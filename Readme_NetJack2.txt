-------------------------------
NetJack for Jackmp
-------------------------------

This branche includes a version of netjack designed for jackmp.
This prerelease is far from being complete and doesn't include some of the features proposed by netjack (no transport yet).
Indeed, the original concept has been completely redesigned to better fit to the jackmp architecture, but also in order to provide additional capabilities, and ultimately a greater robustness.

This document describes the major changes between those two systems, then a simple how-to for setting up a basic usage of 'netjackmp'.

-------------------------------
Major changes and architecture
-------------------------------

The biggest difference between netjack and netjackmp is the way of slicing audio and midi streams into network packets.
For one audio cycle, netjack used to take all audio and midi buffers (one per channel), put butt all of them, then send it over the network.
The problem is that a network packet has a fixed maximum size, depending on the network infrastructure (for 100mb, it reaches 1500bytes - MTU of the network).
The solution is then to slice those buffers into smaller ones, and then send as many packets as we need. This cutting up can be done by network equipments, but it's more efficient and secure to include it in the software data management.
Still this slicing brings another issue : all the packets are not pleased with any emission order and are unfortunately received in a random order, thanks to UDP.
So we can't deal with data as it comes, we need to re-bufferize incoming streams in order to rebuild complete audio buffers.

With netjackmp, the main idea is to make this slicing depending on the network capabilities.
If we can put only 128 complete audio frames (128 samples for all audio channels) in a network packet, the elementary packet will so carry 128 frames, and in one cycle, we will transmit as many packet as we need. We take the example of 128 frames because it's the current value for 2 channels. This value is determinated by taking the maximum 'power of 2' frames we can put in a packet.
If we take 2 channels, 4 bytes per sample (float values), we get 8 bytes per frame, with 128 frames, we now have 1024 bytes, so we can put these 1024 bytes in one packet, and add a small header which identify the packet.
This technique allows to separate the packets (in time) so they can be received in the order they have been emitted.
If the master is running at 512 frames per second, four audio packets are sent per cycle and the slave deals with them as they arrive.
With gigabytes networks, the MTU is larger, so we can put more data in one packet (in this example, we can even put the complete cycle in one packet).
For midi data, netjack used to send the whole buffer, in this example, 512 frames * 4 bytes per sample and per midi port. Those 2048 bytes are in 99% of the time filled to a few bytes, but rarely more. This means that if we have 2 audio and 2 midi channels to transmit, everything happens as if we had 4 audio channels, which is quite a waste of bandwidth.
In netjackmp, the idea is to take into account that fact, by sending only the useful bytes, and not more. It's completely unappropriate to overload the network with useless data.
So we now have : 99% of the time one midi packet (of a few dozen of bytes), followed by four audio packets (in this example).
Those five packets are preceded by a synchronization packet, which will make the slave directly synched on the master's cycle rythm.

This way of separating audio and midi is quite important. We deal here with network transmissions, and also need to be 'realtime'. We need a system which allow to carry as many audio and midi data streams as we need and can, as if the distant computer was in fact a simple jack client. With all those constraints, we can't avoid packets loss. The better thing to do is to deal with it.
But to loose an audio packet is different from skipping a midi one. Indeed, an audio loss leads to audio click, or undesirable, but very short side effect. Whereas a midi data loss can be completely disastrous. Imagine that we play some notes, with sustain, and we loose the sustain 0 value, which stops the effect. The sustain keeps going on on all following notes until the next 'sustain off' event. A simple missing byte can put all the midi system offside (that's the purpose of all the big PANIC buttons on midi softwares...).
That's why we need to separate audio (more than one per cycle) from midi (one packet at 99% of the time). If we loose an audio packet, we probably still have an available midi packet, so we can use what we received, even if some audio is missing.

The second main difference between netjack and netjackmp is the way the two computers (master and slave) synchronise their parametering and launch.
In netjack, once the slave configured (by the command line) and launched, it was waiting for the first incoming packet to synchronize (launch its first audio cycle) then run.
The two computers needed to be configured separately but with the same parameters to run correctly.

In netjackmp, the only thing you have to set for the slave is its number of in/out midi and audio channels. No more need to choose and set parameters depending on the master, they are automatically determinated and communicated to the slave.
This first synchronization step uses a multicast communication, no more need to know by advance all the IP addresses. The slave says on a multicast address "hey, I'm available". A master get the message, and communicate parametering to the slave. Once synchronization done, audio transfers can start. Moreover, the master being still listening on the multicast address, it can catch other slaves and manage them (create a jack client to communicate with the slave, and neatily close everything when the slave is gone).
The loaded internal client is no longer only an interface for the slave, like in netjack. It's now called 'network manager', it doesn't deal with audio or midi, just with some kind of 'network logistical messages'. The manager automatically create a new internal client as soon as a new slave is seen on the network (by sending messages on the multicast address the manager is listening on). This manager is also able to remove one of its internal client as soon as a slave has left the network. This conception allow a complete separation of audio exchanges from parametering and management.
The 'unloading' of the internal client (the manager) will cause a full cleaning of the infrastructure. The jack clients are all removed from the server, the slave are all turned available again, ready to be cought by another master etc. When a slave quits, it's also automatically removed from the manager's slaves list.


-------------------------------
Actual limitations
-------------------------------

The netjackmp's development has just started. This prerelease is operational, but involve some serious limitations :
- You can't set the bitdepth of the transmitted audio samples. Floating point values are used for now. The possibility to use 16 or 24bits data will be included soon, like in netjack. It's important to notice that change the bitdepth id very expensive in processing ressources (because it is a sample to sample operation).
- The 'inter-slave' communication is not optimized yet. If you connect one slave to another, the network streams go through the master. While sending network addresses to the slaves when they are inter-jack-connected, we could make the slave possibly independant of the master (except for the synchronization, sent over the network). This is the next step of the netjackmp development.
- This is not actually working on windows, because the windows socket API (the network development tools) aren't the same as under unix systems (linux and macosx).
- Because of the current development, the parametering of the whole system is minimalistic. For now you can't set another value of the MTU (allowing larger packets, meaning less packet, so less packet loss). The MTU issue is also a quite big one. In fact, the better thing to do seems to include an automatic detection of the MTU (called path MTU discovery) between master and slave. But this detection is quite controversial because it actually needs ICMP messages, which are more and more often unwanted nay blocked or ignored by network devices such as routers or firewalls (for security reasons). So in a first time, the MTU will possibly be a parameter of the master, like it is now in netjack. The fact is in this version, the MTU is set to 1500bytes, and it works fine on local 100mb networks. Future versions will include the possibility to set the MTU, which can be larger on gigabit networks (9000bytes for jumbo frames).
- Unlike netjack, this version doesn't include transport, this is naturraly planned for the fully operational version.
- Netjackmp can't use audio hardware to capture or play some sound. All these resampling processes will also be developed soon.


-------------------------------
How-to use this ?
-------------------------------

Netjackmp is very simple to use. On the master's side, an internal client deals with the slaves, and the slaves themselves are classical jack servers running under a 'network audio driver'.
The difference between the two versions is that the master now has a manager, which takes care of the slaves, while listening on the multicast address and create a new master as soon as a slave is available. But everything is transparent to the user, that's why it uses multicast (someone says "hello", and anyone who wants to hear it just has to listen).

So, just compile and install jackmp as you are used to, on linux, using 'scons' then 'scons install', on macosx, you can use the xcode project.

On the master, just launch a classical jack server, the period size doesn't matter.
Then, load the network manager using jack_load :

'jack_load netmanager'

This will load the internal client, which will wait for an available slave (see the message window on qjackctl - or the console output).

On the slave, just launch a new jack server using :

'jackd -R -S -d net' (or 'jackdmp -R -S -d net')

The '-S' (synchronous) option is mandatory. Indeed, the jack server isn't able to deal with the current cycle produced audio data because when the 'send' is called, data are not necessarily already available. So in a given 'n' cycle, the server will send the data produced at the 'n-1' cycle.
In this case, there is no effect on latency because all the subcycles of the slave (the previous 4 subcycles for instance) are run in the given master's cycle.
You can specify some options, like '-n name' (will give a name to the slave, default is the network hostname), '-C input_ports' (the number of master-->slave channels), '-P output_ports' (the number of slave-->master channels), default is 2 ; or '-i midi_in_ports' and '-o midi_out_ports', default is 0. Don't use '-a address' and '-p port', these are here to specify others multicast address and port, but it's not implemented on the master yet.


-------------------------------
What's next ?
-------------------------------

The development of 'netjackmp' continues and some things are going to change soon (see the 'actual limitations' section).
If you use it, please report encountered bugs, ideas or anything you think about.

If you have any question, you can subscribe the jackaudio developpers mailing list at http://www.jackaudio.org/ or join the IRC channel 'jack' on FreeNode.

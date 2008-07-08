To launch Jack with default drivers, start 'Jack Portaudio' or 'Jack NetDriver'.
Once Jack Server Started, you can launch QJackctl with the 'Jack Control' shortcut.

'Jack Command' starts a terminal from your installation folder. You can get a list of available Portaudio drivers with :

'jackdmp -d portaudio -l'

And then just launch :

'jackdmp -R -S -d portaudio -d "your driver's name"

More options with 'jackdmp'.
To use the Jack Router, just select the 'JackRouter' asio driver in your audio software.
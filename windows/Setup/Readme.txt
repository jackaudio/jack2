This folder contains a script to create an installer for windows.
It uses 'CreateInstall Free'(http://www.createinstall.com), a little software allowing to make simple installers.

You can use the 'jack.ci' script to make the installer. For that, you need to build the Code::Blocks workspace in order to have '.exe' and libraries. You also need 'qjackctl' binaries and libraries ('qjackctl.exe', 'mingwm10.dll', 'QtCore4.dll', 'QtGui.dll' and 'QtXml4.dll'). You can recompile qjackctl with qt4 or directly get the binaries. The five files are expected in the 'qjackctl' folder.

Once all binaries available, just execute the script in 'CreateInstall' to make 'setup.exe'.
The setup will copy all binaries to a specified folder, register the JackRouter (in order to have it in the ASIO drivers list) and create some shortcuts in the start menu.
It's a good and proper way to get jack installed on windows.
#include "version.iss"

[Setup]
AppName=JACK2
AppPublisher=jackaudio.org
AppPublisherURL=https://github.com/jackaudio/jack2/
AppSupportURL=https://github.com/jackaudio/jack2/issues/
AppUpdatesURL=https://github.com/jackaudio/jack2-releases/releases/
AppVersion={#VERSION}
DefaultDirName={commonpf32}\JACK2
DisableDirPage=yes
OutputBaseFilename=jack2-win32-{#VERSION}
OutputDir=.
UsePreviousAppDir=no

[Types]
Name: "full"; Description: "Full installation (without JACK-Router)";
Name: "router"; Description: "Full installation (with JACK-Router)";
Name: "custom"; Description: "Custom installation"; Flags: iscustom;

[Components]
Name: jackserver; Description: "JACK Server and tools"; Types: full router custom; Flags: fixed;
Name: qjackctl; Description: "QjackCtl application (recommended)"; Types: full router;
Name: router; Description: "JACK-Router ASIO Driver"; Types: router;
Name: dev; Description: "Developer resources"; Types: full router;

[Files]
; icon
Source: "jack.ico"; DestDir: "{app}"; Components: jackserver;
; jackd and server libs
Source: "win32\bin\jackd.exe"; DestDir: "{app}"; Components: jackserver;
Source: "win32\lib\libjacknet.dll"; DestDir: "{app}"; Components: jackserver;
Source: "win32\lib\libjackserver.dll"; DestDir: "{app}"; Components: jackserver;
; drivers
Source: "win32\lib\jack\*.dll"; DestDir: "{app}\jack"; Components: jackserver;
; tools
Source: "win32\bin\jack_*.exe"; DestDir: "{app}\tools"; Components: jackserver;
; jack client lib (NOTE goes into windir)
Source: "win32\lib\libjack.dll"; DestDir: "{win}"; Components: jackserver;
; qjackctl
Source: "win32\bin\qjackctl.exe"; DestDir: "{app}\qjackctl"; Components: qjackctl;
Source: "Qt5*.dll"; DestDir: "{app}\qjackctl"; Components: qjackctl;
Source: "qwindows.dll"; DestDir: "{app}\qjackctl\platforms"; Components: qjackctl;
; dev
Source: "win32\include\jack\*.h"; DestDir: "{app}\include\jack"; Components: dev;
Source: "win32\lib\*.a"; DestDir: "{app}\lib"; Components: dev;
Source: "win32\lib\*.def"; DestDir: "{app}\lib"; Components: dev;
Source: "win32\lib\*.lib"; DestDir: "{app}\lib"; Components: dev;
Source: "win32\lib\jack\*.a"; DestDir: "{app}\lib\jack"; Components: dev;
; router
Source: "win32\jack-router\README.txt"; DestDir: "{app}\jack-router"; Components: router;
Source: "win32\jack-router\win32\JackRouter.dll"; DestDir: "{app}\jack-router\win32"; Components: router; Flags: regserver;
Source: "win32\jack-router\win32\JackRouter.ini"; DestDir: "{app}\jack-router\win32"; Components: router;

[Icons]
Name: "{commonprograms}\QjackCtl"; Filename: "{app}\qjackctl\qjackctl.exe"; IconFilename: "{app}\jack.ico"; WorkingDir: "{app}"; Comment: "Graphical Interface for JACK"; Components: qjackctl;

[Registry]
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "ServerExecutable"; ValueData: "{app}\jackd.exe"
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "Version"; ValueData: "{#VERSION}"

#include "version.iss"

[Setup]
ArchitecturesInstallIn64BitMode=x64
AppName=JACK2
AppPublisher=jackaudio.org
AppPublisherURL=https://github.com/jackaudio/jack2/
AppSupportURL=https://github.com/jackaudio/jack2/issues/
AppUpdatesURL=https://github.com/jackaudio/jack2-releases/releases/
AppVersion={#VERSION}
DefaultDirName={commonpf64}\JACK2
DisableDirPage=yes
OutputBaseFilename=jack2-win64-{#VERSION}
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
Source: "jack.ico"; DestDir: "{app}"; Components: jackserver; Flags: ignoreversion;
; jackd and server libs
Source: "win64\bin\jackd.exe"; DestDir: "{app}"; Components: jackserver; Flags: ignoreversion;
Source: "win64\lib\libjacknet64.dll"; DestDir: "{app}"; Components: jackserver; Flags: ignoreversion;
Source: "win64\lib\libjackserver64.dll"; DestDir: "{app}"; Components: jackserver; Flags: ignoreversion;
; drivers
Source: "win64\lib\jack\*.dll"; DestDir: "{app}\jack"; Components: jackserver; Flags: ignoreversion;
; tools
Source: "win64\bin\jack_*.exe"; DestDir: "{app}\tools"; Components: jackserver; Flags: ignoreversion;
; jack client lib (NOTE goes into windir)
Source: "win64\lib\libjack64.dll"; DestDir: "{win}"; Components: jackserver; Flags: ignoreversion;
Source: "win64\lib32\libjack.dll"; DestDir: "{win}"; Components: jackserver; Flags: ignoreversion;
; qjackctl
Source: "win64\bin\qjackctl.exe"; DestDir: "{app}\qjackctl"; Components: qjackctl; Flags: ignoreversion;
Source: "Qt5*.dll"; DestDir: "{app}\qjackctl"; Components: qjackctl; Flags: ignoreversion;
Source: "qwindows.dll"; DestDir: "{app}\qjackctl\platforms"; Components: qjackctl; Flags: ignoreversion;
; dev
Source: "win64\include\jack\*.h"; DestDir: "{app}\include\jack"; Components: dev; Flags: ignoreversion;
Source: "win64\lib\*.a"; DestDir: "{app}\lib"; Components: dev; Flags: ignoreversion;
Source: "win64\lib\*.def"; DestDir: "{app}\lib"; Components: dev; Flags: ignoreversion;
Source: "win64\lib\*.lib"; DestDir: "{app}\lib"; Components: dev; Flags: ignoreversion;
Source: "win64\lib32\*.a"; DestDir: "{app}\lib32"; Components: dev; Flags: ignoreversion;
Source: "win64\lib32\*.def"; DestDir: "{app}\lib32"; Components: dev; Flags: ignoreversion;
Source: "win64\lib32\*.lib"; DestDir: "{app}\lib32"; Components: dev; Flags: ignoreversion;
Source: "win64\lib\jack\*.a"; DestDir: "{app}\lib\jack"; Components: dev; Flags: ignoreversion;
; router
Source: "win64\jack-router\README.txt"; DestDir: "{app}\jack-router"; Components: router; Flags: ignoreversion;
Source: "win64\jack-router\win32\JackRouter.dll"; DestDir: "{app}\jack-router\win32"; Components: router; Flags: ignoreversion regserver 32bit;
Source: "win64\jack-router\win32\JackRouter.ini"; DestDir: "{app}\jack-router\win32"; Components: router; Flags: ignoreversion;
Source: "win64\jack-router\win64\JackRouter.dll"; DestDir: "{app}\jack-router\win64"; Components: router; Flags: ignoreversion regserver 64bit;
Source: "win64\jack-router\win64\JackRouter.ini"; DestDir: "{app}\jack-router\win64"; Components: router; Flags: ignoreversion;

[Icons]
Name: "{commonprograms}\QjackCtl"; Filename: "{app}\qjackctl\qjackctl.exe"; IconFilename: "{app}\jack.ico"; WorkingDir: "{app}"; Comment: "Graphical Interface for JACK"; Components: qjackctl;

[Registry]
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "ServerExecutable"; ValueData: "{app}\jackd.exe"
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "Version"; ValueData: "{#VERSION}"
; 32bit compat keys
Root: HKLM; Subkey: "Software\WOW6432Node\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "ServerExecutable"; ValueData: "{app}\jackd.exe"
Root: HKLM; Subkey: "Software\WOW6432Node\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKLM; Subkey: "Software\WOW6432Node\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "Version"; ValueData: "{#VERSION}"

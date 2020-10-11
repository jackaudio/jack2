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
OutputBaseFilename=jack2-win32-v{#VERSION}
OutputDir=.
UsePreviousAppDir=no

[Types]
Name: "full"; Description: "Full installation";
Name: "custom"; Description: "Custom installation"; Flags: iscustom;

[Components]
Name: qjackctl; Description: "QJackCtl application (recommended)"; Types: full;
Name: dev; Description: "Developer resources"; Types: full;

[Files]
; icon
Source: "jack.ico"; DestDir: "{app}";
; jackd and server libs
Source: "win32\bin\jackd.exe"; DestDir: "{app}";
Source: "win32\lib\libjacknet.dll"; DestDir: "{app}";
Source: "win32\lib\libjackserver.dll"; DestDir: "{app}";
; drivers
Source: "win32\lib\jack\*.dll"; DestDir: "{app}\jack";
; tools
Source: "win32\bin\jack_*.exe"; DestDir: "{app}\tools";
; jack client lib (NOTE goes into windir)
Source: "win32\lib\libjack.dll"; DestDir: "{win}";
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

[Icons]
Name: "{commonprograms}\QJackCtl"; Filename: "{app}\qjackctl\qjackctl.exe"; IconFilename: "{app}\jack.ico"; WorkingDir: "{app}"; Comment: "Graphical Interface for JACK"; Components: qjackctl;

[Registry]
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "Location"; ValueData: "{app}\jackd.exe"
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "Version"; ValueData: "{#VERSION}"

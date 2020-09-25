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
OutputBaseFilename=jack2-win64-v{#VERSION}
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
Source: "win64\bin\jackd.exe"; DestDir: "{app}";
Source: "win64\lib\libjacknet64.dll"; DestDir: "{app}";
Source: "win64\lib\libjackserver64.dll"; DestDir: "{app}";
; drivers
Source: "win64\lib\jack\*.dll"; DestDir: "{app}\jack";
; tools
Source: "win64\bin\jack_*.exe"; DestDir: "{app}\tools";
; jack client lib (NOTE goes into windir)
Source: "win64\lib\libjack64.dll"; DestDir: "{win}";
Source: "win64\lib32\libjack.dll"; DestDir: "{win}";
; qjackctl
Source: "win64\bin\qjackctl.exe"; DestDir: "{app}\qjackctl"; Components: qjackctl;
Source: "Qt5*.dll"; DestDir: "{app}\qjackctl"; Components: qjackctl;
Source: "qwindows.dll"; DestDir: "{app}\qjackctl\platforms"; Components: qjackctl;
; dev
Source: "win64\include\jack\*.h"; DestDir: "{app}\include\jack"; Components: dev;
Source: "win64\lib\*.a"; DestDir: "{app}\lib"; Components: dev;
Source: "win64\lib\*.def"; DestDir: "{app}\lib"; Components: dev;
Source: "win64\lib\*.lib"; DestDir: "{app}\lib"; Components: dev;
Source: "win64\lib32\*.a"; DestDir: "{app}\lib32"; Components: dev;
Source: "win64\lib32\*.def"; DestDir: "{app}\lib32"; Components: dev;
Source: "win64\lib32\*.lib"; DestDir: "{app}\lib32"; Components: dev;
Source: "win64\lib\jack\*.a"; DestDir: "{app}\lib\jack"; Components: dev;

[Icons]
Name: "{commonprograms}\QJackCtl"; Filename: "{app}\qjackctl\qjackctl.exe"; IconFilename: "{app}\jack.ico"; WorkingDir: "{app}"; Comment: "Graphical Interface for JACK"; Components: qjackctl;

[Registry]
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "Location"; ValueData: "{app}\jackd.exe"
Root: HKLM; Subkey: "Software\JACK"; Flags: deletevalue uninsdeletekeyifempty uninsdeletevalue; ValueType: string; ValueName: "Version"; ValueData: "{#VERSION}"

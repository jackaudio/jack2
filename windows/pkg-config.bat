@ECHO OFF
REM This file should be on the PATH when building Jack on Windows with MSVC
REM after bootstrapping with install_vcpkg_deps.bat. 

SET HAS_LIBS=false
:Loop
IF "%1"=="" GOTO Fail
IF "%1"=="--libs" (
	SET HAS_LIBS=true
)
IF "%1"=="portaudio-2.0" GOTO PortAudio
IF "%1"=="samplerate" GOTO SampleRate
SHIFT
GOTO Loop

:Fail
EXIT /b 1

:PortAudio
if "%HAS_LIBS%"=="true" (
	ECHO -lportaudio_pure
)
GOTO Continue

:SampleRate
if "%HAS_LIBS%"=="true" (
	ECHO -llibsamplerate-0
)
GOTO Continue

:Continue


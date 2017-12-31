@ECHO off
REM Requires VS2017 being installed, and git on the PATH.
REM This script downloads vcpkg, then downloads and builds all dependencies
REM for Windows, then copies the built binaries to a local deps directory,
REM then renames portaudio.lib to portaudio_x86.lib to avoid conflicts with
REM jack_portaudio's import library - also called portaudio.lib, then renames
REM pthreadsVC2.lib to pthread.lib for convenience, since it's named
REM similarily on the other platforms.

SETLOCAL

WHERE /q git
IF ERRORLEVEL 1 (
	ECHO ERROR: git must be on the path
	EXIT /B 1
)

IF "%VCPKG_ROOT%" == "" (
	ECHO VCPKG_ROOT should point at the root of a vcpkg installation. Using ".\vcpkg"
	SET VCPKG_ROOT=%~dp0\vcpkg
)

IF NOT EXIST "%VCPKG_ROOT%" (	
	git clone https://github.com/Microsoft/vcpkg.git "%VCPKG_ROOT%"
	CALL "%VCPKG_ROOT%\bootstrap-vcpkg.bat"
	IF ERRORLEVEL 1 (
		ECHO ERROR: vcpkg bootstrap failed
		EXIT /B 1
	)
)

IF NOT EXIST "%VCPKG_ROOT%\vcpkg.exe" (
	ECHO ERROR: Cannot find %VCPKG_ROOT%\vcpkg.exe in VCPKG_ROOT
	EXIT /B 1
)

IF NOT EXIST "%~dp0\vcpkg_extras" (
	git clone https://github.com/clvn/vcpkg_extras.git "%~dp0\vcpkg_extras"
)
xcopy /e /y /i /q "%~dp0\vcpkg_extras\ports\*.*" "%VCPKG_ROOT%\ports"

"%VCPKG_ROOT%\vcpkg.exe" install portaudio:x86-windows pthreads:x86-windows-static libsamplerate:x86-windows-static tre:x86-windows-static
IF ERRORLEVEL 1 (
	ECHO ERROR: building x86 dependencies failed
	EXIT /B 1
)

"%VCPKG_ROOT%\vcpkg.exe" install portaudio:x64-windows pthreads:x64-windows-static libsamplerate:x64-windows-static tre:x64-windows-static
IF ERRORLEVEL 1 (
	ECHO ERROR: building x64 dependencies failed
	EXIT /B 1
)

IF NOT EXIST "%~dp0\deps\include" MKDIR "%~dp0\deps\include"
IF NOT EXIST "%~dp0\deps\lib_x86" MKDIR "%~dp0\deps\lib_x86"
IF NOT EXIST "%~dp0\deps\lib_x64" MKDIR "%~dp0\deps\lib_x64"

COPY /Y "%VCPKG_ROOT%\installed\x86-windows\lib\portaudio.lib" "%~dp0\deps\lib_x86\portaudio_x86.lib"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows\include\portaudio.h" "%~dp0\deps\include"

COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\lib\libsamplerate-0.lib" "%~dp0\deps\lib_x86"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\include\samplerate.h" "%~dp0\deps\include"

COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\lib\pthreadsVC2.lib" "%~dp0\deps\lib_x86\pthread.lib"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\include\pthread.h" "%~dp0\deps\include"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\include\semaphore.h" "%~dp0\deps\include"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\include\sched.h" "%~dp0\deps\include"

COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\lib\tre.lib" "%~dp0\deps\lib_x86"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\include\tre.h" "%~dp0\deps\include"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\include\tre-config.h" "%~dp0\deps\include"
COPY /Y "%VCPKG_ROOT%\installed\x86-windows-static\include\regex.h" "%~dp0\deps\include"

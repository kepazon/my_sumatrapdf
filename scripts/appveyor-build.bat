@ECHO OFF
SETLOCAL

REM assumes we're being run from top-level directory as:
REM scripts\appveyor-build.bat

CALL scripts\vc.bat
IF ERRORLEVEL 1 EXIT /B 1

REM add our nasm.exe to the path
SET PATH=%CD%\bin;%PATH%

nmake -f makefile.msvc CFG=rel PLATFORM=X86 SumatraPDF Uninstaller

@rem TODO: build with more platforms, build and run tests etc.

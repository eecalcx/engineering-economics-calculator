@echo off
setlocal
set SRC=Main.c
set OUT=EECalc.exe
set RC=appicon.rc
set RES=appicon.res

where windres >nul 2>nul
if errorlevel 1 (
    echo windres was not found. Install MinGW-w64 with windres and try again.
    exit /b 1
)

windres %RC% -O coff -o %RES%
if errorlevel 1 (
    echo Failed to compile the icon resource.
    exit /b 1
)

gcc -O2 -Wall -Wextra -mwindows %SRC% %RES% -o %OUT% -lcomctl32 -luxtheme -ldwmapi -luser32 -lgdi32 -lole32
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Build complete: %OUT%
endlocal

@echo off
setlocal
set SRC="Percentage Calculator.c"
set OUT="Percentage Calculator.exe"
gcc -O2 -Wall -Wextra -mwindows %SRC% -o %OUT% -lcomctl32 -luxtheme -ldwmapi -luser32 -lgdi32 -lole32
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
echo Build complete: %OUT%
endlocal

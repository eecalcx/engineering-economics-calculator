# Engineering Economics Calculator

A Windows desktop calculator built in C with a dark/light theme and a simple input form for common engineering economics formulas.

## Features
- Percentage increase/decrease
- Present/future worth calculations
- Uniform series and gradient calculations
- Effective annual rate and inflation-adjusted calculations
- Benefit-cost ratio estimation
- Dark/light mode support

## Build on Windows
Use MinGW / MSYS2 GCC with the following command:

```powershell
gcc -O2 -Wall -Wextra -mwindows "Percentage Calculator.c" -o "Percentage Calculator.exe" -lcomctl32 -luxtheme -ldwmapi -luser32 -lgdi32 -lole32
```

## Run
Double-click the generated `Percentage Calculator.exe` file.

## Notes
- The executable is intentionally excluded from source control via `.gitignore`.
- The app stores its theme preference in `theme_mode.txt` next to the executable at runtime.

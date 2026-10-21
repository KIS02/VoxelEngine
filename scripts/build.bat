@echo off
setlocal

cd /d "%~dp0.."
set "ROOT=%CD%"
set "PATH=%ROOT%\tools\mingw\bin;%PATH%"
set "CMAKE=%ROOT%\tools\cmake\bin\cmake.exe"
set "GCC=%ROOT%\tools\mingw\bin\gcc.exe"
set "GXX=%ROOT%\tools\mingw\bin\g++.exe"
set "MAKE=%ROOT%\tools\mingw\bin\mingw32-make.exe"

if not exist "%CMAKE%" (
    echo [ERROR] CMake not found: %CMAKE%
    exit /b 1
)
if not exist "%GXX%" (
    echo [ERROR] MinGW compiler not found: %GXX%
    exit /b 1
)
if not exist "%ROOT%\external\glfw" (
    echo [ERROR] external\glfw is missing.
    exit /b 1
)
if not exist "%ROOT%\external\glad\src\gl.c" (
    echo [ERROR] external\glad is missing.
    exit /b 1
)

"%CMAKE%" -S "%ROOT%" -B "%ROOT%\build" -G "MinGW Makefiles" ^
    "-DCMAKE_C_COMPILER=%GCC%" "-DCMAKE_CXX_COMPILER=%GXX%" ^
    "-DCMAKE_MAKE_PROGRAM=%MAKE%" "-DCMAKE_BUILD_TYPE=Debug"
if errorlevel 1 exit /b 1

"%CMAKE%" --build "%ROOT%\build" --parallel 4
if errorlevel 1 exit /b 1

if not exist "%ROOT%\bin" mkdir "%ROOT%\bin"
copy /Y "%ROOT%\build\VoxelEngine.exe" "%ROOT%\bin\VoxelEngine.exe" >nul
if errorlevel 1 exit /b 1

if exist "%ROOT%\tools\mingw\bin\libwinpthread-1.dll" (
    copy /Y "%ROOT%\tools\mingw\bin\libwinpthread-1.dll" "%ROOT%\bin\libwinpthread-1.dll" >nul
)

echo [PASS] VoxelEngine built: %ROOT%\bin\VoxelEngine.exe
exit /b 0

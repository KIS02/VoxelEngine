@echo off
setlocal

cd /d "%~dp0.."

set "ROOT=%CD%"
set "MINGW=%ROOT%\tools\mingw"
set "GLFW=%ROOT%\external\glfw"

rem 프로젝트 내부 MinGW를 이 프로세스에서만 PATH에 추가
set "PATH=%MINGW%\bin;%PATH%"

if not exist "%ROOT%\bin" mkdir "%ROOT%\bin"

echo ========================================
echo Building VoxelEngine
echo ========================================

"%MINGW%\bin\g++.exe" ^
    "%ROOT%\src\main.cpp" ^
    "%ROOT%\src\core\Application.cpp" ^
    "%ROOT%\src\core\Window.cpp" ^
    -I"%ROOT%\src" ^
    -I"%GLFW%\include" ^
    -L"%GLFW%\lib-mingw-w64" ^
    -g ^
    -static-libgcc ^
    -static-libstdc++ ^
    -o "%ROOT%\bin\VoxelEngine.exe" ^
    -lglfw3 ^
    -lopengl32 ^
    -lgdi32

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESS
echo ========================================

exit /b 0
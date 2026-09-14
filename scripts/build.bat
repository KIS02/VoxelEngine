@echo off
setlocal

cd /d "%~dp0.."

if not exist bin mkdir bin

echo ========================================
echo Building VoxelEngine
echo ========================================

tools\mingw\bin\g++.exe ^
    src\main.cpp ^
    src\core\Application.cpp ^
    src\core\Window.cpp ^
    -Isrc ^
    -Iexternal\glfw\include ^
    -Lexternal\glfw\lib-mingw-w64 ^
    -g ^
    -static-libgcc ^
    -static-libstdc++ ^
    -o bin\VoxelEngine.exe ^
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
echo bin\VoxelEngine.exe
echo ========================================

exit /b 0
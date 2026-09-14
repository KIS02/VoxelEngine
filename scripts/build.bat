@echo off
setlocal

rem ========================================
rem 프로젝트 루트로 이동
rem build.bat 위치:
rem VoxelEngine/scripts/build.bat
rem ========================================

cd /d "%~dp0.."

set "ROOT=%CD%"
set "MINGW=%ROOT%\tools\mingw"
set "GLFW=%ROOT%\external\glfw"

rem 이 build.bat가 실행되는 동안에만
rem 프로젝트 내부 MinGW를 PATH에 추가
set "PATH=%MINGW%\bin;%PATH%"


rem ========================================
rem bin 폴더 생성
rem ========================================

if not exist "%ROOT%\bin" (
    mkdir "%ROOT%\bin"
)


echo ========================================
echo Building VoxelEngine
echo ========================================


rem ========================================
rem VoxelEngine 컴파일
rem ========================================

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


rem ========================================
rem 컴파일 실패 검사
rem ========================================

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    exit /b 1
)


rem ========================================
rem 필요한 MinGW Runtime DLL 복사
rem ========================================

if not exist "%MINGW%\bin\libwinpthread-1.dll" (
    echo.
    echo ERROR:
    echo libwinpthread-1.dll not found.
    echo Expected:
    echo %MINGW%\bin\libwinpthread-1.dll
    exit /b 1
)

copy /Y ^
    "%MINGW%\bin\libwinpthread-1.dll" ^
    "%ROOT%\bin\libwinpthread-1.dll" >nul


echo.
echo ========================================
echo BUILD SUCCESS
echo ========================================
echo.
echo Output:
echo %ROOT%\bin\VoxelEngine.exe
echo.
echo Runtime:
echo %ROOT%\bin\libwinpthread-1.dll
echo.

exit /b 0
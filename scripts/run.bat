@echo off
setlocal

cd /d "%~dp0.."

set "PATH=%CD%\tools\mingw\bin;%PATH%"

bin\VoxelEngine.exe
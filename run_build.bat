@echo off
rem Load Visual Studio 2022 Environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto :error

rem Build
echo Starting Build with VS 2022...
msbuild out\Telegram\Telegram.vcxproj /m:1 /nr:false /v:minimal /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto :error

echo Build Process Finished.
exit /b 0

:error
echo Build Process Failed.
exit /b 1

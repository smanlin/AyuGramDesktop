@echo off
setlocal

set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VCVARS%" (
  echo Cannot find vcvars64.bat
  exit /b 1
)

call "%VCVARS%"
if errorlevel 1 goto :error

echo Starting FAST build...
msbuild out\Telegram\Telegram.vcxproj ^
  /m ^
  /nr:false ^
  /v:minimal ^
  /p:Configuration=Release ^
  /p:Platform=x64 ^
  /p:MultiProcessorCompilation=true ^
  /p:UseMultiToolTask=true
if errorlevel 1 goto :error

echo Build Process Finished.
exit /b 0

:error
echo Build Process Failed.
exit /b 1


@echo off
rem Load Visual Studio 2022 Environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

rem Go to Telegram directory
cd /d "%~dp0Telegram"

rem Configure
echo Starting Configure with VS 2022...
call configure.bat x64 -D TDESKTOP_API_ID=611335 -D TDESKTOP_API_HASH=d524b414d21f4d37f08684c1df41ac9c -D DESKTOP_APP_DISABLE_AUTOUPDATE=ON -D DESKTOP_APP_DISABLE_CRASH_REPORTS=ON -D DESKTOP_APP_NO_PDB=ON

echo Done.

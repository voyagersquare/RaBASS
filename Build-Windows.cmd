@echo off
setlocal
cd /d "%~dp0"
if /i "%~1"=="--no-pause" set "RABASS_NO_PAUSE=1"
set "RABASS_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%RABASS_VSWHERE%" goto :missingvs
for /f "usebackq tokens=*" %%I in (`"%RABASS_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "RABASS_VS=%%I"
if not defined RABASS_VS goto :missingvs
call "%RABASS_VS%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 goto :failed
set "PATH=%RABASS_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%RABASS_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
where cmake >nul 2>&1
if errorlevel 1 goto :missingtools
where ninja >nul 2>&1
if errorlevel 1 goto :missingtools
where git >nul 2>&1
if errorlevel 1 goto :missingtools
cmake --preset windows-x64
if errorlevel 1 goto :failed
cmake --build --preset windows-release --target RaBASS_VST3 RaBASSDspTests RaBASSProcessorTests --parallel 2
if errorlevel 1 goto :failed
ctest --preset windows-release
if errorlevel 1 goto :failed
echo.
echo BUILD SUCCEEDED: build\windows-x64\RaBASS_artefacts\Release\VST3\RaBASS.vst3
echo Run Build-Installer.cmd to create the Windows setup wizard.
if not defined RABASS_NO_PAUSE pause
exit /b 0
:missingvs
echo Install Visual Studio 2026 with Desktop development with C++.
goto :failed
:missingtools
echo Install Git for Windows and the C++ CMake tools for Windows component in Visual Studio Installer.
goto :failed
:failed
echo.
echo Build failed. See the error above.
if not defined RABASS_NO_PAUSE pause
exit /b 1

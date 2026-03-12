@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "DiagnosisScript=%RepositoryRoot%\DiagnoseSc2Startup.py"
set "LogsDirectory=%RepositoryRoot%\out\logs"
set "LogPath=%LogsDirectory%\Sc2StartupDiagnosis_latest.txt"
set "ScreenshotPath=%LogsDirectory%\Sc2StartupDiagnosis_screen.png"
set "TestFilter=sc2::TestAbilityRemap"
set "SampleSeconds=30"

if not "%~1"=="" set "TestFilter=%~1"
if not "%~2"=="" set "SampleSeconds=%~2"

if not exist "%DiagnosisScript%" (
    echo Could not find diagnosis script: "%DiagnosisScript%"
    exit /b 1
)

where python >nul 2>nul
if errorlevel 1 (
    echo Python was not found on PATH.
    exit /b 1
)

if not exist "%LogsDirectory%" mkdir "%LogsDirectory%"

echo Diagnosing SC2 startup with filter: "%TestFilter%"
echo Writing log to: "%LogPath%"
echo Writing screenshot to: "%ScreenshotPath%"

python "%DiagnosisScript%" --repository-root "%RepositoryRoot%" --log-path "%LogPath%" --screenshot-path "%ScreenshotPath%" --test-filter "%TestFilter%" --sample-seconds %SampleSeconds%

set "ExitCode=%ERRORLEVEL%"
echo Exit code: %ExitCode%
echo Diagnosis log path: "%LogPath%"
exit /b %ExitCode%

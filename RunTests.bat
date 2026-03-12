@echo off
setlocal

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "WatchdogScript=%RepositoryRoot%\RunSc2TestWithWatchdog.py"
set "LogsDirectory=%RepositoryRoot%\out\logs"
set "LogPath=%LogsDirectory%\TestLog_latest.txt"
set "TimeoutSeconds=900"
set "TestFilter="

if not "%~1"=="" set "TestFilter=%~1"
if not "%~2"=="" set "TimeoutSeconds=%~2"

if not exist "%WatchdogScript%" (
    echo Could not find watchdog script: "%WatchdogScript%"
    exit /b 1
)

where python >nul 2>nul
if errorlevel 1 (
    echo Python was not found on PATH.
    exit /b 1
)
if not exist "%LogsDirectory%" mkdir "%LogsDirectory%"

set "SC2_WAIT_ON_EXIT=0"

echo Resolving newest test runner under: "%RepositoryRoot%\out\build"
echo Writing log to: "%LogPath%"
if defined TestFilter (
    echo Test filter: "%TestFilter%"
    python "%WatchdogScript%" --repository-root "%RepositoryRoot%" --log-path "%LogPath%" --timeout-seconds %TimeoutSeconds% --test-filter "%TestFilter%"
) else (
    echo Running full test suite.
    python "%WatchdogScript%" --repository-root "%RepositoryRoot%" --log-path "%LogPath%" --timeout-seconds %TimeoutSeconds%
)

set "ExitCode=%ERRORLEVEL%"
echo Exit code: %ExitCode%
echo Log path: "%LogPath%"
exit /b %ExitCode%

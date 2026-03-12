@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "BuildScript=%RepositoryRoot%\BuildAllTests.bat"
set "DiagnosisScript=%RepositoryRoot%\DiagnoseSc2Startup.bat"
set "WatchdogScript=%RepositoryRoot%\RunSc2TestWithWatchdog.py"
set "LogsDirectory=%RepositoryRoot%\out\logs"
set "LogPath=%LogsDirectory%\TestLog_latest.txt"
set "FilteredTimeoutSeconds=900"
set "FullSuiteTimeoutSeconds=1800"
set "TimeoutSeconds="
set "TestFilter="
set "RunFullSuite=0"
set "RunBuildBeforeTests=0"

if /I "%~1"=="build" goto RunBuildCommand
if /I "%~1"=="compile" goto RunBuildCommand
if /I "%~1"=="diagnose" goto RunDiagnoseCommand
if /I "%~1"=="startup" goto RunDiagnoseCommand
if /I "%~1"=="regression" (
    set "RunBuildBeforeTests=1"
    shift
)
if /I "%~1"=="regress" (
    set "RunBuildBeforeTests=1"
    shift
)

:ParseArguments
if "%~1"=="" goto AfterParseArguments

if /I "%~1"=="full" (
    set "RunFullSuite=1"
    shift
    goto ParseArguments
)

if /I "%~1"=="--full" (
    set "RunFullSuite=1"
    shift
    goto ParseArguments
)

if /I "%~1"=="all" (
    set "RunFullSuite=1"
    shift
    goto ParseArguments
)

if /I "%~1"=="--help" goto ShowUsage
if /I "%~1"=="-h" goto ShowUsage

if /I "%~1"=="--filter" (
    if "%~2"=="" (
        echo Missing test filter after --filter.
        exit /b 1
    )
    set "TestFilter=%~2"
    shift
    shift
    goto ParseArguments
)

if /I "%~1"=="--timeout" (
    if "%~2"=="" (
        echo Missing timeout value after --timeout.
        exit /b 1
    )
    set "TimeoutSeconds=%~2"
    shift
    shift
    goto ParseArguments
)

set "NumericArgumentValue="
echo(%~1| findstr /r "^[0-9][0-9]*$" >nul && set "NumericArgumentValue=%~1"
if not defined TestFilter if not defined TimeoutSeconds if defined NumericArgumentValue (
    set "RunFullSuite=1"
    set "TimeoutSeconds=%NumericArgumentValue%"
    shift
    goto ParseArguments
)

if not defined TestFilter (
    set "TestFilter=%~1"
    shift
    goto ParseArguments
)

if not defined TimeoutSeconds (
    set "TimeoutSeconds=%~1"
    shift
    goto ParseArguments
)

echo Unrecognized extra argument: "%~1"
exit /b 1

:AfterParseArguments
if "%RunFullSuite%"=="1" set "TestFilter="

if "%RunBuildBeforeTests%"=="1" (
    if not exist "%BuildScript%" (
        echo Could not find build script: "%BuildScript%"
        exit /b 1
    )

    echo Building all_tests before running the regression suite.
    call "%BuildScript%"
    set "BuildExitCode=%ERRORLEVEL%"
    if not "%BuildExitCode%"=="0" (
        echo Build step failed with exit code: %BuildExitCode%
        exit /b %BuildExitCode%
    )
    set "RunFullSuite=1"
    set "TestFilter="
)

if not defined TimeoutSeconds (
    if defined TestFilter (
        set "TimeoutSeconds=%FilteredTimeoutSeconds%"
    ) else (
        set "TimeoutSeconds=%FullSuiteTimeoutSeconds%"
    )
)

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

:RunBuildCommand
if not exist "%BuildScript%" (
    echo Could not find build script: "%BuildScript%"
    exit /b 1
)

call "%BuildScript%"
exit /b %ERRORLEVEL%

:RunDiagnoseCommand
if not exist "%DiagnosisScript%" (
    echo Could not find diagnosis script: "%DiagnosisScript%"
    exit /b 1
)

shift
call "%DiagnosisScript%" %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:ShowUsage
echo Usage:
echo   RunTests.bat
echo   RunTests.bat build
echo   RunTests.bat diagnose
echo   RunTests.bat diagnose sc2::TestAbilityRemap 6
echo   RunTests.bat full
echo   RunTests.bat regression
echo   RunTests.bat regression 1800
echo   RunTests.bat --timeout 1800
echo   RunTests.bat --filter "sc2::TestCommandAuthorityScheduling" --timeout 120
exit /b 0

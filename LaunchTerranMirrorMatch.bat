@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "BuildScript=%RepositoryRoot%\Build.bat"
set "BuildConfiguration=Debug"
set "SkipBuild=0"
set "MaxGameLoop=0"

:ParseArguments
if "%~1"=="" goto AfterParseArguments

if /I "%~1"=="--help" goto ShowUsage
if /I "%~1"=="-h" goto ShowUsage

if /I "%~1"=="--config" (
    if "%~2"=="" (
        echo Missing configuration value after --config.
        exit /b 1
    )
    set "BuildConfiguration=%~2"
    shift
    shift
    goto ParseArguments
)

if /I "%~1"=="--skip-build" (
    set "SkipBuild=1"
    shift
    goto ParseArguments
)

if /I "%~1"=="--max-loop" (
    if "%~2"=="" (
        echo Missing value after --max-loop.
        exit /b 1
    )
    set "MaxGameLoop=%~2"
    shift
    shift
    goto ParseArguments
)

echo Unrecognized extra argument: "%~1"
exit /b 1

:AfterParseArguments
if not exist "%BuildScript%" (
    echo Could not find build script: "%BuildScript%"
    exit /b 1
)

if "%SkipBuild%"=="0" (
    call "%BuildScript%" --config %BuildConfiguration% --target tutorial
    if errorlevel 1 (
        echo Build step failed.
        exit /b %ERRORLEVEL%
    )
)

set "BuildDirectory=%RepositoryRoot%\out\build\codex-x64-Debug"
set "TutorialExecutablePath=%BuildDirectory%\bin\tutorial.exe"
set "TutorialExecutableDirectory=%BuildDirectory%\bin"

if not exist "%TutorialExecutablePath%" (
    echo Could not find tutorial executable: "%TutorialExecutablePath%"
    exit /b 1
)

echo Cleaning up stale StarCraft II processes before launch...
taskkill /IM SC2_x64.exe /T /F >nul 2>&1
timeout /t 1 /nobreak >nul

echo Launching Terran mirror match (agent on both sides) from: "%TutorialExecutablePath%"

set "SC2_TUTORIAL_MIRROR_MATCH=1"
if not "%MaxGameLoop%"=="0" (
    set "SC2_TUTORIAL_MAX_GAME_LOOP=%MaxGameLoop%"
    echo Max game loop override: %MaxGameLoop%
)

pushd "%TutorialExecutableDirectory%" >nul
"%TutorialExecutablePath%"
set "ExitCode=%ERRORLEVEL%"
popd >nul

echo Cleaning up StarCraft II processes after launch...
taskkill /IM SC2_x64.exe /T /F >nul 2>&1

echo Exit code: %ExitCode%
exit /b %ExitCode%

:ShowUsage
echo Usage:
echo   LaunchTerranMirrorMatch.bat
echo   LaunchTerranMirrorMatch.bat --skip-build
echo   LaunchTerranMirrorMatch.bat --config Release
echo   LaunchTerranMirrorMatch.bat --max-loop 6000
exit /b 0

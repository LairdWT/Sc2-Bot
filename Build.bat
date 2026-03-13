@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "LogsDirectory=%RepositoryRoot%\out\logs"
set "BuildLogPath=%LogsDirectory%\BuildLog_latest.txt"
set "RequestedBuildDirectory="
set "BuildDirectory="
set "BuildConfiguration=Debug"
set "BuildTarget="
set "BuildVerbose=0"
set "CleanFirst=0"

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

if /I "%~1"=="--target" (
    if "%~2"=="" (
        echo Missing target value after --target.
        exit /b 1
    )
    set "BuildTarget=%~2"
    shift
    shift
    goto ParseArguments
)

if /I "%~1"=="--build-dir" (
    if "%~2"=="" (
        echo Missing directory value after --build-dir.
        exit /b 1
    )
    set "RequestedBuildDirectory=%~2"
    shift
    shift
    goto ParseArguments
)

if /I "%~1"=="--clean-first" (
    set "CleanFirst=1"
    shift
    goto ParseArguments
)

if /I "%~1"=="--verbose" (
    set "BuildVerbose=1"
    shift
    goto ParseArguments
)

if /I "%~1"=="Debug" (
    set "BuildConfiguration=Debug"
    shift
    goto ParseArguments
)

if /I "%~1"=="Release" (
    set "BuildConfiguration=Release"
    shift
    goto ParseArguments
)

if not defined BuildTarget (
    set "BuildTarget=%~1"
    shift
    goto ParseArguments
)

echo Unrecognized extra argument: "%~1"
exit /b 1

:AfterParseArguments
if not exist "%LogsDirectory%" mkdir "%LogsDirectory%"

if defined RequestedBuildDirectory (
    for %%I in ("%RequestedBuildDirectory%") do set "BuildDirectory=%%~fI"
) else (
    for /f "usebackq delims=" %%D in (`powershell -NoProfile -Command ^
        "$CandidateDirectory = Get-ChildItem -Path '%RepositoryRoot%\out\build' -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName 'CMakeCache.txt') } | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName; if ($CandidateDirectory) { Write-Output $CandidateDirectory }"`) do (
        set "BuildDirectory=%%D"
    )
)

if not defined BuildDirectory (
    set "BuildDirectory=%RepositoryRoot%\out\build\codex-x64-%BuildConfiguration%"
)

if not exist "%BuildDirectory%" mkdir "%BuildDirectory%"

if not exist "%BuildDirectory%\CMakeCache.txt" (
    echo Configuring build directory: "%BuildDirectory%"
    cmake -S "%RepositoryRoot%" -B "%BuildDirectory%"
    if errorlevel 1 (
        echo Configure step failed.
        exit /b 1
    )
)

set "TargetArgument="
if defined BuildTarget set "TargetArgument=--target %BuildTarget%"

set "CleanArgument="
if "%CleanFirst%"=="1" set "CleanArgument=--clean-first"

set "VerboseArgument="
if "%BuildVerbose%"=="1" set "VerboseArgument=--verbose"

echo Building from: "%BuildDirectory%"
echo Configuration: "%BuildConfiguration%"
if defined BuildTarget (
    echo Target: "%BuildTarget%"
) else (
    echo Target: default
)
echo Clean first: "%CleanFirst%"
echo Verbose: "%BuildVerbose%"
echo Writing build log to: "%BuildLogPath%"

powershell -NoProfile -Command ^
    "$ErrorActionPreference = 'Continue';" ^
    "cmake --build '%BuildDirectory%' --config %BuildConfiguration% %TargetArgument% %CleanArgument% %VerboseArgument% 2>&1 | Tee-Object -FilePath '%BuildLogPath%' -Append;" ^
    "exit $LASTEXITCODE"

set "ExitCode=%ERRORLEVEL%"
echo Exit code: %ExitCode%
echo Build log path: "%BuildLogPath%"
exit /b %ExitCode%

:ShowUsage
echo Usage:
echo   Build.bat
echo   Build.bat tutorial
echo   Build.bat --target all_tests --verbose
echo   Build.bat --target tutorial --clean-first
echo   Build.bat --config Release --target tutorial
echo   Build.bat --build-dir "L:\Sc2_Bot\out\build\codex-x64-Debug-20260311" --target all_tests
exit /b 0

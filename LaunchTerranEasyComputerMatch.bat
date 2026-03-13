@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "BuildScript=%RepositoryRoot%\Build.bat"
set "BuildConfiguration=Debug"
set "RequestedBuildDirectory="
set "BuildDirectory="
set "SkipBuild=0"
set "TutorialExecutablePath="
set "TutorialExecutableDirectory="

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

if /I "%~1"=="--skip-build" (
    set "SkipBuild=1"
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
    if defined RequestedBuildDirectory (
        call "%BuildScript%" --config %BuildConfiguration% --build-dir "%RequestedBuildDirectory%" --target tutorial
    ) else (
        call "%BuildScript%" --config %BuildConfiguration% --target tutorial
    )
    if errorlevel 1 (
        echo Build step failed.
        exit /b %ERRORLEVEL%
    )
)

if defined RequestedBuildDirectory (
    for %%I in ("%RequestedBuildDirectory%") do set "BuildDirectory=%%~fI"
) else (
    for /f "usebackq delims=" %%D in (`powershell -NoProfile -Command ^
        "$CandidateDirectory = Get-ChildItem -Path '%RepositoryRoot%\out\build' -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName 'bin\tutorial.exe') } | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName; if ($CandidateDirectory) { Write-Output $CandidateDirectory }"`) do (
        set "BuildDirectory=%%D"
    )
)

if not defined BuildDirectory (
    echo Could not find a build directory containing tutorial.exe under "%RepositoryRoot%\out\build".
    exit /b 1
)

set "TutorialExecutablePath=%BuildDirectory%\bin\tutorial.exe"
set "TutorialExecutableDirectory=%BuildDirectory%\bin"

if not exist "%TutorialExecutablePath%" (
    echo Could not find tutorial executable: "%TutorialExecutablePath%"
    exit /b 1
)

echo Launching visible Terran match versus Easy computer from: "%TutorialExecutablePath%"
pushd "%TutorialExecutableDirectory%" >nul
"%TutorialExecutablePath%"
set "ExitCode=%ERRORLEVEL%"
popd >nul

echo Exit code: %ExitCode%
exit /b %ExitCode%

:ShowUsage
echo Usage:
echo   LaunchTerranEasyComputerMatch.bat
echo   LaunchTerranEasyComputerMatch.bat --skip-build
echo   LaunchTerranEasyComputerMatch.bat --config Release
echo   LaunchTerranEasyComputerMatch.bat --build-dir "L:\Sc2_Bot\out\build\codex-x64-Debug-20260311"
exit /b 0

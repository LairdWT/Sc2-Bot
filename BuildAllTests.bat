@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "LogsDirectory=%RepositoryRoot%\out\logs"
set "BuildLogPath=%LogsDirectory%\BuildLog_latest.txt"
set "BuildDirectory="

if not exist "%LogsDirectory%" mkdir "%LogsDirectory%"

for /f "usebackq delims=" %%D in (`powershell -NoProfile -Command ^
    "$CandidateDirectory = Get-ChildItem -Path '%RepositoryRoot%\out\build' -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName 'bin\all_tests.exe') } | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName; if ($CandidateDirectory) { Write-Output $CandidateDirectory }"`) do (
    set "BuildDirectory=%%D"
)

if not defined BuildDirectory (
    echo Could not find a build directory containing all_tests.exe under "%RepositoryRoot%\out\build".
    exit /b 1
)

echo Building all_tests from: "%BuildDirectory%"
echo Writing build log to: "%BuildLogPath%"

powershell -NoProfile -Command "cmake --build '%BuildDirectory%' --config Debug --target all_tests --verbose *>&1 | Tee-Object -FilePath '%BuildLogPath%' -Append"

set "ExitCode=%ERRORLEVEL%"
echo Exit code: %ExitCode%
echo Build log path: "%BuildLogPath%"
exit /b %ExitCode%

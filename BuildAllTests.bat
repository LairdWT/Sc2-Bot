@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "BuildScript=%RepositoryRoot%\Build.bat"

if not exist "%BuildScript%" (
    echo Could not find build script: "%BuildScript%"
    exit /b 1
)

call "%BuildScript%" --target all_tests --verbose %*
exit /b %ERRORLEVEL%

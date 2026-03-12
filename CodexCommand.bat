@echo off
setlocal EnableExtensions

set "RepositoryRoot=%~dp0"
if "%RepositoryRoot:~-1%"=="\" set "RepositoryRoot=%RepositoryRoot:~0,-1%"

set "PrimaryCommand=regression"
set "CommandArgumentOne=1800"
set "CommandArgumentTwo="
set "CommandArgumentThree="
set "CommandArgumentFour="

call "%RepositoryRoot%\RunTests.bat" %PrimaryCommand% %CommandArgumentOne% %CommandArgumentTwo% %CommandArgumentThree% %CommandArgumentFour%
exit /b %ERRORLEVEL%

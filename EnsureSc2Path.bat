@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "InstallRootPath="
set "ShouldWriteUserEnvironment=1"
set "CurrentUserSc2Path="
set "VersionsDirectoryPath="
set "LatestBaseDirectoryName="
set "ResolvedExecutablePath="

call :ParseArguments %*
if errorlevel 1 exit /b 1

call :ResolveInstallRootPath
if errorlevel 1 exit /b 1

set "VersionsDirectoryPath=%InstallRootPath%\Versions"
if not exist "%VersionsDirectoryPath%\" (
    echo ERROR: Could not find the Versions directory under "%InstallRootPath%".
    exit /b 1
)

for /f "delims=" %%D in ('dir /b /ad /o:n "%VersionsDirectoryPath%\Base*" 2^>nul') do (
    set "LatestBaseDirectoryName=%%D"
)

if not defined LatestBaseDirectoryName (
    echo ERROR: No Base* directory was found under "%VersionsDirectoryPath%".
    exit /b 1
)

set "ResolvedExecutablePath=%VersionsDirectoryPath%\%LatestBaseDirectoryName%\SC2_x64.exe"
if not exist "%ResolvedExecutablePath%" (
    set "ResolvedExecutablePath=%VersionsDirectoryPath%\%LatestBaseDirectoryName%\SC2.exe"
)

if not exist "%ResolvedExecutablePath%" (
    echo ERROR: No StarCraft II executable was found in "%VersionsDirectoryPath%\%LatestBaseDirectoryName%".
    exit /b 1
)

for /f "skip=2 tokens=1,2,*" %%A in ('reg query HKCU\Environment /v SC2PATH 2^>nul') do (
    if /I "%%A"=="SC2PATH" (
        set "CurrentUserSc2Path=%%C"
    )
)

echo Install root : %InstallRootPath%
echo Latest build : %LatestBaseDirectoryName%
echo Resolved exe : %ResolvedExecutablePath%
if defined CurrentUserSc2Path (
    echo Current user SC2PATH : %CurrentUserSc2Path%
) else (
    echo Current user SC2PATH : ^<not set^>
)

if /I "%CurrentUserSc2Path%"=="%ResolvedExecutablePath%" (
    echo Result : SC2PATH is already up to date.
    exit /b 0
)

if "%ShouldWriteUserEnvironment%"=="0" (
    echo Result : Dry run only. SC2PATH would be updated.
    exit /b 0
)

setx SC2PATH "%ResolvedExecutablePath%" >nul
if errorlevel 1 (
    echo ERROR: Failed to update the user SC2PATH environment variable.
    exit /b 1
)

echo Result : Updated the user SC2PATH environment variable.
echo Note   : Open a new terminal or restart the app so the updated variable is visible.
exit /b 0

:ParseArguments
if "%~1"=="" exit /b 0

if /I "%~1"=="/?" (
    call :PrintUsage
    exit /b 1
)

if /I "%~1"=="-?" (
    call :PrintUsage
    exit /b 1
)

if /I "%~1"=="/NoWrite" (
    set "ShouldWriteUserEnvironment=0"
    shift
    goto :ParseArguments
)

if /I "%~1"=="--dry-run" (
    set "ShouldWriteUserEnvironment=0"
    shift
    goto :ParseArguments
)

if defined InstallRootPath (
    echo ERROR: Multiple install root paths were provided.
    call :PrintUsage
    exit /b 1
)

set "InstallRootPath=%~1"
shift
goto :ParseArguments

:ResolveInstallRootPath
if defined InstallRootPath (
    if exist "%InstallRootPath%\" (
        for %%P in ("%InstallRootPath%") do set "InstallRootPath=%%~fP"
        exit /b 0
    )

    echo ERROR: The supplied install root does not exist: "%InstallRootPath%"
    exit /b 1
)

if defined SC2PATH (
    for %%P in ("%SC2PATH%\..\..") do set "InstallRootPath=%%~fP"
    if exist "%InstallRootPath%\Versions\" exit /b 0
)

echo ERROR: No install root was provided and SC2PATH is not usable.
call :PrintUsage
exit /b 1

:PrintUsage
echo Usage:
echo   EnsureSc2Path.bat "C:\Path\To\StarCraft II"
echo   EnsureSc2Path.bat "C:\Path\To\StarCraft II" /NoWrite
echo   EnsureSc2Path.bat /NoWrite
echo.
echo Notes:
echo   - Provide the StarCraft II install root, not the executable path.
echo   - If no path is provided, the script derives the install root from the current SC2PATH value.
echo   - /NoWrite or --dry-run verifies the detected path without updating the user environment.
pause
exit /b 0

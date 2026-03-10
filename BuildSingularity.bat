@echo off
setlocal

set "RepoRoot=%~dp0"
pushd "%RepoRoot%" >nul

set "BuildConfig=Debug"
set "BuildTarget=tutorial"
set "CleanBuild=0"

:ParseArgs
if "%~1"=="" goto AfterArgs
if /I "%~1"=="clean" (
    set "CleanBuild=1"
    shift
    goto ParseArgs
)
if /I "%~1"=="Debug" (
    set "BuildConfig=Debug"
    shift
    goto ParseArgs
)
if /I "%~1"=="Release" (
    set "BuildConfig=Release"
    shift
    goto ParseArgs
)
set "BuildTarget=%~1"
shift
goto ParseArgs

:AfterArgs
set "BuildDir=out\build\x64-%BuildConfig%-rendered"
set "LegacyStampDir=out\build\x64-%BuildConfig%\_deps\civetweb-subbuild\civetweb-populate-prefix\src\civetweb-populate-stamp"
set "BuildStampDir=%BuildDir%\_deps\civetweb-subbuild\civetweb-populate-prefix\src\civetweb-populate-stamp"

echo Repo Root   : %RepoRoot%
echo Build Dir   : %BuildDir%
echo Config      : %BuildConfig%
echo Target      : %BuildTarget%
echo Clean Build : %CleanBuild%
echo.

if "%CleanBuild%"=="1" (
    echo Removing existing build directory...
    cmake -E rm -rf "%BuildDir%"
    if errorlevel 1 goto Fail
)

if exist "%LegacyStampDir%" (
    echo Touching legacy civetweb patch stamp...
    cmake -E touch "%LegacyStampDir%\civetweb-populate-patch"
    if errorlevel 1 goto Fail
)

if exist "%BuildStampDir%" (
    echo Touching build civetweb patch stamp...
    cmake -E touch "%BuildStampDir%\civetweb-populate-patch"
    if errorlevel 1 goto Fail
)

echo Configuring CMake...
cmake -S . -B "%BuildDir%" -DCMAKE_BUILD_TYPE=%BuildConfig%
if errorlevel 1 goto Fail

echo Building target %BuildTarget%...
cmake --build "%BuildDir%" --config "%BuildConfig%" --target "%BuildTarget%" --verbose
if errorlevel 1 goto Fail

echo.
echo Build completed successfully.
popd >nul
exit /b 0

:Fail
echo.
echo Build failed.
popd >nul
exit /b 1

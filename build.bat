@echo off
REM Simple script to build bin2cpp with VC++ 

:configure_v140
echo Configuring VC++ 2015...
if not exist "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" (
    echo Failed to located VC++ 2015!
    goto:eof
)
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" x86_amd64 || goto:eof
goto:msbuild

:msbuild
pushd build-msvc

REM we build on x86 to be compatible with non x64 Windows
msbuild bin2cpp.vcxproj /p:Configuration=Release /p:Platform=x86

set OUTDIR=Release
if not exist %OUTDIR%\bin2cpp.exe (
    echo Failure!
    goto:eof
)

mkdir bin 2> nul
copy /Y %OUTDIR%\bin2cpp.exe bin\

:clean
del /s /f /q %OUTDIR%
rd /s /q %OUTDIR%
rd /s /q x64
popd

echo.
echo Success!

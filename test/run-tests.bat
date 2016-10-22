@echo off
REm Unit tests on bin2cpp

set BIN2CPP=%~dp0..\build-msvc\bin\bin2cpp.exe
if not exist %BIN2CPP% exit /b 1

:test_invalid_command_line
REM test with no argument (should display help)
%BIN2CPP% || goto:command_line_check_failed
echo =======

%BIN2CPP% -h || goto:command_line_check_failed
echo =======

%BIN2CPP% -ns && goto:command_line_check_failed
echo =======

%BIN2CPP% -o && goto:command_line_check_failed
echo =======

REM test with invalid input file
%BIN2CPP% missing_file && goto:command_line_check_failed
echo =======

REM process file with default values
%BIN2CPP% golden_master.bin || goto:command_line_check_failed
if not exist bin2cpp.h goto:command_line_check_failed
if not exist bin2cpp.cpp goto:command_line_check_failed
del bin2cpp.h bin2cpp.cpp
echo =======

:test_generated_cpp
REM see test.cpp for details
mkdir files || goto:test_failed
copy golden_master.bin files\  || goto:test_failed
%BIN2CPP% -ns myNS -o generated files || goto:test_failed
if not exist generated.h goto:test_failed
if not exist generated.cpp goto:test_failed
del /q files\*
rd /q files

:build_test_cpp
call build-and-run-cpp-test.bat || goto:test_failed
del generated.h
del generated.cpp

REM OK!
exit /b 0

:command_line_check_failed
echo Command line check failed!

:test_failed
echo Test failed!
exit /b 1
@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

:: compilation settings
set CC=cl
set CFLAGS= /D _CRT_SECURE_NO_WARNINGS /nologo /W3
set LDFLAGS=/link /NOIMPLIB /NOEXP /LTCG git2.lib Advapi32.lib Winhttp.lib Ole32.lib Rpcrt4.lib Crypt32.lib zlib.lib pcre.lib

:: common shared sources
set COMPATSRC=reallocarray.c strlcat.c strlcpy.c

:: main
set "ARG=%~1"

if "%ARG%"=="" (
  call :build_target stagit
  echo -----
  call :build_target stagit-index
  goto :eof
)

if /I "%ARG%"=="stagit" (
  call :build_target stagit
  goto :eof
)

if /I "%ARG%"=="stagit-index" (
  call :build_target stagit-index
  goto :eof
)

if /I "%ARG%"=="clean" (
  call :clean
  goto :eof
)

if /I "%ARG%"=="clean-all" (
  call :clean_all
  goto :eof
)

:: call help
echo Comando desconocido: %ARG%
call :show_help
goto :eof

:: success msg
:success_msg
powershell -Command "Write-Host 'Build success: %BIN%.' -ForegroundColor Green"
goto:eof

:: failure msg
:failure_msg
powershell -Command "Write-Host 'Build failed: %BIN% not found.' -ForegroundColor Red"
goto :eof

:: compilation function
:build_target
set SRC=%1.c
set BIN=%1.exe
echo Building %BIN%...
%CC% /Fe:%BIN% %CFLAGS% %SRC% %COMPATSRC% %LDFLAGS%
if exist %BIN% (
  call :success_msg
) else (
  call :failure_msg
)
goto :eof

:: clean function
:clean
echo Cleaning *.obj...
del /q *.obj 2>nul
goto :eof

:: clean all function
:clean_all
echo Cleaning *.obj and *.exe...
del /q *.obj *.exe 2>nul
goto :eof

:: help function
:show_help
  echo Unknown command: %1
  echo Usage:
  echo   build.bat               - build all
  echo   build.bat stagit        - build stagit.exe
  echo   build.bat stagit-index  - build stagit-index.exe
  echo   build.bat clean         - delete .obj files
  echo   build.bat clean-all     - delete .obj and .exe files
  echo.
  echo Required libraries:
  echo   [MSVC/Windows SDK]
  echo     Advapi32.lib Winhttp.lib Ole32.lib Rpcrt4.lib Crypt32.lib
  echo   [External]
  echo     git2.lib zlib.lib pcre.lib
  echo.
  echo Tip: I used and recommend VCPKG to install the external libraries.
goto :eof

endlocal


@echo off

rem
rem Copyright (c) 2020 Rally Tactical Systems, Inc.
rem

setlocal enableextensions

set DESIRED_VERSION=%1
if "%DESIRED_VERSION%" == "" (
    goto showHelp
)

set BIN_PLATFORM=
if "%Platform%" == "" (
    echo Error: The 'Platform' environment variable is not defined
    goto end
) else if "%Platform%" == "x64" (
    set BIN_PLATFORM=win_x64
) else if "%Platform%" == "x86" (
    set BIN_PLATFORM=win_ia32
) else (
    echo Error: The 'Platform' environment variable value of "%Platform%" is not supported
    goto end
)


:checkIfVersionExistsAndExitIfNot
	set TMP_FILE=.\checkIfVersionExistsAndExitIfNot.tmp
	set ERROR_ENCOUNTERED=0

	call :doSafeDelete %TMP_FILE%
    curl -f -s -o %TMP_FILE% -L https://bintray.com/rallytac/pub/download_file?file_path=%DESIRED_VERSION%/api/c/include/EngageInterface.h"
	if errorlevel 1 (
		set ERROR_ENCOUNTERED=1
	)

	if not exist  %TMP_FILE% (
		set ERROR_ENCOUNTERED=1
	)

	call :doSafeDelete %TMP_FILE%

	if "%ERROR_ENCOUNTERED%" == "1" (
		echo Error encountered while checking for Engage version %DESIRED_VERSION%.  This may be an invalid version or a connection could not be established to the file publication system.
		goto end
	)


:fetchVersionFiles
    if exist engage (
        rd engage /s /q
    )
    
    md engage
    cd engage

    call :fetchBintrayFile %DESIRED_VERSION% api/c/include EngageInterface.h
	call :fetchBintrayFile %DESIRED_VERSION% api/c/include EngageIntegralDataTypes.h
	call :fetchBintrayFile %DESIRED_VERSION% api/c/include ConfigurationObjects.h
	call :fetchBintrayFile %DESIRED_VERSION% api/c/include Constants.h
	call :fetchBintrayFile %DESIRED_VERSION% api/c/include Platform.h
    call :fetchBintrayFile %DESIRED_VERSION% %BIN_PLATFORM% engage-shared.lib
    call :fetchBintrayFile %DESIRED_VERSION% %BIN_PLATFORM% engage-shared.dll

    goto end


:fetchBintrayFile
    echo Fetching %3 from %1/%2 ...
    call :doSafeDelete %3

    curl -f -s -o %3 -L https://bintray.com/rallytac/pub/download_file?file_path=%1/%2/%3
    if errorlevel 1 (
        call :doSafeDelete %3
        echo ERROR: Error while downloading %3
        goto end
    )

    exit /B 0


:doSafeDelete
    if exist %1 (
        del /q %1
    )
    exit /B 0    


:showHelp
    echo "usage: 'getengage.cmd <version_number>' or 'nmake /F depends VER=<version_number>' if called from nmake"
    goto end


:end
    endlocal

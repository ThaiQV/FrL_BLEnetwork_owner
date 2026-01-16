@echo off
setlocal enabledelayedexpansion

echo "*************** start of post build *****************"
echo "[MERGE] Bootloader and App ...."
set NAME="Unknown"
findstr /C:"//#define TBS_GATEWAY_DEVICE" ..\drivers.h >nul
if %errorlevel%==1 (
    set NAME="Gateway"
) else (
    for /f "tokens=3" %%A in ('findstr /C:"#define TBS_COUNTER_DEVICE" ..\drivers.h') do (
        set VALUE=%%A
        goto gotCounterValue
    )
    :gotCounterValue
    if "!VALUE!"=="0" (
    	set NAME="PowerMeter"
    ) else (
        set NAME="Counter"
    )
)

echo [INFO] PROJECT_BUILD = %NAME%

if /I %NAME%=="Gateway" (
	set BOOT=..\output/B91_DFU.bin
    set APP=..\output/FrL_Network_Master.bin
) else if /I %NAME%=="Counter" (
	set BOOT=..\output/B91_DFU.bin
    set APP=..\output/FrL_Network.bin
) else if /I %NAME%=="PowerMeter" (
	set BOOT=..\output/B91_DFU_PMT.bin
    set APP=..\output/FrL_Network.bin
)

set OUT=..\output/TBS_%NAME%.bin
set SREC=..\tools\srec\srec_cat.exe

%SREC% %BOOT% -binary -offset 0x00000 ^
        %APP% -binary -offset 0x10000 ^
        -fill 0xFF 0x9000 0x10000 ^
        -o %OUT% -binary

echo "[MERGE] FW: %OUT%"
echo "**************** end of post build ******************"
endlocal
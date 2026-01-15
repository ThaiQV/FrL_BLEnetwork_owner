@echo off
setlocal enabledelayedexpansion

echo "*************** start of post build *****************"
echo "[MERGE] Bootloader and App ...."
set NAME="Master"
findstr /r /c:"^[^/]*#define COUNTER_DEVICE" ..\vendor\TBS_dev\TBS_dev_config.h >nul
if %errorlevel%==0 (
   set NAME="Counter"
) else (
   findstr /r /c:"#define POWER_METER_DEVICE" ..\vendor\TBS_dev\TBS_dev_config.h >nul
   if %errorlevel%==0 (
   	set NAME="PowerMeter"
   )
)
echo [INFO] PROJECT_BUILD = %NAME%
if /I %NAME%=="Master" (
    set APP=..\output/FrL_Network_Master.bin
) else if /I %NAME%=="Counter" (
    set APP=..\output/FrL_Network.bin
) else if /I %NAME%=="PowerMeter" (
    set APP=..\output/FrL_Network.bin
)
set BOOT=..\output/B91_DFU.bin
set OUT=..\output/TBS_%NAME%.bin
set SREC=..\tools\srec\srec_cat.exe
%SREC% %BOOT% -binary -offset 0x00000 ^
		%APP% -binary -offset 0x10000 ^
		-fill 0xFF 0x9000 0x10000 ^
		-o %OUT% -binary
echo "[MERGE] FW: %OUT%"
echo "**************** end of post build ******************"
endlocal
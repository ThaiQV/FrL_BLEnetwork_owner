@echo off
setlocal enabledelayedexpansion

echo "*************** start of post build *****************"
echo "[MERGE] Bootloader and App ...."

for /f "tokens=3" %%a in ('findstr /r /c:"#define __NAME_PROJECT__" ..\vendor\TBS_dev\TBS_dev_config.h') do (
    set NAME=%%a
)
set NAME=!NAME:"=!

set SREC=..\tools\srec\srec_cat.exe
set BOOT=..\output\B91_DFU.bin
set APP=..\output\FrL_Network.bin
set OUT=..\output\!NAME!.bin

%SREC% %BOOT% -binary -offset 0x00000 ^
        %APP% -binary -offset 0x10000 ^
        -fill 0xFF 0x9000 0x10000 ^
        -o %OUT% -binary
echo "[MERGE] FW: %OUT%"

set BOOT=..\output\B91_DFU.bin
set APP=..\output\FrL_Network_Master.bin
set OUT=..\output\!NAME!_Gateway.bin

%SREC% %BOOT% -binary -offset 0x00000 ^
        %APP% -binary -offset 0x10000 ^
        -fill 0xFF 0x9000 0x10000 ^
        -o %OUT% -binary
echo "[MERGE] FW: %OUT%"

echo "**************** end of post build ******************"
endlocal
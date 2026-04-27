@echo off
setlocal enabledelayedexpansion

echo "*************** start of post build *****************"
echo "[MERGE] Bootloader and App ...."
set NAME="Unknown"
findstr /C:"//#define TBS_GATEWAY_DEVICE" ..\drivers.h >nul
if %errorlevel%==1 (
    set NAME=Gateway
) else (
    for /f "tokens=3" %%A in ('findstr /C:"#define TBS_COUNTER_DEVICE" ..\drivers.h') do (
        set VALUE=%%A
        goto gotCounterValue
    )
    :gotCounterValue
    if "!VALUE!"=="0" (
    	set NAME=PowerMeter
    ) else (
        set NAME=Counter
    )
)

echo [INFO] PROJECT_BUILD = %NAME%

set SREC=..\tools\srec\srec_cat.exe

:: Detect version from file code (main.c)
for /f "tokens=3-5 delims=,{};= " %%A in ('findstr /C:"fl_version_t _fw" ..\vendor\FrL_network\main.c') do (
    set MAJOR=%%A
    set MINOR=%%B
    set PATCH=%%C
)

set FW_VERSION=%MAJOR%.%MINOR%.%PATCH%

:: already config.txt
set OLD_VERSION_NUM=0
if exist ..\tools\fota\config.txt (
    for /f "tokens=2 delims=:" %%V in ('findstr /C:"version_num" ..\tools\fota\config.txt') do (
        set OLD_VERSION_NUM=%%V
        set OLD_VERSION_NUM=!OLD_VERSION_NUM: =!
    )
    for /f "tokens=2 delims=:" %%V in ('findstr /C:"fw_version" ..\tools\fota\config.txt') do (
        set OLD_FW_VERSION=%%V
        set OLD_FW_VERSION=!OLD_FW_VERSION: =!
    )
)

:: 
if "%FW_VERSION%"=="%OLD_FW_VERSION%" (
    set /a VERSION_NUM=%OLD_VERSION_NUM%
) else (
    set /a VERSION_NUM=%OLD_VERSION_NUM%+1
)

set OUT=..\output/TBS_%NAME%_%FW_VERSION%.bin

if /I %NAME%==Gateway (
	set BOOT=..\output/B91_DFU.bin
    set APP=..\output/FrL_Network_Master.bin
    set FILE=TBS_%NAME%_%FW_VERSION%.bin
    set OUTPUT=TBS_Gateway.bin
    set DEVICETYPE=2   
    set OUT_FOLDER=gw_ota
    riscv32-elf-objcopy -S -O binary FrL_Network_Master.elf  ..\tools\fota/TBS_Gateway.bin
) else if /I %NAME%==Counter (
	set BOOT=..\output/B91_DFU.bin
    set APP=..\output/FrL_Network.bin
    set FILE=TBS_%NAME%_%FW_VERSION%.bin
    set OUTPUT=TBS_Counter.bin
    set DEVICETYPE=0
    set OUT_FOLDER=counter_ota
    riscv32-elf-objcopy -S -O binary FrL_Network.elf  ..\tools\fota/TBS_Counter.bin
) else if /I %NAME%==PowerMeter (
	set BOOT=..\output/B91_DFU_PMT.bin
    set APP=..\output/FrL_Network.bin
    set FILE=TBS_%NAME%_%FW_VERSION%.bin
    set OUTPUT=TBS_PowerMeter.bin
    set DEVICETYPE=1
    set OUT_FOLDER=powermeter_ota
    riscv32-elf-objcopy -S -O binary FrL_Network.elf  ..\tools\fota/TBS_PowerMeter.bin
)

%SREC% %BOOT% -binary -offset 0x00000 ^
        %APP% -binary -offset 0x10000 ^
        -fill 0xFF 0x9000 0x10000 ^
        -o %OUT% -binary

echo [MERGE] FW: %OUT%

:: reload config.txt
(
echo file: %OUTPUT%
echo output: TBS_%NAME%_ota.bin
echo device type: %DEVICETYPE%
echo version_num: %VERSION_NUM%
echo fw_version: %FW_VERSION%
) > ..\tools\fota\config.txt
echo [INFO] config.txt created!(%VERSION_NUM%)

pushd ..\tools\fota
if not exist %OUT_FOLDER%_%FW_VERSION% mkdir %OUT_FOLDER%_%FW_VERSION%
create_otw_fw.exe >nul
del /q "TBS_*"
move *.bin %OUT_FOLDER%_%FW_VERSION%
::del /q "%OUT_FOLDER%_%FW_VERSION%\TBS_*"
xcopy "%OUT_FOLDER%_%FW_VERSION%" "..\..\output\%OUT_FOLDER%_%FW_VERSION%" /E /I /Y
rd /s /q "%OUT_FOLDER%_%FW_VERSION%"
popd
echo "**************** end of post build ******************"
endlocal
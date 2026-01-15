echo "*************** start of post build *****************"
echo "[MERGE] Bootloader and App ...."
set SREC=..\tools\srec\srec_cat.exe
set BOOT=..\output/B91_DFU.bin
set APP=..\output/FrL_Network.bin
set OUT=..\output/TBS_Counter.bin

%SREC% %BOOT% -binary -offset 0x00000 ^
        %APP% -binary -offset 0x10000 ^
        -fill 0xFF 0x9000 0x10000 ^
        -o %OUT% -binary
echo "[MERGE] FW: %OUT%"

set BOOT=..\output/B91_DFU.bin
set APP=..\output/FrL_Network_Master.bin
set OUT=..\output/TBS_Gateway.bin

%SREC% 	%BOOT% -binary -offset 0x00000 ^
        %APP% -binary -offset 0x10000 ^
        -fill 0xFF 0x9000 0x10000 ^
        -o %OUT% -binary
echo "[MERGE] FW: %OUT%"
echo "**************** end of post build ******************"
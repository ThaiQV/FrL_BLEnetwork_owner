echo "*************** start of post build *****************"
echo "this is post build!!! current configure is : %1"

riscv32-elf-objcopy -S -O binary %1.elf output\%1.bin
..\tl_check_fw2.exe output\%1.bin
riscv32-elf-objcopy -S -O binary %1.elf ..\output\%1.bin

if "%1"=="B91_DFU_PMT" (
    if exist "..\output\TBS_PowerMeter*" del /Q "..\output\TBS_PowerMeter*"
    if exist "..\output\FrL_Network.bin" del /Q "..\output\FrL_Network.bin"
) else (
    if exist "..\output\TBS_Counter*" del /Q "..\output\TBS_Counter*"
    if exist "..\output\TBS_Gateway*" del /Q "..\output\TBS_Gateway*"
    if exist "..\output\FrL_Network_Master.bin" del /Q "..\output\FrL_Network_Master.bin"
    if exist "..\output\FrL_Network.bin" del /Q "..\output\FrL_Network.bin"
)

echo "**************** end of post build ******************"
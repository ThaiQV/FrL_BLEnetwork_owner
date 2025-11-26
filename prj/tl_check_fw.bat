echo "*************** start of post build *****************"
echo "this is post build!!! current configure is :%1"
riscv32-elf-objcopy -S -O binary %1.elf  output/%1.bin
..\tl_check_fw2.exe  output/%1.bin
riscv32-elf-objcopy -S -O binary %1.elf  ..\output/%1.bin

call "%~dp0merge_app.bat"

echo "**************** end of post build ******************"
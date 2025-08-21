################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Freelux_libs/SPI_FLASH.c \
../Freelux_libs/fl_ble_wifi_protocol.c \
../Freelux_libs/fl_sys_datetime.c \
../Freelux_libs/nvm.c \
../Freelux_libs/os_queue.c \
../Freelux_libs/plog.c \
../Freelux_libs/queue_fifo.c \
../Freelux_libs/storage_weekly_data.c 

OBJS += \
./Freelux_libs/SPI_FLASH.o \
./Freelux_libs/fl_ble_wifi_protocol.o \
./Freelux_libs/fl_sys_datetime.o \
./Freelux_libs/nvm.o \
./Freelux_libs/os_queue.o \
./Freelux_libs/plog.o \
./Freelux_libs/queue_fifo.o \
./Freelux_libs/storage_weekly_data.o 

C_DEPS += \
./Freelux_libs/SPI_FLASH.d \
./Freelux_libs/fl_ble_wifi_protocol.d \
./Freelux_libs/fl_sys_datetime.d \
./Freelux_libs/nvm.d \
./Freelux_libs/os_queue.d \
./Freelux_libs/plog.d \
./Freelux_libs/queue_fifo.d \
./Freelux_libs/storage_weekly_data.d 


# Each subdirectory must supply rules for building sources it contributes
Freelux_libs/%.o: ../Freelux_libs/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=0 -D__PROJECT_B91_FREELUX_NWK__=1 -DMASTER_CORE=1 -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj/vendor/FrL_Network" -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj/Freelux_libs" -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



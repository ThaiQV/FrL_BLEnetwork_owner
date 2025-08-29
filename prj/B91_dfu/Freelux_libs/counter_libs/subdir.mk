################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Freelux_libs/counter_libs/SPI_FLASH.c \
../Freelux_libs/counter_libs/dfu.c \
../Freelux_libs/counter_libs/lcd16x2.c \
../Freelux_libs/counter_libs/lcd_app.c \
../Freelux_libs/counter_libs/led_7_seg.c \
../Freelux_libs/counter_libs/led_7_seg_app.c \
../Freelux_libs/counter_libs/nvm.c \
../Freelux_libs/counter_libs/os_queue.c \
../Freelux_libs/counter_libs/product_counter.c \
../Freelux_libs/counter_libs/storage_weekly_data.c \
../Freelux_libs/counter_libs/tca9555.c \
../Freelux_libs/counter_libs/tca9555_app.c 

OBJS += \
./Freelux_libs/counter_libs/SPI_FLASH.o \
./Freelux_libs/counter_libs/dfu.o \
./Freelux_libs/counter_libs/lcd16x2.o \
./Freelux_libs/counter_libs/lcd_app.o \
./Freelux_libs/counter_libs/led_7_seg.o \
./Freelux_libs/counter_libs/led_7_seg_app.o \
./Freelux_libs/counter_libs/nvm.o \
./Freelux_libs/counter_libs/os_queue.o \
./Freelux_libs/counter_libs/product_counter.o \
./Freelux_libs/counter_libs/storage_weekly_data.o \
./Freelux_libs/counter_libs/tca9555.o \
./Freelux_libs/counter_libs/tca9555_app.o 

C_DEPS += \
./Freelux_libs/counter_libs/SPI_FLASH.d \
./Freelux_libs/counter_libs/dfu.d \
./Freelux_libs/counter_libs/lcd16x2.d \
./Freelux_libs/counter_libs/lcd_app.d \
./Freelux_libs/counter_libs/led_7_seg.d \
./Freelux_libs/counter_libs/led_7_seg_app.d \
./Freelux_libs/counter_libs/nvm.d \
./Freelux_libs/counter_libs/os_queue.d \
./Freelux_libs/counter_libs/product_counter.d \
./Freelux_libs/counter_libs/storage_weekly_data.d \
./Freelux_libs/counter_libs/tca9555.d \
./Freelux_libs/counter_libs/tca9555_app.d 


# Each subdirectory must supply rules for building sources it contributes
Freelux_libs/counter_libs/%.o: ../Freelux_libs/counter_libs/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_DFU__=1 -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj/Freelux_libs/counter_libs" -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



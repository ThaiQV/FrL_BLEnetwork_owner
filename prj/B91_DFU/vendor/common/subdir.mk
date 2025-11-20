################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/common/battery_check.c \
../vendor/common/blt_common.c \
../vendor/common/blt_fw_sign.c \
../vendor/common/blt_led.c \
../vendor/common/blt_soft_timer.c \
../vendor/common/common_dbg.c \
../vendor/common/custom_pair.c \
../vendor/common/flash_fw_check.c 

OBJS += \
./vendor/common/battery_check.o \
./vendor/common/blt_common.o \
./vendor/common/blt_fw_sign.o \
./vendor/common/blt_led.o \
./vendor/common/blt_soft_timer.o \
./vendor/common/common_dbg.o \
./vendor/common/custom_pair.o \
./vendor/common/flash_fw_check.o 

C_DEPS += \
./vendor/common/battery_check.d \
./vendor/common/blt_common.d \
./vendor/common/blt_fw_sign.d \
./vendor/common/blt_led.d \
./vendor/common/blt_soft_timer.d \
./vendor/common/common_dbg.d \
./vendor/common/custom_pair.d \
./vendor/common/flash_fw_check.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/common/%.o: ../vendor/common/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=0 -D__PROJECT_B91_FREELUX_NWK__=0 -D__PROJECT_B91_DFU__=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



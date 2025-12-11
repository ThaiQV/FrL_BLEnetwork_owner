################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/TBS_dev/TBS_dev_app/driver/tca9555/tca9555.c 

OBJS += \
./vendor/TBS_dev/TBS_dev_app/driver/tca9555/tca9555.o 

C_DEPS += \
./vendor/TBS_dev/TBS_dev_app/driver/tca9555/tca9555.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/TBS_dev/TBS_dev_app/driver/tca9555/%.o: ../vendor/TBS_dev/TBS_dev_app/driver/tca9555/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=0 -D__PROJECT_B91_FREELUX_NWK__=1 -DHW_SAMPLE_TEST=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



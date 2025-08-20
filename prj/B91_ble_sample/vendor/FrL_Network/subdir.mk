################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/FrL_Network/app.c \
../vendor/FrL_Network/app_att.c \
../vendor/FrL_Network/app_buffer.c \
../vendor/FrL_Network/app_ui.c \
../vendor/FrL_Network/main.c 

OBJS += \
./vendor/FrL_Network/app.o \
./vendor/FrL_Network/app_att.o \
./vendor/FrL_Network/app_buffer.o \
./vendor/FrL_Network/app_ui.o \
./vendor/FrL_Network/main.o 

C_DEPS += \
./vendor/FrL_Network/app.d \
./vendor/FrL_Network/app_att.d \
./vendor/FrL_Network/app_buffer.d \
./vendor/FrL_Network/app_ui.d \
./vendor/FrL_Network/main.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/FrL_Network/%.o: ../vendor/FrL_Network/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



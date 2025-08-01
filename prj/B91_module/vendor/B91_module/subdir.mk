################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/B91_module/app.c \
../vendor/B91_module/app_att.c \
../vendor/B91_module/app_buffer.c \
../vendor/B91_module/main.c \
../vendor/B91_module/spp.c 

OBJS += \
./vendor/B91_module/app.o \
./vendor/B91_module/app_att.o \
./vendor/B91_module/app_buffer.o \
./vendor/B91_module/main.o \
./vendor/B91_module/spp.o 

C_DEPS += \
./vendor/B91_module/app.d \
./vendor/B91_module/app_att.d \
./vendor/B91_module/app_buffer.d \
./vendor/B91_module/main.d \
./vendor/B91_module/spp.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/B91_module/%.o: ../vendor/B91_module/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_MODULE__=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I../drivers/B91 -I../vendor/Common -I../common -O2 -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



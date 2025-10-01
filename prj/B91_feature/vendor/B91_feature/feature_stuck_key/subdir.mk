################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/B91_feature/feature_stuck_key/app.c \
../vendor/B91_feature/feature_stuck_key/app_buffer.c \
../vendor/B91_feature/feature_stuck_key/main.c 

OBJS += \
./vendor/B91_feature/feature_stuck_key/app.o \
./vendor/B91_feature/feature_stuck_key/app_buffer.o \
./vendor/B91_feature/feature_stuck_key/main.o 

C_DEPS += \
./vendor/B91_feature/feature_stuck_key/app.d \
./vendor/B91_feature/feature_stuck_key/app_buffer.d \
./vendor/B91_feature/feature_stuck_key/main.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/B91_feature/feature_stuck_key/%.o: ../vendor/B91_feature/feature_stuck_key/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_FEATURE_TEST__=1 -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I../drivers/B91 -I../vendor/Common -I../common -O2 -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



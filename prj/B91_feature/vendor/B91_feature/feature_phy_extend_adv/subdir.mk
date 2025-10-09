################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/B91_feature/feature_phy_extend_adv/app.c \
../vendor/B91_feature/feature_phy_extend_adv/app_buffer.c \
../vendor/B91_feature/feature_phy_extend_adv/main.c 

OBJS += \
./vendor/B91_feature/feature_phy_extend_adv/app.o \
./vendor/B91_feature/feature_phy_extend_adv/app_buffer.o \
./vendor/B91_feature/feature_phy_extend_adv/main.o 

C_DEPS += \
./vendor/B91_feature/feature_phy_extend_adv/app.d \
./vendor/B91_feature/feature_phy_extend_adv/app_buffer.d \
./vendor/B91_feature/feature_phy_extend_adv/main.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/B91_feature/feature_phy_extend_adv/%.o: ../vendor/B91_feature/feature_phy_extend_adv/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_FEATURE_TEST__=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I../drivers/B91 -I../vendor/Common -I../common -O2 -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



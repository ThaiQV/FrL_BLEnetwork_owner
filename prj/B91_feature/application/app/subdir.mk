################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../application/app/usbaud.c \
../application/app/usbcdc.c \
../application/app/usbkb.c \
../application/app/usbmouse.c 

OBJS += \
./application/app/usbaud.o \
./application/app/usbcdc.o \
./application/app/usbkb.o \
./application/app/usbmouse.o 

C_DEPS += \
./application/app/usbaud.d \
./application/app/usbcdc.d \
./application/app/usbkb.d \
./application/app/usbmouse.d 


# Each subdirectory must supply rules for building sources it contributes
application/app/%.o: ../application/app/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_FEATURE_TEST__=1 -I"/cygdrive/E/All_projects/TELINK/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I../drivers/B91 -I../vendor/Common -I../common -O2 -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



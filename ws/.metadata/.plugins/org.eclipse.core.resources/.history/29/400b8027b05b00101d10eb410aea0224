################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../common/usb_dbg/myudb_usb.c \
../common/usb_dbg/myudb_usbdesc.c 

OBJS += \
./common/usb_dbg/myudb_usb.o \
./common/usb_dbg/myudb_usbdesc.o 

C_DEPS += \
./common/usb_dbg/myudb_usb.d \
./common/usb_dbg/myudb_usbdesc.d 


# Each subdirectory must supply rules for building sources it contributes
common/usb_dbg/%.o: ../common/usb_dbg/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_FEATURE_TEST__=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I../drivers/B91 -I../vendor/Common -I../common -O2 -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



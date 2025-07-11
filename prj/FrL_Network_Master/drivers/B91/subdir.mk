################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/B91/adc.c \
../drivers/B91/aes.c \
../drivers/B91/analog.c \
../drivers/B91/audio.c \
../drivers/B91/clock.c \
../drivers/B91/flash.c \
../drivers/B91/gpio.c \
../drivers/B91/i2c.c \
../drivers/B91/lpc.c \
../drivers/B91/mdec.c \
../drivers/B91/npe.c \
../drivers/B91/pwm.c \
../drivers/B91/s7816.c \
../drivers/B91/spi.c \
../drivers/B91/stimer.c \
../drivers/B91/timer.c \
../drivers/B91/uart.c \
../drivers/B91/usbhw.c 

OBJS += \
./drivers/B91/adc.o \
./drivers/B91/aes.o \
./drivers/B91/analog.o \
./drivers/B91/audio.o \
./drivers/B91/clock.o \
./drivers/B91/flash.o \
./drivers/B91/gpio.o \
./drivers/B91/i2c.o \
./drivers/B91/lpc.o \
./drivers/B91/mdec.o \
./drivers/B91/npe.o \
./drivers/B91/pwm.o \
./drivers/B91/s7816.o \
./drivers/B91/spi.o \
./drivers/B91/stimer.o \
./drivers/B91/timer.o \
./drivers/B91/uart.o \
./drivers/B91/usbhw.o 

C_DEPS += \
./drivers/B91/adc.d \
./drivers/B91/aes.d \
./drivers/B91/analog.d \
./drivers/B91/audio.d \
./drivers/B91/clock.d \
./drivers/B91/flash.d \
./drivers/B91/gpio.d \
./drivers/B91/i2c.d \
./drivers/B91/lpc.d \
./drivers/B91/mdec.d \
./drivers/B91/npe.d \
./drivers/B91/pwm.d \
./drivers/B91/s7816.d \
./drivers/B91/spi.d \
./drivers/B91/stimer.d \
./drivers/B91/timer.d \
./drivers/B91/uart.d \
./drivers/B91/usbhw.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/B91/%.o: ../drivers/B91/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=0 -D__PROJECT_B91_FREELUX_NWK__=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



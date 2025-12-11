################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../3rd-party/freertos-V5/croutine.c \
../3rd-party/freertos-V5/event_groups.c \
../3rd-party/freertos-V5/list.c \
../3rd-party/freertos-V5/queue.c \
../3rd-party/freertos-V5/stream_buffer.c \
../3rd-party/freertos-V5/tasks.c \
../3rd-party/freertos-V5/timers.c 

OBJS += \
./3rd-party/freertos-V5/croutine.o \
./3rd-party/freertos-V5/event_groups.o \
./3rd-party/freertos-V5/list.o \
./3rd-party/freertos-V5/queue.o \
./3rd-party/freertos-V5/stream_buffer.o \
./3rd-party/freertos-V5/tasks.o \
./3rd-party/freertos-V5/timers.o 

C_DEPS += \
./3rd-party/freertos-V5/croutine.d \
./3rd-party/freertos-V5/event_groups.d \
./3rd-party/freertos-V5/list.d \
./3rd-party/freertos-V5/queue.d \
./3rd-party/freertos-V5/stream_buffer.d \
./3rd-party/freertos-V5/tasks.d \
./3rd-party/freertos-V5/timers.d 


# Each subdirectory must supply rules for building sources it contributes
3rd-party/freertos-V5/%.o: ../3rd-party/freertos-V5/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=0 -D__PROJECT_B91_FREELUX_NWK__=1 -DHW_SAMPLE_TEST=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



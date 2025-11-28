################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/B91_dfu/app.c \
../vendor/B91_dfu/app_att.c \
../vendor/B91_dfu/app_buffer.c \
../vendor/B91_dfu/app_ui.c \
../vendor/B91_dfu/fl_adv_proc.c \
../vendor/B91_dfu/fl_adv_repeat.c \
../vendor/B91_dfu/fl_input_ext.c \
../vendor/B91_dfu/fl_nwk_database.c \
../vendor/B91_dfu/fl_nwk_handler.c \
../vendor/B91_dfu/fl_nwk_master_handler.c \
../vendor/B91_dfu/fl_nwk_protocol.c \
../vendor/B91_dfu/fl_nwk_slave_handler.c \
../vendor/B91_dfu/main.c 

OBJS += \
./vendor/B91_dfu/app.o \
./vendor/B91_dfu/app_att.o \
./vendor/B91_dfu/app_buffer.o \
./vendor/B91_dfu/app_ui.o \
./vendor/B91_dfu/fl_adv_proc.o \
./vendor/B91_dfu/fl_adv_repeat.o \
./vendor/B91_dfu/fl_input_ext.o \
./vendor/B91_dfu/fl_nwk_database.o \
./vendor/B91_dfu/fl_nwk_handler.o \
./vendor/B91_dfu/fl_nwk_master_handler.o \
./vendor/B91_dfu/fl_nwk_protocol.o \
./vendor/B91_dfu/fl_nwk_slave_handler.o \
./vendor/B91_dfu/main.o 

C_DEPS += \
./vendor/B91_dfu/app.d \
./vendor/B91_dfu/app_att.d \
./vendor/B91_dfu/app_buffer.d \
./vendor/B91_dfu/app_ui.d \
./vendor/B91_dfu/fl_adv_proc.d \
./vendor/B91_dfu/fl_adv_repeat.d \
./vendor/B91_dfu/fl_input_ext.d \
./vendor/B91_dfu/fl_nwk_database.d \
./vendor/B91_dfu/fl_nwk_handler.d \
./vendor/B91_dfu/fl_nwk_master_handler.d \
./vendor/B91_dfu/fl_nwk_protocol.d \
./vendor/B91_dfu/fl_nwk_slave_handler.d \
./vendor/B91_dfu/main.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/B91_dfu/%.o: ../vendor/B91_dfu/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=0 -D__PROJECT_B91_FREELUX_NWK__=0 -D__PROJECT_B91_DFU__=1 -I"/cygdrive/E/All_projects/TELINK/TBS/FrL_BLEnetwork_owner/prj" -I"/cygdrive/E/All_projects/TELINK/TBS/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/E/All_projects/TELINK/TBS/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



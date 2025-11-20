################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/FrL_network/app.c \
../vendor/FrL_network/app_att.c \
../vendor/FrL_network/app_buffer.c \
../vendor/FrL_network/app_ui.c \
../vendor/FrL_network/fl_adv_proc.c \
../vendor/FrL_network/fl_adv_repeat.c \
../vendor/FrL_network/fl_ble_wifi.c \
../vendor/FrL_network/fl_input_ext.c \
../vendor/FrL_network/fl_nwk_database.c \
../vendor/FrL_network/fl_nwk_handler.c \
../vendor/FrL_network/fl_nwk_master_handler.c \
../vendor/FrL_network/fl_nwk_protocol.c \
../vendor/FrL_network/fl_nwk_slave_handler.c \
../vendor/FrL_network/fl_wifi2ble_fota.c \
../vendor/FrL_network/main.c \
../vendor/FrL_network/test_api.c \
../vendor/FrL_network/test_fota.c 

OBJS += \
./vendor/FrL_network/app.o \
./vendor/FrL_network/app_att.o \
./vendor/FrL_network/app_buffer.o \
./vendor/FrL_network/app_ui.o \
./vendor/FrL_network/fl_adv_proc.o \
./vendor/FrL_network/fl_adv_repeat.o \
./vendor/FrL_network/fl_ble_wifi.o \
./vendor/FrL_network/fl_input_ext.o \
./vendor/FrL_network/fl_nwk_database.o \
./vendor/FrL_network/fl_nwk_handler.o \
./vendor/FrL_network/fl_nwk_master_handler.o \
./vendor/FrL_network/fl_nwk_protocol.o \
./vendor/FrL_network/fl_nwk_slave_handler.o \
./vendor/FrL_network/fl_wifi2ble_fota.o \
./vendor/FrL_network/main.o \
./vendor/FrL_network/test_api.o \
./vendor/FrL_network/test_fota.o 

C_DEPS += \
./vendor/FrL_network/app.d \
./vendor/FrL_network/app_att.d \
./vendor/FrL_network/app_buffer.d \
./vendor/FrL_network/app_ui.d \
./vendor/FrL_network/fl_adv_proc.d \
./vendor/FrL_network/fl_adv_repeat.d \
./vendor/FrL_network/fl_ble_wifi.d \
./vendor/FrL_network/fl_input_ext.d \
./vendor/FrL_network/fl_nwk_database.d \
./vendor/FrL_network/fl_nwk_handler.d \
./vendor/FrL_network/fl_nwk_master_handler.d \
./vendor/FrL_network/fl_nwk_protocol.d \
./vendor/FrL_network/fl_nwk_slave_handler.d \
./vendor/FrL_network/fl_wifi2ble_fota.d \
./vendor/FrL_network/main.d \
./vendor/FrL_network/test_api.d \
./vendor/FrL_network/test_fota.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/FrL_network/%.o: ../vendor/FrL_network/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_B91_BLE_SAMPLE__=0 -D__PROJECT_B91_FREELUX_NWK__=0 -D__PROJECT_B91_DFU__=1 -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/portable/GCC/RISC-V" -I"/cygdrive/D/Job/Freelux/Working/TBS_Monitoring/FrL_BLEnetwork_owner/prj/3rd-party/freertos-V5/include" -I../drivers/B91 -I../vendor/Common -I../common -O2 -mcmodel=medium -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



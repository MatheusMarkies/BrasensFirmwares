################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../NFC/Target/lib_NDEF_config.c 

OBJS += \
./NFC/Target/lib_NDEF_config.o 

C_DEPS += \
./NFC/Target/lib_NDEF_config.d 


# Each subdirectory must supply rules for building sources it contributes
NFC/Target/%.o NFC/Target/%.su NFC/Target/%.cyclo: ../NFC/Target/%.c NFC/Target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32L011xx -c -I../Core/Inc -I"C:/Users/Matheus Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc/Legacy" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Device/ST/STM32L0xx/Include" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Include" -I../NFC -I../NFC/Target -I../NFC/App -I"C:/Users/Matheus Markies/STM32Cube/Repository//Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Middlewares/ST/lib_nfc/lib_NDEF/Core/inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository//Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Middlewares/ST/lib_nfc/Common/inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-NFC-2f-Target

clean-NFC-2f-Target:
	-$(RM) ./NFC/Target/lib_NDEF_config.cyclo ./NFC/Target/lib_NDEF_config.d ./NFC/Target/lib_NDEF_config.o ./NFC/Target/lib_NDEF_config.su

.PHONY: clean-NFC-2f-Target


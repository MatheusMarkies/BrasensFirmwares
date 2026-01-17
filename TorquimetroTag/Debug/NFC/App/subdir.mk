################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../NFC/App/app_x-cube-nfc4.c 

OBJS += \
./NFC/App/app_x-cube-nfc4.o 

C_DEPS += \
./NFC/App/app_x-cube-nfc4.d 


# Each subdirectory must supply rules for building sources it contributes
NFC/App/%.o NFC/App/%.su NFC/App/%.cyclo: ../NFC/App/%.c NFC/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32L011xx -c -I../Core/Inc -I"C:/Users/Matheus Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc/Legacy" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Device/ST/STM32L0xx/Include" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Include" -I../NFC -I../NFC/Target -I../NFC/App -I"C:/Users/Matheus Markies/STM32Cube/Repository//Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Middlewares/ST/lib_nfc/lib_NDEF/Core/inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository//Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Middlewares/ST/lib_nfc/Common/inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-NFC-2f-App

clean-NFC-2f-App:
	-$(RM) ./NFC/App/app_x-cube-nfc4.cyclo ./NFC/App/app_x-cube-nfc4.d ./NFC/App/app_x-cube-nfc4.o ./NFC/App/app_x-cube-nfc4.su

.PHONY: clean-NFC-2f-App


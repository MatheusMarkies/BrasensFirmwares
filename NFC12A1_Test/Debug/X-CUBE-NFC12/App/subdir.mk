################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../X-CUBE-NFC12/App/app_x-cube-nfc12.c 

OBJS += \
./X-CUBE-NFC12/App/app_x-cube-nfc12.o 

C_DEPS += \
./X-CUBE-NFC12/App/app_x-cube-nfc12.d 


# Each subdirectory must supply rules for building sources it contributes
X-CUBE-NFC12/App/%.o X-CUBE-NFC12/App/%.su X-CUBE-NFC12/App/%.cyclo: ../X-CUBE-NFC12/App/%.c X-CUBE-NFC12/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -DST25R500 -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../X-CUBE-NFC12/Target -I../Middlewares/ST/rfal/Inc -I../Drivers/BSP/Components/st25r500 -I../Middlewares/ST/rfal/Src -I../Drivers/BSP/NFC12A1 -I../X-CUBE-NFC12/App -I../Drivers/BSP/STM32F1xx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-X-2d-CUBE-2d-NFC12-2f-App

clean-X-2d-CUBE-2d-NFC12-2f-App:
	-$(RM) ./X-CUBE-NFC12/App/app_x-cube-nfc12.cyclo ./X-CUBE-NFC12/App/app_x-cube-nfc12.d ./X-CUBE-NFC12/App/app_x-cube-nfc12.o ./X-CUBE-NFC12/App/app_x-cube-nfc12.su

.PHONY: clean-X-2d-CUBE-2d-NFC12-2f-App


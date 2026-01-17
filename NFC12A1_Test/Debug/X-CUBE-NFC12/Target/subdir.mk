################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../X-CUBE-NFC12/Target/nfc_conf.c \
../X-CUBE-NFC12/Target/timer.c 

OBJS += \
./X-CUBE-NFC12/Target/nfc_conf.o \
./X-CUBE-NFC12/Target/timer.o 

C_DEPS += \
./X-CUBE-NFC12/Target/nfc_conf.d \
./X-CUBE-NFC12/Target/timer.d 


# Each subdirectory must supply rules for building sources it contributes
X-CUBE-NFC12/Target/%.o X-CUBE-NFC12/Target/%.su X-CUBE-NFC12/Target/%.cyclo: ../X-CUBE-NFC12/Target/%.c X-CUBE-NFC12/Target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -DST25R500 -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../X-CUBE-NFC12/Target -I../Middlewares/ST/rfal/Inc -I../Drivers/BSP/Components/st25r500 -I../Middlewares/ST/rfal/Src -I../Drivers/BSP/NFC12A1 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-X-2d-CUBE-2d-NFC12-2f-Target

clean-X-2d-CUBE-2d-NFC12-2f-Target:
	-$(RM) ./X-CUBE-NFC12/Target/nfc_conf.cyclo ./X-CUBE-NFC12/Target/nfc_conf.d ./X-CUBE-NFC12/Target/nfc_conf.o ./X-CUBE-NFC12/Target/nfc_conf.su ./X-CUBE-NFC12/Target/timer.cyclo ./X-CUBE-NFC12/Target/timer.d ./X-CUBE-NFC12/Target/timer.o ./X-CUBE-NFC12/Target/timer.su

.PHONY: clean-X-2d-CUBE-2d-NFC12-2f-Target


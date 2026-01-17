################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../NFC/custom_nfc04a1.c \
../NFC/custom_nfc04a1_nfctag.c 

OBJS += \
./NFC/custom_nfc04a1.o \
./NFC/custom_nfc04a1_nfctag.o 

C_DEPS += \
./NFC/custom_nfc04a1.d \
./NFC/custom_nfc04a1_nfctag.d 


# Each subdirectory must supply rules for building sources it contributes
NFC/%.o NFC/%.su NFC/%.cyclo: ../NFC/%.c NFC/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32L011xx -c -I../Core/Inc -I"C:/Users/Matheus Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc/Legacy" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Device/ST/STM32L0xx/Include" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Include" -I../NFC -I../NFC/Target -I../NFC/App -I"C:/Users/Matheus Markies/STM32Cube/Repository//Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Middlewares/ST/lib_nfc/lib_NDEF/Core/inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository//Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Middlewares/ST/lib_nfc/Common/inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-NFC

clean-NFC:
	-$(RM) ./NFC/custom_nfc04a1.cyclo ./NFC/custom_nfc04a1.d ./NFC/custom_nfc04a1.o ./NFC/custom_nfc04a1.su ./NFC/custom_nfc04a1_nfctag.cyclo ./NFC/custom_nfc04a1_nfctag.d ./NFC/custom_nfc04a1_nfctag.o ./NFC/custom_nfc04a1_nfctag.su

.PHONY: clean-NFC


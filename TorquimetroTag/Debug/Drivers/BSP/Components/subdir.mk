################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Matheus\ Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/ST25DV/st25dv.c \
C:/Users/Matheus\ Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/ST25DV/st25dv_reg.c 

OBJS += \
./Drivers/BSP/Components/st25dv.o \
./Drivers/BSP/Components/st25dv_reg.o 

C_DEPS += \
./Drivers/BSP/Components/st25dv.d \
./Drivers/BSP/Components/st25dv_reg.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/st25dv.o: C:/Users/Matheus\ Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/ST25DV/st25dv.c Drivers/BSP/Components/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32L011xx -c -I../Core/Inc -I"C:/Users/Matheus Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc/Legacy" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Device/ST/STM32L0xx/Include" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Include" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Drivers/BSP/Components/st25dv_reg.o: C:/Users/Matheus\ Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/ST25DV/st25dv_reg.c Drivers/BSP/Components/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32L011xx -c -I../Core/Inc -I"C:/Users/Matheus Markies/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-NFC4/3.0.0/Drivers/BSP/Components/" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/STM32L0xx_HAL_Driver/Inc/Legacy" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Device/ST/STM32L0xx/Include" -I"C:/Users/Matheus Markies/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.3/Drivers/CMSIS/Include" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-Components

clean-Drivers-2f-BSP-2f-Components:
	-$(RM) ./Drivers/BSP/Components/st25dv.cyclo ./Drivers/BSP/Components/st25dv.d ./Drivers/BSP/Components/st25dv.o ./Drivers/BSP/Components/st25dv.su ./Drivers/BSP/Components/st25dv_reg.cyclo ./Drivers/BSP/Components/st25dv_reg.d ./Drivers/BSP/Components/st25dv_reg.o ./Drivers/BSP/Components/st25dv_reg.su

.PHONY: clean-Drivers-2f-BSP-2f-Components


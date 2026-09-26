################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/touch/gt911.c 

OBJS += \
./Drivers/touch/gt911.o 

C_DEPS += \
./Drivers/touch/gt911.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/touch/%.o Drivers/touch/%.su Drivers/touch/%.cyclo: ../Drivers/touch/%.c Drivers/touch/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I"C:/CIUFFOLI/Software/STM32/Module/FK743M2-IIT6-test3/Drivers/touch" -I"C:/CIUFFOLI/Software/STM32/Module/FK743M2-IIT6-test3/Drivers" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../Core/Inc/Images -O0 -ffunction-sections -fdata-sections -Wall -mno-unaligned-access -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-touch

clean-Drivers-2f-touch:
	-$(RM) ./Drivers/touch/gt911.cyclo ./Drivers/touch/gt911.d ./Drivers/touch/gt911.o ./Drivers/touch/gt911.su

.PHONY: clean-Drivers-2f-touch


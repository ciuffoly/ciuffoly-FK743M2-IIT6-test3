################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/ui/images.c \
../Core/ui/screens.c \
../Core/ui/styles.c \
../Core/ui/ui.c 

OBJS += \
./Core/ui/images.o \
./Core/ui/screens.o \
./Core/ui/styles.o \
./Core/ui/ui.o 

C_DEPS += \
./Core/ui/images.d \
./Core/ui/screens.d \
./Core/ui/styles.d \
./Core/ui/ui.d 


# Each subdirectory must supply rules for building sources it contributes
Core/ui/%.o Core/ui/%.su Core/ui/%.cyclo: ../Core/ui/%.c Core/ui/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I"C:/CIUFFOLI/Software/STM32/Module/FK743M2-IIT6-test2/Drivers/touch" -I"C:/CIUFFOLI/Software/STM32/Module/FK743M2-IIT6-test2/Core/ui" -I"C:/CIUFFOLI/Software/STM32/Module/FK743M2-IIT6-test2/Drivers" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../Core/Inc/Images -O0 -ffunction-sections -fdata-sections -Wall -mno-unaligned-access -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-ui

clean-Core-2f-ui:
	-$(RM) ./Core/ui/images.cyclo ./Core/ui/images.d ./Core/ui/images.o ./Core/ui/images.su ./Core/ui/screens.cyclo ./Core/ui/screens.d ./Core/ui/screens.o ./Core/ui/screens.su ./Core/ui/styles.cyclo ./Core/ui/styles.d ./Core/ui/styles.o ./Core/ui/styles.su ./Core/ui/ui.cyclo ./Core/ui/ui.d ./Core/ui/ui.o ./Core/ui/ui.su

.PHONY: clean-Core-2f-ui


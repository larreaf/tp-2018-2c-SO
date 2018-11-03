################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../consola.c \
../fifa.c \
../mdj.c \
../mdj_functions.c 

OBJS += \
./consola.o \
./fifa.o \
./mdj.o \
./mdj_functions.o 

C_DEPS += \
./consola.d \
./fifa.d \
./mdj.d \
./mdj_functions.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2018-2c-Ensalada-C-sar/ensalada" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



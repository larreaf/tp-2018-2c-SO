################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../consola.c \
../pcp.c \
../plp.c \
../sAFA.c \
../servidor_safa.c 

OBJS += \
./consola.o \
./pcp.o \
./plp.o \
./sAFA.o \
./servidor_safa.o 

C_DEPS += \
./consola.d \
./pcp.d \
./plp.d \
./sAFA.d \
./servidor_safa.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-2c-Ensalada-C-sar/ensalada" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



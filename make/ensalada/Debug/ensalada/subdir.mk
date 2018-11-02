################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ensalada/com.c \
../ensalada/dtb.c \
../ensalada/mensaje.c \
../ensalada/protocolo.c \
../ensalada/servidor.c \
../ensalada/validacion.c 

OBJS += \
./ensalada/com.o \
./ensalada/dtb.o \
./ensalada/mensaje.o \
./ensalada/protocolo.o \
./ensalada/servidor.o \
./ensalada/validacion.o 

C_DEPS += \
./ensalada/com.d \
./ensalada/dtb.d \
./ensalada/mensaje.d \
./ensalada/protocolo.d \
./ensalada/servidor.d \
./ensalada/validacion.d 


# Each subdirectory must supply rules for building sources it contributes
ensalada/%.o: ../ensalada/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



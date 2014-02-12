################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../log_test/log_test.c 

OBJS += \
./log_test/log_test.o 

C_DEPS += \
./log_test/log_test.d 


# Each subdirectory must supply rules for building sources it contributes
log_test/%.o: ../log_test/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/michael/cplusplus/TestBed/includes" -include"/home/michael/cplusplus/TestBed/includes/udt.h" -includegdsl -include"/home/michael/cplusplus/TestBed/includes/utp.h" -include"/home/michael/cplusplus/TestBed/includes/utypes.h" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



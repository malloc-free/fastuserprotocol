################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../affinity_test/aff_test.c 

OBJS += \
./affinity_test/aff_test.o 

C_DEPS += \
./affinity_test/aff_test.d 


# Each subdirectory must supply rules for building sources it contributes
affinity_test/%.o: ../affinity_test/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/michael/cplusplus/TestBed/includes" -include"/home/michael/cplusplus/TestBed/includes/udt.h" -includegdsl -include"/home/michael/cplusplus/TestBed/includes/utp.h" -include"/home/michael/cplusplus/TestBed/includes/utypes.h" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



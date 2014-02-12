################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/tb_client.c \
../src/tb_common.c \
../src/tb_epoll.c \
../src/tb_file_io.c \
../src/tb_listener.c \
../src/tb_logging.c \
../src/tb_protocol.c \
../src/tb_server.c \
../src/tb_session.c \
../src/tb_sock_opt.c \
../src/tb_testbed.c \
../src/tb_udp.c \
../src/tb_utp.c \
../src/tb_worker.c \
../src/tb_worker_pair.c 

OBJS += \
./src/tb_client.o \
./src/tb_common.o \
./src/tb_epoll.o \
./src/tb_file_io.o \
./src/tb_listener.o \
./src/tb_logging.o \
./src/tb_protocol.o \
./src/tb_server.o \
./src/tb_session.o \
./src/tb_sock_opt.o \
./src/tb_testbed.o \
./src/tb_udp.o \
./src/tb_utp.o \
./src/tb_worker.o \
./src/tb_worker_pair.o 

C_DEPS += \
./src/tb_client.d \
./src/tb_common.d \
./src/tb_epoll.d \
./src/tb_file_io.d \
./src/tb_listener.d \
./src/tb_logging.d \
./src/tb_protocol.d \
./src/tb_server.d \
./src/tb_session.d \
./src/tb_sock_opt.d \
./src/tb_testbed.d \
./src/tb_udp.d \
./src/tb_utp.d \
./src/tb_worker.d \
./src/tb_worker_pair.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/michael/cplusplus/TestBed/includes" -include"/home/michael/cplusplus/TestBed/includes/udt.h" -includegdsl -include"/home/michael/cplusplus/TestBed/includes/utp.h" -include"/home/michael/cplusplus/TestBed/includes/utypes.h" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



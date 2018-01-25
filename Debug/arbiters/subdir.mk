################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../arbiters/arbiter.cpp \
../arbiters/matrix_arb.cpp \
../arbiters/prio_arb.cpp \
../arbiters/roundrobin_arb.cpp 

OBJS += \
./arbiters/arbiter.o \
./arbiters/matrix_arb.o \
./arbiters/prio_arb.o \
./arbiters/roundrobin_arb.o 

CPP_DEPS += \
./arbiters/arbiter.d \
./arbiters/matrix_arb.d \
./arbiters/prio_arb.d \
./arbiters/roundrobin_arb.d 


# Each subdirectory must supply rules for building sources it contributes
arbiters/%.o: ../arbiters/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



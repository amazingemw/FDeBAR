################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Johns/iq_router_base.cpp \
../Johns/iq_router_baseline.cpp 

OBJS += \
./Johns/iq_router_base.o \
./Johns/iq_router_baseline.o 

CPP_DEPS += \
./Johns/iq_router_base.d \
./Johns/iq_router_baseline.d 


# Each subdirectory must supply rules for building sources it contributes
Johns/%.o: ../Johns/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../bgui.cpp \
../booksim_config.cpp \
../buffer_state.cpp \
../config_tab.cpp \
../config_utils.cpp \
../configlex.cpp \
../credit.cpp \
../flit.cpp \
../flitchannel.cpp \
../injection.cpp \
../main.cpp \
../misc_utils.cpp \
../module.cpp \
../outputset.cpp \
../power_module.cpp \
../random_utils.cpp \
../rng.cpp \
../rng_double.cpp \
../rng_double_wrapper.cpp \
../rng_wrapper.cpp \
../routefunc.cpp \
../stats.cpp \
../traffic.cpp \
../trafficmanager.cpp \
../vc.cpp 

C_SRCS += \
../config.tab.c 

OBJS += \
./bgui.o \
./booksim_config.o \
./buffer_state.o \
./config.tab.o \
./config_tab.o \
./config_utils.o \
./configlex.o \
./credit.o \
./flit.o \
./flitchannel.o \
./injection.o \
./main.o \
./misc_utils.o \
./module.o \
./outputset.o \
./power_module.o \
./random_utils.o \
./rng.o \
./rng_double.o \
./rng_double_wrapper.o \
./rng_wrapper.o \
./routefunc.o \
./stats.o \
./traffic.o \
./trafficmanager.o \
./vc.o 

C_DEPS += \
./config.tab.d 

CPP_DEPS += \
./bgui.d \
./booksim_config.d \
./buffer_state.d \
./config_tab.d \
./config_utils.d \
./configlex.d \
./credit.d \
./flit.d \
./flitchannel.d \
./injection.d \
./main.d \
./misc_utils.d \
./module.d \
./outputset.d \
./power_module.d \
./random_utils.d \
./rng.d \
./rng_double.d \
./rng_double_wrapper.d \
./rng_wrapper.d \
./routefunc.d \
./stats.d \
./traffic.d \
./trafficmanager.d \
./vc.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



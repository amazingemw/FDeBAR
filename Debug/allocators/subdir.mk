################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../allocators/allocator.cpp \
../allocators/islip.cpp \
../allocators/loa.cpp \
../allocators/maxsize.cpp \
../allocators/pim.cpp \
../allocators/selalloc.cpp \
../allocators/separable.cpp \
../allocators/separable_input_first.cpp \
../allocators/separable_output_first.cpp \
../allocators/wavefront.cpp 

OBJS += \
./allocators/allocator.o \
./allocators/islip.o \
./allocators/loa.o \
./allocators/maxsize.o \
./allocators/pim.o \
./allocators/selalloc.o \
./allocators/separable.o \
./allocators/separable_input_first.o \
./allocators/separable_output_first.o \
./allocators/wavefront.o 

CPP_DEPS += \
./allocators/allocator.d \
./allocators/islip.d \
./allocators/loa.d \
./allocators/maxsize.d \
./allocators/pim.d \
./allocators/selalloc.d \
./allocators/separable.d \
./allocators/separable_input_first.d \
./allocators/separable_output_first.d \
./allocators/wavefront.d 


# Each subdirectory must supply rules for building sources it contributes
allocators/%.o: ../allocators/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



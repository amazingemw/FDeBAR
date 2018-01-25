################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../networks/anynet.cpp \
../networks/cmesh.cpp \
../networks/cmeshx2.cpp \
../networks/cmo.cpp \
../networks/dragonfly.cpp \
../networks/fattree.cpp \
../networks/flatfly_onchip.cpp \
../networks/fly.cpp \
../networks/isolated_mesh.cpp \
../networks/kncube.cpp \
../networks/mecs.cpp \
../networks/network.cpp \
../networks/qtree.cpp \
../networks/singlenet.cpp \
../networks/tree4.cpp 

OBJS += \
./networks/anynet.o \
./networks/cmesh.o \
./networks/cmeshx2.o \
./networks/cmo.o \
./networks/dragonfly.o \
./networks/fattree.o \
./networks/flatfly_onchip.o \
./networks/fly.o \
./networks/isolated_mesh.o \
./networks/kncube.o \
./networks/mecs.o \
./networks/network.o \
./networks/qtree.o \
./networks/singlenet.o \
./networks/tree4.o 

CPP_DEPS += \
./networks/anynet.d \
./networks/cmesh.d \
./networks/cmeshx2.d \
./networks/cmo.d \
./networks/dragonfly.d \
./networks/fattree.d \
./networks/flatfly_onchip.d \
./networks/fly.d \
./networks/isolated_mesh.d \
./networks/kncube.d \
./networks/mecs.d \
./networks/network.d \
./networks/qtree.d \
./networks/singlenet.d \
./networks/tree4.d 


# Each subdirectory must supply rules for building sources it contributes
networks/%.o: ../networks/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



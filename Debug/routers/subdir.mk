################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../routers/MECSChannels.cpp \
../routers/MECSCombiner.cpp \
../routers/MECSCreditChannel.cpp \
../routers/MECSCreditCombiner.cpp \
../routers/MECSCreditForwarder.cpp \
../routers/MECSForwarder.cpp \
../routers/MECSRouter.cpp \
../routers/chaos_router.cpp \
../routers/event_router.cpp \
../routers/iq_router_base.cpp \
../routers/iq_router_baseline.cpp \
../routers/iq_router_combined.cpp \
../routers/iq_router_split.cpp \
../routers/router.cpp 

OBJS += \
./routers/MECSChannels.o \
./routers/MECSCombiner.o \
./routers/MECSCreditChannel.o \
./routers/MECSCreditCombiner.o \
./routers/MECSCreditForwarder.o \
./routers/MECSForwarder.o \
./routers/MECSRouter.o \
./routers/chaos_router.o \
./routers/event_router.o \
./routers/iq_router_base.o \
./routers/iq_router_baseline.o \
./routers/iq_router_combined.o \
./routers/iq_router_split.o \
./routers/router.o 

CPP_DEPS += \
./routers/MECSChannels.d \
./routers/MECSCombiner.d \
./routers/MECSCreditChannel.d \
./routers/MECSCreditCombiner.d \
./routers/MECSCreditForwarder.d \
./routers/MECSForwarder.d \
./routers/MECSRouter.d \
./routers/chaos_router.d \
./routers/event_router.d \
./routers/iq_router_base.d \
./routers/iq_router_baseline.d \
./routers/iq_router_combined.d \
./routers/iq_router_split.d \
./routers/router.d 


# Each subdirectory must supply rules for building sources it contributes
routers/%.o: ../routers/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/ch32v30x_it.c \
../User/main.c \
../User/medbox_schedule.c \
../User/medbox_ui.c \
../User/system_ch32v30x.c 

C_DEPS += \
./User/ch32v30x_it.d \
./User/main.d \
./User/medbox_schedule.d \
./User/medbox_ui.d \
./User/system_ch32v30x.d 

OBJS += \
./User/ch32v30x_it.o \
./User/main.o \
./User/medbox_schedule.o \
./User/medbox_ui.o \
./User/system_ch32v30x.o 

DIR_OBJS += \
./User/*.o \

DIR_DEPS += \
./User/*.d \

DIR_EXPANDS += \
./User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/Users/29213/mounriver-studio-projects/medbox/Debug" -I"c:/Users/29213/mounriver-studio-projects/medbox/Core" -I"c:/Users/29213/mounriver-studio-projects/medbox/User" -I"c:/Users/29213/mounriver-studio-projects/medbox/Peripheral/inc" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/SPI" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/LCD" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/TOUCH" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/GUI" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/DHT11" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/SGP30" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/ESP8266" -I"c:/Users/29213/mounriver-studio-projects/medbox/Hardware/BH1750" -I"c:/Users/29213/mounriver-studio-projects/medbox/Network" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"


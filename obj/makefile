################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include Startup/subdir.mk
-include Peripheral/src/subdir.mk
-include Hardware/TOUCH/subdir.mk
-include Hardware/SPI/subdir.mk
-include Hardware/SGP30/subdir.mk
-include Hardware/LCD/subdir.mk
-include Hardware/GUI/subdir.mk
-include Hardware/ESP8266/subdir.mk
-include Hardware/DHT11/subdir.mk
-include Hardware/BH1750/subdir.mk
-include Debug/subdir.mk
-include Core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
medbox.hex \

SECONDARY_LIST += \
medbox.lst \

SECONDARY_SIZE += \
medbox.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: medbox.elf secondary-outputs

# Tool invocations
medbox.elf: $(OBJS) $(USER_OBJS_ESCAPE)
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "c:\Users\29213\mounriver-studio-projects\medbox\Ld\Link.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,"medbox.map" --specs=nano.specs --specs=nosys.specs -o "medbox.elf" $(OBJS) $(LIBS)
medbox.hex: medbox.elf
	@	riscv-none-embed-objcopy -O ihex "medbox.elf" "medbox.hex"
medbox.lst: medbox.elf
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble -M xw "medbox.elf" > "medbox.lst"
medbox.siz: medbox.elf
	riscv-none-embed-size --format=berkeley "medbox.elf"

# Other Targets
clean:
	-$(RM) $(DIR_OBJS) $(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) medbox.elf
	-$(RM) $(DIR_EXPANDS) $(CALLGRAPH_DOT)
	-$(RM) $(DIR_DEPS)
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
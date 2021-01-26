ifeq ($(RELEASE),1)
CONFIGURATION	:= release
RELEASE_OPT		?= 3
OPTIMIZATION	:= -O$(RELEASE_OPT) -g
else
CONFIGURATION	:= debug
OPTIMIZATION	:= -Og -g
DEFS			+= DEBUG
endif

ROOTDIR		:= $(dir $(firstword $(MAKEFILE_LIST)))
BASEDIR		:= $(dir $(lastword $(MAKEFILE_LIST)))
MXDIR		:= $(ROOTDIR)$(TARGET).cubemx/
OUTPUT_DIR	:= $(ROOTDIR)build/$(CONFIGURATION)/
OBJDIR		:= $(OUTPUT_DIR)/obj

INCDIRS		+= \
$(BASEDIR)inc \
$(BASEDIR)inc/base \
$(BASEDIR)inc/base/internal \
$(BASEDIR)chip \

SOURCES		+= \
$(BASEDIR)src/abi.cpp \
$(BASEDIR)src/build_information.cpp \
$(BASEDIR)src/fault_handler.c \
$(BASEDIR)src/hash.cpp \
$(BASEDIR)src/std.cpp 




DEFS		+= STM32_PCLK1=$(STM32_PCLK1) STM32_TIMCLK1=$(STM32_TIMCLK1)


ifeq ($(RTOS),none)
DEFS		+= UAVCAN_STM32_BAREMETAL=1 FW_USE_RTOS=0 FW_TYPE_BAREMETAL
else ifeq ($(RTOS),freertos)
DEFS		+= UAVCAN_STM32_FREERTOS=1 FW_USE_RTOS=1 FW_TYPE_FREERTOS
else
$(error Unknown firmware type)
endif

# CubeMX
include $(TARGET).cubemx/Makefile

SOURCES += $(foreach source,$(C_SOURCES) $(ASM_SOURCES),$(MXDIR)$(source))
INCDIRS += $(C_INCLUDES:-I%=$(MXDIR)%)
INCDIRS += $(AS_INCLUDES:-I%=$(MXDIR)%)

LDSCRIPT := $(MXDIR)$(LDSCRIPT)

# Tracealyzer
ifeq ($(TRACEALYZER_SUPPORT), 1)
ifeq ($(RTOS),freertos)
INCDIRS	+= $(BASEDIR)Tracealyzer/Include
SOURCES	+= $(wildcard $(BASEDIR)Tracealyzer/Src/*.c)
endif
endif

include $(BASEDIR)mk/build-info.mk

# Include actual rules
include $(BASEDIR)mk/rules.mk

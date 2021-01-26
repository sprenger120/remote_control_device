TARGET  := remote_control_device
RTOS    := freertos
DEVICE  := stm32f302r8

STM32_PCLK1     := 32000000
STM32_TIMCLK1   := 64000000
DEFS += BUILDCONFIG_EMBEDDED_BUILD=1

# generate object dictionary
DICTIONARY_FILE := objectDictionary/RemoteControlDevice.od
DICTIONARY_OUT := objectDictionary/generatedOD/RemoteControlDevice.c
$(info $(shell python2 canfestival/objdictgen/objdictgen.py $(DICTIONARY_FILE) $(DICTIONARY_OUT)))

# DEFS += DEBUG_ERR_CONSOLE_ON=1 # canfestival logging

include canfestival/canfestival.mk

INCDIRS         := src/CanFestival \
                   $(CANFESTIVAL_INC) \
                   src \
                   objectDictionary \
                   span/include

SOURCES         := $(CANFESTIVAL_SRC) \
                   src/CanFestival/CanFestivalTimers.cpp \
                   src/CanFestival/CanFestivalLocker.cpp \
                   src/CanFestival/CanFestivalLogging.cpp \
                   src/PeripheralDrivers/CanIO.cpp \
                   src/PeripheralDrivers/TerminalIO.cpp \
                   src/PeripheralDrivers/ReceiverModule.cpp \
                   src/Statemachine/Statemachine.cpp \
                   src/Statemachine/Canopen.cpp \
                   src/Statemachine/RemoteControl.cpp \
                   src/Statemachine/HardwareSwitches.cpp \
                   src/Statemachine/LEDUpdater.cpp \
                   src/Statemachine/State.cpp \
                   src/Statemachine/States.cpp \
                   src/Statemachine/StateSources.cpp \
                   src/SpecialAssert.cpp \
                   src/Logging.cpp \
                   src/SBUSDecoder.cpp \
                   src/LEDs.cpp \
                   src/Wrapper/Sync.cpp \
                   src/Application.cpp \
                   src/Wrapper/Task.cpp



TRACEALYZER_SUPPORT := 0

# Actual build engine
include stm32_project_base/include.mk


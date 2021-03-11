cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0048 NEW)

# fixes compiler detection with arm-none-eabi-gcc as cmake tries to
# build an executable but bare metal doesn't work like this
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(remote_control_device VERSION 0.0.1 LANGUAGES CXX C ASM)



# enable cross compiling for arm-none-eabi
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
#todo dont hardcode compiler paths but find them
set(ToolsPrefix /usr/bin/arm-none-eabi)
set(CMAKE_C_COMPILER ${ToolsPrefix}-gcc CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER ${ToolsPrefix}-g++ CACHE INTERNAL "")
set(CMAKE_MAKE_PROGRAM /usr/bin/make CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)



# run specifically prepared make file that outputs valid cmake code and load
execute_process(COMMAND make --no-print-directory -C ${PROJECT_SOURCE_DIR}/remote_control_device.cubemx/
        OUTPUT_FILE ${CMAKE_CACHEFILE_DIR}/MakeExport.cmake)
include(${CMAKE_CACHEFILE_DIR}/MakeExport.cmake)



# generate canfestival's object dictionary code
set(CANFESTIVAL_PATH canfestival)
function(BUILD_OD name)
    set(filename "objectDictionary/generatedOD/${name}.c")
    message("-- Generating ${filename}")
    execute_process(COMMAND python2 "${PROJECT_SOURCE_DIR}/${CANFESTIVAL_PATH}/objdictgen/objdictgen.py"
            "${PROJECT_SOURCE_DIR}/objectDictionary/${name}.od"
            "${PROJECT_SOURCE_DIR}/${filename}")
    set(CANFESTIVAL_OD_FILES "${filename};${CANFESTIVAL_OD_FILES}" PARENT_SCOPE)
endfunction()
BUILD_OD("RemoteControlDevice")



# convert whitespace separated entries to list
# prefix so path is correct
set(FirmwareName remote_control_device)
set(HalDir ${FirmwareName}.cubemx)
string(REPLACE "-I" "" MakeExport_AS_INCLUDES ${MakeExport_AS_INCLUDES})
string(REPLACE "-I" "" MakeExport_C_INCLUDES ${MakeExport_C_INCLUDES})
string(REPLACE " " ";" MakeExport_AS_INCLUDES ${MakeExport_AS_INCLUDES})
string(REPLACE " " ";" MakeExport_C_INCLUDES ${MakeExport_C_INCLUDES})
list(TRANSFORM MakeExport_AS_INCLUDES PREPEND "${HalDir}/")
list(TRANSFORM MakeExport_C_INCLUDES PREPEND "${HalDir}/")
include_directories(
    ${CANFESTIVAL_PATH}/include
    src
    objectDictionary
    span/include
    ${MakeExport_AS_INCLUDES}
    ${MakeExport_C_INCLUDES}
)



string(REPLACE " " ";" MakeExport_C_SOURCES ${MakeExport_C_SOURCES})
string(REPLACE " " ";" MakeExport_ASM_SOURCES ${MakeExport_ASM_SOURCES})
list(TRANSFORM MakeExport_C_SOURCES PREPEND "${HalDir}/")
list(TRANSFORM MakeExport_ASM_SOURCES PREPEND "${HalDir}/")
set(SOURCE_FILES
        src/CanFestival/CanFestivalTimers.cpp
	src/CanFestival/CanFestivalLocker.cpp
	src/CanFestival/CanFestivalLogging.cpp
	src/PeripheralDrivers/CanIO.cpp
	src/PeripheralDrivers/TerminalIO.cpp
	src/PeripheralDrivers/ReceiverModule.cpp
	src/Statemachine/Statemachine.cpp
	src/Statemachine/Canopen.cpp
	src/Statemachine/RemoteControl.cpp
	src/Statemachine/HardwareSwitches.cpp
	src/Statemachine/LEDUpdater.cpp
	src/Statemachine/State.cpp
	src/Statemachine/States.cpp
	src/Statemachine/StateSources.cpp
	src/SpecialAssert.cpp
	src/Logging.cpp
	src/SBUSDecoder.cpp
	src/LEDs.cpp
	src/Wrapper/Sync.cpp
	src/Application.cpp
	src/Wrapper/Task.cpp
	src/FirmwareHasher.cpp
	${MakeExport_C_SOURCES}
	${MakeExport_ASM_SOURCES}
	stm32_project_base/src/abi.cpp
	stm32_project_base/src/build_information.cpp
	stm32_project_base/src/fault_handler.c
	stm32_project_base/src/hash.cpp
	stm32_project_base/src/std.cpp
        ${CANFESTIVAL_PATH}/src/dcf.c
	${CANFESTIVAL_PATH}/src/emcy.c
	${CANFESTIVAL_PATH}/src/lifegrd.c
	${CANFESTIVAL_PATH}/src/lss.c
	${CANFESTIVAL_PATH}/src/nmtMaster.c
	${CANFESTIVAL_PATH}/src/nmtSlave.c
	${CANFESTIVAL_PATH}/src/objacces.c
	${CANFESTIVAL_PATH}/src/pdo.c
	${CANFESTIVAL_PATH}/src/sdo.c
	${CANFESTIVAL_PATH}/src/states.c
	${CANFESTIVAL_PATH}/src/sync.c
)



# embedded build
# todo only build when parameter is set
# todo add test app target
set(Cstd c11)
set(CXXstd c++17)
set(Optimisation -Os)
set(Specs --specs=nano.specs --specs=nosys.specs)
set(Defs BUILDCONFIG_EMBEDDED_BUILD=1 DEBUG)

set(CFlags -std=${Cstd} ${Optimisation} -fno-builtin-log)

set(CppFlags -ffunction-sections -fdata-sections -fno-common
        -pedantic -Wall -Wextra
        -Wno-unused-parameter -Wno-unused-variable
        -fexec-charset=cp1252
        ${Specs})

set(CxxFlags -std=${CXXstd} ${Optimisation}
        -fno-exceptions -fno-rtti -fno-unwind-tables -Wno-register -fno-math-errno)

add_executable(${FirmwareName} ${SOURCE_FILES})

set_target_properties(
        ${FirmwareName}
        PROPERTIES
        OUTPUT_NAME ${FirmwareName}
        SUFFIX ".elf"
)

target_include_directories(${FirmwareName} PRIVATE canopen_stack_canfestival/include
        src
        objectDictionary
        span/include
        stm32_project_base/inc
        canopen_stack_canfestival/include
        src/CanFestival
        stm32_project_base/inc/base)


target_compile_options(${FirmwareName} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:${CFlags} ${CppFlags} ${MakeExport_MCU}>
        $<$<COMPILE_LANGUAGE:CXX>:${CxxFlags} ${CppFlags} ${MakeExport_MCU}>
        $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp ${CFlags} ${CppFlags} ${MakeExport_MCU}>
)

target_compile_definitions(${FirmwareName} PRIVATE ${Defs} ${MakeExport_C_DEFS} ${MakeExport_AS_DEFS})

target_link_options(${FirmwareName} PRIVATE
        ${Specs}
        --static
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -Wl,-Map=${FirmwareName}.map
        -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
        -T${PROJECT_SOURCE_DIR}/${HalDir}/${MakeExport_LDSCRIPT}
        ${MakeExport_MCU}
        )

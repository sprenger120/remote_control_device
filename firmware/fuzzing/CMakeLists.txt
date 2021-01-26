cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0048 NEW)
project(fuzzing VERSION 0.0.1 LANGUAGES CXX C)

add_definitions(-DUSE_HAL_DRIVER)
add_definitions(-DSTM32F302x8)
add_definitions(-Dremote_control_device_TESTING)
add_definitions(-Dremote_control_device_FUZZING)
#add_definitions(-DDEBUG_ERR_CONSOLE_ON)
#add_definitions(-DDEBUG_WAR_CONSOLE_ON)
#add_definitions(-DTESTING_LOGGING)

set(CMAKE_CXX_STANDARD 17)

# -Wno-int-to-pointer-cast suppresses warnings from HAL code that wants to access registers
# this code isn't even used in the tests so nothing to worry

# -Wno-deprecated-declarations is neccessary as std::uncaught_exception (used within gtest)
# is deprecated in c++17 and has to be upgraded by google to not throw these errors
set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations -Wno-int-to-pointer-cast")

include_directories( 
    ../test/inc

    # canfestival
    ../src/CanFestival
    ../objectDictionary
    ../canopen_stack_canfestival/include

    #freertos
    ../test/gcc_FreeRTOS_posix_port
    ../test/inc/freertos

    # rcd files
    ../src

    # cubehal headers without freertos, danger zone
    ../remote_control_device.cubemx/Core/Inc
    ../remote_control_device.cubemx/Drivers/STM32F3xx_HAL_Driver/Inc
    ../remote_control_device.cubemx/Drivers/STM32F3xx_HAL_Driver/Inc/Legacy
    ../remote_control_device.cubemx/Drivers/CMSIS/Device/ST/STM32F3xx/Include
    ../remote_control_device.cubemx/Drivers/CMSIS/Include
    ../remote_control_device.cubemx/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2

    # base
    ../stm32_project_base/inc
)


add_executable(fuzzapp
# testapp
src/main.cpp

# hal stubs
../test/stub/hal.cpp
../test/stub/can.cpp
../test/stub/uart.cpp
../test/stub/gpio.cpp
../test/stub/tim.cpp

# freertos fakes
../test/fake/task.cpp

# freertos simulator
../test/gcc_FreeRTOS_posix_port/utils/wait_for_event.c
../test/gcc_FreeRTOS_posix_port/port.c
../test/gcc_FreeRTOS_posix_port/croutine.c
../test/gcc_FreeRTOS_posix_port/event_groups.c
../test/gcc_FreeRTOS_posix_port/heap_1.c
../test/gcc_FreeRTOS_posix_port/list.c
../test/gcc_FreeRTOS_posix_port/port.c
../test/gcc_FreeRTOS_posix_port/queue.c
../test/gcc_FreeRTOS_posix_port/stream_buffer.c
../test/gcc_FreeRTOS_posix_port/tasks.c
../test/gcc_FreeRTOS_posix_port/timers.c
../test/gcc_FreeRTOS_posix_port/hooks_test.cpp


# canfestival 
../canopen_stack_canfestival/src/dcf.c
../canopen_stack_canfestival/src/emcy.c 
../canopen_stack_canfestival/src/lifegrd.c 
../canopen_stack_canfestival/src/lss.c 
../canopen_stack_canfestival/src/nmtMaster.c 
../canopen_stack_canfestival/src/nmtSlave.c 
../canopen_stack_canfestival/src/objacces.c 
../canopen_stack_canfestival/src/pdo.c 
../canopen_stack_canfestival/src/sdo.c 
../canopen_stack_canfestival/src/states.c 
../canopen_stack_canfestival/src/sync.c 

# firmware
../src/SpecialAssert.cpp
../src/CanFestival/CanFestivalLocker.cpp
../src/Sync.cpp
../src/CanFestival/CanFestivalTimers.cpp
../src/PeripheralDrivers/CanIO.cpp
../src/Logging.cpp
../src/PeripheralDrivers/TerminalIO.cpp
../src/PeripheralDrivers/ReceiverModule.cpp
../src/SBUSDecoder.cpp
../src/Statemachine/HardwareSwitches.cpp
../src/Statemachine/RemoteControl.cpp
../src/LEDs.cpp
../src/Statemachine/Canopen.cpp
../src/CanFestival/CanFestivalLogging.cpp
../src/Statemachine/Statemachine.cpp
../src/Statemachine/LEDUpdater.cpp
../src/Statemachine/State.cpp
../src/Statemachine/StateSources.cpp
../src/StackHighMarkPrinter.cpp

# base
../stm32_project_base/src/build_information.cpp
)

target_link_libraries(fuzzapp pthread)
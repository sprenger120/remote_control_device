cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0048 NEW)
project(my_gtest_pkgconfig VERSION 0.0.1 LANGUAGES CXX C)

find_package(PkgConfig)
pkg_search_module(GTEST REQUIRED gtest)
pkg_search_module(GMOCK REQUIRED gmock)

add_definitions(-DUSE_HAL_DRIVER)
add_definitions(-DSTM32F302x8)
add_definitions(-DBUILDCONFIG_TESTING_BUILD)
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
    # mocking 
    ../canopen_stack_canfestival/hippomocks/HippoMocks
    inc

    # canfestival
    ../src/CanFestival
    ../objectDictionary
    ../canopen_stack_canfestival/include

    #freertos
    gcc_FreeRTOS_posix_port
    inc/freertos

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

    ../span/include
)


add_executable(testapp 
# testapp
src/main.cpp

# hal stubs
stub/hal.cpp
stub/can.cpp
stub/uart.cpp
stub/gpio.cpp
stub/tim.cpp
stub/nvic.cpp
stub/iwdg.cpp

# freertos fakes
fake/task.cpp

# freertos simulator
gcc_FreeRTOS_posix_port/utils/wait_for_event.c
gcc_FreeRTOS_posix_port/port.c
gcc_FreeRTOS_posix_port/croutine.c
gcc_FreeRTOS_posix_port/event_groups.c
gcc_FreeRTOS_posix_port/heap_1.c
gcc_FreeRTOS_posix_port/list.c
gcc_FreeRTOS_posix_port/port.c
gcc_FreeRTOS_posix_port/queue.c
gcc_FreeRTOS_posix_port/stream_buffer.c
gcc_FreeRTOS_posix_port/tasks.c
gcc_FreeRTOS_posix_port/timers.c
gcc_FreeRTOS_posix_port/hooks_test.cpp


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
../src/Wrapper/Sync.cpp
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
../src/Statemachine/States.cpp
../src/Statemachine/StateSources.cpp
../src/Wrapper/Task.cpp

# base
../stm32_project_base/src/build_information.cpp


#  test
src/CanFestivalTimersTest.cpp
src/CanIOTest.cpp
src/TestDataSBUSFrame.cpp
src/ReceiverModuleTest.cpp
src/SBUSDecoderTest.cpp
src/HardwareSwitchesTest.cpp
src/RemoteControlTest.cpp
src/LEDTest.cpp
src/LoggingTest.cpp
src/TerminalIOTest.cpp
src/Canopen/CanopenTestFixture.cpp
src/Canopen/CanopenTestFixture.hpp
src/StatemachineTest.cpp
src/LEDUpdaterTest.cpp
src/Canopen/MapValueTest.cpp
src/Canopen/PDOPublishingTest.cpp
src/Canopen/HeartbeatMonitoringTest.cpp
src/Canopen/ClientNodeStateChangeTest.cpp
src/Canopen/CouplingChangeSDOTest.cpp
src/Canopen/MalformedSyncCrashTest.cpp
src/Canopen/CouplingChangeSDOTest.hpp inc/mock/CanFestivalTimersMock.hpp inc/mock/LoggingMock.hpp inc/mock/ReceiverModuleMock.h stub/iwdg.cpp)


target_link_libraries(testapp ${GTEST_LDFLAGS} ${GMOCK_LDFLAGS})
target_compile_options(testapp PUBLIC ${GTEST_CFLAGS} ${GMOCK_CFLAGS})

include(CTest)
add_test(first_and_only_test testapp)
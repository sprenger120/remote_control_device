# Testing

This project is quite challenging to unit test as much of the code communicates with the hardware of the chip. 
Testing on the device itself is not feasible as there is hardly enough FLASH for the programm itself. Also testing 
environments don't play nicely with the multi-task structure necessary to not lose sanity while programming
embedded.  
The solution I've come up with is multifaceted

1. Design the application so that global variables / state is minimized, not required for the testable code itself
2. Task's run functions are abstracted so a test can run them 
3. Use an x86_64 port of FreeRTOS for basic functionality like Mutexes, Queues, Streams
4. Include HAL's headers for compatiblility but use stubs for implementation
5. Run the test environment within FreeRTOS task context to not immediately SEGFAULT when a blocking function is called
6. Use a test environment that runs everything sequentially on one core


### 1. Global Variables

To communicate with the chip's perihperals you need to have some feedback. This is provided in the form of interrupt service routines. Internally these are just jump instructions to parts of the code. CubeHAL does a good job of simplifying handling these routines but only offers C bindings for registering hooks. 
To interface with C++ code I've added static callback functions which rely on an instance() function which in turn relies on the class being a singleton and thus reside in global scope. This is horrible for testing as you want to reset the 
class after every test case to rule out side effects from test to test. As a solution instance() is only available
in embedded context and any HAL global variables need to be given to a class by constructor so the tests don't rely on 
HAL externs.


### 2. Abstracted run functions

Every FreeRTOS task has a run function in which the logic is executed. A task may be interrupted by the scheduler and 
later resumed. It can also block itself, waiting for a signal to continue execution. This multi-tasked structure is not 
compatible with testing as waiting for asynchronous events would be a nightmare and produce convoluted, brittle tests. 
So every class hosting logic for a task implements a dispatch() function which takes signal flags which can come from
actual interrupt service routines in embedded context or from a unit test. Also dispatch() is not allowed to block 
the task and must rely on embedded-only outside code to do it for them. 

### 3. x86_64 FreeRTOS

I've tried to add stubs / fakes for FreeRTOS but the amount of missing symbols would make this take way too long. 
Luckily FreeRTOS is written to be compatible with hundreds of architectures / compilers and there is a port for normal
PCs. Setting it wasn't that difficult and required a low amount of boiler plate code. The task system itself is not actively used within the tests (meaning its only there for code compatibility). Queues, Mutexes, Semaphores, Streams all work nicely
and testing is made quite a bit more fun when something doesn't immediately break upon usage.

### 4. HAL Headers

Headers of CubeHAL are used everywhere but the implementations will not run on x86_64 as they often write to registers
which are just hard-coded memory addresses. Trying to access this on a normal pc will immediately SEGFAULT the programm.
So to not write my fingers down to the bone just the headers are included as there is hardly any register writing there
and to satisfy the linker stubs are added. (The test's CMakeList file ignores some warnings because of register writing
macros. These are never used inside the code.)

### 5. gTest in FreeRTOS task 

Just a small workaround for FreeRTOS functions crashing the programm when not executed from within a task's context.
See test/src/main.cpp.

### 6. Sequential test execution

Most of the tests don't care about order of execution with the exception of the Canopen unit tests which require CanFestival to be initialized which in turn requires CanFestivalTimers and the absolute mess that is CO_Data. See for 
yourself in src/CanFestival/CanFestivalLocker.cpp what it takes to reset the OD.


# Test Coverage

The application consists of the following units

1. **CanFestival** brings it's own test code which is executed in CI. The libraries manual states that it is tested compatible with CiA 301 / 405 so I consider it tested enough. For bugs that appeared in production test cases are created in test/.

2. **HAL Code** is maintained by ST Microelectronics. Testing this would be an absolute nightmare and is way out of scope of this project. The code is manually tested to do what it is supposed to do and errors in prodution are fixed by hand.

3. **FreeRTOS** is maintained by Amazon and thus tested rigourosly by them. Any serious bugs won't make it far as there are many devices running this kernel.

4. **Main Logic** (Statemachine. Peripheral Drivers, Support Code) tests can be found in test/. Code that is expected to run in ISR contex can't be tested. Task interactions are also not 
covered by tests as it would require much setup for little benefit. Every major component has multiple test cases to ensure functionality.

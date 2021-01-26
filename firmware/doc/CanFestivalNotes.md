# Notes on canfestival usage/changes in this project

### Linking, Including

Canfestival is designed to be either linked statically for embedded devices or dynamically for everything else. 
I chose not to use the supplied build system as its quicker to just create a quick Makefile include (canfestival/canfestival.mk) 
and add the library directly.
This poses some challenges as all the includes are not prefixed by e.g. canfestival so there are conflicts with the CubeMX generated code 
under remote_control_device.cubemx. I've changed all includes inside the library to point to canfestival/...h.  

### Driver 

The library also comes with driver code even with CMSIS compatible ones. As CubeMX auto generates setup code for the CAN device, 
using the canfestival driver code would be redundant. All I have to add some small functions for sending.

### Scheduling / Timers

Inside the library there is a micro scheduler for timed events that just uses a hardware timer's interrupt. This is problematic
in this project as I'm using an RTOS and canfestival is not thread safe (+ you can't aquire mutexes inside an interrupt). 
I've replaced the whole subsystem with the canFestivalTimer task.

### Configuration

Canfestival requires four configuration files to work. These have been copied from multiple places of the library and modified to 
suit the needs of the project. You can find them in src/canfestival_config/canfestival/*.h

### objdictgen

CANopen nodes require an object dictionary and canfestival brings and editor (objdictedit.py) and a code generator for .c/.h files (objdictgen.py).
The library is somewhat old and these programms are written with python 2 and might be hard to run later.   

objdictgen.py only requires the gnosis xml parser to run. As pip is removing support for python2 soon I've added the complete library for 
"future-proofing" at least this part. Python2 itself should be available at least for some time more.   

objdictedit.py is trickier as it requires wxPython which itself is a wrapper for some gui stuff. As long as wx has python2 support there should be
no problem. It might be required to port it though.


### Testing

There are some tests supplied with the library to ensure basic functionality. gTest is used and can be installed with your favorite packet manager. 
HippoMock used to be retrieved with a power shell script. To avoid this nonsense I've added the framework directly.

I've also added a CMake configuration to build the test app. As I'm not using the drivers of the library I also needed to add some
empty functions so the linker is satisfied and HippoMock can work its magic.  

Testing will be done with gitlab CI
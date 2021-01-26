### Specific Documentation

- [CanFestival usage](firmware/doc/CanFestivalNotes.md)
- [Software Structure](firmware/doc/Structure.md)
- [Testing methodology](firmware/doc/TestingNotes.md)


### How to change object dictionary

Canfestival brings its own OD editor. The software requires Python 2 and wxgtk (sudo apt install python-wxgtk3.0-dev). Start program with

```bash
python2 ./src/canfestival/objdictgen/objectdictedit.py
``` 

You can now open *./objectDictionary/RemoteControlDevice.od* and start editing it to your liking. 
To apply changes just save the .od file and execute make. The code is automatically generated. 
Be careful as some tests are brittle any may require changing. Also make sure you see the notes in src/CanFestival/CanFestivalLocker.cpp.  
When **changing Heartbeat Consumers** please keep it in sync with src/Statemachine/Canopen.cpp


### Object dictionary configuration
- Based off of configuraition from 2020-12-07
- Node Id: 01h
- Manufacturer Specific 2000h - 5FFFh
  - 2000h: WheelTargetTorque (MCA_M_soll)
  - 2001h: BrakeTargetForce (MCB_F_soll)
  - 2002h: SteeringTargetAngle (MCL_delta_soll)
  - 2003h: SelfState 
  - 2004h: RTD_State
  - 2005h: BikeEmergency (not currently used)
- TPDO Mappings, Transmit parameters:
  - TPDO1: {SelfState}, COB: 181h, EventTime: 100ms (10Hz)
  - TPDO2: {BrakeTargetForce}, COB: 220h, EventTime: 20ms (50Hz)    
  - TPDO3: {SteeringTargetAngle}, COB: 230h, EventTime: 20ms (50Hz)    
  - TPDO4: {WheelTargetTorque}, COB: 210h, EventTime: 20ms (50Hz)  
  - TPDO5: {WheelTargetTorque, BrakeTargetForce, SteeringTargetAngle}, COB 200h, EventTime: 20ms (50Hz)
  - Every PDO's transmission type is "manufacturer specific (0xFE)" with an inhibit time of 0
  - Statemachine runs a 50Hz loop and transmits every cycle in remote mode
- RPDO Mappings, Receive parameters
  - RPDO1: {RTD_State}, COB: 182h, Event Time: 125ms (8Hz)
- Heartbeat Producer with 10Hz
- Heartbeat Consumer 
  - 10h Wheel Motor Controller, Timeout after 500ms
  - 20h Brake Actuator, Timeout after 500ms
  - 30h Steering Actuator, Timeout after 500ms
  - 21h Brake Pressure Sensor, Timeout after 500ms
  - 31h Steering Angle Sensor, Timeout after 500ms
- SDO Parameters
  - SDO Client to Brake Actuator and Steering Actuator


### Compile environment (Linux)

Use the CI artifacts for slow but easiest way to build the firmware. If you want to build locally install

```bash
sudo apt-get install binutils-arm-none-eabi build-essential gcc-arm-none-eabi git python2 bear
```

Please make sure your *arm-none-eabi-gcc* is at least version 9 or else C++17 features used are not available. Also the old compilers won't generate small enough code to fit the FLASH memory of the chip.

```bash
# >=9
arm-none-eabi-gcc --version 
```

If your compiler is too old get **purge the old gcc-arm-none-eabi installation** and get [9-2020-q2-update for x86-64 Linux](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads). Extract it to somewhere of your liking and edit your ~/.bashrc and add the compiler path to your PATH variable

```bash
# example
PATH=$PATH:/home/micha/Tools/gcc-arm-none-eabi-9-2020-q2-update/bin
```


### Compile environment (Windows)

Good luck! Get MinGW for make and git. Get [9-2020-q2-update for Windows](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads). That should get you close.


### Compiling 

*Assuming your terminal is in the folder of this document*

```
make -j8 RELEASE=1 RELEASE_OPT=s 
``` 

Your binary is located in the build/release folder.


### How to flash (Linux)

Freshly bought boards have ST-Link as programmer firmware but I've changed it to Segger JLink (see Development (Linux)). Install "JLink Toolchain for Linux" and execute the following command to flash the firmware.

```bash
make RELEASE=1 RELEASE_OPT=s jflash
```

### How to flash (Windows)

Compile the firmware and use the [Windows JLink](https://www.segger.com/downloads/jlink/JLink_Windows.exe).

### Serial  debug 

Connect the NUCLEO board as shown in the previous chapter. Install a serial monitor (gtkterm for Linux, PuTTY for Windows). The board will show as a /dev/ttyACMx / COMx. Use 115200-8-N-1 configuration You will now receive debug output. 

### Development (Linux)

Use [Visual Studio **Code**](https://code.visualstudio.com/). The project is configurated to work nicely with it. Install the recommended extensions. Compiling is done with Ctrl + Shift + B. Use F5 to flash and debug. You'll need to install *gdb-multiarch* and create a link. 

```bash
sudo ln -s /usr/bin/gdb-multiarch /usr/bin/arm-none-eabi-gdb
```

The standard flash / debug device on the NUCLEO boards is not smart enough to be aware of any realtime OS; so only one thread is shown. For a better experience install the [JLink Toolchain for Linux](https://www.segger.com/downloads/jlink/JLink_Linux_x86_64.deb) and change the NUCLEO's debugger's firmware to JLink with [this Tool](https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/). Remember to also chage the default Run Taks to "Debug (J-Link, RTOS-aware)".
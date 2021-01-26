#### File Format
Everything is in binary without spacers. XML syntax is only used for clarification.

```
<Instruction><Type size=1 /><Payload size=* /></Instruction><Instruction>...
```

- Instruction *0x0* Modifies RCSwitchState and HardwareSwitchState.
  - **Payload**
  - Index 0: States (One bit for State). See main.cpp for bit meanings.
- Instruction *0x1* "Sends" a can frame to the stack. Currently everything **not** 0x0 is interpreted as 0x1.
  - **Playload** 
  - Index 0: Data Length (0-8 bytes) 
  - Index 1: RTR Flag (0 for no, else for yes)
  - Index 2: High Byte of CanId
  - Index 3: Low Byte of CanId
  - (Index 4 - 12: Frame data, starting with highest byte)

#### goodTestData.bin Interpretation 

```cpp
# sw: neutral rc: timeout=true switches neutral 
00  40 
# can: some pdos unrelated to programm
01  04  00  0190 00110022
01  00  01  0250
01  08  00  03ff 0011223344556677
# can: rtd status bootup
01  01  00  0182 00
# -> to state: startup 

# sw: neutral rc: timeout=false switches neutral 
00  00
# can: rtd status ready
01  01  00  0182 01
# can: monitored devices bootup
01  01  00  0710  05
01  01  00  0720  05
01  01  00  0721  05
01  01  00  0730  05
01  01  00  0731  05
# -> to state: idle

# sw: neutral rc: timeout=false unlock, remote switch
00  4C
# -> to state: remote

# sw: neutral rc: timeout=false unlock, autonomous switch
00  5C
# -> to state: autonomous

# sw: neutral rc: timeout=false, button emcy
00  20
# -> to state: softemcy

# sw: manual rc: timeout=true
00  41
# -> to state: manual

# can: rtd status emcy
01  01  00  0182 05

# sw: bikeemcy rc: timeout=true
00  42
# -> to state: hardemcy
```
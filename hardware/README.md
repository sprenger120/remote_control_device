# Hardware
- NUCLEO-64 with STM32F302R8 processor with a custom adapter board ontop 
- FrSky XM+ receiver module soldered to the adapter board
- FrSky QX7 remote control with custom emergency stop button and LiIon batteries
- Designed with KiCad 5

### Configuration of remote control
Settings applied to the remote control. You may use OpenTX Companion to copy the config from one radio to the next.

Radio Setup:

- set date, time
- set batt range 9,9-12.6
- batt low warning 10v
- inactivity 60min
- backlight mode: keys
- global functions:
  - 01: on, volume, Input11(s1), active

Setup:

- model name: bike 1
 - s-warning: a,b deactivated
 - internal rf
   - d16
   - ch1-16
   - rx num 1
   - binding:  ch1-8 telem on
   - failsafe: no pulses
- inputs: T, A, E, R, SA, SB, SC, SD, SH, SF, S1, S2, MAX, MAX, MAX, MAX
 - SA input weight -100, offset -100
- mixer click every entry once to assign input
- logical switches
  - 01: a>x, SC, -50
  - 02: a>x, SD, -50
  - 03: AND, L01, L02
- special functions:
  - 01: L03, play sound, wrn1, 1 
  - 02: SA half, play sound, wrn2, -


### Modification of remote control
Opened the unit and removed switch SA and replaced it with a pushbutton. Original hole needs to be enlarged with a drill.

### adapterBoard v1 PCB  Notes
- Buck converter feedback resistive divider component R7 should be 52.3K instead of 52.3R (installed 51K as substitute), corrected in schematic
- XM+ receiver power switch is correctly denoted as normally closed, normally open was accidentally bought, removed SW1 and bridged pads so receiver is always on
- 10uF MLCC were specified as 1210 (imp.) components, 1206 (imp.) were bought, installed 1206 components without issue
- D-SUB connector needs to be soldered to bottom layer because of accidental pinout mirroring
- Modifications to Nucleo board so PCB fits ontop for soldering
   - Snip off Arduino headers on both sides
   - Remove User and Reset button caps and snip button stem even with button casing 
   - Snip off two pin header with jumpers and solder bridges so originl configuration with jumpers is estabilished
- "debugUART" (USART1) and XM+ USART (USART2) need to be switched because the built-in STLinks UART converter is connected to USART2
to achive this the following was done
  - SB14, SB13 resistors have been removed
  - wiring from USART1's pins has been added to the STLinks side of the resistors
- Removed U2 as it is used incorrectly and bridges SW01 and SW02

### Assembly
- assemble adapter board according to interactive bom in adapterBoard/bom
- don't forget to do the buck converter and switch fixes mentioned above
- solder on the XM+ with the help of 3x1 Dupont pin header
- remove original antennas very carefully (do not pull straight up but use a lever below the crimp to pry it off)
- attach the two MHF4 -> RP-SMA adapter cables
- 3D print enclosure (3D folder, [online cad](https://cad.onshape.com/documents/3ef6522cbbdee6598b7177e3/w/a3b5db2935905c5482c54099/e/5c57edc258b28d56febac476)) 
- do modifications to the nucleo board as seen above
- slot adapter board onto nucleo and use nylon standoff to reeinforce connection between the two boards
- screw in the RP-SMA terminals while only using the plain washer but not the split ring
- insert the board into the enclosure and use three M3 screws with appropriate length (25mm) to fasten the board stack to the enclosure
- fasten the enclosure's top with small short wood screws
- flash the latest firmware

### External resources other than datasheets
* [ST Nucleo64 footprint](https://github.com/mobilinkd/stm-morpho-template)

# DSB-2408-controller
controll the DSB-2408 digital stagebox made by AlbertAV

the stagebox is controlled by midi (channel 2-4) and ControlChange
## user interface:
<pre>
select                             value
           --------------------
encoder    | 01  -20dB  V=off |   encoder
   A       | XXXXXXXX      45 |      B
           --------------------
-20dB pad      short press       backlight on/off  
   menue        long press       +48V phantom power
</pre>

### midi channels:
- channel 2 is for audio input  1 ..  8
- channel 3 is for audio input  9 .. 16
- channel 4 is for audio input 17 .. 24

### midi controller:
<pre>
- PhantomGroup   20         -->  (0 = 1-4 off, 1 - 127 = 1-4 +48V) - not used
- PhantomGroup   21         -->  (0 = 5-8 off, 1 - 127 = 5-8 +48V) - not used
- Gain           24 ..  31  -->  (0/1 = 0dB , 2/3 = 10dB, 4/5 = 11dB ... 112-127=65dB)  --> n = 2*(xdB - 9dB)
- Pad           104 .. 111  -->  (0 = off , 1 - 127 = -20dB)
- Phantom       112 .. 119  -->  (0 = off , 1 - 127 = +48V)
</pre>

### hardware:
- 2x rotary encoder with pushbutton
- 1x LCD1602 i2c
- MCU = ATmega328 (arduino nano / uno / ProMini) 

### Pins:
-  0 midi transmit
-  2 encoder A Clock
-  3 encoder B Clock 
-  4 encoder A Data
-  5 encoder B Data
-  6 push    A
-  7 push    B
- 13 LED
- A4 SDA
- A5 SCL

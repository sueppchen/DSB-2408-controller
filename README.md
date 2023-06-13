# DSB-2408-controller
controll the DSB-2408 digital stagebox made by AlbertAV
the stagebox is controlled by Midi (channel 2-4) and ControlChange

midi - channel 2 is for audio input  1 ..  8
midi - channel 3 is for audio input  9 .. 16
midi - channel 4 is for audio input 17 .. 24
Controller:
PhantomGroup cc20 (0= 1-4 off, else = 1-4 +48V) - not used
PhantomGroup cc21 (0= 5-8 off, else = 5-8 +48V) - not used
Gain    cc 24..cc 31 (0/1 = 0dB , 2/3 = 10dB, 4/5 = 11dB .. 112-127=65dB)  --> n = 2*(xdB - 9dB)
Pad     cc104..cc111 (0=off, else = -20dB)
Phantom cc112..cc119 (0=off, else = +48V)

User interface:
 2x rotary encoder with pushbutton
 1x LCD1602 i2c
 
select                            value
          --------------------
encoder   | 01  -20dB  V=off |   encoder
   A      | XXXXXXXX      45 |      B              
          --------------------

-20dB pad      short press       backlight on/off  
 menue         long press        +48V phantom power


MCU = ATmega328 (arduino nano / uno / micro) 
Pins:
  0 midi transmit
  2 encoder A Clock
  3 encoder B Clock 
  4 encoder A Data
  5 encoder B Data
  6 push    A
  7 push    B
 13 LED
 A4 SDA
 A5 SCL

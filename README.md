# Bluetooth audio streaming with Infineon MA12070P multilevel classD audio amplifier
##
This design shows how easy a full featured Bluetooth audio streaming application can be build 
using the ESP32 and a multilevel classD amp. 

### System
The design used Bluedroid aka the Android bluetooth stack for audio streaming and application control. 
The Bluetooth A2DP and SPP protocols are used. A2DP for audio streaming, meta media info, playback control 
and volume. SPP is used for appliction control - channel volume and mute in the audio processor in the MA12070P 
and cross over frequency of the 4th order high/low pass filter that can be looped in to the audio steam in 
the ESP32. 

The audio amplifer is powered from a PVDD rail of 18-24 volt and only consumes around 8 mA in duing idle and low audio 
levels - the power consumption is dominated by the ESP32 until the volume hits party level. Use a good power tool battery and
your will have god play back time. 
The amplifier changes modes of operation, swiching frequency and modulation mode based on the input level to 
keep high effeicency over a large dynamic range.

### System hookup 
```
Power supply : 20V powertool battery 
ESP32 module : TTgo T7-v1.4 Wrower based module in small form factor - has PSRAM for audio buffers 
               that is need when WIFI audio streaming is used 
MA12070P     : Small from factor 2 x 80W, PVDD 15-25Volt 2xBTL or PBTL mode
```

### The amplifier module only needs the following control signals:
```
I2S for audio interface : 
  I2S_BCK and MCLK : Bit clock 2.784 MHz for 32bits 44.1KHz sample 
  I2S_WS           : The left/right framing signal 
  I2S_D0           : The audio data bits
I2C for internal register control (Hardwired to address 0x20)
  I2C_SCL          : Clock        
  I2C_SDA          : Data
Static control signal  
  Nenable          : Pulled to ground from the ESP32 at startup 
```

```
ESP32 TTgo T7-v1.4                   Merus Audio MA12070P 

TX0    GND                         27  I2S_BCK
RX0    27                          25  I2S_WS
22     25                              GND
21     32                          32  I2S_D0
17     12                         (27) MCLK 
16     4                               
GND    0pu                         22  I2C_SCL
5V     2pd                         21  I2C_SDA  
15pu   SD1                             GND   
SD0    CLK                         15  ENABLE  
                                   nc  MUTE 
```
The repo push the following pin configs - but can be changed using make menuconfig system 

### SPP app control 





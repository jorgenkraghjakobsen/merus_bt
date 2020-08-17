# Merus Audio Bluetooth Streaming 

ESP32 TTgo T7-v1.4                   Merus Audio MA12070P 

TX0    GND                             I2S_BCK
RX0    27                              I2S_WS
22     25                              GND
21     32                              I2S_D0
17     12                              MCLK 
16     4                               
GND    0pu                             I2C_SCL
5V     2pd                             I2C_SDA  
15pu   SD1                             GND   
SD0    CLK                             ENABLE  
                                       MUTE 
Use the slide like web framework 

Introduction 
============================================================================================================================
Title: Rapid         prototype                   Bluetooth    Amplifier     Platform 
(why   fast - fraigle build prove of concept - Easy ubugity  - MA12070P  - what is next)

Why not just go to Frey and by the latest Marshall BT speaker - super build quality and uses the Infineon MA12070P audio amp 

Na I am more into the 2-3 year out look and need to build a platform that can do want i need now and what to come

For that I need 
Power : Lidl X20V power tool battery - a 2Ah 20V 5s2 std li low cost easy access able 
Micro processor : ESP32 from espresssif the chinise wunter buy - IPO'ed last year base on a high performance team where the 
key acchiment is a dual core tencilica SOC with SW development kit orcasted by the russian hacker Igor 
Audio ampliter : Infineon MA120070P - Digial input multilevel class D - highly configurarle and low power with wide dynamic range. 

Idear cell, esp32 chip , ma12070p 
prototype 
Lidl pack, wrower module (ttgo T7 v1.4) and IFX MA12070P snowball 

Block diagram 
============================================================================================================================
simple 
MCU BT   audio amp  speakers
power 

Detail overview application    
Mobilephone    ->    BT_A2DP    ->   ESP32 Bluedroid     ->  Ring buffer  ->   DSP Processor  ->  I2S DMA driver  ->  MA12070P 
spotify             Audio, AV        Android BT stack  
                    meta data        Audio decoding 
                                     message/cmd passing  

IFX app              BT_SPP          ESP32 Bluedroid     ->  protocol handler                 ->  I2C Merus driver -> MA12070P   
                     RX/TX           



Prototyping 
===============================================================================================================================
Show video of protoype - moduel wires and back and decer and hacker psu 

Fritzing wire diagram boards  

3D print of base module - Videos of 3D flow - scad/slicer/octopi/printer



Get durty 
==============================================================================================================================
Now your compoter need to assmple all we been thrug until now and compile a progama that will run on the esp32 connnect to you 
android phone and play musuk thrug the esp32 

Walk thrugh git check out  
  make menuconfigre 
  make falsh and monitor 

SPP Ampinventer 
  start uptool and link to share 
    




 


















Figures 

Simple overview hardware 
Lidl batteri  ->      

Detail overview application    
Mobilephone    ->    BT_A2DP    ->   ESP32 Bluedroid     ->  Ring buffer  ->   DSP Processor  ->  I2S DMA driver  ->  MA12070P 
spotify             Audio, AV        Android BT stack  
                    meta data        Audio decoding 
                                     message/cmd passing  

IFX app              BT_SPP          ESP32 Bluedroid     ->  protocol handler                 ->  I2C Merus driver -> MA12070P   
                     RX/TX           





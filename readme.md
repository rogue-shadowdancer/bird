# Introduction
A simple tutorial on how to start ESP32:  
https://www.bilibili.com/video/BV1tv411w74d/?spm_id_from=333.999.0.0&vd_source=a46712a55409cd80ae631162ea124439  
Esp32 will collect the acc data from jy60, and send out as udp package.  
The scripts folder has a python script to test if the udp package can be correctly received.  
# connection
Battery 3.7V -> VIN ESP32
Battery GND -> GND ESP32

ESP32 3.3V -> VCC JY60
ESP32 GND -> GND JY60
ESP32 RX D16 -> TX JY60
ESP32 TX D17 -> RX JY60
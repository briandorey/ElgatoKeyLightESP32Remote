# Elgato Key Light ESP32 Remote
Using an ESP32 Wi-FI module to control Elgato Key Lights

The elgatolightremote folder contains a basic Arduino program to use with an ESP32 WiFi module to control a Elgato Key Light, WiFi controlled light.

The elgatolightremotev2 folder contains a more advanced Arduino program to use with an ESP32 WiFi module to control a Elgato Key Light, WiFi controlled light. This version uses a struct to hold the data for the Key Light so you can create multiple lights and control them using the ESP32 module. Any changes made using the Elgato apps will be loaded using the Arduino code for brightness and colour temp.

The threeelgatolightremote folder contains the code for the three button remote controller. More details for this project can be found on my blog at https://www.briandorey.com/post/esp32-three-elgato-key-light-remote


Elgato Light Controller Case.stl - STL file for three button remote case

Elgato Light Controller.dip - PCB file in Diptrace format for three button remote controller

Elgato Light Controller_gerber.zip - PCB files for three button remote controller
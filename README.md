# An IoTivity Demo that Works on ESP32
# Table of Contents
- [Introduction](#Introduction)
- [Part 1: Prerequisites](#prerequisites)
- [Part 2: SDK and Tools Preparation](#tools_prepare)
- [Part 3: Configuring and building](#config_build)
- [Part 4: Result shows](#results)
- [TroubleShoot](#troubleshoot)

<span id = "Introduction">Introduction</span>
 ------------------------------
###### ESP32 is one of gorgeous ioT device that can interface with other systems to provide Wi-Fi and Bluetooth functionality through the SPI / SDIO or I2C / UART interfaces.for more details, click https://espressif.com/en/products/hardware/esp32/overview  

###### IoTivity is an open source software framework enabling seamless device-to-device connectivity to address the emerging needs of the Internet of Things,for more details, click https://www.iotivity.org  
**Aim:**
##### This page would guide you to make your ESP32 works under the IoTivity framework.There would be two ESP32 chips,one as the OIC client and the other as the OIC server.Firstly, we would config our Host PC and flash code to each of chips.Secondly,both of them would connect to the same AP(WIFI),and then would transmit messages to each other by IoTivity framework.the iotivity framework on ESP32 came from git repository:https://github.com/iotivity/iotivity-constrained  
main workflow depicts as following:
![IoTivityESP32workflow](https://github.com/ustccw/RepoForShareData/blob/master/OCF/Photos/IoTivityESP32Workflow.png)

<span id = "prerequisites">Part 1: Prerequisites</span>
 ------------------------------
- **ubuntu environment** for building your demo.
- **two ESP32 devices** for running the OIC client and the OIC server.  
![ESP32 device](https://github.com/ustccw/RepoForShareData/blob/master/Microsoft/AzureData/Photos/ESP32-DevKitC.png)

<span id = "tools_prepare">Part 2: SDK and Tools Preparation</span>
 ------------------------------
#### 2.1 **SDK get**
 you could get IoTivityESP32-SDK from https://github.com/ustccw/IoTivityESP32.  
 IoTivityESP32-SDK could make your ESP32(or Device with ESP32) work under the IoTivity framework.  
 you could get IDF-SDK from https://github.com/espressif/esp-idf.  
 IDF-SDK could make ESP32 get started and work well.  
	**Important:** there would be some submodules in SDK and **submodule of SDK**,please run **'git submodule init'** and **'git submodule update'** firstly.

#### 2.2 **Compiler get**  
 follow the guide: http://esp-idf.readthedocs.io/en/latest/get-started/linux-setup.html  
 xtensa-esp32-elf compiler could build ESP32 code and flash code to the chip.  

<span id = "config_build">Part 3: Configuring and building</span>
------------------------------
### 3.1 config you ESP32 server 
```
$make menuconfig
```
> * config your Default serial port
> * config your Default baud rate
> * config your WiFi SSID and WiFi Password
> * choose your iotivity mode(iotivity server)
> * enable your SO_REUSEADDR option

### 3.2 build your OIC server and flash code to ESP32
```
$make flash
```
if failed,try:
 - make sure that ESP32 had connect to PC by serial port 
 - make sure you flash to correct serial port
 - try type command:
   > sudo usermod -a -G dialout $USER

### 3.3 clean your workspace for OIC client
```
$make clean	
```
you should run **'make clean'** before every **'make menuconfig'** and **'make flash'**.

### 3.4 build your ESP32 client
```
$make menuconfig
```
> * config your Default serial port
> * config your Default baud rate
> * config your WiFi SSID and WiFi Password
> * choose your iotivity mode(iotivity client)
> * enable your SO_REUSEADDR option

### 3.5 build your OIC client and flash to ESP32
```
$make flash
```

<span id = "results">Part 4: Result shows</span>
 ------------------------------
 - restart your ESP32 server(OIC server)
 - restart your ESP32 client(OIC client)
 - you would see that light state is changing between esp32 client and esp32 server.

 <span id = "troubleshoot">TroubleShoot</span>
 ------------------------------
 - close some firewall settings
 - build failed,try:
   - git submodule init
   - git submodule update
   - export your compiler path 
   - export your SDK path
   - get start with http://espressif.com/en/support/download/documents?keys=&field_type_tid%5B%5D=13
 - make sure you could run hello_world demo on ESP32

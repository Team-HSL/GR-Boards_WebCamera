# GR-Boards_WebCamera
**Please see [here](README_JPN.md) for Japanese version.**  

This is a sample program that works on GR-PEACH or GR-LYCHEE.  

## Overview
This is a web camera sample program. When accessing from the Web browser, the following will be displayed.  

* Camera input image
* Control screen of devices connected to the I2C bus
* LED control screen

## Requirements

### In the case of GR-PEACH
* [GR-PEACH](https://os.mbed.com/platforms/Renesas-GR-PEACH/)
* One of the following cameras  
  * NTSC camera
  * MT9V111 and [GR-PEACH AUDIO CAMERA Shield](https://os.mbed.com/teams/Renesas/wiki/Audio_Camera-shield)
  * OV5642 and [GR-PEACH AUDIO CAMERA Shield](https://os.mbed.com/teams/Renesas/wiki/Audio_Camera-shield)
  * GR-PEACH Wireless CAMERA Shield : OV7725
* One of the following networks
  * Ethernet
  * BP3592 (wifi)
  * GR-PEACH Wireless CAMERA Shield : ESP32 (wifi)


### In the case of GR-LYCHEE
* [GR-LYCHEE](https://os.mbed.com/platforms/Renesas-GR-LYCHEE/)

## [Preliminary preparation] Replace ESP 32 with firmware for AT command
When using this sample program, you need to rewrite ESP32 to "esp32-at".  
(Since ESP32 of ``GR-LYCHEE`` and ``GR-PEACH Wireless CAMERA Shield`` has "esp32-at" written as an initial farm, it can be used as it is.)  
Please refer to the following for rewrite method.  
https://github.com/d-kato/GR-Boards_ESP32_Serial_Bridge  

## How to use
1. Turn on the power to the GR-Board and start the terminal software.

2. When you press the reset button of GR-Board, the scan result of Wifi will be output on Terminal. Please refer to the following link for usage of Terminal. The default value of the bow rate with mbed is 9600, and in this sample we use the baud rate 9600.   
  https://developer.mbed.org/teams/Renesas/wiki/GR-PEACH-Getting-Started#install-the-usb-serial-communication  
  https://developer.mbed.org/handbook/SerialPC  

  ```
  ********* PROGRAM START ***********
  Network Setting up...
  Scan:
  No.0 Network: SSID_1 secured: WPA/WPA2 BSSID: xx:xx:xx:xx:xx:xx RSSI: -52 Ch: 1
  No.1 Network: SSID_2 secured: Unknown BSSID: xx:xx:xx:xx:xx:xx RSSI: -67 Ch: 2
  2 networks available.

  Please enter the number of the network you want to connect.
  Enter key:[0]-[1], (If inputting the other key, it's scanned again.)
  ```

  Enter the number of the network you want to connect from the keyboard.
  To select SSID_1, press "0" on the keyboard.  

  ```
  [SSID_1] is selected.
  please enter the PSK.
  ```

  Finally, enter the password to connect to the network.    

3. If the connection is successful, the IP address is output on Terminal.  
  ```
  connecting...
  MAC Address is xx:xx:xx:xx:xx:xx
  IP Address is 192.168.0.2
  NetMask is 255.255.255.240
  Gateway Address is 192.168.0.1
  Network Setup OK
  ```

4. When you open **IP Address** ("192.168.0.2" in the above example) by a Web browser, top screen is indicated. The configuration of the top screen is a menu screen on the left side, a description screen of the sample program on the right side. If you click on each menu on the left side of the screen, the screen along the menu is indicated on the right screen.

5. When you click the "Web camera" in the menu screen, the pictures of a camera is indicated. It can be changed at a slider bar in "Wait time" at the timing of a renewal of a camera picture. (Defaults are 500ms.)

6. When you click the "Setting by I2C" of the menu screen, the control screen of the device connected to the I2C bus is indicated. By sending a command of a format listed below("Format of the device set by I2C") in Direct input or File reference, data transmission and reception is possible with respect to devices connected to I2C_SDA and I2C_SCL terminals of I2C. The communication log of transmission and reception by "Format of the device set by I2C" is displayed in the log window. When you press the "Clear" button, the log is cleared. When you press the "Save" button, the log is saved.  

7. When you click the "LED On/Off" of the menu screen, the LED control screen is indicated. Each switch switches of GR-Boards to ON or OFF. Each switch indicates the current LED state. When the LED is On, the color of switch will change the LED color.

8. When you click the "Top Page" of the menu screen, the top screen is indicated.


### Switch to web page in SD card
When connecting the SD card, ``SDBlockDevice`` will be displayed on the terminal and the web page displayed in the web browser will switch from the built-in ROM to the SD card. The top page is ``index.htm``.  


### Change network connection
You can change the network connection by changing the following macro in ``main.cpp``.
GR-LYCHEEを付属品のみで動作させる場合はNETWORK_TYPE 2と3が選択できます。  

```cpp
/**** User Selection *********/
#define NETWORK_TYPE           (2)                 /* Select  0(Ethernet), 1(BP3595), 2(ESP32 STA) ,3(ESP32 AP) */
#if (NETWORK_TYPE >= 1)
  #define SCAN_NETWORK         (1)                 /* Select  0(Use WLAN_SSID, WLAN_PSK, WLAN_SECURITY) or 1(To select a network using the terminal.) */
  #define WLAN_SSID            ("SSIDofYourAP")    /* SSID */
  #define WLAN_PSK             ("PSKofYourAP")     /* PSK(Pre-Shared Key) */
  #define WLAN_SECURITY        NSAPI_SECURITY_WPA_WPA2 /* NSAPI_SECURITY_NONE, NSAPI_SECURITY_WEP, NSAPI_SECURITY_WPA, NSAPI_SECURITY_WPA2 or NSAPI_SECURITY_WPA_WPA2 */
#endif
```

``NETWORK_TYPE`` に設定する値により、以下の接続方法に切り替わります。  

| Number | Connection | Description                                   |
|:-------|:-----------|:----------------------------------------------|
| 0      | Ethernet   | GR-PEACH only                                 |
| 1      | BP3595     | It is necessary to prepare BP3595 separately. |
| 2      | ESP32 STA  | Use ESP32 in STA mode.                        |
| 3      | ESP32 AP   | Use ESP32 in AP mode.                         |

``GR-PEACH Wireless CAMERA Shield `` is required separately when using ESP32 with GR-PEACH.  

* In the case ``NETWORK_TYPE = 1 or 2``  
  WLAN_SSID, WLAN_PSK and WLAN_SECURITY set the information of the access point to be connected. However, in the case of ``SCAN_NETWORK = 1``, these values are not referenced, and the connection destination is selected based on the scan result displayed on Terminal.  

* In the case ``NETWORK_TYPE = 3``  
 WLAN_SSID, WLAN_PSK and WLAN_SECURITY set the information as AP published by ESP32.  

### Change the size of camera input images
You can change the size of camera input images by changing the macro below in ``main.cpp``.   
``JPEG_ENCODE_QUALITY`` sets the quality of JPEG encoding.
The upper limit of "SetQuality()" is **100**, but consider the size of the memory storing the JPEG conversion result etc., the upper limit should be about **75**.  

```cpp
/**** User Selection *********/
/** JPEG out setting **/
#define JPEG_ENCODE_QUALITY    (75)                /* JPEG encode quality (min:1, max:75 (Considering the size of JpegBuffer, about 75 is the upper limit.)) */
#define VFIELD_INT_SKIP_CNT    (0)                 /* A guide for GR-LYCHEE.  0:60fps, 1:30fps, 2:20fps, 3:15fps, 4:12fps, 5:10fps */
/*****************************/
```

In addition, you can change the number of pixels of the image by changing the following. As the number of pixels decreases, the transfer data decreases.

```cpp
#define VIDEO_PIXEL_HW       (320u)  /* QVGA */
#define VIDEO_PIXEL_VW       (240u)  /* QVGA */
```

### Camera selection
To select the Camera, add `` camera-type`` to `` mbed_app.json``.  
```json
{
    "config": {
        "camera":{
            "help": "0:disable 1:enable",
            "value": "1"
        },
        "camera-type":{
            "help": "Please see mbed-gr-libs/README.md",
            "value": "CAMERA_CVBS"
        },
        "lcd":{
            "help": "0:disable 1:enable",
            "value": "1"
        }
    }
}
```

| camera-type "value"     | Description                        |
|:------------------------|:-----------------------------------|
| CAMERA_CVBS             | GR-PEACH NTSC signal               |
| CAMERA_MT9V111          | GR-PEACH MT9V111                   |
| CAMERA_OV7725           | GR-LYHCEE attached camera          |
| CAMERA_OV5642           | GR-PEACH OV5642                    |
| CAMERA_WIRELESS_CAMERA  | GR-PEACH Wireless CAMERA Shield (OV7725) |

When camera-type is not selected, the following is set.  
* GR-PEACH : CAMERA_MT9V111  
* GR-LYCHEE : CAMERA_OV7725  

# GR-Boards_WebCamera
GR-PEACH、および、GR-LYCHEEで動作するサンプルプログラムです。  

## 概要
Webカメラのサンプルです。Webブラウザからアクセスすると以下が表示されます。  

* Camera入力画像
* I2Cバスに繋がっているデバイスの制御画面
* LED操作画面

## 構成

### GR-PEACHの場合
* [GR-PEACH](https://os.mbed.com/platforms/Renesas-GR-PEACH/)
* 以下のいずれかのカメラ
  * NTSC アナログカメラ
  * MT9V111 and [GR-PEACH AUDIO CAMERA Shield](https://os.mbed.com/teams/Renesas/wiki/Audio_Camera-shield)
  * OV5642 and [GR-PEACH AUDIO CAMERA Shield](https://os.mbed.com/teams/Renesas/wiki/Audio_Camera-shield)
  * GR-PEACH Wireless CAMERA Shield : OV7725
* 以下のいずれかのネットワーク
  * Ethernet
  * BP3592 (wifi)
  * GR-PEACH Wireless CAMERA Shield : ESP32 (wifi)


### GR-LYCHEEの場合
* [GR-LYCHEE](https://os.mbed.com/platforms/Renesas-GR-LYCHEE/)

## [事前準備] ESP32をATコマンド用のファームウェアに書き換える
本サンプルプログラムを使用する際は``ESP32をATコマンド用のファームウェア「esp32-at」``に書き換える必要があります。  
(GR-LYCHEEのESP32には初期ファームとして「esp32-at」が書き込まれているため、そのままご利用になれます。)  
書き換え方法については下記を参照ください。  
https://github.com/d-kato/GR-Boards_ESP32_Serial_Bridge  

## 使い方
1. GR-Boardに電源を入れ、Terminalソフトを立ち上げます。  

2. GR-Boardのリセットボタンを押すと、Terminal上にWifiのスキャン結果が出力されます。  
  Terminaｌの使い方は以下のリンクを参照下さい。  
  Mbedでのボーレートのデフォルト値は9600で、このサンプルではボーレート9600を使います。  
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

  接続したいネットワークの番号をキーボードから入力します。  
  0番のSSID_1を選択する場合はキーボードの "0" を押します。  

  ```
  [SSID_1] is selected.
  please enter the PSK.
  ```

  最後にネットワークに接続するためのパスワードを入力します。    

3. 接続に成功すると、Terminal上にIPアドレスが出力されます。  
  ```
  connecting...
  MAC Address is xx:xx:xx:xx:xx:xx
  IP Address is 192.168.0.2
  NetMask is 255.255.255.240
  Gateway Address is 192.168.0.1
  Network Setup OK
  ```

4. PCのWebブラウザで Terminalに表示された **IP Address** (上記例では 192.168.0.2)を開くと、トップ画面が表示されます。トップ画面は、左側にメニュー画面、右側にサンプルプログラムの説明画面という構成になっており、左画面の各メニューをクリックすると、メニューに沿った画面が右画面に表示されます。

5. メニュー画面の”Web Camera”をクリックすると、Camera画像が表示されます。  
  "Wait time"のスライダーバーでCamera画像の更新タイミングが変更できます。(初期値は500msです)

6. メニュー画面の"Setting by I2C"をクリックすると、I2Cバスに繋がっているデバイスの制御画面が表示されます。直接入力(Direct input)欄又はファイル参照(File reference)欄にて、下記("I2Cによるデバイス設定のフォーマット")に記載されているフォーマットのコマンドを送信する事で、I2CのI2C_SDA,I2C_SCL端子に繋がっているデバイスに対して、データの送受信が可能です。"I2Cによるデバイス設定のフォーマット"による送受信の通信ログは、ログウィンドウに表示されます。"Clear"ボタンを押すとログのクリア、"Save"ボタンを押すとログの保存ができます。  

7. メニュー画面の"LED On/Off"をクリックすると、LED操作画面が表示されます。各スイッチはGR-BoardのLED ON/OFFを切り替えます。スイッチはそれぞれGR-BoardのLEDの現在の状態を表しており、ONにすると対応するLEDの色になります。  

8. メニュー画面の"Top Page"をクリックすると、 トップ画面が表示されます。


### SDカード内のWebページに切り替える
SDカードを接続するとTerminal上に ``SDBlockDevice`` と表示され、Webブラウザで表示されるWebページが内蔵ROMからSDカードに切り替わります。トップページはS ``index.htm`` となります。  


### ネットワークの接続方法を変更する
``main.cpp``の下記マクロを変更することでネットワークの接続方法を変更できます。  
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

| 番号 | 接続方法 | 説明                                          |
|:-----|:---------|:----------------------------------------------|
| 0    | Ethernet | GR-PEACHのみ。                                |
| 1    | BP3595   | 別途BP3595を用意する必要があります。          |
| 2    | ESP32 STA| ESP32をSTAモードで使用します。                |
| 3    | ESP32 AP | ESP32をAPモードで使用します。                 |

GR-PEACHでESP32を使用する場合は別途``GR-PEACH Wireless/Cameraシールド``が必要です。  

* ``NETWORK_TYPE = 1 or 2``で動作させる場合  
 WLAN_SSID、WLAN_PSK、WLAN_SECURITYは接続先アクセスポイント(AP)の情報を設定します。但し、``SCAN_NETWORK = 1``の場合はこれらの値は参照されず、Terminal上に表示されるscan結果を基に接続先を選択します。  

* ``NETWORK_TYPE = 3``で動作させる場合  
 WLAN_SSID、WLAN_PSK、WLAN_SECURITYはESP32が公開するAPとしての情報を設定します。  


### カメラ画像のサイズを変更する
``main.cpp``の下記マクロを変更することでカメラ画像のサイズを変更できます。  
``JPEG_ENCODE_QUALITY``はJPEGエンコード時の品質(画質)を設定します。
API``SetQuality()``の上限は**100**ですが、JPEG変換結果を格納するメモリのサイズなどを考慮すると,上限は**75**程度としてください。  

```cpp
/**** User Selection *********/
/** JPEG out setting **/
#define JPEG_ENCODE_QUALITY    (75)                /* JPEG encode quality (min:1, max:75 (Considering the size of JpegBuffer, about 75 is the upper limit.)) */
#define VFIELD_INT_SKIP_CNT    (0)                 /* A guide for GR-LYCHEE.  0:60fps, 1:30fps, 2:20fps, 3:15fps, 4:12fps, 5:10fps */
/*****************************/
```

また、以下を変更することで画像の画素数を変更できます。画素数が小さくなると転送データは少なくなります。

```cpp
#define VIDEO_PIXEL_HW       (320u)  /* QVGA */
#define VIDEO_PIXEL_VW       (240u)  /* QVGA */
```

### カメラの設定
カメラの指定を行う場合は``mbed_app.json``に``camera-type``を追加してください。
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

| camera-type "value"     | 説明                               |
|:------------------------|:-----------------------------------|
| CAMERA_CVBS             | GR-PEACH NTSC信号                  |
| CAMERA_MT9V111          | GR-PEACH MT9V111                   |
| CAMERA_OV7725           | GR-LYHCEE 付属カメラ               |
| CAMERA_OV5642           | GR-PEACH OV5642                    |
| CAMERA_WIRELESS_CAMERA  | GR-PEACH Wireless/Cameraシールド (OV7725) |

camera-typeとlcd-typeを指定しない場合は以下の設定となります。  
* GR-PEACH、カメラ：CAMERA_MT9V111  
* GR-LYCHEE、カメラ：CAMERA_OV7725  
